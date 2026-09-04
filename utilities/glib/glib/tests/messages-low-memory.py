#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright © 2022 Emmanuel Fleury <emmanuel.fleury@gmail.com>
# Copyright © 2022 Marco Trevisan <mail@3v1n0.net>
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

""" Integration tests for g_message functions on low-memory. """

import os
import unittest

import taptestrunner
import testprogramrunner


class TestMessagesLowMemory(testprogramrunner.TestProgramRunner):
    """Integration test for checking g_message()’s behavior on low memory.

    This can be run when installed or uninstalled. When uninstalled,
    it requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

    The idea with this test harness is to test if g_message and friends
    assert instead of crashing if memory is exhausted, printing the expected
    error message.
    """

    PROGRAM_NAME = "messages-low-memory"
    PROGRAM_TYPE = testprogramrunner.ProgramType.NATIVE

    def test_message_memory_allocation_failure(self):
        """Test running g_message() when memory is exhausted."""
        result = self.runTestProgram([], should_fail=True)

        if result.info.returncode == 77:
            self.skipTest("Not supported")

        if os.name == "nt":
            self.assertEqual(result.info.returncode, 3)
        else:
            self.assertEqual(result.info.returncode, -6)
        self.assertIn("failed to allocate memory", result.err)


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())
