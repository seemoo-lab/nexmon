Title: D-Bus Introspection Data
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 David Zeuthen
SPDX-FileCopyrightText: 2010 Matthias Clasen

# D-Bus Introspection Data

Various data structures and convenience routines to parse and
generate D-Bus introspection XML. Introspection information is
used when registering objects with [method@Gio.DBusConnection.register_object].

The format of D-Bus introspection XML is specified in the
[D-Bus specification](http://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format).

The main introspection data structures are:
 * [type@Gio.DBusNodeInfo]
 * [type@Gio.DBusInterfaceInfo]
 * [type@Gio.DBusPropertyInfo]
 * [type@Gio.DBusMethodInfo]
 * [type@Gio.DBusSignalInfo]
 * [type@Gio.DBusArgInfo]
 * [type@Gio.DBusAnnotationInfo]

