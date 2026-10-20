Title: Unix-specific Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2011 Colin Walters

# Unix-specific Utilities

Most of GLib is intended to be portable; in contrast, this set of
functions is designed for programs which explicitly target Unix,
or are using it to build higher level abstractions which would be
conditionally compiled if the platform matches `G_OS_UNIX`.

To use these functions, you must explicitly include the
`glib-unix.h` header.

## File Descriptors

 * [func@GLibUnix.open_pipe]
 * [func@GLibUnix.set_fd_nonblocking]
 * [func@GLibUnix.fd_query_path]

## Pipes

The [struct@GLibUnix.Pipe] structure can be used to conveniently open and
manipulate a Unix pipe.

The methods for it are all static inline for efficiency. They are:

 * [method@GLibUnix.Pipe.open]
 * [method@GLibUnix.Pipe.get]
 * [method@GLibUnix.Pipe.steal]
 * [method@GLibUnix.Pipe.close]
 * [method@GLibUnix.Pipe.clear]

## Signals

 * [func@GLibUnix.signal_add]
 * [func@GLibUnix.signal_add_full]
 * [func@GLibUnix.signal_source_new]

## Polling

 * [func@GLibUnix.fd_add]
 * [func@GLibUnix.fd_add_full]
 * [func@GLibUnix.fd_source_new]

## User Database

 * [func@GLibUnix.get_passwd_entry]
