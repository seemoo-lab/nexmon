#!/usr/bin/python3
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


def init(args, options):
    assert options["version"] == 1


class HeaderCodeGenerator:
    def __init__(self, generator):
        self.ifaces = generator.ifaces
        self.outfile = generator.outfile

        generator.skeleton_type_camel = "NewDBusInterfaceSkeleton"

    def generate_includes(self):
        self.outfile.write("/* codegen-test-extension include */\n")

    def declare_types(self):
        for i in self.ifaces:
            self.outfile.write(
                "/* codegen-test-extension declare type for iface %s */\n" % i.name
            )


class CodeGenerator:
    def __init__(self, generator):
        self.ifaces = generator.ifaces
        self.outfile = generator.outfile

        generator.skeleton_type_upper = "NEW_TYPE_DBUS_INTERFACE_SKELETON"

    def generate_body_preamble(self):
        self.outfile.write("/* codegen-test-extension body preamble */\n")

    def generate(self):
        for i in self.ifaces:
            self.outfile.write(
                "/* codegen-test-extension generate for iface %s */\n" % i.name
            )
