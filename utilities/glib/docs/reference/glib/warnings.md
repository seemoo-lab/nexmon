Title: Warnings and Assertions
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2018 Endless Mobile, Inc.
SPDX-FileCopyrightText: 2023 Daniel Carpenter

# Warnings and Assertions

GLib defines several warning functions and assertions which can be used to
warn of programmer errors when calling functions, and print error messages
from command line programs.

## Pre-condition Assertions

The [func@GLib.return_if_fail], [func@GLib.return_val_if_fail],
[func@GLib.return_if_reached] and [func@GLib.return_val_if_reached] macros are
intended as pre-condition assertions, to be used at the top of a public function
to check that the function’s arguments are acceptable. Any failure of such a
pre-condition assertion is considered a programming error on the part of the
caller of the public API, and the program is considered to be in an undefined
state afterwards. They are similar to the libc [`assert()`](man:assert(3))
function, but provide more context on failures.

For example:
```c
gboolean
g_dtls_connection_shutdown (GDtlsConnection  *conn,
                            gboolean          shutdown_read,
                            gboolean          shutdown_write,
                            GCancellable     *cancellable,
                            GError          **error)
{
  // local variable declarations

  g_return_val_if_fail (G_IS_DTLS_CONNECTION (conn), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  // function body

  return return_val;
}
```

[func@GLib.warn_if_fail] and [func@GLib.warn_if_reached] behave similarly, but
they will not abort the program on failure. The program should be considered to
be in an undefined state if they fail, however.

## Messages

[func@GLib.print] and [func@GLib.printerr] are intended to be used for
output from command line applications, since they output to standard output
and standard error by default — whereas functions like [func@GLib.message] and
[func@GLib.log] may be redirected to special purpose message windows, files, or
the system journal.

The default print handlers may be overridden with [func@GLib.set_print_handler]
and [func@GLib.set_printerr_handler].

### Encoding

If the console encoding is not UTF-8 (as specified by
[func@GLib.get_console_charset]) then these functions convert the message first.
Any Unicode characters not defined by that charset are replaced by `'?'`. On
Linux, [`setlocale()`](man:setlocale(3)) must be called early in `main()` to
load the encoding. This behaviour can be changed by providing custom handlers to
[func@GLib.set_print_handler], [func@GLib.set_printerr_handler] and
[func@GLib.log_set_handler].

## Debugging Utilities

 * [func@GLib.on_error_query]
 * [func@GLib.on_error_stack_trace]
 * [func@GLib.BREAKPOINT]
