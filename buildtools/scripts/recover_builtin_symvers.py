#!/usr/bin/env python3
"""Recover missing built-in symbol metadata from this kernel's export artifact.

Writes only the specified supplemental table; leaves the kernel tree untouched.
"""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument("kernel", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
source = (args.kernel / ".vmlinux.export.c").read_text()
exports = dict(re.findall(r'KSYMTAB_(?:FUNC|DATA)\((\w+), "([^"]*)"\);', source))
flags = dict(re.findall(r"SYMBOL_FLAGS\((\w+), (0x[0-9a-fA-F]+)\);", source))
crcs = dict(re.findall(r"SYMBOL_CRC\((\w+), (0x[0-9a-fA-F]+)\);", source))
assert exports and exports.keys() == flags.keys() == crcs.keys(), "Incomplete export artifact"
existing = {line.split()[1] for line in (args.kernel / "Module.symvers").read_text().splitlines()}
rows = []
for name in sorted(exports):
    if name in existing:
        continue
    flag = int(flags[name], 16)
    assert flag in (0, 1), f"Unsupported export flags for {name}: {flag}"
    export_type = "EXPORT_SYMBOL_GPL" if flag else "EXPORT_SYMBOL"
    rows.append(f"{crcs[name]}\t{name}\tvmlinux\t{export_type}\t{exports[name]}\n")
args.output.write_text("".join(rows))
print(f"Recovered {len(rows)} missing built-in exports from {args.kernel / '.vmlinux.export.c'}")
