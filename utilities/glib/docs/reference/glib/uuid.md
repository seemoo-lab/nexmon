Title: GUuid
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2017 Bastien Nocera
SPDX-FileCopyrightText: 2017 Marc-André Lureau

# GUuid

A UUID, or Universally unique identifier, is intended to uniquely
identify information in a distributed environment. For the
definition of UUID, see [RFC 4122](https://tools.ietf.org/html/rfc4122.html).

The creation of UUIDs does not require a centralized authority.

UUIDs are of relatively small size (128 bits, or 16 bytes). The
common string representation (ex:
`1d6c0810-2bd6-45f3-9890-0268422a6f14`) needs 37 bytes.
[func@GLib.uuid_string_is_valid] can be used to check whether a string is a
valid UUID.

The UUID specification defines 5 versions, and calling
[func@GLib.uuid_string_random] will generate a unique (or rather random)
UUID of the most common version, version 4.

UUID support was added to GLib in version 2.52.

