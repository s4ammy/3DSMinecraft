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
    ("Dictation", 0x41efa8, 0x3a48, ".text.BuildDictationBindings"),
    ("Main", 0x4a403c, 0x1350, ".text.BuildKeyboardBindings"),
    ("Modified", 0x710a14, 0x600, ".text.BuildModifiedKeyboardBindings"),
    ("Shared", 0x711014, 0x5d0, ".text.BuildSharedKeyboardBindings"),
    ("Extra", 0x35c750, 0x38c, None),
    ("Hooks", 0x918824, 0x100, ".controlsHooks"),
    ("Disk", 0x918944, 0x14c, ".text._ZN10TouchStick8DrawDiskEfffPKf"),
]
nativeReservations = reservations + [
    ("ChunkInit", 0x7bbcf0, 0x7bbe64 - 0x7bbcf0, ".text.InitializeRuntimeChunks"),
    ("ChunkTail", 0x7bbf28, 0x7bc084 - 0x7bbf28, None),
    ("LightRead", 0x384900, 0xac, ".text.ReadPackedLight"),
    ("Overlay", 0x35cae0, 0x35d5f0 - 0x35cae0, ".text.DrawDebugOverlay"),
    ("ReadOnly", 0xa28464, 0xa29000 - 0xa28464, None),
]
entryPoints = ["RuntimeInputHook", "RuntimeHudDestroyHook", "RuntimeAccessoryHook", "RuntimeShutdownHook",
               "BuildControlRows", "HandleControlButton", "DispatchControlRelease", "DestroyControlRows", "DrawRuntimeControls",
               "LoadControlSettings", "SaveControlSettings", "RenderRuntimeParticles", "RuntimeWeatherHook", "RuntimeMapHook",
               "RuntimeTextureHook", "SkipHiddenBlockFaces", "DrawRuntimePrompt", "GetBiomeFromCachedChunk", "LimitDeferredBuilderPool"]


