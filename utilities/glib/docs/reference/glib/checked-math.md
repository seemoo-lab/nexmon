Title: Bounds-checking Integer Arithmetic
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2015 Allison Lortie

# Bounds-checking Integer Arithmetic

GLib offers a set of macros for doing additions and multiplications
of unsigned integers, with checks for overflows.

The helpers all have three arguments.  A pointer to the destination
is always the first argument and the operands to the operation are
the other two.

Following standard GLib convention, the helpers return true in case
of success (ie: no overflow).

The helpers may be macros, normal functions or inlines.  They may be
implemented with inline assembly or compiler intrinsics where
available.

Since: 2.48

The APIs are:

 * [func@GLib.uint_checked_add]
 * [func@GLib.uint_checked_mul]
 * [func@GLib.uint64_checked_add]
 * [func@GLib.uint64_checked_mul]
 * [func@GLib.size_checked_add]
 * [func@GLib.size_checked_mul]
