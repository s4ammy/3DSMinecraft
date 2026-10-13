import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
referenceHash = "902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65"
reservations = [
    ("Main", 0x4a403c, 0x1350, ".text.BuildKeyboardBindings"),
    ("Modified", 0x710a14, 0x600, ".text.BuildModifiedKeyboardBindings"),
    ("Shared", 0x711014, 0x5d0, ".text.BuildSharedKeyboardBindings"),
    ("Extra", 0x35c750, 0x38c, None),
    ("Hooks", 0x918824, 0x100, ".controlsHooks"),
    ("Disk", 0x918944, 0x14c, ".text._ZN10TouchStick8DrawDiskEfffPKf"),
]
entryPoints = ["RuntimeInputHook", "RuntimeHudDestroyHook", "RuntimeAccessoryHook", "RuntimeShutdownHook",
               "BuildControlRows", "HandleControlButton", "DispatchControlRelease", "DestroyControlRows", "DrawRuntimeControls",
               "LoadControlSettings", "SaveControlSettings"]


def Main():
    parser = argparse.ArgumentParser(description="Build the unified update controls payload with stock ABI guards")
    parser.add_argument("--reference-code", type=Path, required=True)
    parser.add_argument("--artifacts-dir", type=Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    reference = arguments.reference_code.read_bytes()
    if hashlib.sha256(reference).hexdigest() != referenceHash:
        raise RuntimeError("Expected the supported stock v9.11.0 update")

    with tempfile.TemporaryDirectory(prefix="mc3ds-runtime-controls-") as temporaryDirectory:
        temporaryPath = Path(temporaryDirectory)
        flags = ["--target=arm-none-eabi", "-march=armv6k", "-marm", "-mfpu=vfpv2", "-mfloat-abi=hard", "-Oz", "-Wno-unused-command-line-argument",
                 "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
                 "-ffunction-sections", "-fdata-sections", "-fno-math-errno"]
        objects = []
        sections = []
        for source in ["RuntimeControls.cpp", "ControlSettings.cpp", "CompactBindings.cpp", "TouchStick.cpp", "TouchStickInput.cpp", "RuntimeControls.s"]:
            objectPath = temporaryPath / (source + ".o")
            subprocess.run(["clang++", *flags, "-std=c++17", "-c", str(projectPath / "patches" / source), "-o", str(objectPath)], check=True)
            objects.append(objectPath)
            listing = subprocess.check_output(["llvm-readelf", "-SW", str(objectPath)], text=True)
            for line in listing.splitlines():
                match = re.search(r"\[\s*\d+\]\s+(\S+)\s+PROGBITS\s+\S+\s+\S+\s+([0-9a-fA-F]+)\s+\S+\s+(\S+)\s+\d+\s+\d+\s+(\d+)", line)
                if not match or "A" not in match[3] or int(match[2], 16) == 0:
                    continue
                if match[1] in (".text.UpdateTouchStick", ".text.DrawTouchStick"):
                    continue
                sections.append((source + ".o", match[1], int(match[2], 16), int(match[4])))

        bins = [[] for _ in reservations]
        used = [0 for _ in reservations]
        for index, (_, _, _, entry) in enumerate(reservations):
            if entry is not None:
                section = next(item for item in sections if item[1] == entry)
                sections.remove(section)
                bins[index].append(section)
                used[index] = section[2]
        for section in sorted(sections, key=lambda item: (-item[2], item[0], item[1])):
            for index, (_, _, capacity, _) in enumerate(reservations):
                if index == 4:
                    continue
                end = (used[index] + section[3] - 1) // section[3] * section[3] + section[2]
                if end <= capacity:
                    bins[index].append(section)
                    used[index] = end
                    break
            else:
                raise RuntimeError(f"No guarded space for {section}: used {used}")

        layout = []
        for (name, address, capacity, _), contents in sorted(zip(reservations, bins), key=lambda item: item[0][1]):
            layout.append(f".controls{name} 0x{address:x} : {{")
            layout += [f"KEEP({item[0]}({item[1]}))" for item in contents]
            layout += ["}", f'ASSERT(SIZEOF(.controls{name}) <= 0x{capacity:x}, "{name} controls payload overflow")']
        linkerPath = temporaryPath / "controls.ld"
        linkerPath.write_text((projectPath / "patches/RuntimeControls.ld").read_text().replace("@SECTIONS@", "\n".join(layout)))
        elfPath = temporaryPath / "controls.elf"
        subprocess.run(["clang++", *flags, "-nostdlib", "-fuse-ld=lld", "-Wl,--no-undefined", "-Wl,--gc-sections",
                        "-Wl,-e,RuntimeInputHook", f"-Wl,-T,{linkerPath}", *[path.name for path in objects], "-o", str(elfPath)], cwd=temporaryPath, check=True)
        symbols = {}
        for line in subprocess.check_output(["llvm-nm", "--defined-only", str(elfPath)], text=True).splitlines():
            parts = line.split()
            if len(parts) == 3:
                symbols[parts[2]] = int(parts[0], 16)
        lines = ["#pragma once", "", '#include "Bytes.h"', "", "namespace Mc3ds {"]
        for name in entryPoints:
            lines.append(f"    inline constexpr std::uint32_t update{name}Address = 0x{symbols[name]:x};")
        lines += ["", "    struct TRuntimeControlsRegion {", "        std::size_t offset;", "        std::size_t capacity;",
                  "        const char *originalHash;", "        const char *payloadHash;", "        const char *hex;", "    };",
                  "", "    inline const TRuntimeControlsRegion updateRuntimeControlsRegions[] = {"]
        artifacts = arguments.artifacts_dir
        if artifacts:
            artifacts.mkdir(parents=True, exist_ok=True)
            (artifacts / "controls.elf").write_bytes(elfPath.read_bytes())
            (artifacts / "symbols.json").write_text(json.dumps(symbols, indent=4) + "\n")
            (artifacts / "controls.ld").write_text(linkerPath.read_text())
        for name, address, capacity, _ in reservations:
            binaryPath = temporaryPath / f"controls{name}.bin"
            subprocess.run(["llvm-objcopy", "-O", "binary", f"--only-section=.controls{name}", str(elfPath), str(binaryPath)], check=True)
            payload = binaryPath.read_bytes()
            offset = address - 0x100000
            digest = hashlib.sha256(reference[offset:offset + capacity]).hexdigest()
            lines.append(f'        {{0x{offset:x}, 0x{capacity:x}, "{digest}", "{hashlib.sha256(payload).hexdigest()}",')
            hexData = payload.hex()
            lines += [f'            "{hexData[index:index + 96]}"' for index in range(0, len(hexData), 96)]
            lines.append("        },")
            if artifacts:
                (artifacts / binaryPath.name).write_bytes(payload)
            print(f"{name}: {len(payload)}/{capacity} bytes")
        lines += ["    };", "}", ""]
        target = projectPath / "src/RuntimeControlsData.h"
        content = "\n".join(lines)
        if arguments.check:
            if target.read_text() != content:
                raise RuntimeError("RuntimeControlsData.h differs from its reproducible build")
        else:
            target.write_text(content)


if __name__ == "__main__":
    Main()
