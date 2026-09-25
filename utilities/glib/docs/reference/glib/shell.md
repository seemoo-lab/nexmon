Title: Shell Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2014 Matthias Clasen

# Shell Utilities

GLib provides the functions [func@GLib.shell_quote] and
[func@GLib.shell_unquote] to handle shell-like quoting in strings. The function
[func@GLib.shell_parse_argv] parses a string similar to the way a POSIX shell
(`/bin/sh`) would.

Note that string handling in shells has many obscure and historical
corner-cases which these functions do not necessarily reproduce. They
are good enough in practice, though.

