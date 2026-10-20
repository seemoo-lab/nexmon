Title: Numerical Definitions
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2001 Havoc Pennington
SPDX-FileCopyrightText: 2010 Red Hat, Inc.

# Numerical Definitions

GLib offers mathematical constants such as [const@GLib.PI] for the value of pi;
many platforms have these in the C library, but some don’t. The GLib
versions always exist.

The [type@GLib.FloatIEEE754] and [type@GLib.DoubleIEEE754] unions are used to
access the sign, mantissa and exponent of IEEE floats and doubles. These unions
are defined as appropriate for a given platform. IEEE floats and doubles are
supported (used for storage) by at least Intel, PPC and Sparc. See
[IEEE 754-2008](http://en.wikipedia.org/wiki/IEEE_float)
for more information about IEEE number formats.

## Floating Point

 * [const@GLib.IEEE754_FLOAT_BIAS]
 * [const@GLib.IEEE754_DOUBLE_BIAS]
 * [type@GLib.FloatIEEE754]
 * [type@GLib.DoubleIEEE754]

## Numerical Constants

 * [const@GLib.E]
 * [const@GLib.LN2]
 * [const@GLib.LN10]
 * [const@GLib.PI]
 * [const@GLib.PI_2]
 * [const@GLib.PI_4]
 * [const@GLib.SQRT2]
 * [const@GLib.LOG_2_BASE_10]
