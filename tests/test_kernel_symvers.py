#!/usr/bin/env python3
"""Regression tests for kernel symbol supplement caching."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "buildtools" / "scripts" / "kernel-symvers-supplement.sh"


class KernelSymversCacheTests(unittest.TestCase):
    EXPORTS = """\
KSYMTAB_FUNC(alpha, \"\");
SYMBOL_FLAGS(alpha, 0x0);
SYMBOL_CRC(alpha, 0x11111111);
KSYMTAB_FUNC(beta, \"ns\");
SYMBOL_FLAGS(beta, 0x1);
SYMBOL_CRC(beta, 0x22222222);
"""

    def run_supplement(self, kernel: Path, cache: Path) -> Path:
        env = os.environ.copy()
        env["XDG_CACHE_HOME"] = str(cache)
        result = subprocess.run(
            [str(SCRIPT), str(kernel)],
            env=env,
            text=True,
            capture_output=True,
            timeout=10,
            check=True,
        )
        self.assertTrue(result.stdout.strip(), result.stderr)
        return Path(result.stdout.strip())

    def test_cache_tracks_tree_and_module_symvers_contents(self):
        with tempfile.TemporaryDirectory(prefix="nexmon-symvers-") as tmp:
            root = Path(tmp)
            cache = root / "cache"
            first_tree = root / "kernel-a"
            second_tree = root / "kernel-b"
            first_tree.mkdir()
            second_tree.mkdir()
            for tree in (first_tree, second_tree):
                (tree / ".vmlinux.export.c").write_text(self.EXPORTS)
                (tree / "Module.symvers").write_text("")

            first = self.run_supplement(first_tree, cache)
            other_tree = self.run_supplement(second_tree, cache)
            self.assertNotEqual(first, other_tree)
            self.assertIn("\talpha\t", first.read_text())
            self.assertIn("\tbeta\t", first.read_text())

            (first_tree / "Module.symvers").write_text(
                "0x11111111\talpha\tvmlinux\tEXPORT_SYMBOL\t\n"
            )
            changed = self.run_supplement(first_tree, cache)
            self.assertNotEqual(first, changed)
            self.assertNotIn("\talpha\t", changed.read_text())
            self.assertIn("\tbeta\t", changed.read_text())
            self.assertEqual(list((cache / "nexmon").glob("*.tmp")), [])

    def test_missing_module_symvers_is_supported(self):
        with tempfile.TemporaryDirectory(prefix="nexmon-symvers-missing-") as tmp:
            root = Path(tmp)
            kernel = root / "kernel"
            kernel.mkdir()
            (kernel / ".vmlinux.export.c").write_text(self.EXPORTS)

            output = self.run_supplement(kernel, root / "cache")
            self.assertEqual(output.read_text().count("\nvmlinux\t"), 0)
            self.assertEqual(len(output.read_text().splitlines()), 2)


if __name__ == "__main__":
    unittest.main()
