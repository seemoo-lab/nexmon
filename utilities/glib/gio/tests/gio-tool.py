#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# Copyright © 2018, 2019 Endless Mobile, Inc.
# Copyright © 2023 Philip Withnall
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301  USA

"""Integration tests for the gio utility."""

import platform
import subprocess
import sys
import tempfile
import unittest

from pathlib import Path

import taptestrunner
import testprogramrunner


class TestGioTool(testprogramrunner.TestProgramRunner):
    """Integration test for running the gio tool.

    This can be run when installed or uninstalled. When uninstalled, it
    requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

    The idea with this test harness is to test the gio utility, its
    handling of command line arguments, its exit statuses, and its actual
    effects on the file system.
    """

    PROGRAM_NAME = "gio"
    PROGRAM_TYPE = testprogramrunner.ProgramType.NATIVE

    def runGio(self, *args, **kwargs):
        return self.runTestProgram(args, **kwargs)

    def test_help(self):
        """Test the --help argument and help subcommand."""
        result = self.runGio("--help")
        result2 = self.runGio("help")
        result3 = self.runGio("help", "help")

        self.assertEqual(result.out, result2.out)
        self.assertEqual(result.err, result2.err)

        self.assertEqual(result2.out, result3.out)
        self.assertEqual(result2.err, result3.err)

        self.assertIn("Usage:\n  gio COMMAND", result.out)
        self.assertIn("List the contents of locations", result.out)

    def test_no_args(self):
        """Test running with no arguments at all."""
        with self.assertRaises(subprocess.CalledProcessError):
            self.runGio()

    def test_info_non_default_attributes(self):
        """Test running `gio info --attributes` with a non-default list."""
        with tempfile.NamedTemporaryFile(dir=self.tmpdir.name) as tmpfile:
            result = self.runGio(
                "info", "--attributes=standard::content-type", tmpfile.name
            )
            if sys.platform == "darwin":
                self.assertIn("standard::content-type: public.text", result.out)
            else:
                self.assertIn(
                    "standard::content-type: application/x-zerosize", result.out
                )

    @unittest.skipUnless(platform.system() == "Darwin", "macOS-specific trash behavior")
    def test_trash_unsupported_modes_on_macos(self):
        """Test macOS-specific unsupported trash sub-modes."""
        for option in ("--restore", "--list", "--empty"):
            result = self.runGio("trash", option, should_fail=True)
            self.assertIn("not supported on macOS", result.err)


@unittest.skipIf(platform.system() == "Darwin", "gio launch not supported on darwin")
class TestGioLaunchExpandsDesktopEntry(testprogramrunner.TestProgramRunner):
    """Integration test for `gio launch` with field code %k in the Exec line.

    This can be run when installed or uninstalled. When uninstalled, it
    requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

    The idea with this test harness is to test that the `gio launch` command
    expands the `%k` field code in a desktop entry's Exec line to its location,
    i.e. its absolute path.
    """

    PROGRAM_NAME = "gio"
    PROGRAM_TYPE = testprogramrunner.ProgramType.NATIVE

    TEMPLATE = """
[Desktop Entry]
Type = Application
Name = Test
Exec = {python} -c 'print("%k")'
"""

    def setUp(self):
        super().setUp()

        self.parent = Path(self.tmpdir.name).resolve()

        self.folder = self.parent / "folder"
        self.entry = self.folder / "desktop.entry"

        self.sibling = self.parent / "sibling"

        self.folder.mkdir(exist_ok=True, parents=True)
        self.sibling.mkdir(exist_ok=True, parents=True)

        with self.entry.open("w", encoding="utf-8") as fd:
            fd.write(self.TEMPLATE.format(python=sys.executable))

    def launchAndCheck(self, entry: Path, cwd: Path = None):
        result = self.runTestProgram(["launch", str(entry)], cwd=str(cwd))

        self.assertIn(str(self.entry), result.out)

    def test_absolute_from_folder(self):
        """Test with absolute path, with changing working directory to folder."""
        self.launchAndCheck(self.entry, cwd=self.folder)

    def test_absolute_from_parent(self):
        """Test with absolute path, with changing working directory to parent."""
        self.launchAndCheck(self.entry, cwd=self.parent)

    def test_absolute_from_sibling(self):
        """Test with absolute path, with changing working directory to sibling."""
        self.launchAndCheck(self.entry, cwd=self.sibling)

    def test_relative_from_folder(self):
        """Test with relative path, with changing working directory to folder."""
        self.launchAndCheck(self.entry.relative_to(self.folder), cwd=self.folder)

    def test_relative_from_parent(self):
        """Test with relative path, with changing working directory to parent."""
        self.launchAndCheck(self.entry.relative_to(self.parent), cwd=self.parent)

    def test_relative_from_sibling(self):
        """Test with relative path, with changing working directory to sibling."""
        self.launchAndCheck(
            Path("..") / self.entry.relative_to(self.parent), cwd=self.sibling
        )


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())
