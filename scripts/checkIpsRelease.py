import argparse
import re
from pathlib import Path

from prepareIpsRelease import FileHash, profiles


def CheckIpsRelease():
    parser = argparse.ArgumentParser(description="Check the six prepared IPS release assets")
    parser.add_argument("--directory", type=Path, required=True)
    arguments = parser.parse_args()

    projectPath = Path(__file__).resolve().parents[1]
    versionMatch = re.search(r"project\(MinecraftOld3dsPatcher VERSION ([0-9.]+)",
                             (projectPath / "CMakeLists.txt").read_text())
    if versionMatch is None or arguments.directory.name != "v" + versionMatch.group(1):
        raise RuntimeError("The IPS directory does not match the project version")

    expectedNames = {"code-" + name + ".ips" for name, _, _, _ in profiles}
    actualNames = {path.name for path in arguments.directory.iterdir() if path.is_file()}
    if actualNames != expectedNames | {"SHA256SUMS.txt"}:
        raise RuntimeError("The IPS directory must contain exactly six profiles and SHA256SUMS.txt")

    lines = (arguments.directory / "SHA256SUMS.txt").read_text(encoding="ascii").splitlines()
    checksums = {}
    for line in lines:
        match = re.fullmatch(r"([0-9a-f]{64})  (code-[a-z-]+\.ips)", line)
        if match is None or match.group(2) in checksums:
            raise RuntimeError("Invalid IPS checksum manifest")

        checksums[match.group(2)] = match.group(1)

    if set(checksums) != expectedNames:
        raise RuntimeError("The IPS checksum manifest does not list every profile")

    for name in sorted(expectedNames):
        path = arguments.directory / name
        data = path.read_bytes()
        if not data.startswith(b"PATCH") or not data.endswith(b"EOF"):
            raise RuntimeError("Invalid IPS file: " + name)

        if FileHash(path) != checksums[name]:
            raise RuntimeError("IPS checksum mismatch: " + name)

    print("Checked six IPS release assets")


if __name__ == "__main__":
    CheckIpsRelease()
