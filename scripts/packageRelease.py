import argparse
import hashlib
import os
import re
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
from pathlib import Path


def FetchSource(tag, checksum, destination):
    archivePath = destination / (tag + ".tar.gz")
    if not archivePath.exists():
        urllib.request.urlretrieve(
            "https://codeload.github.com/3DSGuy/Project_CTR/tar.gz/refs/tags/" + tag,
            archivePath)

    if hashlib.sha256(archivePath.read_bytes()).hexdigest() != checksum:
        raise RuntimeError("Upstream source checksum mismatch: " + tag)

    return archivePath


def PackageRelease():
    parser = argparse.ArgumentParser(description="Package the patcher, helper tools, and their notices")
    parser.add_argument("--platform", choices=["linux", "windows"], required=True)
    parser.add_argument("--binary-dir", dest="binaryDir", type=Path, required=True)
    parser.add_argument("--version", default=os.environ.get("RELEASE_VERSION", "dev"))
    parser.add_argument("--output", type=Path, default=Path("dist"))
    arguments = parser.parse_args()
    projectPath = Path(__file__).resolve().parents[1]
    version = arguments.version
    if version != "dev":
        if not re.fullmatch(r"v\d+\.\d+\.\d+(?:-[A-Za-z0-9]+(?:[.-][A-Za-z0-9]+)*)?", version):
            raise RuntimeError("Use dev or a version tag such as v0.3.0 or v0.3.0-rc1")

        projectVersion = re.search(r"project\(MinecraftOld3dsPatcher VERSION ([0-9.]+)",
                                  (projectPath / "CMakeLists.txt").read_text()).group(1)
        if version[1:].split("-", 1)[0] != projectVersion:
            raise RuntimeError("Release tag does not match the CMake project version")

    binaryPath = arguments.binaryDir.resolve()
    outputPath = arguments.output.resolve()
    outputPath.mkdir(parents=True, exist_ok=True)
    extension = ".exe" if arguments.platform == "windows" else ""
    packageName = "mc3ds-patcher-" + version + "-" + arguments.platform + "-x64"
    archiveFormat = "zip" if arguments.platform == "windows" else "gztar"
    archiveSuffix = ".zip" if arguments.platform == "windows" else ".tar.gz"
    if (outputPath / (packageName + archiveSuffix)).exists():
        raise RuntimeError("Release archive already exists")

    sourcePath = binaryPath / "tools/sources"
    sourcePath.mkdir(parents=True, exist_ok=True)
    sources = [
        FetchSource("ctrtool-v1.3.0", "6c0314928dea722f769cfa7257a963df0d7503962b2998a8c91ec6879fb86075", sourcePath),
        FetchSource("makerom-v0.19.0", "446bd23919b7e9fa10540a784202d388a0b93ef4d7165f3990481edd2aa2f946", sourcePath),
    ]
    with tempfile.TemporaryDirectory(prefix="minecraft-release-") as temporaryDirectory:
        stagingPath = Path(temporaryDirectory) / packageName
        (stagingPath / "tools/sources").mkdir(parents=True)
        (stagingPath / "licenses").mkdir()
        for relativeName in ["mc3ds-patcher", "tools/ctrtool", "tools/makerom"]:
            shutil.copy2(binaryPath / (relativeName + extension), stagingPath / (relativeName + extension))
            (stagingPath / (relativeName + extension)).chmod(0o755)

        for name in ["README.md", "COMPATIBILITY.md", "INPUTS_EXPLAINED.md", "PATCHES.md", "THIRD_PARTY.md", "LICENSE"]:
            shutil.copy2(projectPath / name, stagingPath / name)

        shutil.copy2(projectPath / "licenses/GPL-3.0.txt", stagingPath / "licenses/GPL-3.0.txt")
        for archivePath in sources:
            shutil.copy2(archivePath, stagingPath / "tools/sources" / archivePath.name)
            with tarfile.open(archivePath) as archive:
                for member in archive.getmembers():
                    parts = Path(member.name).parts[1:]
                    if not member.isfile() or not parts or parts[-1] not in ["LICENSE", "LICENSE.rst", "NOTICE"]:
                        continue

                    if any(part in ["..", "."] for part in parts):
                        raise RuntimeError("Invalid upstream notice path")

                    noticePath = stagingPath / "licenses" / archivePath.name.removesuffix(".tar.gz") / Path(*parts)
                    noticePath.parent.mkdir(parents=True, exist_ok=True)
                    noticePath.write_bytes(archive.extractfile(member).read())

        commandPrefix = ["wine"] if arguments.platform == "windows" and os.name != "nt" else []
        programs = [
            ("mc3ds-patcher", "--help", "Minecraft Old 3DS Patcher", 0),
            ("tools/ctrtool", "-h", "CTRTool v1.3.0", 1),
            ("tools/makerom", "-help", "CTR MAKEROM v0.19.0", 254),
        ]
        for name, option, marker, expectedExit in programs:
            command = commandPrefix + [str(stagingPath / (name + extension)), option]
            result = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=60)
            if result.returncode & 255 != expectedExit or marker not in result.stdout + result.stderr:
                raise RuntimeError("Packaged program did not start: " + name + "\n" + result.stdout + result.stderr)

            print("Packaged program checked: " + name, flush=True)

        archivePath = Path(shutil.make_archive(str(outputPath / packageName), archiveFormat,
                                              root_dir=temporaryDirectory, base_dir=packageName))

    checksum = hashlib.sha256(archivePath.read_bytes()).hexdigest()
    archivePath.with_name(archivePath.name + ".sha256").write_text(checksum + "  " + archivePath.name + "\n", encoding="ascii")
    print("Release package: " + str(archivePath))


if __name__ == "__main__":
    PackageRelease()
