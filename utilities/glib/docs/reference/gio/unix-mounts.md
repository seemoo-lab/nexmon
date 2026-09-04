Title: UNIX Mounts
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2007 Andrew Walton
SPDX-FileCopyrightText: 2008 Matthias Clasen

# UNIX Mounts

Routines for managing mounted UNIX mount points and paths.

Note that `<gio/gunixmounts.h>` belongs to the UNIX-specific GIO
interfaces, thus you have to use the `gio-unix-2.0.pc` pkg-config
file or the `GioUnix-2.0` GIR namespace when using it.

There are three main classes:

 * [struct@GioUnix.MountEntry]
 * [struct@GioUnix.MountPoint]
 * [class@GioUnix.MountMonitor]

Various helper functions for querying mounts:

 * [func@GioUnix.mount_points_get]
 * [func@GioUnix.MountPoint.at]
 * [func@GioUnix.mounts_get]
 * [func@GioUnix.mount_at]
 * [func@GioUnix.mount_for]
 * [func@GioUnix.mounts_changed_since]
 * [func@GioUnix.mount_points_changed_since]

And several helper functions for checking the type of a mount or path:

 * [func@GioUnix.is_mount_path_system_internal]
 * [func@GioUnix.is_system_fs_type]
 * [func@GioUnix.is_system_device_path]

