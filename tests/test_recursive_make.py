#!/usr/bin/env python3
"""Regression tests for recursive Make failure propagation."""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class RecursiveMakeTests(unittest.TestCase):
    def test_firmware_clean_propagates_submake_failure(self):
        with tempfile.TemporaryDirectory(prefix="nexmon-firmware-make-") as tmp:
            root = Path(tmp)
            shutil.copy2(ROOT / "firmwares" / "Makefile", root / "Makefile")
            child = root / "chip"
            child.mkdir()
            (child / "Makefile").write_text("clean:\n\t@false\n")

            result = subprocess.run(
                ["make", "clean"],
                cwd=root,
                text=True,
                capture_output=True,
                timeout=10,
            )
            self.assertNotEqual(result.returncode, 0)

    def test_utility_clean_propagates_submake_failure(self):
        with tempfile.TemporaryDirectory(prefix="nexmon-utility-make-") as tmp:
            root = Path(tmp)
            shutil.copy2(ROOT / "utilities" / "Makefile", root / "Makefile")
            child = root / "utility"
            child.mkdir()
            (child / "Makefile").write_text("clean:\n\t@false\n")

            result = subprocess.run(
                ["make", "clean", "ARCH=x86"],
                cwd=root,
                text=True,
                capture_output=True,
                timeout=10,
            )
            self.assertNotEqual(result.returncode, 0)

    def test_recursive_build_layers_use_make_variable(self):
        makefiles = [
            ROOT / "firmwares" / "Makefile",
            ROOT / "utilities" / "Makefile",
            ROOT / "buildtools" / "Makefile",
            ROOT
            / "patches"
            / "bcm43455c0"
            / "7_45_265-28bca26-CY"
            / "nexmon"
            / "Makefile",
            *sorted((ROOT / "firmwares").glob("*/Makefile")),
        ]
        literal_recursive_make = re.compile(r"^\t.*(?<!\$\(MAKE\))\bmake\s+-C\b")
        ignored_failure = re.compile(r"\bmake\s+-C\b.*\|\|\s*true")

        violations = []
        for makefile in makefiles:
            for line_number, line in enumerate(makefile.read_text().splitlines(), 1):
                if literal_recursive_make.search(line) or ignored_failure.search(line):
                    violations.append(f"{makefile}:{line_number}: {line}")

        self.assertEqual(violations, [])


if __name__ == "__main__":
    unittest.main()
