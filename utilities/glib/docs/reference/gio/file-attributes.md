Title: File Attributes
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2007 Andrew Walton
SPDX-FileCopyrightText: 2007 Alexander Larsson
SPDX-FileCopyrightText: 2008, 2014 Matthias Clasen
SPDX-FileCopyrightText: 2011 Murray Cumming
SPDX-FileCopyrightText: 2012 David King

# File Attributes

File attributes in GIO consist of a list of key-value pairs.

Keys are strings that contain a key namespace and a key name, separated
by a colon, e.g. `namespace::keyname`. Namespaces are included to sort
key-value pairs by namespaces for relevance. Keys can be retrieved
using wildcards, e.g. `standard::*` will return all of the keys in the
`standard` namespace.

The list of possible attributes for a filesystem (pointed to by a
[iface@Gio.File]) is available as a [struct@Gio.FileAttributeInfoList]. This
list is queryable by key names as indicated earlier.

Information is stored within the list in [struct@Gio.FileAttributeInfo]
structures. The info structure can store different types, listed in the enum
[type@Gio.FileAttributeType]. Upon creation of a [struct@Gio.FileAttributeInfo],
the type will be set to `G_FILE_ATTRIBUTE_TYPE_INVALID`.

Classes that implement [iface@Gio.File] will create a
[struct@Gio.FileAttributeInfoList] and install default keys and values for their
given file system, architecture, and other possible implementation details
(e.g., on a UNIX system, a file attribute key will be registered for the user ID
for a given file).

## Default Namespaces

- `"standard"`: The ‘Standard’ namespace. General file information that
  any application may need should be put in this namespace. Examples
  include the file’s name, type, and size.
- `"etag`: The [Entity Tag](gfile.html#entity-tags) namespace. Currently, the
  only key in this namespace is `value`, which contains the value of the current
  entity tag.
- `"id"`: The ‘Identification’ namespace. This namespace is used by file
  managers and applications that list directories to check for loops and
  to uniquely identify files.
- `"access"`: The ‘Access’ namespace. Used to check if a user has the
  proper privileges to access files and perform file operations. Keys in
  this namespace are made to be generic and easily understood, e.g. the
  `can_read` key is true if the current user has permission to read the
  file. UNIX permissions and NTFS ACLs in Windows should be mapped to
  these values.
- `"mountable"`: The ‘Mountable’ namespace. Includes simple boolean keys
  for checking if a file or path supports mount operations, e.g. mount,
  unmount, eject. These are used for files of type `G_FILE_TYPE_MOUNTABLE`.
- `"time"`: The ‘Time’ namespace. Includes file access, changed, created
  times.
- `"unix"`: The ‘Unix’ namespace. Includes UNIX-specific information and
  may not be available for all files. Examples include the UNIX UID,
  GID, etc.
- `"dos"`: The ‘DOS’ namespace. Includes DOS-specific information and may
  not be available for all files. Examples include `is_system` for checking
  if a file is marked as a system file, and `is_archive` for checking if a
  file is marked as an archive file.
- `"owner"`: The ‘Owner’ namespace. Includes information about who owns a
  file. May not be available for all file systems. Examples include `user`
  for getting the user name of the file owner. This information is often
  mapped from some backend specific data such as a UNIX UID.
- `"thumbnail"`: The ‘Thumbnail’ namespace. Includes information about file
  thumbnails and their location within the file system. Examples of keys in
  this namespace include `path` to get the location of a thumbnail, `failed`
  to check if thumbnailing of the file failed, and `is-valid` to check if
  the thumbnail is outdated.
- `"filesystem"`: The ‘Filesystem’ namespace. Gets information about the
  file system where a file is located, such as its type, how much space is
  left available, and the overall size of the file system.
- `"gvfs"`: The ‘GVFS’ namespace. Keys in this namespace contain information
  about the current GVFS backend in use.
- `"xattr"`: The ‘xattr’ namespace. Gets information about extended user
  attributes. See [`xattr(7)`](man:xattr(7)). The `user.` prefix of the extended
  user attribute name is stripped away when constructing keys in this namespace,
  e.g. `xattr::mime_type` for the extended attribute with the name
  `user.mime_type`. Note that this information is only available if
  GLib has been built with extended attribute support.
- `"xattr-sys"`: The ‘xattr-sys’ namespace. Gets information about
  extended attributes which are not user-specific. See [`xattr(7)`](man:xattr(7)).
  Note that this information is only available if GLib has been built with
  extended attribute support.
- `"selinux"`: The ‘SELinux’ namespace. Includes information about the
  SELinux context of files. Note that this information is only available
  if GLib has been built with SELinux support.

Please note that these are not all of the possible namespaces.
More namespaces can be added from GIO modules or by individual applications.
For more information about writing GIO modules, see [class@Gio.IOModule].

<!-- TODO: Implementation note about using extended attributes on supported
file systems -->

## Default Keys

For a list of the built-in keys and their types, see the [class@Gio.FileInfo]
documentation.

Note that there are no predefined keys in the `xattr` and `xattr-sys`
namespaces. Keys for the `xattr` namespace are constructed by stripping
away the `user.` prefix from the extended user attribute, and prepending
`xattr::`. Keys for the `xattr-sys` namespace are constructed by
concatenating `xattr-sys::` with the extended attribute name. All extended
attribute values are returned as hex-encoded strings in which bytes outside
the ASCII range are encoded as escape sequences of the form `\xnn`
where `nn` is a 2-digit hexadecimal number.

