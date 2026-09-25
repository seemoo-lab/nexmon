/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2006-2007 Red Hat, Inc.
 * Copyright 2026 Christian Hergert
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

/* keep sorted for bsearch */
static const char *system_mount_paths[] = {
  /* Includes all FHS 2.3 toplevel dirs and other specialized
   * directories that we want to hide from the user.
   */
  "/",              /* we already have "Filesystem root" in Nautilus */
  "/bin",
  "/boot",
  "/compat/linux/proc",
  "/compat/linux/sys",
  "/dev",
  "/etc",
  "/home",
  "/lib",
  "/lib64",
  "/libexec",
  "/live/cow",
  "/live/image",
  "/media",
  "/mnt",
  "/net",
  "/opt",
  "/proc",
  "/rescue",
  "/root",
  "/sbin",
  "/sbin",
  "/srv",
  "/sys",
  "/tmp",
  "/usr",
  "/usr/X11R6",
  "/usr/local",
  "/usr/obj",
  "/usr/ports",
  "/usr/src",
  "/usr/xobj",
  "/var",
  "/var/crash",
  "/var/local",
  "/var/log",
  "/var/log/audit", /* https://bugzilla.redhat.com/show_bug.cgi?id=333041 */
  "/var/mail",
  "/var/run",
  "/var/tmp",       /* https://bugzilla.redhat.com/show_bug.cgi?id=335241 */
};
