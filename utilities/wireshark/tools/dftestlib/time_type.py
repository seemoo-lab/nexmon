# Copyright (c) 2013 by Gilbert Ramirez <gram@alumni.rice.edu>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import os

from dftestlib import dftest

# Force the timezone to UTC so the checks below work regardless of what time
# zone we're in.
os.environ['TZ'] = "UTC"

class testTime(dftest.DFTest):
    trace_file = "http.pcap"

    def test_eq_1(self):
        dfilter = 'frame.time == "Dec 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_eq_2(self):
        dfilter = 'frame.time == "Jan 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_ne_1(self):
        dfilter = 'frame.time != "Dec 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_ne_2(self):
        dfilter = 'frame.time != "Jan 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_gt_1(self):
        dfilter = 'frame.time > "Dec 31, 2002 13:54:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_gt_2(self):
        dfilter = 'frame.time > "Dec 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_gt_3(self):
        dfilter = 'frame.time > "Dec 31, 2002 13:56:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_ge_1(self):
        dfilter = 'frame.time >= "Dec 31, 2002 13:54:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_ge_2(self):
        dfilter = 'frame.time >= "Dec 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_ge_3(self):
        dfilter = 'frame.time >= "Dec 31, 2002 13:56:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_lt_1(self):
        dfilter = 'frame.time < "Dec 31, 2002 13:54:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_lt_2(self):
        dfilter = 'frame.time < "Dec 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_lt_3(self):
        dfilter = 'frame.time < "Dec 31, 2002 13:56:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_le_1(self):
        dfilter = 'frame.time <= "Dec 31, 2002 13:54:31.3"'
        self.assertDFilterCount(dfilter, 0)

    def test_le_2(self):
        dfilter = 'frame.time <= "Dec 31, 2002 13:55:31.3"'
        self.assertDFilterCount(dfilter, 1)

    def test_le_3(self):
        dfilter = 'frame.time <= "Dec 31, 2002 13:56:31.3"'
        self.assertDFilterCount(dfilter, 1)

