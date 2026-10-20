Title: gnetworking.h
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 Dan Winship

# gnetworking.h

The `<gio/gnetworking.h>` header can be included to get
various low-level networking-related system headers, automatically
taking care of certain portability issues for you.

This can be used, for example, if you want to call
[`setsockopt()`](man:setsockopt(2)) on a [class@Gio.Socket].

Note that while WinSock has many of the same APIs as the
traditional UNIX socket API, most of them behave at least slightly
differently (particularly with respect to error handling). If you
want your code to work under both UNIX and Windows, you will need
to take these differences into account.

Also, under GNU libc, certain non-portable functions are only visible
in the headers if you define `_GNU_SOURCE` before including them. Note
that this symbol must be defined before including any headers, or it
may not take effect.

There is one function provided specifically for initialising the networking
APIs, which needs to be called only if they are being used before GLib
initialises itself:

 * [func@Gio.networking_init]
