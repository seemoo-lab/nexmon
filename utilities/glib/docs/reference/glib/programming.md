Title: Writing GLib Applications
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2012 Red Hat, Inc.
SPDX-FileCopyrightText: 2023 Endless OS Foundation, LLC

# Writing GLib Applications

## Memory Allocations

Unless otherwise specified, all functions which allocate memory in GLib will
abort the process if heap allocation fails. This is because it is too hard to
recover from allocation failures in any non-trivial program and, in particular,
to test all the allocation failure code paths.

## UTF-8 and String Encoding

All GLib, GObject and GIO functions accept and return strings in
[UTF-8 encoding](https://en.wikipedia.org/wiki/UTF-8) unless otherwise
specified.

Input strings to function calls are *not* checked to see if
they are valid UTF-8: it is the application developer’s responsibility to
validate input strings at the time of input, either at the program or library
boundary, and to only use valid UTF-8 string constants in their application.
If GLib were to UTF-8 validate all string inputs to all functions, there would
be a significant drop in performance.

Similarly, output strings from functions are guaranteed to be in UTF-8,
and this does not need to be validated by the calling function. If a function
returns invalid UTF-8 (and is not documented as doing so), that’s a bug.

This equally applies to functions which take an input length in bytes: the
string up to that point must be valid UTF-8, which in particular means it must
not end part-way through a character.

See [func@GLib.utf8_validate] and [func@GLib.utf8_make_valid] for validating
UTF-8 input.

## Threads

The general policy of GLib is that all functions are invisibly threadsafe
with the exception of data structure manipulation functions, where, if
you have two threads manipulating the *same* data structure, they must use a
lock to synchronize their operation.

GLib creates a worker thread for its own purposes so GLib applications
will always have at least 2 threads.

In particular, this means that programs must only use
[async-signal-safe functions](man:signal-safety(7)) between
calling [`fork()`](man:fork(2)) and [`exec()`](man:exec(3)), even if
they haven’t explicitly spawned another thread yet.

See the sections on [threads](threads.html) and [struct@GLib.ThreadPool] for
GLib APIs that support multithreaded applications.

## Security and setuid Use

When writing code that runs with elevated privileges, it is important
to follow some basic rules of secure programming. David Wheeler has an
excellent book on this topic,
[Secure Programming for Linux and Unix HOWTO](http://www.dwheeler.com/secure-programs/Secure-Programs-HOWTO/index.html).

When it comes to GLib and its associated libraries, GLib and
GObject are generally fine to use in code that runs with elevated
privileges; they don’t load modules (executable code in shared objects)
or run other programs ‘behind your back’. GIO, however, is not designed to be
used in privileged programs, either ones which are spawned by a privileged
process, or ones which are run with a setuid bit set.

setuid programs should always reset their environment to contain only
known-safe values before calling into non-trivial libraries such as GIO. This
reduces the risk of an attacker-controlled environment variable being used to
get a privileged GIO process to run arbitrary code via loading a GIO module or
similar.
