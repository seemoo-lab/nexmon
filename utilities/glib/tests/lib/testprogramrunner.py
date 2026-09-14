#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# Copyright © 2018 Endless Mobile, Inc.
# Copyright © 2025 Canonical Ltd.
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

"""Integration tests for GLib utilities."""

import os
import shutil
import subprocess
import tempfile
import unittest
import sys

from dataclasses import dataclass
from enum import Enum


class ProgramType(Enum):
    """Enum to define the kind of tool to use"""

    NATIVE = 1
    INTERPRETED = 2


@dataclass
class Result:
    """Class for keeping track of the executable result."""

    info: subprocess.CompletedProcess
    out: str
    err: str
    subs: dict


class TestProgramRunner(unittest.TestCase):
    """Integration test for running glib-based tools.

    This can be run when installed or uninstalled. When uninstalled, it
    requires G_TEST_BUILDDIR or _G_TEST_PROGRAM_RUNNER_PATH to be set.
    """

    PROGRAM_NAME: str = None
    PROGRAM_TYPE: ProgramType = ProgramType.NATIVE
    INTERPRETER: str = None

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.assertTrue(cls.PROGRAM_NAME, "class PROGRAM_NAME must be set")

        ext = ""
        if cls.PROGRAM_TYPE == ProgramType.NATIVE and os.name == "nt":
            ext = ".exe"

        cls._program_name = f"{cls.PROGRAM_NAME}{ext}"

        if "_G_TEST_PROGRAM_RUNNER_PATH" in os.environ:
            cls.__program = os.path.join(
                os.environ["_G_TEST_PROGRAM_RUNNER_PATH"], cls._program_name
            )
        elif "G_TEST_BUILDDIR" in os.environ:
            cls.__program = os.path.join(
                os.environ["G_TEST_BUILDDIR"], cls._program_name
            )
        else:
            cls.__program = os.path.join(os.path.dirname(__file__), cls._program_name)
            if not os.path.exists(cls.__program):
                cls.__program = shutil.which(cls._program_name)

    def setUp(self):
        print(f"{self.PROGRAM_NAME}: {self.__program}")
        self.assertTrue(os.path.exists(self.__program))

        self.tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmpdir.cleanup)
        old_cwd = os.getcwd()
        self.addCleanup(os.chdir, old_cwd)
        os.chdir(self.tmpdir.name)
        print("tmpdir:", self.tmpdir.name)

    def runTestProgram(
        self,
        *args,
        should_fail=False,
        wrapper_args=[],
        environment={},
        cwd=None,
    ) -> Result:
        argv = [self.__program]

        argv.extend(*args)

        # shebang lines are not supported on native
        # Windows consoles
        if self.PROGRAM_TYPE == ProgramType.INTERPRETED and os.name == "nt":
            argv.insert(0, self.INTERPRETER if self.INTERPRETER else sys.executable)

        argv = wrapper_args + argv

        env = os.environ.copy()
        env["LC_ALL"] = "C.UTF-8"
        env["G_DEBUG"] = "fatal-warnings"
        env.update(environment)

        print("Running:", argv)

        if cwd is not None:
            print("Working Directory:", cwd)

        # We want to ensure consistent line endings...
        info = subprocess.run(
            argv,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            universal_newlines=True,
            text=True,
            encoding="utf-8",
            check=False,
            cwd=cwd,
        )

        result = Result(
            info=info,
            out=info.stdout.strip(),
            err=info.stderr.strip(),
            subs=self._getSubs(),
        )

        print("Return code:", result.info.returncode)
        print("Output:\n", result.out)
        print("Error:\n", result.err)

        if should_fail:
            with self.assertRaises(subprocess.CalledProcessError):
                info.check_returncode()
        else:
            info.check_returncode()

        return result

    def _getSubs(self) -> dict:
        return {}
