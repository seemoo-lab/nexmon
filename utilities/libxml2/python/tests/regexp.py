#!/usr/bin/env python3
import sys
import setup_test
import libxml2

# Memory debug specific
libxml2.debugMemory(1)


def check_regex(regex, inside, outside):
    re = libxml2.regexpCompile(regex)
    for c in inside:
        if re.regexpExec(c) != 1:
            print("error checking inside", repr(regex), repr(c))
            sys.exit(1)
    for c in outside:
        if re.regexpExec(c) != 0:
            print("error checking outside", repr(regex), repr(c))
            sys.exit(1)
    if re.regexpIsDeterminist() != 1:
        print("error checking determinism")
        sys.exit(1)
    del re


check_regex("a|b", "ab", ["ab", ""])
# https://gitlab.gnome.org/GNOME/libxml2/-/issues/1086
check_regex(r"[\t-\r]", "\t\n\r", ["a", "(", ""])
check_regex("[\\t-\\r]", "\t\n\r", ["a", "(", ""])
check_regex(r"[\[-\]]", "[]\\", "Z^")
check_regex(r"[\*-/]", "*+,-./", "()0")
# Non-standard escape sequences
check_regex(r"[\u0041-\u005A]", "ABCMYZ", ["a", "0", "[", ""])
check_regex(r"[\u0009\u0061-\u007A]", "\tabcmyz", ["A", "0", "{", "", "\n"])
check_regex(r"[\!-\%]", '!"#$%', ["&", " ", "a", ""])
check_regex(r"[\:-\>]", ':;<=>',  ["?", "9", "@", ""])

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print("OK")
else:
    print("Memory leak %d bytes" % (libxml2.debugMemory(1)))
