import argparse
import hashlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


profiles = (
    ("base-l-circle-pad", "base", "patchedCodeHash", ()),
    ("base-circle-pad-pro", "base", "circlePadProCodeHash", ("--controls", "circle-pad-pro")),
    ("update-l-circle-pad", "update", "updateLCirclePadCodeHash", ()),
    ("update-circle-pad-pro", "update", "updateCirclePadProCodeHash", ("--controls", "circle-pad-pro")),
    ("update-l-circle-pad-overlay", "update", "updateLCirclePadOverlayCodeHash", ("--overlay",)),
    ("update-circle-pad-pro-overlay", "update", "updateCirclePadProOverlayCodeHash",
     ("--controls", "circle-pad-pro", "--overlay")),
)


def ReadHash(data, name):
    match = re.search(r'\b' + re.escape(name) + r' = "([0-9a-f]{64})";', data)
    if match is None:
        raise RuntimeError("Missing reference hash: " + name)

    return match.group(1)


def FileHash(path):
    digest = hashlib.sha256()
    with path.open("rb") as inputFile:
        for chunk in iter(lambda: inputFile.read(1024 * 1024), b""):
            digest.update(chunk)

    return digest.hexdigest()


def PrepareIpsRelease():
    parser = argparse.ArgumentParser(description="Generate the six release IPS files from private reference CIAs")
    parser.add_argument("--patcher", type=Path, required=True)
    parser.add_argument("--base-cia", dest="baseCia", type=Path, required=True)
    parser.add_argument("--update-cia", dest="updateCia", type=Path, required=True)
    parser.add_argument("--seed-file", dest="seedFile", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()

    projectPath = Path(__file__).resolve().parents[1]
    cmakeData = (projectPath / "CMakeLists.txt").read_text()
    versionMatch = re.search(r"project\(MinecraftOld3dsPatcher VERSION ([0-9.]+)", cmakeData)
    if versionMatch is None:
        raise RuntimeError("Cannot read the project version")

    version = versionMatch.group(1)
    outputPath = arguments.output.resolve()
    if outputPath.exists():
        raise RuntimeError("Release IPS output already exists: " + str(outputPath))

    patcherPath = arguments.patcher.resolve(strict=True)
    helpResult = subprocess.run([str(patcherPath), "--help"], capture_output=True, text=True, check=True)
    if "Minecraft Old 3DS Patcher " + version + "\n" not in helpResult.stdout:
        raise RuntimeError("The patcher binary does not match the source version " + version)

    patchData = (projectPath / "src/Patches.h").read_text()
    inputs = {"base": arguments.baseCia.resolve(strict=True), "update": arguments.updateCia.resolve(strict=True)}
    for kind, hashName in (("base", "testedCiaHash"), ("update", "updateCiaHash")):
        if FileHash(inputs[kind]) != ReadHash(patchData, hashName):
            raise RuntimeError("The " + kind + " CIA is not the project's reference input")

    seedPath = arguments.seedFile.resolve(strict=True)
    if seedPath.stat().st_size != 16:
        raise RuntimeError("The base title seed must be exactly 16 bytes")

    with tempfile.TemporaryDirectory(prefix="mc3ds-release-ips-") as temporaryDirectory:
        stagingPath = Path(temporaryDirectory)
        checksums = []
        for name, kind, expectedHashName, options in profiles:
            workPath = stagingPath / name
            command = [str(patcherPath), str(inputs[kind]), "--output", str(workPath)]
            if kind == "base":
                command.extend(("--seed-file", str(seedPath)))

            command.extend(options)
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode != 0:
                raise RuntimeError(name + " failed:\n" + result.stdout + result.stderr)

            report = (workPath / "report.txt").read_text()
            expectedHash = ReadHash(patchData, expectedHashName)
            if "Patched executable SHA-256 after IPS: " + expectedHash + "\n" not in report:
                raise RuntimeError(name + " did not produce the expected executable")

            patchPath = workPath / "code.ips"
            patchHash = FileHash(patchPath)
            if "code.ips SHA-256: " + patchHash + "\n" not in report:
                raise RuntimeError(name + " did not produce the expected IPS checksum")

            releaseName = "code-" + name + ".ips"
            shutil.copy2(patchPath, stagingPath / releaseName)
            checksums.append(patchHash + "  " + releaseName)
            print(releaseName + ": " + patchHash, flush=True)

        outputPath.mkdir(parents=True)
        for checksum in checksums:
            releaseName = checksum.split("  ", 1)[1]
            shutil.copy2(stagingPath / releaseName, outputPath / releaseName)

        (outputPath / "SHA256SUMS.txt").write_text("\n".join(checksums) + "\n", encoding="ascii")


if __name__ == "__main__":
    PrepareIpsRelease()
