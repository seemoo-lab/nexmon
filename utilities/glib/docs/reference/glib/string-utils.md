Title: String Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 1999 Owen Taylor
SPDX-FileCopyrightText: 2000 Red Hat, Inc.
SPDX-FileCopyrightText: 2002, 2003, 2014 Matthias Clasen
SPDX-FileCopyrightText: 2015 Collabora, Ltd.

# String Utilities

This section describes a number of utility functions for creating,
duplicating, and manipulating strings.

Note that the functions [func@GLib.printf], [func@GLib.fprintf],
[func@GLib.sprintf], [func@GLib.vprintf], [func@GLib.vfprintf],
[func@GLib.vsprintf] and [func@GLib.vasprintf] are declared in the header
`gprintf.h` which is not included in `glib.h`
(otherwise using `glib.h` would drag in `stdio.h`), so you'll have to
explicitly include `<glib/gprintf.h>` in order to use the GLib
`printf()` functions.

## String precision pitfalls

While you may use the `printf()` functions to format UTF-8 strings,
notice that the precision of a `%Ns` parameter is interpreted
as the number of bytes, not characters to print. On top of that,
the GNU libc implementation of the `printf()` functions has the
‘feature’ that it checks that the string given for the `%Ns`
parameter consists of a whole number of characters in the current
encoding. So, unless you are sure you are always going to be in an
UTF-8 locale or your know your text is restricted to ASCII, avoid
using `%Ns`. If your intention is to format strings for a
certain number of columns, then `%Ns` is not a correct solution
anyway, since it fails to take wide characters (see [func@GLib.unichar_iswide])
into account.

Note also that there are various `printf()` parameters which are platform
dependent. GLib provides platform independent macros for these parameters
which should be used instead. A common example is [const@GLib.GUINT64_FORMAT],
which should be used instead of `%llu` or similar parameters for formatting
64-bit integers. These macros are all named `G_*_FORMAT`; see
[Basic Types](types.html).

## General String Manipulation

 * [func@GLib.strdup]
 * [func@GLib.strndup]
 * [func@GLib.strdupv]
 * [func@GLib.strnfill]
 * [func@GLib.stpcpy]
 * [func@GLib.strstr_len]
 * [func@GLib.strrstr]
 * [func@GLib.strrstr_len]
 * [func@GLib.str_has_prefix]
 * [func@GLib.str_has_suffix]
 * [func@GLib.strcmp0]
 * [func@GLib.str_to_ascii]
 * [func@GLib.str_tokenize_and_fold]
 * [func@GLib.str_match_string]

For users of GLib in C, the [func@GLib.set_str] and [func@GLib.set_str_take]
inline functions also exist to set a string and handle copying the new value
and freeing the old one. Similarly, [func@GLib.set_strv] and
[func@GLib.set_strv_take] exist to set a string array.

## String Copying

 * [func@GLib.strlcpy]
 * [func@GLib.strlcat]

## Printing

 * [func@GLib.strdup_printf]
 * [func@GLib.strdup_vprintf]
 * [func@GLib.printf]
 * [func@GLib.vprintf]
 * [func@GLib.fprintf]
 * [func@GLib.vfprintf]
 * [func@GLib.sprintf]
 * [func@GLib.vsprintf]
 * [func@GLib.snprintf]
 * [func@GLib.vsnprintf]
 * [func@GLib.vasprintf]
 * [func@GLib.printf_string_upper_bound]

## ASCII

 * [func@GLib.str_is_ascii]
 * [func@GLib.ascii_isalnum]
 * [func@GLib.ascii_isalpha]
 * [func@GLib.ascii_iscntrl]
 * [func@GLib.ascii_isdigit]
 * [func@GLib.ascii_isgraph]
 * [func@GLib.ascii_islower]
 * [func@GLib.ascii_isprint]
 * [func@GLib.ascii_ispunct]
 * [func@GLib.ascii_isspace]
 * [func@GLib.ascii_isupper]
 * [func@GLib.ascii_isxdigit]

## ASCII Parsing

 * [func@GLib.ascii_digit_value]
 * [func@GLib.ascii_xdigit_value]

## ASCII Comparisons

 * [func@GLib.ascii_strcasecmp]
 * [func@GLib.ascii_strncasecmp]

## ASCII Case Manipulation

 * [func@GLib.ascii_strup]
 * [func@GLib.ascii_strdown]
 * [func@GLib.ascii_tolower]
 * [func@GLib.ascii_toupper]

## ASCII String Manipulation

 * [func@GLib.strreverse]

## ASCII Number Manipulation

 * [func@GLib.ascii_strtoll]
 * [func@GLib.ascii_strtoull]
 * [const@GLib.ASCII_DTOSTR_BUF_SIZE]
 * [func@GLib.ascii_strtod]
 * [func@GLib.ascii_dtostr]
 * [func@GLib.ascii_formatd]
 * [func@GLib.strtod]

## ASCII Number Parsing

 * [type@GLib.NumberParserError]
 * [func@GLib.ascii_string_to_signed]
 * [func@GLib.ascii_string_to_unsigned]

## Whitespace Removal

 * [func@GLib.strchug]
 * [func@GLib.strchomp]
 * [func@GLib.strstrip]

## Find and Replace

 * [func@GLib.strdelimit]
 * [const@GLib.STR_DELIMITERS]
 * [func@GLib.strescape]
 * [func@GLib.strcompress]
 * [func@GLib.strcanon]

## Splitting and Joining

 * [func@GLib.strsplit]
 * [func@GLib.strsplit_set]
 * [func@GLib.strconcat]
 * [func@GLib.strjoin]
 * [func@GLib.strjoinv]

## String Arrays

 * [type@GLib.Strv]
 * [func@GLib.strfreev]
 * [func@GLib.strv_length]
 * [func@GLib.strv_contains]
 * [func@GLib.strv_equal]

## String Array Builder

 * [type@GLib.StrvBuilder]
 * [ctor@GLib.StrvBuilder.new]
 * [method@GLib.StrvBuilder.ref]
 * [method@GLib.StrvBuilder.unref]
 * [method@GLib.StrvBuilder.add]
 * [method@GLib.StrvBuilder.addv]
 * [method@GLib.StrvBuilder.add_many]
 * [method@GLib.StrvBuilder.take]
 * [method@GLib.StrvBuilder.end]

## POSIX Errors

 * [func@GLib.strerror]
 * [func@GLib.strsignal]

## Deprecated API

 * [func@GLib.strup]
 * [func@GLib.strdown]
 * [func@GLib.strcasecmp]
 * [func@GLib.strncasecmp]
