Title: Content Types
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2006, 2007 Red Hat, Inc.
SPDX-FileCopyrightText: 2007 Alexander Larsson

# Content Types

A content type is a platform specific string that defines the type of a file.
On UNIX it is a [MIME type](http://www.wikipedia.org/wiki/Internet_media_type)
like `text/plain` or `image/png`.
On Win32 it is an extension string like `.doc`, `.txt` or a perceived
string like `audio`. Such strings can be looked up in the registry at
`HKEY_CLASSES_ROOT`.
On macOS it is a [Uniform Type Identifier](https://en.wikipedia.org/wiki/Uniform_Type_Identifier)
such as `com.apple.application`.
