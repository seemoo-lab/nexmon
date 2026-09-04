/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2017 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "gopenuriportal.h"
#include "xdp-dbus.h"
#include "gstdio.h"

#ifdef G_OS_UNIX
#include "gunixfdlist.h"
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#else
#define HAVE_O_CLOEXEC 1
#endif

gboolean
g_openuri_portal_open_file (GFile       *file,
                            const char  *parent_window,
                            const char  *startup_id,
                            GError     **error)
{
  GXdpOpenURI *openuri;
  GVariantBuilder opt_builder;
  gboolean res;

  openuri = gxdp_open_uri_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
                                                  "org.freedesktop.portal.Desktop",
                                                  "/org/freedesktop/portal/desktop",
                                                  NULL,
                                                  error);

  if (openuri == NULL)
    {
      g_prefix_error (error, "Failed to create OpenURI proxy: ");
      return FALSE;
    }

  g_variant_builder_init_static (&opt_builder, G_VARIANT_TYPE_VARDICT);

  if (startup_id)
    g_variant_builder_add (&opt_builder, "{sv}",
                           "activation_token",
                           g_variant_new_string (startup_id));

  if (g_file_is_native (file))
    {
      char *path = NULL;
      GUnixFDList *fd_list = NULL;
      int fd, fd_id, errsv;

      path = g_file_get_path (file);

      fd = g_open (path, O_RDONLY | O_CLOEXEC);
      errsv = errno;
      if (fd == -1)
        {
          g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                       "Failed to open ‘%s’: %s", path, g_strerror (errsv));
          g_free (path);
          g_variant_builder_clear (&opt_builder);
          return FALSE;
        }

#ifndef HAVE_O_CLOEXEC
      fcntl (fd, F_SETFD, FD_CLOEXEC);
#endif
      fd_list = g_unix_fd_list_new_from_array (&fd, 1);
      fd = -1;
      fd_id = 0;

      res = gxdp_open_uri_call_open_file_sync (openuri,
                                               parent_window ? parent_window : "",
                                               g_variant_new ("h", fd_id),
                                               g_variant_builder_end (&opt_builder),
                                               fd_list,
                                               NULL,
                                               NULL,
                                               NULL,
                                               error);
      g_free (path);
      g_object_unref (fd_list);
    }
  else
    {
      char *uri = NULL;

      uri = g_file_get_uri (file);

      res = gxdp_open_uri_call_open_uri_sync (openuri,
                                              parent_window ? parent_window : "",
                                              uri,
                                              g_variant_builder_end (&opt_builder),
                                              NULL,
                                              NULL,
                                              error);
      g_free (uri);
    }

  g_prefix_error (error, "Failed to call OpenURI portal: ");

  g_clear_object (&openuri);

  return res;
}

enum {
  XDG_DESKTOP_PORTAL_SUCCESS   = 0,
  XDG_DESKTOP_PORTAL_CANCELLED = 1,
  XDG_DESKTOP_PORTAL_FAILED    = 2
};

typedef struct
{
  GXdpOpenURI *proxy;
  char *response_handle;
  unsigned int response_signal_id;
  gboolean open_file;
} CallData;

static CallData *
call_data_new (void)
{
  return g_new0 (CallData, 1);
}

static void
call_data_free (gpointer data)
{
  CallData *call = data;

  g_assert (call->response_signal_id == 0);
  g_clear_object (&call->proxy);
  g_clear_pointer (&call->response_handle, g_free);
  g_free_sized (data, sizeof (CallData));
}

