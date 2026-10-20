#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import re
import os

debug = os.getenv("GIO_GENTYPEFUNCS_DEBUG") is not None

out_file = sys.argv[1]
in_files = sys.argv[2:]

funcs = []


if debug:
    print("Output file: ", out_file)

if debug:
    print(len(in_files), "input files")

for filename in in_files:
    if debug:
        print("Input file: ", filename)
    with open(filename, "rb") as f:
        for line in f:
            line = line.rstrip(b"\n").rstrip(b"\r")
            match = re.search(rb"\bg_[a-zA-Z0-9_]*_get_type\b", line)
            if match:
                func = match.group(0).decode("utf-8")
                if func not in funcs:
                    funcs.append(func)
                    if debug:
                        print("Found ", func)

file_output = "G_GNUC_BEGIN_IGNORE_DEPRECATIONS\n"

funcs = sorted(funcs)

# These types generally emit critical warnings if constructed in the wrong
# environment (for example, without D-Bus running), so skip them.
ignored_types = [
    "g_io_extension_get_type",
    "g_settings_backend_get_type",
    "g_debug_controller_dbus_get_type",
    "g_file_icon_get_type",
    "g_unix_credentials_message_get_type",
    "g_unix_socket_address_get_type",
]

for f in funcs:
    if f not in ignored_types:
        file_output += "*tp++ = {0} ();\n".format(f)

file_output += "G_GNUC_END_IGNORE_DEPRECATIONS\n"

if debug:
    print(len(funcs), "functions")

ofile = open(out_file, "w")
ofile.write(file_output)
ofile.close()
