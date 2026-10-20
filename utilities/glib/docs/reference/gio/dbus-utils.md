Title: D-Bus Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 David Zeuthen

# D-Bus Utilities

Various utility routines related to D-Bus.

GUID support:
 * [func@Gio.dbus_is_guid]
 * [func@Gio.dbus_generate_guid]

Name validation:
 * [func@Gio.dbus_is_name]
 * [func@Gio.dbus_is_unique_name]
 * [func@Gio.dbus_is_member_name]
 * [func@Gio.dbus_is_interface_name]
 * [func@Gio.dbus_is_error_name]

Conversion between [type@GLib.Variant] and [type@GObject.Value]:
 * [func@Gio.dbus_gvariant_to_gvalue]
 * [func@Gio.dbus_gvalue_to_gvariant]

Path escaping:
 * [func@Gio.dbus_escape_object_path_bytestring]
 * [func@Gio.dbus_escape_object_path]
 * [func@Gio.dbus_unescape_object_path]

