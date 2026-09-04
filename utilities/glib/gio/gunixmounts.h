/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_UNIX_MOUNTS_H__
#define __G_UNIX_MOUNTS_H__

#include <gio/gio.h>
#include <stdint.h>

G_BEGIN_DECLS

/**
 * GUnixMountEntry:
 *
 * Defines a Unix mount entry (e.g. `/media/cdrom`).
 * This corresponds roughly to a mtab entry.
 **/
typedef struct _GUnixMountEntry GUnixMountEntry;

#define G_TYPE_UNIX_MOUNT_ENTRY (g_unix_mount_entry_get_type ())
GIO_AVAILABLE_IN_2_54
GType g_unix_mount_entry_get_type (void);

/**
 * GUnixMountPoint:
 *
 * Defines a Unix mount point (e.g. `/dev`).
 * This corresponds roughly to a fstab entry.
 **/
typedef struct _GUnixMountPoint GUnixMountPoint;

#define G_TYPE_UNIX_MOUNT_POINT (g_unix_mount_point_get_type ())
GIO_AVAILABLE_IN_2_54
GType g_unix_mount_point_get_type (void);

/**
 * GUnixMountMonitor:
 *
 * Watches for changes to the set of mount entries and mount points in the
 * system.
 *
 * Connect to the [signal@GioUnix.MountMonitor::mounts-changed] signal to be
 * notified of changes to the [struct@GioUnix.MountEntry] list.
 *
 * Connect to the [signal@GioUnix.MountMonitor::mountpoints-changed] signal to
 * be notified of changes to the [struct@GioUnix.MountPoint] list.
 **/
typedef struct _GUnixMountMonitor      GUnixMountMonitor;
typedef struct _GUnixMountMonitorClass GUnixMountMonitorClass;

