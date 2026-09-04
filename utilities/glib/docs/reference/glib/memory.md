Title: Memory Allocation
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2000, 2019 Red Hat, Inc.
SPDX-FileCopyrightText: 2007 Emmanuele Bassi
SPDX-FileCopyrightText: 2018 Pavlo Solntsev
SPDX-FileCopyrightText: 2020 Endless Mobile, Inc.

# Memory Allocation

These functions provide support for allocating and freeing memory.

If any call to allocate memory using functions [func@GLib.new],
[func@GLib.new0], [func@GLib.renew], [func@GLib.malloc], [func@GLib.malloc0],
[func@GLib.malloc0_n], [func@GLib.realloc] and [func@GLib.realloc_n]
fails, the application is terminated. This also means that there is no
need to check if the call succeeded.

On the other hand, the `g_try_…()` family of functions returns `NULL` on failure
that can be used as a check for unsuccessful memory allocation. The application
is not terminated in this case.

As all GLib functions and data structures use [func@GLib.malloc] internally,
unless otherwise specified, any allocation failure will result in the
application being terminated.

It’s important to match [func@GLib.malloc] (and wrappers such as
[func@GLib.new]) with [func@GLib.free], [func@GLib.slice_alloc] (and wrappers
such as [func@GLib.slice_new]) with [func@GLib.slice_free], plain
[`malloc()`](man:malloc(3)) with [`free()`](man:free(3)), and (if you’re using
C++) `new` with `delete` and `new[]` with `delete[]`. Otherwise bad things can
happen, since these allocators may use different memory pools (and
`new`/`delete` call constructors and destructors).

Since GLib 2.46, [func@GLib.malloc] is hardcoded to always use the system malloc
implementation.

## Struct Allocations

 * [func@GLib.new]
 * [func@GLib.new0]
 * [func@GLib.renew]
 * [func@GLib.try_new]
 * [func@GLib.try_new0]
 * [func@GLib.try_renew]

## Block Allocations

 * [func@GLib.malloc]
 * [func@GLib.malloc0]
 * [func@GLib.realloc]
 * [func@GLib.try_malloc]
 * [func@GLib.try_malloc0]
 * [func@GLib.try_realloc]
 * [func@GLib.malloc_n]
 * [func@GLib.malloc0_n]
 * [func@GLib.realloc_n]
 * [func@GLib.try_malloc_n]
 * [func@GLib.try_malloc0_n]
 * [func@GLib.try_realloc_n]

## Free Functions

 * [func@GLib.free]
 * [func@GLib.free_sized]
 * [func@GLib.clear_pointer]
 * [func@GLib.steal_pointer]

In addition, the `g_mem_gc_friendly` exported variable will be true if GLib has
been [run with `G_DEBUG=gc-friendly`](running.html#environment-variables). If
so, memory to be freed will be cleared to zero before being freed.

## Stack Allocations

 * [func@GLib.alloca]
 * [func@GLib.alloca0]
 * [func@GLib.newa]
 * [func@GLib.newa0]

## Aligned Allocations

 * [func@GLib.aligned_alloc]
 * [func@GLib.aligned_alloc0]
 * [func@GLib.aligned_free]
 * [func@GLib.aligned_free_sized]

## Copies and Moves

 * [func@GLib.memmove]
 * [func@GLib.memdup2]

## Deprecated API

 * [func@GLib.memdup]
 * [struct@GLib.MemVTable]
 * [func@GLib.mem_set_vtable]
 * [func@GLib.mem_is_system_malloc]
 * `glib_mem_profiler_table` exported variable
 * [func@GLib.mem_profile]

