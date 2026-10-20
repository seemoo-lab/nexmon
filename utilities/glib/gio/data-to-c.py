#!/usr/bin/env python3

import sys

if len(sys.argv) < 4:
    print("Usage: {0} <filename> <variable> <output>")

with open(sys.argv[1], "r", encoding="utf-8", errors="backslashreplace") as f:
    in_data = f.read()

b = [r"\x{:02x}".format(ord(c)) for c in in_data]

out_data = 'const char {0}[] = "'.format(sys.argv[2])
out_data += "".join(b) + '";\n'

with open(sys.argv[3], "w") as f:
    f.write(out_data)
