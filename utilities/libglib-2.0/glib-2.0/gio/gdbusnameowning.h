/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_DBUS_NAME_OWNING_H__
#define __G_DBUS_NAME_OWNING_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * GBusAcquiredCallback:
 * @connection: the connection to a message bus
 * @name: the name that is requested to be owned
 * @user_data: user data passed to [func@Gio.bus_own_name]
 *
 * Invoked when a connection to a message bus has been obtained.
 *
 * Since: 2.26
 */
typedef void (*GBusAcquiredCallback) (GDBusConnection *connection,
                                      const gchar     *name,
                                      gpointer         user_data);

/**
 * GBusNameAcquiredCallback:
 * @connection: the connection on which to acquired the name
 * @name: the name being owned
 * @user_data: user data passed to [func@Gio.bus_own_name] or
 *   [func@Gio.bus_own_name_on_connection]
 *
 * Invoked when the name is acquired.
 *
 * Since: 2.26
 */
typedef void (*GBusNameAcquiredCallback) (GDBusConnection *connection,
                                          const gchar     *name,
                                          gpointer         user_data);

/**
 * GBusNameLostCallback:
 * @connection: the connect on which to acquire the name or `NULL` if
 *   the connection was disconnected
 * @name: the name being owned
 * @user_data: user data passed to [func@Gio.bus_own_name] or
 *   [func@Gio.bus_own_name_on_connection]
 *
 * Invoked when the name is lost or @connection has been closed.
 *
 * Since: 2.26
 */
typedef void (*GBusNameLostCallback) (GDBusConnection *connection,
                                      const gchar     *name,
                                      gpointer         user_data);

GIO_AVAILABLE_IN_ALL
guint g_bus_own_name                 (GBusType                  bus_type,
                                      const gchar              *name,
                                      GBusNameOwnerFlags        flags,
                                      GBusAcquiredCallback      bus_acquired_handler,
                                      GBusNameAcquiredCallback  name_acquired_handler,
                                      GBusNameLostCallback      name_lost_handler,
                                      gpointer                  user_data,
                                      GDestroyNotify            user_data_free_func);

GIO_AVAILABLE_IN_ALL
guint g_bus_own_name_on_connection   (GDBusConnection          *connection,
                                      const gchar              *name,
                                      GBusNameOwnerFlags        flags,
                                      GBusNameAcquiredCallback  name_acquired_handler,
                                      GBusNameLostCallback      name_lost_handler,
                                      gpointer                  user_data,
                                      GDestroyNotify            user_data_free_func);

GIO_AVAILABLE_IN_ALL
guint g_bus_own_name_with_closures   (GBusType                  bus_type,
                                      const gchar              *name,
                                      GBusNameOwnerFlags        flags,
                                      GClosure                 *bus_acquired_closure,
                                      GClosure                 *name_acquired_closure,
                                      GClosure                 *name_lost_closure);

GIO_AVAILABLE_IN_ALL
guint g_bus_own_name_on_connection_with_closures (
                                      GDBusConnection          *connection,
                                      const gchar              *name,
                                      GBusNameOwnerFlags        flags,
                                      GClosure                 *name_acquired_closure,
                                      GClosure                 *name_lost_closure);

GIO_AVAILABLE_IN_ALL
void  g_bus_unown_name               (guint                     owner_id);

G_END_DECLS

#endif /* __G_DBUS_NAME_OWNING_H__ */
