#!/usr/bin/env python
#
# Looks for registration routines in the protocol dissectors,
# and assembles C code to call all the routines.
#
# This is a Python version of the make-reg-dotc shell script.
# Running the shell script on Win32 is very very slow because of
# all the process-launching that goes on --- multiple greps and
# seds for each input file.  I wrote this python version so that
# less processes would have to be started.

import os
import sys
import re
import pickle
import hashlib
from stat import *

VERSION_KEY = '_VERSION'
CUR_VERSION = '$Id$'

#
# The first argument is the directory in which the source files live.
#
srcdir = sys.argv[1]

#
# The second argument is either "plugin" or "dissectors"; if it's
# "plugin", we build a plugin.c for a plugin, and if it's
# "dissectors", we build a register.c for libwireshark.
#
registertype = sys.argv[2]
if registertype in ("plugin", "plugin_wtap"):
    final_filename = "plugin.c"
    cache_filename = None
    preamble = """\
/*
 * Do not modify this file. Changes will be overwritten.
 *
 * Generated automatically from %s.
 */
""" % (sys.argv[0])
elif registertype in ("dissectors", "dissectorsinfile"):
    final_filename = "register.c"
    cache_filename = "register-cache.pkl"
    preamble = """\
/*
 * Do not modify this file. Changes will be overwritten.
 *
 * Generated automatically by the "register.c" target in
 * epan/dissectors/Makefile using %s
 * and information in epan/dissectors/register-cache.pkl.
 *
 * You can force this file to be regenerated completely by deleting
 * it along with epan/dissectors/register-cache.pkl.
 */
""" % (sys.argv[0])
else:
    print(("Unknown output type '%s'" % registertype))
    sys.exit(1)


#
# All subsequent arguments are the files to scan
# or the name of a file containing the files to scan
#
if registertype == "dissectorsinfile":
    try:
        dissector_f = open(sys.argv[3])
    except IOError:
        print(("Unable to open input file '%s'" % sys.argv[3]))
        sys.exit(1)

    files = [line.rstrip() for line in dissector_f]
else:
    files = sys.argv[3:]

# Create the proper list of filenames
filenames = []
for file in files:
    if os.path.isfile(file):
        filenames.append(file)
    else:
        filenames.append(os.path.join(srcdir, file))

if len(filenames) < 1:
    print("No files found")
    sys.exit(1)


# Look through all files, applying the regex to each line.
# If the pattern matches, save the "symbol" section to the
# appropriate set.
regs = {
        'proto_reg': set(),
        'handoff_reg': set(),
        'wtap_register': set(),
        }

# For those that don't know Python, r"" indicates a raw string,
# devoid of Python escapes.
proto_regex = r"(?P<symbol>proto_register_[_A-Za-z0-9]+)\s*\(\s*void\s*\)[^;]*$"

handoff_regex = r"(?P<symbol>proto_reg_handoff_[_A-Za-z0-9]+)\s*\(\s*void\s*\)[^;]*$"

wtap_reg_regex = r"(?P<symbol>wtap_register_[_A-Za-z0-9]+)\s*\([^;]+$"

# This table drives the pattern-matching and symbol-harvesting
patterns = [
        ( 'proto_reg', re.compile(proto_regex, re.MULTILINE) ),
        ( 'handoff_reg', re.compile(handoff_regex, re.MULTILINE) ),
        ( 'wtap_register', re.compile(wtap_reg_regex, re.MULTILINE) ),
        ]

# Open our registration symbol cache
cache = None
if cache_filename:
    try:
        cache_file = open(cache_filename, 'rb')
        cache = pickle.load(cache_file)
        cache_file.close()
        if VERSION_KEY not in cache or cache[VERSION_KEY] != CUR_VERSION:
            cache = {VERSION_KEY: CUR_VERSION}
    except:
        cache = {VERSION_KEY: CUR_VERSION}

    print(("Registering %d files, %d cached" % (len(filenames), len(list(cache.keys()))-1)))

# Grep
cache_hits = 0
cache_misses = 0
for filename in filenames:
    file = open(filename)
    cur_mtime = os.fstat(file.fileno())[ST_MTIME]
    if cache and filename in cache:
        cdict = cache[filename]
        if cur_mtime == cdict['mtime']:
            cache_hits += 1
#                       print "Pulling %s from cache" % (filename)
            regs['proto_reg'] |= set(cdict['proto_reg'])
            regs['handoff_reg'] |= set(cdict['handoff_reg'])
            regs['wtap_register'] |= set(cdict['wtap_register'])
            file.close()
            continue
    # We don't have a cache entry
    if cache is not None:
        cache_misses += 1
        cache[filename] = {
                'mtime': cur_mtime,
                'proto_reg': [],
                'handoff_reg': [],
                'wtap_register': [],
                }
#       print "Searching %s" % (filename)
    # Read the whole file into memory
    contents = file.read()
    for action in patterns:
        regex = action[1]
        for match in regex.finditer(contents):
            symbol = match.group("symbol")
            sym_type = action[0]
            regs[sym_type].add(symbol)
            if cache is not None:
