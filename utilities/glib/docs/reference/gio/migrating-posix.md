Title: Migrating from POSIX to GIO
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2007 Matthias Clasen

# Migrating from POSIX to GIO

## Comparison of POSIX and GIO concepts

The final two entries in this table require including `gio-unix-2.0.pc` as well
as `gio-2.0.pc` in your build (or, in GIR namespace terms, `GioUnix-2.0` as well
as `Gio-2.0`).

| POSIX                 | GIO                                                         |
|-----------------------|-------------------------------------------------------------|
| `char *path`          | [iface@Gio.File]                                            |
| `struct stat *buf`    | [class@Gio.FileInfo]                                        |
| `struct statvfs *buf` | [class@Gio.FileInfo]                                        |
| `int fd`              | [class@Gio.InputStream] or [class@Gio.OutputStream]         |
| `DIR *`               | [class@Gio.FileEnumerator]                                  |
| `fstab entry`         | [`GUnixMountPoint`](../gio-unix/struct.UnixMountPoint.html) |
| `mtab entry`          | [`GUnixMountEntry`](../gio-unix/struct.UnixMountEntry.html) |
