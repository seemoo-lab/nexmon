Title: Atomic Operations
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2012 Dan Winship

# Atomic Operations

The following is a collection of compiler macros to provide atomic
access to integer and pointer-sized values.

The macros that have ‘int’ in the name will operate on pointers to `gint` and
`guint`.  The macros with ‘pointer’ in the name will operate on pointers to any
pointer-sized value, including `guintptr`.

There is no support for 64-bit operations on platforms with 32-bit pointers
because it is not generally possible to perform these operations atomically.

The get, set and exchange operations for integers and pointers
nominally operate on `gint` and `gpointer`, respectively.  Of the
arithmetic operations, the ‘add’ operation operates on (and returns)
signed integer values (`gint` and `gssize`) and the ‘and’, ‘or’, and
‘xor’ operations operate on (and return) unsigned integer values
(`guint` and `gsize`).

All of the operations act as a full compiler and (where appropriate)
hardware memory barrier.  Acquire and release or producer and
consumer barrier semantics are not available through this API.

It is very important that all accesses to a particular integer or
pointer be performed using only this API and that different sizes of
operation are not mixed or used on overlapping memory regions.  Never
read or assign directly from or to a value — always use this API.

For simple reference counting purposes you should use `gatomicrefcount`
(see [func@GLib.atomic_ref_count_init]) rather than [func@GLib.atomic_int_inc]
and [func@GLib.atomic_int_dec_and_test].

Uses of [func@GLib.atomic_int_inc] and [func@GLib.atomic_int_dec_and_test]
that fall outside of simple counting patterns are prone to
subtle bugs and occasionally undefined behaviour.  It is also worth
noting that since all of these operations require global
synchronisation of the entire machine, they can be quite slow.  In
the case of performing multiple atomic operations it can often be
faster to simply acquire a mutex lock around the critical area,
perform the operations normally and then release the lock.

## Atomic Integer Operations

 * [func@GLib.atomic_int_get]
 * [func@GLib.atomic_int_set]
 * [func@GLib.atomic_int_inc]
 * [func@GLib.atomic_int_dec_and_test]
 * [func@GLib.atomic_int_compare_and_exchange]
 * [func@GLib.atomic_int_compare_and_exchange_full]
 * [func@GLib.atomic_int_exchange]
 * [func@GLib.atomic_int_add]
 * [func@GLib.atomic_int_and]
 * [func@GLib.atomic_int_or]
 * [func@GLib.atomic_int_xor]

## Atomic Pointer Operations

 * [func@GLib.atomic_pointer_get]
 * [func@GLib.atomic_pointer_set]
 * [func@GLib.atomic_pointer_compare_and_exchange]
 * [func@GLib.atomic_pointer_compare_and_exchange_full]
 * [func@GLib.atomic_pointer_exchange]
 * [func@GLib.atomic_pointer_add]
 * [func@GLib.atomic_pointer_and]
 * [func@GLib.atomic_pointer_or]
 * [func@GLib.atomic_pointer_xor]

## Deprecated API

 * [func@GLib.atomic_int_exchange_and_add]

