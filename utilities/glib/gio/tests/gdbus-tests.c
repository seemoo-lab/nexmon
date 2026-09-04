/* GLib testing framework examples and tests
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

#include <gio/gio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "gdbus-tests.h"

#include "gdbusprivate.h"

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GMainLoop *loop;
  gboolean   timed_out;
} PropertyNotifyData;

static void
on_property_notify (GObject    *object,
                    GParamSpec *pspec,
                    gpointer    user_data)
{
  PropertyNotifyData *data = user_data;
  g_main_loop_quit (data->loop);
}

static gboolean
on_property_notify_timeout (gpointer user_data)
{
  PropertyNotifyData *data = user_data;
  data->timed_out = TRUE;
  g_main_loop_quit (data->loop);
  return G_SOURCE_CONTINUE;
}

gboolean
_g_assert_property_notify_run (gpointer     object,
                               const gchar *property_name)
{
  gchar *s;
  gulong handler_id;
  guint timeout_id;
  PropertyNotifyData data;

  data.loop = g_main_loop_new (g_main_context_get_thread_default (), FALSE);
  data.timed_out = FALSE;
  s = g_strdup_printf ("notify::%s", property_name);
  handler_id = g_signal_connect (object,
                                 s,
                                 G_CALLBACK (on_property_notify),
                                 &data);
  g_free (s);
  timeout_id = g_timeout_add_seconds (30,
                                      on_property_notify_timeout,
                                      &data);
  g_main_loop_run (data.loop);
  g_signal_handler_disconnect (object, handler_id);
  g_source_remove (timeout_id);
  g_main_loop_unref (data.loop);

  return data.timed_out;
}

static gboolean
_give_up (gpointer data)
{
  g_error ("%s", (const gchar *) data);
  g_return_val_if_reached (G_SOURCE_CONTINUE);
}

typedef struct
{
  GMainContext *context;
  gboolean name_appeared;
  gboolean unwatch_complete;
} WatchData;

static void
name_appeared_cb (GDBusConnection *connection,
                  const gchar     *name,
                  const gchar     *name_owner,
                  gpointer         user_data)
{
  WatchData *data = user_data;

  g_assert (name_owner != NULL);
  data->name_appeared = TRUE;
  g_main_context_wakeup (data->context);
}

static void
watch_free_cb (gpointer user_data)
{
  WatchData *data = user_data;

  data->unwatch_complete = TRUE;
  g_main_context_wakeup (data->context);
}

void
ensure_gdbus_testserver_up (GDBusConnection *connection,
                            GMainContext    *context)
{
  GSource *timeout_source = NULL;
  guint watch_id;
  WatchData data = { context, FALSE, FALSE };

  g_main_context_push_thread_default (context);

  watch_id = g_bus_watch_name_on_connection (connection,
                                             "com.example.TestService",
                                             G_BUS_NAME_WATCHER_FLAGS_NONE,
                                             name_appeared_cb,
                                             NULL,
                                             &data,
                                             watch_free_cb);

  timeout_source = g_timeout_source_new_seconds (60);
  g_source_set_callback (timeout_source, _give_up,
                         "waited more than ~ 60s for gdbus-testserver to take its bus name",
                         NULL);
  g_source_attach (timeout_source, context);

  while (!data.name_appeared)
    g_main_context_iteration (context, TRUE);

  g_bus_unwatch_name (watch_id);

  while (!data.unwatch_complete)
    g_main_context_iteration (context, TRUE);

  g_source_destroy (timeout_source);
  g_source_unref (timeout_source);

  g_main_context_pop_thread_default (context);
}

/*
 * connection_wait_for_bus:
 * @conn: A connection
 *
 * Wait for asynchronous messages from @conn to have been processed
 * by the message bus, as a sequence point so that we can make
 * "happens before" and "happens after" assertions relative to this.
 * The easiest way to achieve this is to call a message bus method that has
 * no arguments and wait for it to return: because the message bus processes
 * messages in-order, anything we sent before this must have been processed
 * by the time this call arrives.
 */
void
connection_wait_for_bus (GDBusConnection *conn)
{
  GError *error = NULL;
  GVariant *call_result;

  call_result = g_dbus_connection_call_sync (conn,
                                             DBUS_SERVICE_DBUS,
                                             DBUS_PATH_DBUS,
                                             DBUS_INTERFACE_DBUS,
                                             "GetId",
                                             NULL,   /* arguments */
                                             NULL,   /* result type */
                                             G_DBUS_CALL_FLAGS_NONE,
                                             -1,
                                             NULL,
                                             &error);
  g_assert_no_error (error);
  g_assert_nonnull (call_result);
  g_variant_unref (call_result);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GMainLoop *loop;
  gboolean   timed_out;
} SignalReceivedData;

static void
on_signal_received (gpointer user_data)
{
  SignalReceivedData *data = user_data;
  g_main_loop_quit (data->loop);
}

static gboolean
on_signal_received_timeout (gpointer user_data)
{
  SignalReceivedData *data = user_data;
  data->timed_out = TRUE;
  g_main_loop_quit (data->loop);
  return G_SOURCE_CONTINUE;
}

gboolean
_g_assert_signal_received_run (gpointer     object,
                               const gchar *signal_name)
{
  gulong handler_id;
  guint timeout_id;
  SignalReceivedData data;

  data.loop = g_main_loop_new (g_main_context_get_thread_default (), FALSE);
  data.timed_out = FALSE;
  handler_id = g_signal_connect_swapped (object,
                                         signal_name,
                                         G_CALLBACK (on_signal_received),
                                         &data);
  timeout_id = g_timeout_add_seconds (30,
                                      on_signal_received_timeout,
                                      &data);
  g_main_loop_run (data.loop);
  g_signal_handler_disconnect (object, handler_id);
  g_source_remove (timeout_id);
  g_main_loop_unref (data.loop);

  return data.timed_out;
}

/* ---------------------------------------------------------------------------------------------------- */

GDBusConnection *
_g_bus_get_priv (GBusType            bus_type,
                 GCancellable       *cancellable,
                 GError            **error)
{
  gchar *address;
  GDBusConnection *ret;

  ret = NULL;

  address = g_dbus_address_get_for_bus_sync (bus_type, cancellable, error);
  if (address == NULL)
    goto out;

  ret = g_dbus_connection_new_for_address_sync (address,
                                                G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                NULL, /* GDBusAuthObserver */
                                                cancellable,
                                                error);
  g_free (address);

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */
