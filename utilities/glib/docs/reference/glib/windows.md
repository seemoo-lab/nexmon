Title: Windows-specific Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2005 Ross Burton

# Windows-specific Utilities

These functions provide some level of Unix emulation on the
Windows platform. If your application really needs the POSIX
APIs, we suggest you try the [Cygwin project](https://cygwin.com/).

 * [type@GLibWin32.OSType]
 * [func@GLibWin32.check_windows_version]
 * [func@GLibWin32.get_command_line]
 * [func@GLibWin32.error_message]
 * [func@GLibWin32.getlocale]
 * [func@GLibWin32.get_package_installation_directory]
 * [func@GLibWin32.get_package_installation_directory_of_module]
 * [func@GLibWin32.get_package_installation_subdirectory]
 * [func@GLibWin32.get_windows_version]
 * [func@GLibWin32.locale_filename_from_utf8]
 * [func@GLibWin32.HAVE_WIDECHAR_API]
 * [func@GLibWin32.IS_NT_BASED]

## Deprecated API

 * [func@GLib.WIN32_DLLMAIN_FOR_DLL_NAME]
