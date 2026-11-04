#!/usr/bin/env python3
"""Unit tests for the Python 3 ported b43-beautifier and libb43."""

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
B43_DEBUG_DIR = ROOT / "buildtools" / "b43-unified" / "debug"
sys.path.insert(0, str(B43_DEBUG_DIR))


class TestB43Beautifier(unittest.TestCase):
    def setUp(self):
        self.beautifier_bin = B43_DEBUG_DIR / "b43-beautifier"
        self.include_dir = B43_DEBUG_DIR / "include"

    def test_libb43_import_and_helpers(self):
        import libb43
        # Verify B43PsmDebug register parsing
        debug_reg = libb43.B43PsmDebug(0x12340567)
        self.assertEqual(debug_reg.getRaw(), 0x12340567)
        self.assertEqual(debug_reg.getPc(), 0x567)

        # Verify B43SymbolicSpr with spr.inc
        spr_inc = self.include_dir / "spr.inc"
        if spr_inc.exists():
            spr_resolver = libb43.B43SymbolicSpr(str(spr_inc))
            # Test resolving a known spr definition from spr.inc
            # spr000 or any present SPR
            self.assertIsInstance(spr_resolver.spr_names, dict)

    def test_beautifier_help_option(self):
        res = subprocess.run(
            [sys.executable, str(self.beautifier_bin), "--help"],
            capture_output=True,
            text=True
        )
        self.assertEqual(res.returncode, 0)
        self.assertIn("b43 firmware assembly code beautifier", res.stdout)
        self.assertIn("--asmfile", res.stdout)
        self.assertIn("--defs", res.stdout)

    def test_beautifier_assembly_processing(self):
        asm_code = """%arch 5
%start start_lbl
start_lbl:
    mov r0, spr848
    or  spr06c, 0, spr06c
"""
        with tempfile.NamedTemporaryFile("w+", suffix=".asm", delete=False) as f:
            f.write(asm_code)
            asm_path = f.name

        try:
            res = subprocess.run(
                [
                    sys.executable,
                    str(self.beautifier_bin),
                    "-a", asm_path,
                    "-d", str(self.include_dir),
                ],
                capture_output=True,
                text=True
            )
            self.assertEqual(res.returncode, 0, f"Stderr: {res.stderr}")
            # Verify #include statements were injected
            self.assertIn('#include "', res.stdout)
            self.assertIn('spr.inc"', res.stdout)
            # Verify start label and instructions remain intact or beautified
            self.assertIn("start_lbl:", res.stdout)
            self.assertIn("mov", res.stdout)
        finally:
            if os.path.exists(asm_path):
                os.remove(asm_path)


if __name__ == "__main__":
    unittest.main()
