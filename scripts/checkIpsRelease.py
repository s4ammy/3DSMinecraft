import argparse
import hashlib
import re
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
releaseVersion = "0.4.19"
releaseIpsHash = "46196cd8db74a83d3fcfbf1ea09400a27ecb60b09ae0e726f151d03d8d907e2b"
releaseCodeHash = "01138f3230df04cbc40bda87f920f9bac7e41706e8042b2e25695a6f4985ea34"


def ReadReleaseVersion():
    versionMatch = re.search(r"project\(MinecraftOld3dsPatcher VERSION ([0-9.]+)",
                             (projectPath / "CMakeLists.txt").read_text())
    if versionMatch is None or versionMatch.group(1) != releaseVersion:
        raise RuntimeError("Update the validated release IPS hashes for the current project version")

    return releaseVersion


def ValidateReleaseIps(data):
    if not data.startswith(b"PATCH") or not data.endswith(b"EOF"):
        raise RuntimeError("Invalid IPS file: code.ips")

    if hashlib.sha256(data).hexdigest() != releaseIpsHash:
        raise RuntimeError("code.ips does not match the validated native-menu/chunk-settings release")


def CheckReleaseDirectory(directory):
    if directory.name != "v" + ReadReleaseVersion():
        raise RuntimeError("The IPS directory does not match the project version")

    ValidateReleaseIps((directory / "code.ips").read_bytes())
    return releaseIpsHash + "  code.ips\n"


def Main():
    parser = argparse.ArgumentParser(description="Check the single validated update code.ips release asset")
    parser.add_argument("--directory", type=Path)
    parser.add_argument("--checksum-output", dest="checksumOutput", type=Path)
    arguments = parser.parse_args()
    directory = arguments.directory or projectPath / "release/ips" / ("v" + ReadReleaseVersion())
    checksum = CheckReleaseDirectory(directory)
    if arguments.checksumOutput is not None:
        with arguments.checksumOutput.open("x", encoding="ascii", newline="\n") as outputFile:
            outputFile.write(checksum)

    print("Checked the unified update code.ips release asset")


if __name__ == "__main__":
    Main()
