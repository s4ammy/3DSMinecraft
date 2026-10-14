import argparse
import hashlib
import re
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
releaseVersion = "0.4.18"
releaseIpsHash = "46bf9d96f15b281a046cd4d51a2f88053b67c27f9ad6bed7454a9d94546fcb09"
releaseCodeHash = "8216d813b0f3af312a0724feb2503155e62e2be74ec393c58fb0bf34bb91a061"


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