#                               print "Caching %s for %s: %s" % (sym_type, filename, symbol)
                cache[filename][sym_type].append(symbol)
    # We're done with the file contents
    contets = ""
    file.close()


if cache is not None and cache_filename is not None:
    cache_file = open(cache_filename, 'wb')
    pickle.dump(cache, cache_file)
    cache_file.close()
    print(("Cache hits: %d, misses: %d" % (cache_hits, cache_misses)))

# Make sure we actually processed something
if len(regs['proto_reg']) < 1:
    print("No protocol registrations found")
    sys.exit(1)

# Convert the sets into sorted lists to make the output pretty
regs['proto_reg'] = sorted(regs['proto_reg'])
regs['handoff_reg'] = sorted(regs['handoff_reg'])
regs['wtap_register'] = sorted(regs['wtap_register'])

reg_code = ""

reg_code += preamble

# Make the routine to register all protocols
if registertype == "plugin" or registertype == "plugin_wtap":
    reg_code += """
#include "config.h"

#include <gmodule.h>

#include "moduleinfo.h"

/* plugins are DLLs */
#define WS_BUILD_DLL
#include "ws_symbol_export.h"

#ifndef ENABLE_STATIC
WS_DLL_PUBLIC_DEF void plugin_register (void);
WS_DLL_PUBLIC_DEF const gchar version[] = VERSION;

"""
else:
    reg_code += """
#include "register.h"

"""

for symbol in regs['proto_reg']:
    reg_code += "extern void %s(void);\n" % (symbol)

if registertype == "plugin" or registertype == "plugin_wtap":
    reg_code += """
/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void
plugin_register (void)
{
"""
else:
    reg_code += """
#define CALLBACK_REGISTER(proto, data) \\
    if (cb) cb(RA_REGISTER, proto, data)

void
register_all_protocols(register_cb cb, gpointer cb_data)
{
"""

for symbol in regs['proto_reg']:
    if registertype != "plugin" and registertype != "plugin_wtap":
        reg_code += "    CALLBACK_REGISTER(\"%s\", cb_data);\n" % (symbol)
    reg_code += "    %s();\n" % (symbol)

reg_code += "}\n\n"


# Make the routine to register all protocol handoffs

for symbol in regs['handoff_reg']:
    reg_code += "extern void %s(void);\n" % (symbol)

if registertype == "plugin" or registertype == "plugin_wtap":
    reg_code += """
WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void);

WS_DLL_PUBLIC_DEF void
plugin_reg_handoff(void)
{
"""
else:
    reg_code += """
#define CALLBACK_HANDOFF(proto, data) \\
    if (cb) cb(RA_HANDOFF, proto, data)

void
register_all_protocol_handoffs(register_cb cb, gpointer cb_data)
{
"""

for symbol in regs['handoff_reg']:
    if registertype != "plugin" and registertype != "plugin_wtap":
        reg_code += "    CALLBACK_HANDOFF(\"%s\", cb_data);\n" % (symbol)
    reg_code += "    %s();\n" % (symbol)

reg_code += "}\n"

if registertype == "plugin":
    reg_code += "#endif\n"
elif registertype == "plugin_wtap":
    reg_code += """
WS_DLL_PUBLIC_DEF void
register_wtap_module(void)
{
"""

    for symbol in regs['wtap_register']:
        line = "    {extern void %s (void); %s ();}\n" % (symbol, symbol)
        reg_code += line

    reg_code += """
}
#endif
"""

else:
    reg_code += """
static gulong proto_reg_count(void)
{
    return %(proto_reg_len)d;
}

static gulong handoff_reg_count(void)
{
    return %(handoff_reg_len)d;
}

gulong register_count(void)
{
    return proto_reg_count() + handoff_reg_count();
}
""" % {
    'proto_reg_len': len(regs['proto_reg']),
    'handoff_reg_len': len(regs['handoff_reg'])
  }


# Compare current and new content and update the file if anything has changed.

try:    # Python >= 2.6, >= 3.0
    reg_code_bytes = bytes(reg_code.encode('utf-8'))
except:
    reg_code_bytes = reg_code

new_hash = hashlib.sha1(reg_code_bytes).hexdigest()

try:
    fh = open(final_filename, 'rb')
    cur_hash = hashlib.sha1(fh.read()).hexdigest()
    fh.close()
except:
    cur_hash = ''

try:
    if new_hash != cur_hash:
        print(('Updating ' + final_filename))
        fh = open(final_filename, 'w')
        fh.write(reg_code)
        fh.close()
    else:
        print((final_filename + ' unchanged.'))
        os.utime(final_filename, None)
except OSError:
    sys.exit('Unable to write ' + final_filename + '.\n')

#
# Editor modelines  -  http://www.wireshark.org/tools/modelines.html
#
# Local variables:
# c-basic-offset: 4
# indent-tabs-mode: nil
# End:
#
# vi: set shiftwidth=4 expandtab:
# :indentSize=4:noTabs=true:
#