def Main():
    parser = argparse.ArgumentParser(description="Build the unified update controls payload with stock ABI guards")
    parser.add_argument("--reference-code", type=Path, required=True)
    parser.add_argument("--artifacts-dir", type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--native-menu", action="store_true", help="Build the separate native menu prototype")
    parser.add_argument("--output-header", type=Path)
    arguments = parser.parse_args()
    selectedReservations = nativeReservations if arguments.native_menu else reservations
    if arguments.native_menu and arguments.output_header is None:
        raise RuntimeError("The native menu prototype requires a separate --output-header")
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
        reusedStrings = {}
        sources = ["RuntimeControls.cpp", "ControlSettings.cpp", "CompactBindings.cpp", "CompactDictation.cpp", "TouchStick.cpp", "TouchStickInput.cpp", "RuntimeControls.s", "RuntimeVisuals.cpp", "TextureSettings.cpp", "RuntimeVisuals.s", "RuntimePrompts.cpp", "PromptBindings.cpp", "ChunkMesh.s"]
        if arguments.native_menu:
            flags += ["-DMC3DS_NATIVE_MENU", "-DMC3DS_OVERLAY_BACKGROUND", "-DMC3DS_ADVANCED_OVERLAY"]
            sources += ["NativeMenus.cpp", "NativeMenus.s", "DebugOverlay.cpp", "OverlayMetrics.cpp", "AdvancedOverlay.cpp", "RuntimeWorld.cpp", "RuntimeWorld.s"]
        for source in sources:
            objectPath = temporaryPath / (source + ".o")
            sourcePath = projectPath / "patches" / source
            if arguments.native_menu and source in ("CompactBindings.cpp", "CompactDictation.cpp"):
                def ReuseString(match):
                    value = match[0][1:-1].encode() + b"\0"
                    start = 0
                    while True:
                        offset = reference.find(value, start)
                        if offset < 0:
                            return match[0]
                        start = offset + 1
                        address = offset + 0x100000
                        if offset > 0 and reference[offset - 1] == 0 and not any(address < base + capacity and address + len(value) > base for _, base, capacity, _ in selectedReservations):
                            reusedStrings[hex(address)] = value.hex()
                            return f"reinterpret_cast<const char *>(0x{address:x})"
                content = sourcePath.read_text()
                content = re.sub(r'"(?:button\.[^"]+|closer|back|close up|overview|lightningstrike|lightning strike|lightingstrike|lighting strike|raise|lower|follow|follow player|stop following|next player|time [^"]+|playspace|play space|place screen|playscreen|place world|playsworld|go back|goback|create world|reality mode|wrap around mode|create screen|auto align|autoalign|autoaline|explore|grab|move|rotate|turn|tilt|resize|zoom|pivot left|pivot right|two dee|three dee|[^"]*tee vee|mark that|debug [^"]+|third person)"', ReuseString, content)
                content = content.replace("constexpr ", "const ").replace("const const ", "const ")
                sourcePath = temporaryPath / source
                sourcePath.write_text(content)
            subprocess.run(["clang++", *flags, "-I", str(projectPath / "patches"), "-std=c++17", "-c", str(sourcePath), "-o", str(objectPath)], check=True)
            objects.append(objectPath)
            listing = subprocess.check_output(["llvm-readelf", "-SW", str(objectPath)], text=True)
            for line in listing.splitlines():
                match = re.search(r"\[\s*\d+\]\s+(\S+)\s+PROGBITS\s+\S+\s+\S+\s+([0-9a-fA-F]+)\s+\S+\s+(\S+)\s+\d+\s+\d+\s+(\d+)", line)
                if not match or "A" not in match[3] or int(match[2], 16) == 0:
                    continue
                if match[1] in (".text.UpdateTouchStick", ".text.DrawTouchStick"):
                    continue
                if not arguments.native_menu and match[1] == ".text.ReadPackedLight":
                    continue
                sections.append((source + ".o", match[1], int(match[2], 16), int(match[4])))

        bins = [[] for _ in selectedReservations]
        used = [0 for _ in selectedReservations]
        for index, (_, _, _, entry) in enumerate(selectedReservations):
            if entry is not None:
                section = next(item for item in sections if item[1] == entry)
                sections.remove(section)
                bins[index].append(section)
                used[index] = section[2]
        for section in sorted(sections, key=lambda item: (-item[2], item[0], item[1])):
            order = list(range(len(selectedReservations)))
            if arguments.native_menu and section[1].startswith(".rodata"):
                order.insert(0, order.pop())
            for index in order:
                name, address, capacity, _ = selectedReservations[index]
                #The pinned hook section includes its fixed slots; native builds may use only its trailing reservation.
                if (name == "Hooks" and not arguments.native_menu) or (name == "ReadOnly" and not section[1].startswith(".rodata")):
                    continue
                if address % section[3] != 0:
                    continue
                end = (address + used[index] + section[3] - 1) // section[3] * section[3] - address + section[2]
                if end <= capacity:
                    bins[index].append(section)
                    used[index] = end
                    break
            else:
                raise RuntimeError(f"No guarded space for {section}: used {used}")

        layout = []
        for (name, address, capacity, _), contents in sorted(zip(selectedReservations, bins), key=lambda item: item[0][1]):
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
        for _, address, _, entry in selectedReservations:
            if entry and entry.startswith(".text.") and symbols[entry[6:]] != address:
                raise RuntimeError(f"Native entry point moved: {entry}")
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
            (artifacts / "reused-strings.json").write_text(json.dumps(reusedStrings, indent=4) + "\n")
            recordLayouts = subprocess.check_output(["clang++", *flags, "-std=c++17", "-Xclang", "-fdump-record-layouts",
                "-fsyntax-only", str(projectPath / "patches/RuntimeControls.cpp")], text=True)
            stateLayout = recordLayouts.split("struct RuntimeControls::TState\n", 1)[1].split("\n\n", 1)[0]
            offsets = {}
            for line in stateLayout.splitlines():
                match = re.match(r"\s*(\d+) \|   (?:  )?(?! ).*? (\w+)$", line)
                if match:
                    offsets[match[2]] = int(match[1])
            (artifacts / "state-layout.json").write_text(json.dumps(offsets, indent=4) + "\n")
        for name, address, capacity, _ in selectedReservations:
            binaryPath = temporaryPath / f"controls{name}.bin"
            subprocess.run(["llvm-objcopy", "-O", "binary", f"--only-section=.controls{name}", str(elfPath), str(binaryPath)], check=True)
            payload = binaryPath.read_bytes()
            offset = address - 0x100000
            digest = hashlib.sha256(reference[offset:offset + capacity]).hexdigest()
            lines.append(f'        {{0x{offset:x}, 0x{capacity:x}, "{digest}", "{hashlib.sha256(payload).hexdigest()}",')
            hexData = payload.hex()
            lines += [f'            "{hexData[index:index + 96]}"' for index in range(0, len(hexData), 96)]
            if not hexData:
                lines.append('            ""')
            lines.append("        },")
            if artifacts:
                (artifacts / binaryPath.name).write_bytes(payload)
            print(f"{name}: {len(payload)}/{capacity} bytes")
        lines += ["    };", "}", ""]
        target = arguments.output_header or projectPath / "src/RuntimeControlsData.h"
        content = "\n".join(lines)
        if arguments.check:
            if target.read_text() != content:
                raise RuntimeError("RuntimeControlsData.h differs from its reproducible build")
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content)


if __name__ == "__main__":
    Main()
