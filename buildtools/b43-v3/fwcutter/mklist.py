#!/usr/bin/env python
#
#  Script for creating a "struct extract" list for fwcutter_list.h
#
#  Copyright (c) 2008 Michael Buesch <m@bues.ch>
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#     1. Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#     2. Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided
#        with the distribution.
#
#   THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
#   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
#   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
#   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys
import os
import re
import hashlib

if len(sys.argv) != 2:
	print "Usage: %s path/to/wl.o" % sys.argv[0]
	sys.exit(1)
fn = sys.argv[1]

pipe = os.popen("objdump -t %s" % fn)
syms = pipe.readlines()
pipe = os.popen("objdump --headers %s" % fn)
headers = pipe.readlines()

# Get the .rodata fileoffset
rodata_fileoffset = None
rofileoff_re = re.compile(r"\d+\s+\.rodata\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+.")
for line in headers:
	line = line.strip()
	m = rofileoff_re.match(line)
	if m:
		rodata_fileoffset = int(m.group(1), 16)
		break
if rodata_fileoffset == None:
	print "ERROR: Could not find .rodata fileoffset"
	sys.exit(1)

md5sum = hashlib.md5(file(fn, "r").read())

print "static struct extract _%s[] =" % md5sum.hexdigest()
print "{"

sym_re = re.compile(r"([0-9a-fA-F]+)\s+g\s+O\s+\.rodata\s+([0-9a-fA-F]+) d11([-_\s\w0-9]+)")
ucode_re = re.compile(r"ucode(\d+)")

for sym in syms:
	sym = sym.strip()
	m = sym_re.match(sym)
	if not m:
		continue
	pos = int(m.group(1), 16) + rodata_fileoffset
	size = int(m.group(2), 16)
	name = m.group(3)
	if name[-2:] == "sz":
		continue

	type = None
	if "initvals" in name:
		type = "EXT_IV"
		size -= 8
	if "pcm" in name:
		type = "EXT_PCM"
	if "bommajor" in name:
		print "\t/* ucode major version at offset 0x%x */" % pos
		continue
	if "bomminor" in name:
		print "\t/* ucode minor version at offset 0x%x */" % pos
		continue
	if "ucode_2w" in name:
		continue
	m = ucode_re.match(name)
	if m:
		corerev = int(m.group(1))
		if corerev <= 4:
			type = "EXT_UCODE_1"
		elif corerev >= 5 and corerev <= 14:
			type = "EXT_UCODE_2"
		else:
			type = "EXT_UCODE_3"
	if not type:
		print "\t/* ERROR: Could not guess data type for: %s */" % name
		continue

	print "\t{ .name = \"%s\", .offset = 0x%X, .type = %s, .length = 0x%X }," % (name, pos, type, size)
print "\tEXTRACT_LIST_END"
print "};"
