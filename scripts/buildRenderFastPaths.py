import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from buildRuntimeControls import projectPath, referenceHash


regions = [("vertex", 0x1b0f68, 0xc4), ("spriteU", 0x709544, 0x60), ("spriteV", 0x7095d0, 0x60)]


def Main():
    parser = argparse.ArgumentParser(description="Build allocation-free EU rendering replacements in their original function slots")
    parser.add_argument("--reference-code", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    stock = args.reference_code.read_bytes()
    if hashlib.sha256(stock).hexdigest() != referenceHash:
        raise ValueError("Expected exact supported EU update")
    args.output.mkdir(parents=True, exist_ok=True)
    flags = ["--target=arm-none-eabi", "-march=armv6k", "-marm", "-mfpu=vfpv2", "-mfloat-abi=hard"]
    with tempfile.TemporaryDirectory(prefix="mc3ds-render-fast-") as temporary:
        objectPath = Path(temporary) / "render.o"
        elfPath = args.output / "render.elf"
        subprocess.run(["clang", *flags, "-c", str(projectPath / "patches/rendering/RenderFastPaths.s"), "-o", str(objectPath)], check=True)
        subprocess.run(["clang", *flags, "-nostdlib", "-fuse-ld=lld", "-Wl,--no-undefined", "-Wl,-e,AppendColorUv",
            f"-Wl,-T,{projectPath / 'patches/rendering/RenderFastPaths.ld'}", str(objectPath), "-o", str(elfPath)], check=True)
        report = []
        for name, address, capacity in regions:
            path = args.output / f"{name}.bin"
            subprocess.run(["llvm-objcopy", "-O", "binary", f"--only-section=.{name}", str(elfPath), str(path)], check=True)
            payload = path.read_bytes()
            if not 0 < len(payload) <= capacity:
                raise ValueError(f"Invalid {name} size")
            report.append({"name": name, "address": address, "capacity": capacity, "size": len(payload),
                "stockSha256": hashlib.sha256(stock[address - 0x100000:address - 0x100000 + capacity]).hexdigest(),
                "payloadSha256": hashlib.sha256(payload).hexdigest()})
        (args.output / "regions.json").write_text(json.dumps(report, indent=4) + "\n")
        print(json.dumps(report, indent=4))


if __name__ == "__main__":
    Main()
