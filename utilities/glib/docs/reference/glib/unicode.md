Title: Unicode
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2011, 2014, 2023 Matthias Clasen
SPDX-FileCopyrightText: 2020 Endless OS Foundation, LLC

# Unicode support

GLib has support for various aspects of Unicode, and provides a number of APIs for dealing
with Unicode characters and strings.

There are analogues of the traditional `ctype.h` character classification and case conversion
functions, UTF-8 analogues of some string utility functions, functions to perform normalization,
case conversion and collation on UTF-8 strings and finally functions to convert between the UTF-8,
UTF-16 and UCS-4 encodings of Unicode.

The implementations of the Unicode functions in GLib are based on the Unicode Character Data tables,
which are available from [www.unicode.org](http://www.unicode.org/).

 - Unicode 4.0 was added in GLib 2.8
 - Unicode 4.1 was added in GLib 2.10
 - Unicode 5.0 was added in GLib 2.12
 - Unicode 5.1 was added in GLib 2.16.3
 - Unicode 6.0 was added in GLib 2.30
 - Unicode 6.1 was added in GLib 2.32
 - Unicode 6.2 was added in GLib 2.36
 - Unicode 6.3 was added in GLib 2.40
 - Unicode 7.0 was added in GLib 2.42
 - Unicode 8.0 was added in GLib 2.48
 - Unicode 9.0 was added in GLib 2.50.1
 - Unicode 10.0 was added in GLib 2.54
 - Unicode 11.10 was added in GLib 2.58
 - Unicode 12.0 was added in GLib 2.62
 - Unicode 12.1 was added in GLib 2.62
 - Unicode 13.0 was added in GLib 2.66
 - Unicode 14.0 was added in GLib 2.71
 - Unicode 15.0 was added in GLib 2.76

