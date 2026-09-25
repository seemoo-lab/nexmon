Title: D-Bus Error Handling
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 David Zeuthen
SPDX-FileCopyrightText: 2012 Aleksander Morgado

# D-Bus Error Handling

All facilities that return errors from remote methods (such as
[method@Gio.DBusConnection.call_sync]) use [type@GLib.Error] to represent both
D-Bus errors (e.g. errors returned from the other peer) and locally in-process
generated errors.

To check if a returned [type@GLib.Error] is an error from a remote peer, use
[func@Gio.DBusError.is_remote_error]. To get the actual D-Bus error name,
use [func@Gio.DBusError.get_remote_error]. Before presenting an error, always
use [func@Gio.DBusError.strip_remote_error].

In addition, facilities used to return errors to a remote peer also use 
[type@GLib.Error]. See [method@Gio.DBusMethodInvocation.return_error] for
discussion about how the D-Bus error name is set.

Applications can associate a [type@GLib.Error] error domain with a set of D-Bus
errors in order to automatically map from D-Bus errors to [type@GLib.Error] and
back. This is typically done in the function returning the [type@GLib.Quark] for
the error domain:

```c
// foo-bar-error.h:

#define FOO_BAR_ERROR (foo_bar_error_quark ())
GQuark foo_bar_error_quark (void);

typedef enum
{
  FOO_BAR_ERROR_FAILED,
  FOO_BAR_ERROR_ANOTHER_ERROR,
  FOO_BAR_ERROR_SOME_THIRD_ERROR,
  FOO_BAR_N_ERRORS / *< skip >* /
} FooBarError;

// foo-bar-error.c:

static const GDBusErrorEntry foo_bar_error_entries[] =
{
  {FOO_BAR_ERROR_FAILED,           "org.project.Foo.Bar.Error.Failed"},
  {FOO_BAR_ERROR_ANOTHER_ERROR,    "org.project.Foo.Bar.Error.AnotherError"},
  {FOO_BAR_ERROR_SOME_THIRD_ERROR, "org.project.Foo.Bar.Error.SomeThirdError"},
};

// Ensure that every error code has an associated D-Bus error name
G_STATIC_ASSERT (G_N_ELEMENTS (foo_bar_error_entries) == FOO_BAR_N_ERRORS);

GQuark
foo_bar_error_quark (void)
{
  static gsize quark = 0;
  g_dbus_error_register_error_domain ("foo-bar-error-quark",
                                      &quark,
                                      foo_bar_error_entries,
                                      G_N_ELEMENTS (foo_bar_error_entries));
  return (GQuark) quark;
}
```

With this setup, a D-Bus peer can transparently pass e.g.
`FOO_BAR_ERROR_ANOTHER_ERROR` and other peers will see the D-Bus error name
`org.project.Foo.Bar.Error.AnotherError`.

If the other peer is using GDBus, and has registered the association with
[func@Gio.DBusError.register_error_domain] in advance (e.g. by invoking the
`FOO_BAR_ERROR` quark generation itself in the previous example) the peer will
see also `FOO_BAR_ERROR_ANOTHER_ERROR` instead of `G_IO_ERROR_DBUS_ERROR`. Note
that GDBus clients can still recover `org.project.Foo.Bar.Error.AnotherError`
using [func@Gio.DBusError.get_remote_error].

Note that the `G_DBUS_ERROR` error domain is intended only for returning errors
from a remote message bus process. Errors generated locally in-process by e.g.
[class@Gio.DBusConnection] should use the `G_IO_ERROR` domain.

