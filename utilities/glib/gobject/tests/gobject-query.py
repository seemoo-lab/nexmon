#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# Copyright © 2022 Endless OS Foundation, LLC
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

"""Integration tests for gobject-query utility."""

import unittest

import taptestrunner
import testprogramrunner


class TestGobjectQuery(testprogramrunner.TestProgramRunner):
    """Integration test for running gobject-query.

    This can be run when installed or uninstalled. When uninstalled, it
    requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

    The idea with this test harness is to test the gobject-query utility, its
    handling of command line arguments, and its exit statuses.
    """

    PROGRAM_NAME = "gobject-query"

    def runGobjectQuery(self, *args):
        return self.runTestProgram(args)

    def test_help(self):
        """Test the --help argument."""
        result = self.runGobjectQuery("--help")
        self.assertIn("usage: gobject-query", result.out)

    def test_version(self):
        """Test the --version argument."""
        result = self.runGobjectQuery("--version")
        self.assertIn("2.", result.out)

    def test_froots(self):
        """Test running froots with no other arguments."""
        result = self.runGobjectQuery("froots")

        self.assertEqual("", result.err)
        self.assertIn("├gboolean", result.out)
        self.assertIn("├GObject", result.out)

    def test_tree(self):
        """Test running tree with no other arguments."""
        result = self.runGobjectQuery("tree")

        self.assertEqual("", result.err)
        self.assertIn("GObject", result.out)


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())
