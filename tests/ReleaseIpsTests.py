import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


projectPath = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(projectPath / "scripts"))

import checkIpsRelease
import prepareIpsRelease


class CReleaseIpsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mc3ds-release-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.directory = self.root / ("v" + checkIpsRelease.releaseVersion)
        self.directory.mkdir()
        self.patch = (projectPath / "release/ips" / self.directory.name / "code.ips").read_bytes()
        (self.directory / "code.ips").write_bytes(self.patch)

    def testSingleAsset(self):
        checksum = checkIpsRelease.CheckReleaseDirectory(self.directory)
        self.assertEqual(checksum, checkIpsRelease.releaseIpsHash + "  code.ips\n")

    def testLegacyFilesAreNotReleaseInputs(self):
        (self.directory / "code-update-controls.ips").write_bytes(b"not a release input")
        (self.directory / "SHA256SUMS.txt").write_text("obsolete local manifest")
        self.assertEqual(checkIpsRelease.CheckReleaseDirectory(self.directory),
                         checkIpsRelease.releaseIpsHash + "  code.ips\n")

    def testMissingMainAsset(self):
        (self.directory / "code.ips").rename(self.directory / "code-update-controls.ips")
        with self.assertRaises(FileNotFoundError):
            checkIpsRelease.CheckReleaseDirectory(self.directory)

    def testWrongDirectoryVersion(self):
        with self.assertRaisesRegex(RuntimeError, "directory does not match"):
            checkIpsRelease.CheckReleaseDirectory(self.root / "v0.0.0")

    def testUnvalidatedProjectVersion(self):
        with mock.patch.object(checkIpsRelease, "releaseVersion", "0.0.0"):
            with self.assertRaisesRegex(RuntimeError, "validated release IPS hashes"):
                checkIpsRelease.ReadReleaseVersion()

    def testCorruptAndOlderPayloads(self):
        for patch in (b"", b"PATCHEOF", self.patch[:-1], self.patch + b"extra",
                      self.patch[:10] + bytes([self.patch[10] ^ 1]) + self.patch[11:]):
            with self.subTest(size=len(patch)):
                with self.assertRaises(RuntimeError):
                    checkIpsRelease.ValidateReleaseIps(patch)

    def testChecksumCliAndNoOverwrite(self):
        output = self.root / "code.ips.sha256"
        command = [sys.executable, str(projectPath / "scripts/checkIpsRelease.py"),
                   "--directory", str(self.directory), "--checksum-output", str(output)]
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        expected = (checkIpsRelease.releaseIpsHash + "  code.ips\n").encode("ascii")
        self.assertEqual(output.read_bytes(), expected)
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output.read_bytes(), expected)

    def testNoChecksumForBadPatch(self):
        (self.directory / "code.ips").write_bytes(b"PATCHEOF")
        output = self.root / "code.ips.sha256"
        result = subprocess.run([sys.executable, str(projectPath / "scripts/checkIpsRelease.py"),
                                 "--directory", str(self.directory), "--checksum-output", str(output)],
                                capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())

    def testPrepareRejectsNonReferenceStock(self):
        stock = self.root / "stock.bin"
        stock.write_bytes(b"wrong stock executable")
        output = self.root / "new" / self.directory.name
        with self.assertRaisesRegex(RuntimeError, "stock v9.11.0"):
            prepareIpsRelease.PrepareRelease(self.directory / "code.ips", stock, output)
        self.assertFalse(output.exists())

    def testPrepareRoundTripAndNoOverwrite(self):
        stock = self.root / "stock.bin"
        stock.write_bytes(bytes(9674752))
        output = self.root / "new" / self.directory.name
        patched = prepareIpsRelease.ApplyIps(stock.read_bytes(), self.patch)
        with mock.patch.object(prepareIpsRelease, "referenceHash", hashlib.sha256(stock.read_bytes()).hexdigest()):
            with self.assertRaisesRegex(RuntimeError, "validated release executable"):
                prepareIpsRelease.PrepareRelease(self.directory / "code.ips", stock, output)
            self.assertFalse(output.exists())
            with mock.patch.object(prepareIpsRelease, "releaseCodeHash", hashlib.sha256(patched).hexdigest()):
                prepareIpsRelease.PrepareRelease(self.directory / "code.ips", stock, output)
                self.assertEqual((output / "code.ips").read_bytes(), self.patch)
                self.assertEqual([path.name for path in output.iterdir()], ["code.ips"])
                with self.assertRaises(FileExistsError):
                    prepareIpsRelease.PrepareRelease(self.directory / "code.ips", stock, output)
                self.assertEqual((output / "code.ips").read_bytes(), self.patch)


if __name__ == "__main__":
    unittest.main()
