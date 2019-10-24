#!/usr/bin/env python
#
# Run this script as root to install the debugging tools
#

from distutils.core import setup
import sys

if len(sys.argv) == 1:
	sys.argv.append("install")	# default to INSTALL

setup(
	name="B43-debug-tools",
	py_modules=["libb43"],
	scripts=["b43-fwdump", "b43-beautifier"]
     )
