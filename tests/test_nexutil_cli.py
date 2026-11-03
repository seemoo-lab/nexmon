#!/usr/bin/env python3
"""Unit tests for nexutil platform detection and auto-buffer sizing."""

import os
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NEXUTIL_BIN = ROOT / "utilities" / "nexutil" / "nexutil"


class TestNexutilCli(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.has_nexutil = NEXUTIL_BIN.is_file() and os.access(NEXUTIL_BIN, os.X_OK)

    def setUp(self):
        if not self.has_nexutil:
            self.skipTest("nexutil binary not built or not executable")

    def test_nexutil_platform_detection(self):
        """Verify that nexutil -V detects the real hardware model on Linux."""
        res = subprocess.run(
            [str(NEXUTIL_BIN), "-V"],
            capture_output=True,
            text=True
        )
        if res.returncode == 0:
            self.assertNotIn("platform             | firmware                         | vendorid | deviceid | radiorev   | chipnum | chiprev | chippackage | corerev | boardid | boardvendor | boardrev | driverrev | ucoderev  | bus | phytype | phyrev | anarev | nvramrev\n-------------------- | -------------------------------- | -------- | -------- | ---------- | ------- | ------- | ----------- | ------- | ------- | ----------- | -------- | --------- | --------- | --- | ------- | ------ | ------ | --------\nunknown", res.stdout)
            self.assertIn("Raspberry Pi", res.stdout)

    def test_nexutil_auto_buffer_sizing(self):
        """Verify that custom queries default to 256 bytes when -l is not given."""
        res_default = subprocess.run(
            [str(NEXUTIL_BIN), "-g", "400"],
            capture_output=True,
            text=True
        )
        if res_default.returncode == 0:
            lines = res_default.stdout.strip().splitlines()
            # 256 bytes dumped at 16 bytes per line = 16 lines
            self.assertGreaterEqual(len(lines), 16)

        res_explicit = subprocess.run(
            [str(NEXUTIL_BIN), "-g", "400", "-l", "4"],
            capture_output=True,
            text=True
        )
        if res_explicit.returncode == 0:
            lines = res_explicit.stdout.strip().splitlines()
            # 4 bytes dumped = 1 line
            self.assertEqual(len(lines), 1)


if __name__ == "__main__":
    unittest.main()
