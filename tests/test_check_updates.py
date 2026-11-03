#!/usr/bin/env python3
"""Regression tests for update version comparison."""

import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "buildtools" / "scripts"))

from check_updates import compare_versions, extract_best_tag_version


class VersionComparisonTests(unittest.TestCase):
    def test_release_prefix_is_not_equality(self):
        self.assertEqual(compare_versions("1.2.3", "1.2.30"), "UPDATE AVAILABLE")

    def test_prerelease_is_older_than_final_release(self):
        self.assertEqual(compare_versions("1.2.3-rc1", "1.2.3"), "UPDATE AVAILABLE")

    def test_newer_prerelease_is_detected(self):
        self.assertEqual(compare_versions("1.2.3-rc1", "1.2.3-rc2"), "UPDATE AVAILABLE")

    def test_abbreviated_commit_hashes_can_match(self):
        self.assertEqual(
            compare_versions("deadbeef", "deadbeef1234567890"),
            "UP TO DATE",
        )

    def test_different_commit_hashes_are_an_update(self):
        self.assertEqual(
            compare_versions("deadbeef", "feedface12345678"),
            "UPDATE AVAILABLE",
        )

    def test_best_tag_prefers_final_release(self):
        self.assertEqual(
            extract_best_tag_version(
                ["v1.2.3-rc2", "v1.2.3", "v1.2.2"],
                prefix_filter="v",
                exclude_pre=False,
            ),
            "1.2.3",
        )


if __name__ == "__main__":
    unittest.main()
