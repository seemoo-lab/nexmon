Title: Hostname Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2008 Dan Winship

# Hostname Utilities

Functions for manipulating internet hostnames; in particular, for
converting between Unicode and ASCII-encoded forms of
Internationalized Domain Names (IDNs).

The
[Internationalized Domain Names for Applications (IDNA)](http://www.ietf.org/rfc/rfc3490.txt)
standards allow for the use
of Unicode domain names in applications, while providing
backward-compatibility with the old ASCII-only DNS, by defining an
ASCII-Compatible Encoding of any given Unicode name, which can be
used with non-IDN-aware applications and protocols. (For example,
“Παν語.org” maps to “xn--4wa8awb4637h.org”.)

## Hostname Conversions

 * [func@GLib.hostname_to_ascii]
 * [func@GLib.hostname_to_unicode]

## Hostname Checks

 * [func@GLib.hostname_is_non_ascii]
 * [func@GLib.hostname_is_ascii_encoded]
 * [func@GLib.hostname_is_ip_address]

