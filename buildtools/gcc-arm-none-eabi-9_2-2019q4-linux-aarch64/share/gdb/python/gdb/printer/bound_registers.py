# Pretty-printers for bounds registers.
# Copyright (C) 2013-2019 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys

import gdb.printing

if sys.version_info[0] > 2:
    # Python 3 removed basestring and long
    basestring = str
    long = int

class MpxBound128Printer:
    """Adds size field to a mpx __gdb_builtin_type_bound128 type."""

    def __init__ (self, val):
        self.val = val

    def to_string (self):
        upper = self.val["ubound"]
        lower = self.val["lbound"]
        size  = (long) ((upper) - (lower))
        if size > -1:
            size = size + 1
        result = '{lbound = %s, ubound = %s} : size %s' % (lower, upper, size)
        return result

gdb.printing.add_builtin_pretty_printer ('mpx_bound128',
                                         '^__gdb_builtin_type_bound128',
                                         MpxBound128Printer)
