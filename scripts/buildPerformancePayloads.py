import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
referenceHash = "902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65"


def Compile(sourceName, address, entry, temporaryPath, cpp=False, linkedSymbols=None):
    outputPath = temporaryPath / (sourceName + ".elf")
    command = ["clang", "--target=arm-none-eabi", "-march=armv6k", "-marm"]
    if cpp:
        command += ["-mfloat-abi=soft", "-O2", "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables"]
    command += ["-Wl,--no-undefined"]
    for name, value in (linkedSymbols or {}).items():
        command += [f"-Wl,--defsym,{name}={value:#x}"]
    command += ["-nostdlib", "-fuse-ld=lld", f"-Wl,-Ttext={address:#x}", f"-Wl,-e,{entry}", str(projectPath / "patches" / sourceName), "-o", str(outputPath)]
    subprocess.run(command, check=True)
    binaryPath = temporaryPath / (sourceName + ".bin")
    subprocess.run(["llvm-objcopy", "-O", "binary", "--only-section=.text", str(outputPath), str(binaryPath)], check=True)
    symbols = {}
    for line in subprocess.check_output(["llvm-nm", "--defined-only", "--extern-only", str(outputPath)], text=True).splitlines():
        parts = line.split()
        if len(parts) == 3:
            symbols[parts[2]] = int(parts[0], 16) - address
    return binaryPath.read_bytes(), symbols


