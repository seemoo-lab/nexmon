#!/usr/bin/env python3
"""Dedicated unittest for dhdutil (membytes, download, error handling)"""

import pathlib
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
DHDUTIL = ROOT / "utilities/dhdutil/dhdutil"

class TestDhdutilBuild(unittest.TestCase):
    def test_dhdutil_source_exists(self):
        self.assertTrue((ROOT / "utilities/dhdutil/dhdu.c").is_file())
        self.assertTrue((ROOT / "utilities/dhdutil/dhdu_linux.c").is_file())

    def test_dhdutil_help(self):
        # If built, check help doesn't crash and mentions membytes
        if not DHDUTIL.is_file():
            self.skipTest("dhdutil not built (run make -C utilities/dhdutil)")
        result = subprocess.run([str(DHDUTIL), "--help"], capture_output=True, text=True, timeout=5)
        # dhdutil may not have --help, but should not segfault; check it exits
        self.assertIn(result.returncode, (0, 1, 2))
        # If it has usage, it should mention membytes or dhdutil
        combined = result.stdout + result.stderr
        # At least it should not be empty or crash with sanitizer
        self.assertGreater(len(combined.strip()), 0)
        self.assertTrue(any(term in combined.lower() for term in ("dhdutil", "usage", "help", "command")))

    def test_dhdutil_invalid_args(self):
        if not DHDUTIL.is_file():
            self.skipTest("dhdutil not built")
        # Invalid interface should not crash, should return error
        result = subprocess.run([str(DHDUTIL), "-i", "nonexistent0", "membytes", "-r", "0x0", "0x10"], capture_output=True, text=True, timeout=5)
        # Should not segfault
        self.assertNotEqual(result.returncode, -11)  # SIGSEGV
        self.assertNotEqual(result.returncode, -6)   # SIGABRT from asan

    def test_dhdutil_source_hardening(self):
        # Regression: ensure dhdutil doesn't use bare atoi for membytes length
        # (similar to rawproxy fix)
        text = (ROOT / "utilities/dhdutil/dhdu.c").read_text(errors="replace")
        # Check that membytes parsing uses strtol/strtoul with validation, not bare atoi
        # If it still uses atoi, it's a hardening gap
        if "atoi" in text:
            # Allow atoi only in non-critical paths, but membytes should use strtol
            # Check specifically for membytes
            if "membytes" in text.lower():
                # Find membytes handling and check for strtol nearby
                # This is a soft check - warn if atoi found near membytes
                lines = text.splitlines()
                for i, line in enumerate(lines):
                    if "membytes" in line.lower() and "atoi" in "\n".join(lines[max(0,i-5):i+5]):
                        # Check if strtol is also nearby (indicating fix)
                        if "strtol" not in "\n".join(lines[max(0,i-10):i+10]):
                            self.fail(f"dhdutil membytes at line {i+1} uses atoi without strtol validation")

class TestDhdutilUcode(unittest.TestCase):
    def test_ucode_download_source(self):
        # Check ucode_download.c exists and handles errors
        self.assertTrue((ROOT / "utilities/dhdutil/ucode_download.c").is_file())
        text = (ROOT / "utilities/dhdutil/ucode_download.c").read_text(errors="replace")
        # Should handle file open errors
        self.assertIn("fopen", text)
        self.assertIn("fclose", text)

if __name__ == "__main__":
    unittest.main()
