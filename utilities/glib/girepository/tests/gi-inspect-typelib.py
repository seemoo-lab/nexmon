#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright © 2025 Marco Trevisan <mail@3v1n0.net>
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

""" Integration tests for gi-inspect-typelib. """

import os
import sys
import unittest

import taptestrunner
import testprogramrunner


class TestGIInspectTypelibBase(testprogramrunner.TestProgramRunner):
    """Integration test base class for checking gi-inspect-typelib behavior"""

    PROGRAM_NAME = "gi-inspect-typelib"
    PROGRAM_TYPE = testprogramrunner.ProgramType.NATIVE

    def runTestProgram(self, *args, **kwargs):
        return super().runTestProgram(args, **kwargs)


class TestGIInspectTypelibCommandLine(TestGIInspectTypelibBase):
    def test_help(self):
        """Test the --help argument."""
        result = self.runTestProgram("--help")
        self.assertIn("Usage:", result.out)
        self.assertIn(
            f"{self._program_name} [OPTION…] NAMESPACE - Inspect GI typelib", result.out
        )
        self.assertIn("--typelib-version=VERSION", result.out)
        self.assertIn("--print-shlibs", result.out)
        self.assertIn("--print-typelibs", result.out)

    def test_no_args(self):
        """Test running with no arguments at all."""
        result = self.runTestProgram(should_fail=True)
        self.assertEqual("Please specify exactly one namespace", result.err)

    def test_invalid_typelib(self):
        res = self.runTestProgram(
            "--print-typelibs", "--print-shlibs", "AnInvalidNameSpace", should_fail=True
        )
        self.assertFalse(res.out)
        self.assertIn(
            "Typelib file for namespace 'AnInvalidNameSpace' (any version) not found",
            res.err,
        )


class TestGIInspectTypelibForGLibTypelib(TestGIInspectTypelibBase):
    """Test introspection of typelib for GLib typelib"""

    TYPELIB_NAMESPACE = "GLib"
    TYPELIB_VERSION = "2.0"
    LIB_SONAME = "0"
    shlib_prefix = ""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()

        if "G_TEST_BUILDDIR" in os.environ:
            os.environ["GI_TYPELIB_PATH"] = os.path.join(
                os.environ["G_TEST_BUILDDIR"], "..", "introspection"
            )
        # Check whether we are running under an MSVC environment
        # (all the following envvars should be there)
        MSVC_ENVVARS = ["VCINSTALLDIR", "WindowsSdkDir", "INCLUDE", "LIB"]
        if not all(var in os.environ for var in MSVC_ENVVARS):
            cls.shlib_prefix = "lib"

    def runTestProgram(self, *args, **kwargs):
        argv = list(args)
        argv.append(self.TYPELIB_NAMESPACE)

        if self.TYPELIB_VERSION:
            argv.append(f"--typelib-version={self.TYPELIB_VERSION}")

        return super().runTestProgram(*argv, **kwargs)

    def get_shlib_ext(self):
        if os.name == "nt":
            return f"-{self.LIB_SONAME}.dll"
        elif sys.platform == "darwin":
            return f".{self.LIB_SONAME}.dylib"

        return f".so.{self.LIB_SONAME}"

    def check_shlib(self, out):
        self.assertIn(
            f"{self.shlib_prefix}{self.TYPELIB_NAMESPACE.lower()}-{self.TYPELIB_VERSION}"
            + f"{self.get_shlib_ext()}",
            out,
        )

    def check_typelib_deps(self, out):
        self.assertNotIn("GLib-2.0", out)
        self.assertNotIn("GModule-2.0", out)
        self.assertNotIn("GObject-2.0", out)
        self.assertNotIn("Gio-2.0", out)

    def test_print_typelibs(self):
        res = self.runTestProgram("--print-typelibs")
        self.assertFalse(res.err)
        if self.TYPELIB_NAMESPACE == "GLib":
            self.assertFalse(res.out)
        self.check_typelib_deps(res.out)

    def test_print_shlibs(self):
        res = self.runTestProgram("--print-shlibs")
        self.assertFalse(res.err)
        self.check_shlib(res.out)
        self.assertNotIn("GLib-2.0", res.out)

    def test_print_typelibs_and_shlibs(self):
        res = self.runTestProgram("--print-typelibs", "--print-shlibs")
        self.assertFalse(res.err)
        self.check_shlib(res.out)


class TestGIInspectTypelibForGObjectTypelib(TestGIInspectTypelibForGLibTypelib):
    """Test introspection of typelib for GObject typelib"""

    TYPELIB_NAMESPACE = "GObject"

    def check_typelib_deps(self, out):
        self.assertIn("GLib-2.0", out)
        self.assertNotIn("GModule-2.0", out)
        self.assertNotIn("GObject-2.0", out)
        self.assertNotIn("Gio-2.0", out)


class TestGIInspectTypelibForGioTypelib(TestGIInspectTypelibForGLibTypelib):
    """Test introspection of typelib for Gio typelib"""

    TYPELIB_NAMESPACE = "Gio"

    def check_typelib_deps(self, out):
        self.assertIn("GLib-2.0", out)
        self.assertIn("GObject-2.0", out)
        self.assertIn("GModule-2.0", out)
        self.assertNotIn("Gio-2.0", out)


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())