#define G_TYPE_UNIX_MOUNT_MONITOR        (g_unix_mount_monitor_get_type ())
#define G_UNIX_MOUNT_MONITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_UNIX_MOUNT_MONITOR, GUnixMountMonitor))
#define G_UNIX_MOUNT_MONITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_UNIX_MOUNT_MONITOR, GUnixMountMonitorClass))
#define G_IS_UNIX_MOUNT_MONITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_UNIX_MOUNT_MONITOR))
#define G_IS_UNIX_MOUNT_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_UNIX_MOUNT_MONITOR))
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixMountMonitor, g_object_unref)

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_free)
void           g_unix_mount_free                    (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
void           g_unix_mount_entry_free              (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_copy)
GUnixMountEntry *g_unix_mount_copy                  (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
GUnixMountEntry *g_unix_mount_entry_copy            (GUnixMountEntry    *mount_entry);

GIO_AVAILABLE_IN_ALL
void           g_unix_mount_point_free              (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_2_54
GUnixMountPoint *g_unix_mount_point_copy            (GUnixMountPoint    *mount_point);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_compare)
gint           g_unix_mount_compare                 (GUnixMountEntry    *mount1,
                                                     GUnixMountEntry    *mount2);
GIO_AVAILABLE_IN_2_84
gint           g_unix_mount_entry_compare           (GUnixMountEntry    *mount1,
                                                     GUnixMountEntry    *mount2);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_get_mount_path)
const char *   g_unix_mount_get_mount_path          (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
const char *   g_unix_mount_entry_get_mount_path    (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_get_device_path)
const char *   g_unix_mount_get_device_path         (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
const char *   g_unix_mount_entry_get_device_path   (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_get_root_path)
const char *   g_unix_mount_get_root_path           (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
const char *   g_unix_mount_entry_get_root_path     (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_get_fs_type)
const char *   g_unix_mount_get_fs_type             (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
const char *   g_unix_mount_entry_get_fs_type       (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_get_options)
const char *   g_unix_mount_get_options             (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
const char *   g_unix_mount_entry_get_options       (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_is_readonly)
gboolean       g_unix_mount_is_readonly             (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
gboolean       g_unix_mount_entry_is_readonly       (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_is_system_internal)
gboolean       g_unix_mount_is_system_internal      (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
gboolean       g_unix_mount_entry_is_system_internal(GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_guess_can_eject)
gboolean       g_unix_mount_guess_can_eject         (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
gboolean       g_unix_mount_entry_guess_can_eject   (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_guess_should_display)
gboolean       g_unix_mount_guess_should_display    (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
gboolean       g_unix_mount_entry_guess_should_display(GUnixMountEntry  *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_guess_name)
char *         g_unix_mount_guess_name              (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
char *         g_unix_mount_entry_guess_name        (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_guess_icon)
GIcon *        g_unix_mount_guess_icon              (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
GIcon *        g_unix_mount_entry_guess_icon        (GUnixMountEntry    *mount_entry);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_guess_symbolic_icon)
GIcon *        g_unix_mount_guess_symbolic_icon     (GUnixMountEntry    *mount_entry);
GIO_AVAILABLE_IN_2_84
GIcon *        g_unix_mount_entry_guess_symbolic_icon(GUnixMountEntry   *mount_entry);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixMountEntry, g_unix_mount_entry_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixMountPoint, g_unix_mount_point_free)

GIO_AVAILABLE_IN_ALL
gint           g_unix_mount_point_compare           (GUnixMountPoint    *mount1,
						     GUnixMountPoint    *mount2);
GIO_AVAILABLE_IN_ALL
const char *   g_unix_mount_point_get_mount_path    (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
const char *   g_unix_mount_point_get_device_path   (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
const char *   g_unix_mount_point_get_fs_type       (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_2_32
const char *   g_unix_mount_point_get_options       (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
gboolean       g_unix_mount_point_is_readonly       (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
gboolean       g_unix_mount_point_is_user_mountable (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
gboolean       g_unix_mount_point_is_loopback       (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
gboolean       g_unix_mount_point_guess_can_eject   (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
char *         g_unix_mount_point_guess_name        (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
GIcon *        g_unix_mount_point_guess_icon        (GUnixMountPoint    *mount_point);
GIO_AVAILABLE_IN_ALL
GIcon *        g_unix_mount_point_guess_symbolic_icon (GUnixMountPoint    *mount_point);


GIO_AVAILABLE_IN_ALL
GList *        g_unix_mount_points_get              (guint64            *time_read);
GIO_AVAILABLE_IN_2_82
GUnixMountPoint **g_unix_mount_points_get_from_file (const char         *table_path,
                                                     uint64_t           *time_read_out,
                                                     size_t             *n_points_out);
GIO_AVAILABLE_IN_2_66
GUnixMountPoint *g_unix_mount_point_at              (const char         *mount_path,
                                                     guint64            *time_read);
GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entries_get)
GList *        g_unix_mounts_get                    (guint64            *time_read);
GIO_AVAILABLE_IN_2_84
GList *        g_unix_mount_entries_get             (guint64            *time_read);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entries_get_from_file)
GUnixMountEntry **g_unix_mounts_get_from_file      (const char          *table_path,
                                                    uint64_t            *time_read_out,
                                                    size_t              *n_entries_out);
GIO_AVAILABLE_IN_2_84
GUnixMountEntry **g_unix_mount_entries_get_from_file(const char          *table_path,
                                                     uint64_t            *time_read_out,
                                                     size_t              *n_entries_out);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_at)
GUnixMountEntry *g_unix_mount_at                    (const char         *mount_path,
						     guint64            *time_read);
GIO_AVAILABLE_IN_2_84
GUnixMountEntry *g_unix_mount_entry_at              (const char         *mount_path,
						     guint64            *time_read);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entry_for)
GUnixMountEntry *g_unix_mount_for                   (const char         *file_path,
                                                     guint64            *time_read);
GIO_AVAILABLE_IN_2_84
GUnixMountEntry *g_unix_mount_entry_for             (const char         *file_path,
                                                     guint64            *time_read);

GIO_DEPRECATED_IN_2_84_FOR(g_unix_mount_entries_changed_since)
gboolean       g_unix_mounts_changed_since          (guint64             time);
GIO_AVAILABLE_IN_2_84
gboolean       g_unix_mount_entries_changed_since   (guint64             time);

GIO_AVAILABLE_IN_ALL
gboolean       g_unix_mount_points_changed_since    (guint64             time);

GIO_AVAILABLE_IN_ALL
GType              g_unix_mount_monitor_get_type       (void);
GIO_AVAILABLE_IN_2_44
GUnixMountMonitor *g_unix_mount_monitor_get            (void);
GIO_DEPRECATED_IN_2_44_FOR(g_unix_mount_monitor_get)
GUnixMountMonitor *g_unix_mount_monitor_new            (void);
GIO_DEPRECATED_IN_2_44
void               g_unix_mount_monitor_set_rate_limit (GUnixMountMonitor *mount_monitor,
                                                        int                limit_msec);

GIO_AVAILABLE_IN_ALL
gboolean g_unix_is_mount_path_system_internal (const char *mount_path);
GIO_AVAILABLE_IN_2_56
gboolean g_unix_is_system_fs_type             (const char *fs_type);
GIO_AVAILABLE_IN_2_56
gboolean g_unix_is_system_device_path         (const char *device_path);

G_END_DECLS

#endif /* __G_UNIX_MOUNTS_H__ */
