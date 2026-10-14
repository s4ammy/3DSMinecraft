import argparse
import hashlib
from pathlib import Path

from buildNativeMenuPrototype import ApplyIps, referenceHash
from checkIpsRelease import ReadReleaseVersion, ValidateReleaseIps, releaseCodeHash


def PrepareRelease(ipsPath, stockCodePath, outputPath):
    if outputPath.name != "v" + ReadReleaseVersion():
        raise RuntimeError("The IPS directory does not match the project version")

    patch = ipsPath.read_bytes()
    ValidateReleaseIps(patch)
    stock = stockCodePath.read_bytes()
    if hashlib.sha256(stock).hexdigest() != referenceHash:
        raise RuntimeError("Expected the supported stock v9.11.0 update executable")

    if hashlib.sha256(ApplyIps(stock, patch)).hexdigest() != releaseCodeHash:
        raise RuntimeError("The IPS does not produce the validated release executable")

    outputPath.mkdir(parents=True, exist_ok=True)
    with (outputPath / "code.ips").open("xb") as outputFile:
        outputFile.write(patch)


def Main():
    parser = argparse.ArgumentParser(description="Prepare the single validated native-menu/chunk-settings release IPS")
    parser.add_argument("--ips", type=Path, required=True)
    parser.add_argument("--stock-code", dest="stockCode", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    PrepareRelease(arguments.ips, arguments.stockCode, arguments.output)
    print("Prepared " + str(arguments.output / "code.ips"))


if __name__ == "__main__":
    Main()
