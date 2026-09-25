#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright © 2022 Emmanuel Fleury <emmanuel.fleury@gmail.com>
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

""" Integration tests for g_assert() functions. """

import os
import shutil
import tempfile
import unittest

import taptestrunner
import testprogramrunner

GDB_SCRIPT = """
# Work around https://sourceware.org/bugzilla/show_bug.cgi?id=22501
set confirm off
set print elements 0
set auto-load safe-path /
run
print *((char**) &__glib_assert_msg)
quit
"""


class TestAssertMessage(testprogramrunner.TestProgramRunner):
    """Integration test for throwing message on g_assert().

    This can be run when installed or uninstalled. When uninstalled,
    it requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

    The idea with this test harness is to test if g_assert() prints
    an error message when called, and that it saves this error
    message in a global variable accessible to gdb, so that developers
    and automated tools can more easily debug assertion failures.
    """

    PROGRAM_NAME = "assert-msg-test"
    PROGRAM_TYPE = testprogramrunner.ProgramType.NATIVE

    def setUp(self):
        super().setUp()
        self.__gdb = shutil.which("gdb")

    def runAssertMessage(self, *args):
        return self.runTestProgram(args, should_fail=True)

    def runGdbAssertMessage(self, *args):
        if self.__gdb is None:
            return testprogramrunner.Result()

        return self.runTestProgram(args, wrapper_args=["gdb", "-n", "--batch"])

    def test_gassert(self):
        """Test running g_assert() and fail the program."""
        result = self.runAssertMessage()

        if os.name == "nt":
            self.assertEqual(result.info.returncode, 3)
        else:
            self.assertEqual(result.info.returncode, -6)
        self.assertIn("assertion failed: (42 < 0)", result.out)

    def test_gdb_gassert(self):
        """Test running g_assert() within gdb and fail the program."""
        if self.__gdb is None:
            self.skipTest("GDB is not installed, skipping this test!")
        if {"thread", "address"} & set(
            os.getenv("_GLIB_TEST_SANITIZERS", "").split(",")
        ):
            self.skipTest("GDB can't run under sanitizers")

        with tempfile.NamedTemporaryFile(
            prefix="assert-msg-test-", suffix=".gdb", mode="w", delete=False
        ) as tmp:
            try:
                tmp.write(GDB_SCRIPT)
                tmp.close()
                result = self.runGdbAssertMessage("-x", tmp.name)
            finally:
                os.unlink(tmp.name)

            # Some CI environments disable ptrace (as they’re running in a
            # container). If so, skip the test as there’s nothing we can do.
            if result.info.returncode != 0 and (
                "ptrace: Operation not permitted" in result.err
                or "warning: opening /proc/PID/mem file for lwp" in result.err
            ):
                self.skipTest("GDB is not functional due to ptrace being disabled")

            self.assertEqual(result.info.returncode, 0)
            self.assertIn("$1 = 0x", result.out)
            self.assertIn("assertion failed: (42 < 0)", result.out)


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())
