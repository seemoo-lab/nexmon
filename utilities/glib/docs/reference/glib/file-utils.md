Title: File Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2012 Dan Winship

# File Utilities

Do not use these APIs unless you are porting a POSIX application to Windows.
A more high-level file access API is provided as GIO; see the documentation
for [`GFile`](../gio/iface.File.html).

## POSIX File Wrappers

There is a group of functions which wrap the common POSIX functions
dealing with filenames:

 * [func@GLib.access]
 * [func@GLib.chdir]
 * [func@GLib.chmod]
 * [func@GLib.close]
 * [func@GLib.creat]
 * [func@GLib.fopen],
 * [func@GLib.freopen]
 * [func@GLib.fsync]
 * [func@GLib.lstat]
 * [func@GLib.mkdir]
 * [func@GLib.open]
 * [func@GLib.remove]
 * [func@GLib.rename]
 * [func@GLib.rmdir]
 * [func@GLib.stat]
 * [func@GLib.unlink]
 * [func@GLib.utime]

The point of these wrappers is to make it possible to handle file names with any
Unicode characters in them on Windows without having to use `#ifdef`s and the
wide character API in the application code.

On some Unix systems, these APIs may be defined as identical to their POSIX
counterparts. For this reason, you must check for and include the necessary
header files (such as `fcntl.h`) before using functions like [func@GLib.creat].
You must also define the relevant feature test macros.

The pathname argument should be in the GLib file name encoding.
On POSIX this is the actual on-disk encoding which might correspond
to the locale settings of the process (or the `G_FILENAME_ENCODING`
environment variable), or not.

On Windows the GLib file name encoding is UTF-8. Note that the
Microsoft C library does not use UTF-8, but has separate APIs for
current system code page and wide characters (UTF-16). The GLib
wrappers call the wide character API if present (on modern Windows
systems), otherwise convert to/from the system code page.

## POSIX Directory Wrappers

Another group of functions allows to open and read directories
in the GLib file name encoding:

 * [ctor@GLib.Dir.open]
 * [method@GLib.Dir.read_name]
 * [method@GLib.Dir.rewind]
 * [method@GLib.Dir.close]

## Error Handling

 * [func@GLib.file_error_from_errno]

## Setting/Getting File Contents

 * [func@GLib.file_get_contents]
 * [func@GLib.file_set_contents]
 * [func@GLib.file_set_contents_full]

## File Tests

 * [func@GLib.file_test]
 * [func@GLib.file_read_link]

## Temporary File Handling

 * [func@GLib.mkdtemp]
 * [func@GLib.mkdtemp_full]
 * [func@GLib.mkstemp]
 * [func@GLib.mkstemp_full]
 * [func@GLib.file_open_tmp]
 * [id@g_dir_make_tmp]

## Building and Manipulating Paths

 * [func@GLib.build_path]
 * [func@GLib.build_pathv]
 * [func@GLib.build_filename]
 * [func@GLib.build_filenamev]
 * [func@GLib.build_filename_valist]
 * [func@GLib.IS_DIR_SEPARATOR]
 * [func@GLib.path_is_absolute]
 * [func@GLib.path_skip_root]
 * [func@GLib.get_current_dir]
 * [func@GLib.path_get_basename]
 * [func@GLib.path_get_dirname]
 * [func@GLib.canonicalize_filename]

## Creating Directories

 * [func@GLib.mkdir_with_parents]

## Deprecated API

 * [func@GLib.basename]

