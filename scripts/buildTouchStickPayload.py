import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
referenceHash = "902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65"


def Main():
    parser = argparse.ArgumentParser(description="Build the update touch-stick payload and its stock ABI guards.")
    parser.add_argument("--reference-code", dest="referenceCode", type=Path, required=True)
    parser.add_argument("--artifacts-dir", dest="artifactsPath", type=Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    reference = arguments.referenceCode.read_bytes()
    if hashlib.sha256(reference).hexdigest() != referenceHash:
        raise RuntimeError("The input is not the supported stock update executable")

    with tempfile.TemporaryDirectory(prefix="mc3ds-touch-build-") as temporaryDirectory:
        temporaryPath = Path(temporaryDirectory)
        elfPath = temporaryPath / "touch.elf"
        command = [
            "clang++", "--target=arm-none-eabi", "-march=armv6k", "-marm", "-mfpu=vfpv2", "-mfloat-abi=hard",
            "-I", str(projectPath / "patches"), "-std=c++17", "-Oz", "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-fno-unwind-tables",
            "-fno-asynchronous-unwind-tables", "-fno-math-errno", "-ffunction-sections", "-fdata-sections",
            "-nostdlib", "-fuse-ld=lld", "-Wl,--no-undefined", "-Wl,-e,TouchInputHook",
            f"-Wl,-T,{projectPath / 'patches/controls/TouchStickUpdate.ld'}",
            str(projectPath / "patches/controls/TouchStick.cpp"), str(projectPath / "patches/controls/TouchStickInput.cpp"),
            str(projectPath / "patches/controls/TouchStickUpdate.s"), "-o", str(elfPath),
        ]
        subprocess.run(command, check=True)
        symbols = {}
        for line in subprocess.check_output(["llvm-nm", "--defined-only", str(elfPath)], text=True).splitlines():
            parts = line.split()
            if len(parts) == 3:
                symbols[parts[2]] = int(parts[0], 16)

        lines = ["#pragma once", "", '#include "core/Bytes.h"', "", "namespace Mc3ds {"]
        for name in ["TouchInputHook", "TouchHudDestroyHook", "DrawTouchStick"]:
            lines.append(f"    inline constexpr std::uint32_t update{name}Address = 0x{symbols[name]:x};")

        for section, offset in [("Main", 0x25c750), ("Hooks", 0x818824), ("Disk", 0x818944)]:
            binaryPath = temporaryPath / f"touch{section}.bin"
            subprocess.run(["llvm-objcopy", "-O", "binary", f"--only-section=.touch{section}", str(elfPath), str(binaryPath)], check=True)
            payload = binaryPath.read_bytes()
            lines += [
                f"    inline constexpr std::size_t updateTouch{section}Offset = 0x{offset:x};",
                f'    inline const std::string updateTouch{section}Hash = "{hashlib.sha256(payload).hexdigest()}";',
                f"    inline const std::string updateTouch{section}Hex =",
            ]
            hexData = payload.hex()
            for index in range(0, len(hexData), 96):
                suffix = ";" if index + 96 >= len(hexData) else ""
                lines.append(f'        "{hexData[index:index + 96]}"{suffix}')
            if arguments.artifactsPath is not None:
                arguments.artifactsPath.mkdir(parents=True, exist_ok=True)
                (arguments.artifactsPath / binaryPath.name).write_bytes(payload)
            print(f"Touch {section}: {len(payload)} bytes")

        if arguments.artifactsPath is not None:
            (arguments.artifactsPath / "touch.elf").write_bytes(elfPath.read_bytes())
            (arguments.artifactsPath / "symbols.json").write_text(json.dumps(symbols, indent=4) + "\n")

    lines += ["", "    struct TTouchGuard {", "        const char *name;", "        std::size_t offset;",
              "        std::size_t size;", "        const char *hash;", "    };", "",
              "    inline const TTouchGuard updateTouchGuards[] = {"]
    guards = (
        ("retired frame-timer quad and label helpers", 0x35c74c, 0x390),
        ("controller loop and touch dispatch", 0x107f30, 0x3a4),
        ("controller sample layout", 0x10b4dc, 0x15c),
        ("touch sampling", 0x112ce4, 0xc0),
        ("current client screen", 0x12d544, 8),
        ("bottom HUD renderer", 0x6762bc, 0xc78),
        ("HUD render dispatch", 0x678fd0, 0x1ec),
        ("HUD destructor", 0x679340, 0x3e0),
        ("GUI color state", 0x1a9948, 0x30),
        ("GUI tessellator begin", 0x1b1bf0, 0x218),
        ("GUI tessellator color", 0x1b1f00, 0xd4),
        ("GUI tessellator vertex", 0x1b2444, 0x3fc),
        ("GUI tessellator draw", 0x1b1904, 0x7c),
        ("native solid GUI quad", 0x6cff68, 0xb8),
    )
    for name, address, size in guards:
        offset = address - 0x100000
        digest = hashlib.sha256(reference[offset:offset + size]).hexdigest()
        lines.append(f'        {{"{name}", 0x{offset:x}, 0x{size:x}, "{digest}"}},')
    lines += ["    };", "}", ""]
    content = "\n".join(lines)
    target = projectPath / "src/data/TouchStickData.h"
    if arguments.check:
        if target.read_text() != content:
            raise RuntimeError("TouchStickData.h does not match the rebuilt payload")
    else:
        target.write_text(content)


if __name__ == "__main__":
    Main()
