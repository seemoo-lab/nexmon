#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# Copyright © 2026 Philip Withnall
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

"""Integration tests for the gdbus utility."""

import subprocess
import unittest

import taptestrunner
import testprogramrunner

try:
    # Do all non-standard imports here so we can skip the tests if any
    # needed packages are not available.
    from gi.repository import Gio

    class TestGdbusTool(testprogramrunner.TestProgramRunner):
        """Integration test for running the gdbus tool.

        This can be run when installed or uninstalled. When uninstalled, it
        requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

        The idea with this test harness is to test the gdbus utility, its
        handling of command line arguments, its exit statuses, and its actual
        effects on the bus.
        """

        PROGRAM_NAME = "gdbus"
        PROGRAM_TYPE = testprogramrunner.ProgramType.NATIVE

        def setUp(self):
            self.test_dbus = Gio.TestDBus.new(Gio.TestDBusFlags.NONE)
            self.test_dbus.up()

        def tearDown(self):
            self.test_dbus.stop()
            self.test_dbus.down()

        def runGdbus(self, *args):
            return self.runTestProgram(
                args,
                environment={
                    "DBUS_SESSION_BUS_ADDRESS": self.test_dbus.get_bus_address(),
                    "DBUS_SYSTEM_BUS_ADDRESS": "/dev/null",
                },
            )

        def test_help(self):
            """Test the --help argument and help subcommand."""
            result = self.runGdbus("--help")
            result2 = self.runGdbus("help")

            self.assertEqual(result.out, result2.out)
            self.assertEqual(result.err, result2.err)

            self.assertIn("Usage:\n  gdbus COMMAND", result.out)
            self.assertIn("Invoke a method on a remote object", result.out)

        def test_no_args(self):
            """Test running with no arguments at all."""
            with self.assertRaises(subprocess.CalledProcessError):
                self.runGdbus()

        def test_call_method(self):
            """Test running `gdbus` with a valid method call."""
            result = self.runGdbus(
                "call",
                "--session",
                "--dest",
                "org.freedesktop.DBus",
                "--object-path",
                "/org/freedesktop/DBus",
                "--method",
                "org.freedesktop.DBus.ListNames",
            )
            self.assertIn("'org.freedesktop.DBus'", result.out)

        def test_call_invalid_method(self):
            """Test running `gdbus` with an invalid method name."""
            with self.assertRaises(subprocess.CalledProcessError):
                result = self.runGdbus(
                    "call",
                    "--session",
                    "--dest",
                    "org.freedesktop.DBus",
                    "--object-path",
                    "/org/freedesktop/DBus",
                    "--method",
                    "org.freedesktop.DBus/ListNames",
                )
                self.assertIn(
                    "Error: Method name “org.freedesktop.DBus/ListNames” is invalid",
                    result.err,
                )

except ImportError as e:

    @unittest.skip("Cannot import %s" % e.name)
    class TestGdbusTool(unittest.TestCase):
        def test_call_method(self):
            pass


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())
