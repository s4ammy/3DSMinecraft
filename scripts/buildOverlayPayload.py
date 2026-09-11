import argparse
import hashlib
import json
import struct
import subprocess
import tempfile
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
referenceHash = "902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65"


def Main():
    parser = argparse.ArgumentParser(description="Build the update-specific debug overlay from freestanding C++.")
    parser.add_argument("--reference-code", dest="referenceCode", type=Path, required=True)
    parser.add_argument("--artifacts-dir", dest="artifactsPath", type=Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    reference = arguments.referenceCode.read_bytes()
    if hashlib.sha256(reference).hexdigest() != referenceHash:
        raise RuntimeError("The input is not the supported stock update executable")
    with tempfile.TemporaryDirectory(prefix="mc3ds-overlay-build-") as temporaryDirectory:
        temporaryPath = Path(temporaryDirectory)
        elfPath = temporaryPath / "overlay.elf"
        binaryPath = temporaryPath / "overlay.bin"
        command = ["clang++", "--target=arm-none-eabi", "-march=armv6k", "-marm", "-mfpu=vfpv2", "-mfloat-abi=hard", "-std=c++17", "-Os", "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-ffunction-sections", "-fdata-sections", "-nostdlib", "-fuse-ld=lld", "-Wl,--no-undefined", "-Wl,-e,DrawDebugOverlay", f"-Wl,-T,{projectPath / 'patches/DebugOverlay.ld'}", str(projectPath / "patches/DebugOverlay.cpp"), str(projectPath / "patches/OverlayMetrics.cpp"), "-o", str(elfPath)]
        subprocess.run(command, check=True)
        subprocess.run(["llvm-objcopy", "-O", "binary", "--only-section=.text", str(elfPath), str(binaryPath)], check=True)
        payload = struct.pack("<I", 0xeaffffff) + binaryPath.read_bytes()
        symbols = {}
        for line in subprocess.check_output(["llvm-nm", "--defined-only", "--extern-only", str(elfPath)], text=True).splitlines():
            parts = line.split()
            if len(parts) == 3:
                symbols[parts[2]] = int(parts[0], 16)
        if symbols["DrawDebugOverlay"] != 0x35CAE0 or not 4 < len(payload) <= 0x35D5F0 - 0x35CADC:
            raise RuntimeError(f"Overlay entry {symbols['DrawDebugOverlay']:#x}, size {len(payload):#x} exceeds the space before the heap-search payload")
        if arguments.artifactsPath is not None:
            arguments.artifactsPath.mkdir(parents=True, exist_ok=True)
            (arguments.artifactsPath / "overlay.elf").write_bytes(elfPath.read_bytes())
            (arguments.artifactsPath / "overlay.bin").write_bytes(payload)
            (arguments.artifactsPath / "symbols.json").write_text(json.dumps(symbols, indent=4) + "\n")
    lines = ["#pragma once", "", '#include "Bytes.h"', "", "namespace Mc3ds {"]
    lines += [
        "    inline constexpr std::size_t updateOverlayOffset = 0x25cadc;",
        "    inline constexpr std::size_t updateOverlayCapacity = 0xbd8;",
        "    inline constexpr std::size_t updateOverlayVblankHookOffset = 0x3f903c;",
        f"    inline constexpr std::uint32_t updateOverlayVblankAddress = 0x{symbols['OnDisplayVblankWithOverlayStats']:x};",
        "    inline constexpr std::size_t updateOverlayGateOffset = 0x131230;",
        "    inline constexpr std::uint32_t updateOverlayStateAddress = 0xac3818;",
        "    inline constexpr std::size_t updateOverlayStateCapacity = 112;",
        f'    inline const std::string updateOverlayHash = "{hashlib.sha256(payload).hexdigest()}";',
        "    inline const std::string updateOverlayHex =",
    ]
    hexData = payload.hex()
    for index in range(0, len(hexData), 96):
        suffix = ";" if index + 96 >= len(hexData) else ""
        lines.append(f'        "{hexData[index:index + 96]}"{suffix}')
    lines += ["", "    struct TOverlayGuard {", "        const char *name;", "        std::size_t offset;", "        std::size_t size;", "        const char *hash;", "    };", "", "    inline const TOverlayGuard updateOverlayGuards[] = {"]
    guards = (
        ("stock frame-timer renderer", 0x35CADC, 0xBD8),
        ("overlay frame dispatch", 0x2311B8, 0x258),
        ("font shadow ABI", 0x55B7BC, 0x18),
        ("font drawing ABI", 0x55BB84, 0xE8),
        ("font shared-string lifetime", 0x55DA20, 0x9F0),
        ("system tick wrapper", 0x4CF728, 0x14),
        ("arena memory snapshot", 0x1399A8, 0xF8),
        ("arena free-list traversal", 0x4BB67C, 0x34),
        ("frame submission wrapper", 0x4FA858, 0xBC),
        ("display callback registration", 0x4F8BE0, 0x464),
        ("display VBlank promotion", 0x4F9044, 0xC4),
        ("GPU completion callback", 0x4F9308, 0x84),
        ("SDK VBlank callbacks", 0x7C69A4, 0x30),
        ("CTR frame submission", 0x4F87C4, 0x400),
    )
    for name, address, size in guards:
        offset = address - 0x100000
        digest = hashlib.sha256(reference[offset:offset + size]).hexdigest()
        lines.append(f'        {{"{name}", 0x{offset:x}, 0x{size:x}, "{digest}"}},')
    lines += ["    };", "}", ""]
    content = "\n".join(lines)
    target = projectPath / "src/OverlayData.h"
    if arguments.check:
        if target.read_text() != content:
            raise RuntimeError("OverlayData.h does not match the rebuilt payload")
    else:
        target.write_text(content)
    print(f"Overlay payload: {len(payload)} bytes, SHA-256 {hashlib.sha256(payload).hexdigest()}")


if __name__ == "__main__":
    Main()
