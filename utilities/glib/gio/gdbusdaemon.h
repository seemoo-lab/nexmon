/*
 * Copyright © 2012 Red Hat, Inc.
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
 * Authors: Alexander Larsson <alexl@redhat.com>
 */

#include <gio/gio.h>

#define G_TYPE_DBUS_DAEMON (_g_dbus_daemon_get_type ())
#define G_DBUS_DAEMON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_DBUS_DAEMON, GDBusDaemon))
#define G_DBUS_DAEMON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), G_TYPE_DBUS_DAEMON, GDBusDaemonClass))
#define G_DBUS_DAEMON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_DBUS_DAEMON, GDBusDaemonClass))
#define G_IS_DBUS_DAEMON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_DBUS_DAEMON))
#define G_IS_DBUS_DAEMON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_DBUS_DAEMON))

typedef struct _GDBusDaemon GDBusDaemon;
typedef struct _GDBusDaemonClass GDBusDaemonClass;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GDBusDaemon, g_object_unref)

GType _g_dbus_daemon_get_type (void);

GDBusDaemon *_g_dbus_daemon_new (const char *address,
				 GCancellable *cancellable,
				 GError **error);

const char *_g_dbus_daemon_get_address (GDBusDaemon *daemon);