static void
response_received (GDBusConnection *connection,
                   const char      *sender_name,
                   const char      *object_path,
                   const char      *interface_name,
                   const char      *signal_name,
                   GVariant        *parameters,
                   gpointer         user_data)
{
  GTask *task = user_data;
  CallData *call_data;
  guint32 response;

  call_data = g_task_get_task_data (task);
  g_dbus_connection_signal_unsubscribe (connection, g_steal_handle_id (&call_data->response_signal_id));

  g_variant_get (parameters, "(u@a{sv})", &response, NULL);

  switch (response)
    {
    case XDG_DESKTOP_PORTAL_SUCCESS:
      g_task_return_boolean (task, TRUE);
      break;
    case XDG_DESKTOP_PORTAL_CANCELLED:
      g_task_return_new_error_literal (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Launch cancelled");
      break;
    case XDG_DESKTOP_PORTAL_FAILED:
    default:
      g_task_return_new_error_literal (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Launch failed");
      break;
    }

  g_object_unref (task);
}

static void
open_call_done (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
  GXdpOpenURI *openuri = GXDP_OPEN_URI (source);
  GDBusConnection *connection;
  GTask *task = user_data;
  CallData *call_data;
  GError *error = NULL;
  gboolean res;
  char *path = NULL;

  call_data = g_task_get_task_data (task);
  connection = g_dbus_proxy_get_connection (G_DBUS_PROXY (openuri));

  if (call_data->open_file)
    res = gxdp_open_uri_call_open_file_finish (openuri, &path, NULL, result, &error);
  else
    res = gxdp_open_uri_call_open_uri_finish (openuri, &path, result, &error);

  if (!res)
    {
      g_task_return_error (task, error);
      g_object_unref (task);
      g_free (path);
      return;
    }

  if (g_strcmp0 (call_data->response_handle, path) != 0)
    {
      guint signal_id;

      g_dbus_connection_signal_unsubscribe (connection, g_steal_handle_id (&call_data->response_signal_id));

      signal_id = g_dbus_connection_signal_subscribe (connection,
                                                      "org.freedesktop.portal.Desktop",
                                                      "org.freedesktop.portal.Request",
                                                      "Response",
                                                      path,
                                                      NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                                      response_received,
                                                      task,
                                                      NULL);
      g_clear_pointer (&call_data->response_handle, g_free);
      call_data->response_signal_id = g_steal_handle_id (&signal_id);
      call_data->response_handle = g_steal_pointer (&path);
    }

  g_free (path);
}

void
g_openuri_portal_open_file_async (GFile               *file,
                                  const char          *parent_window,
                                  const char          *startup_id,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  CallData *call_data;
  GError *error = NULL;
  GDBusConnection *connection;
  GXdpOpenURI *openuri;
  GTask *task;
  GVariant *opts = NULL;
  int i;
  guint signal_id;

  openuri = gxdp_open_uri_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
                                                  "org.freedesktop.portal.Desktop",
                                                  "/org/freedesktop/portal/desktop",
                                                  NULL,
                                                  &error);

  if (openuri == NULL)
    {
      g_prefix_error (&error, "Failed to create OpenURI proxy: ");
      g_task_report_error (NULL, callback, user_data, NULL, error);
      return;
    }

  connection = g_dbus_proxy_get_connection (G_DBUS_PROXY (openuri));

  if (callback)
    {
      GVariantBuilder opt_builder;
      char *token;
      char *sender;
      char *handle;

      task = g_task_new (NULL, cancellable, callback, user_data);

      token = g_strdup_printf ("gio%d", g_random_int_range (0, G_MAXINT));
      sender = g_strdup (g_dbus_connection_get_unique_name (connection) + 1);
      for (i = 0; sender[i]; i++)
        if (sender[i] == '.')
          sender[i] = '_';

      handle = g_strdup_printf ("/org/freedesktop/portal/desktop/request/%s/%s", sender, token);
      g_free (sender);

      signal_id = g_dbus_connection_signal_subscribe (connection,
                                                      "org.freedesktop.portal.Desktop",
                                                      "org.freedesktop.portal.Request",
                                                      "Response",
                                                      handle,
                                                      NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                                      response_received,
                                                      task,
                                                      NULL);

      g_variant_builder_init_static (&opt_builder, G_VARIANT_TYPE_VARDICT);
      g_variant_builder_add (&opt_builder, "{sv}", "handle_token", g_variant_new_string (token));
      g_free (token);

      if (startup_id)
        g_variant_builder_add (&opt_builder, "{sv}",
                               "activation_token",
                               g_variant_new_string (startup_id));

      opts = g_variant_builder_end (&opt_builder);

      call_data = call_data_new ();
      call_data->proxy = g_object_ref (openuri);
      call_data->response_handle = g_steal_pointer (&handle);
      call_data->response_signal_id = g_steal_handle_id (&signal_id);
      g_task_set_task_data (task, call_data, call_data_free);
    }
  else
    {
      call_data = NULL;
      task = NULL;
    }

  if (g_file_is_native (file))
    {
      char *path = NULL;
      GUnixFDList *fd_list = NULL;
      int fd, fd_id, errsv;

      if (call_data)
        call_data->open_file = TRUE;

      path = g_file_get_path (file);
      fd = g_open (path, O_RDONLY | O_CLOEXEC);
      errsv = errno;
      if (fd == -1)
        {
          g_task_return_new_error (task,
                                   G_IO_ERROR, g_io_error_from_errno (errsv),
                                   "Failed to open ‘%s’: %s", path, g_strerror (errsv));

          if (call_data != NULL)
            g_dbus_connection_signal_unsubscribe (connection, g_steal_handle_id (&call_data->response_signal_id));
          g_clear_object (&task);
          g_clear_object (&openuri);
          return;
        }

#ifndef HAVE_O_CLOEXEC
      fcntl (fd, F_SETFD, FD_CLOEXEC);
#endif
      fd_list = g_unix_fd_list_new_from_array (&fd, 1);
      fd = -1;
      fd_id = 0;

      gxdp_open_uri_call_open_file (openuri,
                                    parent_window ? parent_window : "",
                                    g_variant_new ("h", fd_id),
                                    opts,
                                    fd_list,
                                    cancellable,
                                    task ? open_call_done : NULL,
                                    task);
      g_object_unref (fd_list);
      g_free (path);
    }
  else
    {
      char *uri = NULL;

      uri = g_file_get_uri (file);

      gxdp_open_uri_call_open_uri (openuri,
                                   parent_window ? parent_window : "",
                                   uri,
                                   opts,
                                   cancellable,
                                   task ? open_call_done : NULL,
                                   task);
      g_free (uri);
    }

  g_clear_object (&openuri);
}

gboolean
g_openuri_portal_open_file_finish (GAsyncResult  *result,
                                   GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}
