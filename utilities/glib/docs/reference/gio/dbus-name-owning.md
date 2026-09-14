Title: D-Bus Name Owning
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 David Zeuthen

# D-Bus Name Owning

Convenience API for owning bus names.

A simple example for owning a name can be found in
[`gdbus-example-own-name.c`](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-own-name.c).

The main API for owning names is:
 * [func@Gio.bus_own_name]
 * [func@Gio.bus_unown_name]
