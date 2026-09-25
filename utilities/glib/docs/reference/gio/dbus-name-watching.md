Title: D-Bus Name Watching
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 David Zeuthen

# D-Bus Name Watching

Convenience API for watching bus names.

A simple example for watching a name can be found in
[`gdbus-example-watch-name.c`](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-watch-name.c).

The main API for watching names is:
 * [func@Gio.bus_watch_name]
 * [func@Gio.bus_unwatch_name]