def Main():
    parser = argparse.ArgumentParser(description="Rebuild checked-in ARM payload data. Requires Clang, LLD, llvm-objcopy and a private stock update executable.")
    parser.add_argument("--reference-code", dest="referenceCode", required=True, type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--artifacts-dir", dest="artifactsPath", type=Path)
    args = parser.parse_args()
    reference = args.referenceCode.read_bytes()
    if hashlib.sha256(reference).hexdigest() != referenceHash:
        raise RuntimeError("The input is not the supported stock update executable")
    with tempfile.TemporaryDirectory(prefix="mc3ds-performance-build-") as temporaryDirectory:
        temporaryPath = Path(temporaryDirectory)
        payload, symbols = Compile("PerformanceOptimizations.s", 0x918BC0, "PerformancePayloadStart", temporaryPath)
        block, _ = Compile("BlockLookup.cpp", 0x174648, "_ZN11Performance10GetBlockIdEPhPNS_12TBlockSourceEPKNS_9TBlockPosE", temporaryPath, cpp=True)
        light, _ = Compile("LightLookup.cpp", 0x918BC0 + len(payload), "GetChunkLight", temporaryPath, cpp=True)
        heapSearch, _ = Compile("HeapFreeSearch.s", 0x35D5F0, "FindFreeBlockNeighbors", temporaryPath,
            linkedSymbols={"OriginalSearch": 0x123BDC, "CoalesceNext": 0x123C04, "CoalescePrevious": 0x123C38})
        alignedAllocation, _ = Compile("AllocateAligned4.s", 0x35D658, "AllocateAligned4", temporaryPath,
            linkedSymbols={"AllocationFailed": 0x112670, "OriginalForwardSearch": 0x112578, "CommitSelectedBlock": 0x1125C8})
    if len(payload) + len(light) > 0x310 or len(block) != 0xE8 or len(payload) % 4 or len(heapSearch) != 68 or len(alignedAllocation) != 72:
        raise RuntimeError("Payload layout changed. Review hook offsets and expected output hashes before regenerating.")
    if args.artifactsPath is not None:
        args.artifactsPath.mkdir(parents=True, exist_ok=True)
        (args.artifactsPath / "performance.bin").write_bytes(payload)
        (args.artifactsPath / "block-lookup.bin").write_bytes(block)
        (args.artifactsPath / "light-lookup.bin").write_bytes(light)
        (args.artifactsPath / "heap-search.bin").write_bytes(heapSearch)
        (args.artifactsPath / "aligned-allocation.bin").write_bytes(alignedAllocation)
        (args.artifactsPath / "symbols.json").write_text(json.dumps(symbols, indent=4) + "\n")
    lines = ["#pragma once", "", '#include "Bytes.h"', "", "namespace Mc3ds {"]
    lines += [f"    inline constexpr std::size_t updatePerformanceSize = 0x{len(payload):x};", f"    inline constexpr std::size_t updateLightLookupSize = 0x{len(light):x};", f"    inline constexpr std::size_t updatePayloadSize = 0x{len(payload) + len(light):x};", ""]
    lines += ["    inline constexpr std::size_t updateHeapSearchOffset = 0x25d5f0;", "    inline constexpr std::size_t updateHeapSearchHookOffset = 0x23bd8;", f"    inline constexpr std::size_t updateHeapSearchSize = 0x{len(heapSearch):x};", ""]
    lines += ["    inline constexpr std::size_t updateAlignedAllocationOffset = 0x25d658;", "    inline constexpr std::size_t updateAlignedAllocationHookOffset = 0x12574;", f"    inline constexpr std::size_t updateAlignedAllocationSize = 0x{len(alignedAllocation):x};", ""]
    for name, offset in sorted(symbols.items()):
        lines.append(f"    inline constexpr std::size_t update{name}Offset = 0x{offset:x};")
    lines.append("")
    for name, data in (("updatePerformance", payload), ("updateBlockLookup", block), ("updateLightLookup", light), ("updateHeapSearch", heapSearch), ("updateAlignedAllocation", alignedAllocation)):
        lines += [f'    inline const std::string {name}Hash = "{hashlib.sha256(data).hexdigest()}";', f"    inline const std::string {name}Hex ="]
        hexData = data.hex()
        for index in range(0, len(hexData), 96):
            suffix = ";" if index + 96 >= len(hexData) else ""
            lines.append(f'        "{hexData[index:index + 96]}"{suffix}')
        lines.append("")
    lines += ["    struct TPerformanceGuard {", "        const char *name;", "        std::size_t offset;", "        std::size_t size;", "        const char *hash;", "    };", "", "    inline const TPerformanceGuard updatePerformanceGuards[] = {"]
    for name, offset, size in (("block getter", 0x74648, 0x144), ("chunk cache resolver", 0x7AB3C, 0x78), ("climate edge pass", 0xBA7D8, 0x25C), ("chunk record reader", 0xBEB80, 0x23C), ("chunk decompressor", 0x2DDD08, 0x220), ("shared string replacement", 0x101120 - 0x100000, 372), ("shared string allocation", 0x2FEB28 - 0x100000, 118), ("zlib stream layout", 0x7C3CC4 - 0x100000, 168), ("chunk light getter", 0x57240, 0xac), ("subchunk index", 0x5e3d2c, 0x7c), ("packed light getter", 0x6346dc, 0x20), ("biome color pass", 0xc7ac8, 0x49c), ("inflate init", 0x6c3b6c, 0x158), ("inflate end", 0x6c3b20, 0x4c)):
        digest = hashlib.sha256(reference[offset:offset + size]).hexdigest()
        lines.append(f'        {{"{name}", 0x{offset:x}, 0x{size:x}, "{digest}"}},')
    for name, offset, size in (("SDK heap free and coalescing", 0x23B98, 0x138), ("locked arena free wrapper", 0x1BBF4, 0x20),
            ("SDK aligned allocation", 0x12530, 0x148), ("SDK split and commit", 0x1BCD4, 0x1DC),
            ("arena allocation options", 0xBA88, 0x64), ("four-byte allocation wrapper", 0x1493C, 0x10)):
        digest = hashlib.sha256(reference[offset:offset + size]).hexdigest()
        lines.append(f'        {{"{name}", 0x{offset:x}, 0x{size:x}, "{digest}"}},')
    lines += ["    };", "}", ""]
    content = "\n".join(lines)
    target = projectPath / "src/PerformanceData.h"
    if args.check:
        if target.read_text() != content:
            raise RuntimeError("PerformanceData.h does not match the rebuilt payloads")
        print("Performance payload data is reproducible")
    else:
        target.write_text(content)
        print(f"Generated {target}")


if __name__ == "__main__":
    Main()
