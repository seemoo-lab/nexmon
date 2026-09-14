/*
 * Copyright © 2024 GNOME Foundation Inc.
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
 * Authors: Julian Sparber <jsparber@gnome.org>
 *          Philip Withnall <philip@tecnocode.co.uk>
 */

/* A stub implementation of xdg-desktop-portal */

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/glib-unix.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>


#include "fake-desktop-portal.h"
#include "fake-openuri-portal-generated.h"
#include "fake-request-portal-generated.h"

struct _GFakeDesktopPortalThread
{
  GObject parent_instance;

  char *address;  /* (not nullable) */
  GCancellable *cancellable;  /* (not nullable) (owned) */
  GThread *thread;  /* (not nullable) (owned) */
  GCond cond;  /* (mutex mutex) */
  GMutex mutex;
  gboolean ready;  /* (mutex mutex) */

  char *request_activation_token;  /* (mutex mutex) */
  char *request_uri;  /* (mutex mutex) */
} FakeDesktopPortalThread;

G_DEFINE_FINAL_TYPE (GFakeDesktopPortalThread, g_fake_desktop_portal_thread, G_TYPE_OBJECT)

static void g_fake_desktop_portal_thread_finalize (GObject *object);

static void
g_fake_desktop_portal_thread_class_init (GFakeDesktopPortalThreadClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_fake_desktop_portal_thread_finalize;
}

static void
g_fake_desktop_portal_thread_init (GFakeDesktopPortalThread *self)
{
  self->cancellable = g_cancellable_new ();
  g_cond_init (&self->cond);
  g_mutex_init (&self->mutex);
}

static void
g_fake_desktop_portal_thread_finalize (GObject *object)
{
  GFakeDesktopPortalThread *self = G_FAKE_DESKTOP_PORTAL_THREAD (object);

  g_assert (self->thread == NULL);  /* should already have been joined */

  g_mutex_clear (&self->mutex);
  g_cond_clear (&self->cond);
  g_clear_object (&self->cancellable);
  g_clear_pointer (&self->address, g_free);

  g_clear_pointer (&self->request_activation_token, g_free);
  g_clear_pointer (&self->request_uri, g_free);

  G_OBJECT_CLASS (g_fake_desktop_portal_thread_parent_class)->finalize (object);
}

static gboolean
on_handle_close (FakeRequest           *object,
                 GDBusMethodInvocation *invocation,
                 gpointer               user_data)
{
  g_test_message ("Got close request");
  fake_request_complete_close (object, invocation);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static char*
get_request_path (GDBusMethodInvocation *invocation,
                  const char            *token)
{
  char *request_obj_path;
  char *sender;

  sender = g_strdup (g_dbus_method_invocation_get_sender (invocation) + 1);

  /* The object path needs to be the specific format.
   * See: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html#org-freedesktop-portal-request */
  for (size_t i = 0; sender[i]; i++)
    if (sender[i] == '.')
      sender[i] = '_';

  request_obj_path = g_strdup_printf ("/org/freedesktop/portal/desktop/request/%s/%s", sender, token);
  g_free (sender);

  return request_obj_path;
}

static gboolean
handle_request (GFakeDesktopPortalThread *self,
                FakeOpenURI             *object,
                GDBusMethodInvocation   *invocation,
                const gchar             *arg_parent_window,
                const gchar             *arg_uri,
                gboolean                 open_file,
                GVariant                *arg_options)
{
  const char *activation_token = NULL;
  GError *error = NULL;
  FakeRequest *interface_request;
  GVariantBuilder opt_builder;
  char *request_obj_path;
  const char *token = NULL;

  if (arg_options)
    {
      g_variant_lookup (arg_options, "activation_token", "&s", &activation_token);
      g_variant_lookup (arg_options, "handle_token", "&s", &token);
    }

  g_set_str (&self->request_activation_token, activation_token);
  g_set_str (&self->request_uri, arg_uri);

  request_obj_path = get_request_path (invocation, token ? token : "t");

  if (open_file)
    {
      g_test_message ("Got open file request for %s", arg_uri);

      fake_open_uri_complete_open_file (object,
                                        invocation,
                                        NULL,
                                        request_obj_path);

    }
  else
    {
      g_test_message ("Got open URI request for %s", arg_uri);

      fake_open_uri_complete_open_uri (object,
                                       invocation,
                                       request_obj_path);

    }

  interface_request = fake_request_skeleton_new ();
  g_variant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);

  g_signal_connect (interface_request,
                    "handle-close",
                    G_CALLBACK (on_handle_close),
                    NULL);

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (interface_request),
                                    g_dbus_method_invocation_get_connection (invocation),
                                    request_obj_path,
                                    &error);
  g_assert_no_error (error);
  g_dbus_interface_skeleton_set_flags (G_DBUS_INTERFACE_SKELETON (interface_request),
                                       G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);
  g_test_message ("Request skeleton exported at %s", request_obj_path);

  /* We can't use `fake_request_emit_response()` because it doesn't set the sender */
  g_dbus_connection_emit_signal (g_dbus_method_invocation_get_connection (invocation),
                                 g_dbus_method_invocation_get_sender (invocation),
                                 request_obj_path,
                                 "org.freedesktop.portal.Request",
                                 "Response",
                                 g_variant_new ("(u@a{sv})",
                                                0, /* Success */
                                                g_variant_builder_end (&opt_builder)),
                                 NULL);

  g_test_message ("Response emitted");

  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (interface_request));
  g_free (request_obj_path);
  g_object_unref (interface_request);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static char *
handle_to_uri (GVariant    *handle,
               GUnixFDList *fd_list)
{
  int fd = -1;
  int fd_id;
  char *path;
  char *uri;
  GError *local_error = NULL;

  fd_id = g_variant_get_handle (handle);
  fd = g_unix_fd_list_get (fd_list, fd_id, NULL);

  if (fd == -1)
    return NULL;

  path = g_unix_fd_query_path (fd, &local_error);
  g_assert_no_error (local_error);

  uri = g_filename_to_uri (path, NULL, NULL);
  g_free (path);
  close (fd);

  return uri;
}

static gboolean
on_handle_open_file (FakeOpenURI           *object,
                     GDBusMethodInvocation *invocation,
                     GUnixFDList           *fd_list,
                     const gchar           *arg_parent_window,
                     GVariant              *arg_fd,
                     GVariant              *arg_options,
                     gpointer               user_data)
{
  GFakeDesktopPortalThread *self = G_FAKE_DESKTOP_PORTAL_THREAD (user_data);
  char *uri = NULL;

  uri = handle_to_uri (arg_fd, fd_list);
  handle_request (self,
                  object,
                  invocation,
                  arg_parent_window,
                  uri,
                  TRUE,
                  arg_options);
  g_free (uri);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static gboolean
on_handle_open_uri (FakeOpenURI           *object,
                    GDBusMethodInvocation *invocation,
                    const gchar           *arg_parent_window,
                    const gchar           *arg_uri,
                    GVariant              *arg_options,
                    gpointer               user_data)
{
  GFakeDesktopPortalThread *self = G_FAKE_DESKTOP_PORTAL_THREAD (user_data);

  handle_request (self,
                  object,
                  invocation,
                  arg_parent_window,
                  arg_uri,
                  TRUE,
                  arg_options);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  GFakeDesktopPortalThread *self = G_FAKE_DESKTOP_PORTAL_THREAD (user_data);

  g_test_message ("Acquired the name %s", name);

  g_mutex_lock (&self->mutex);
  self->ready = TRUE;
  g_cond_signal (&self->cond);
  g_mutex_unlock (&self->mutex);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  g_test_message ("Lost the name %s", name);
}

static gboolean
cancelled_cb (GCancellable *cancellable,
              gpointer      user_data)
{
  g_test_message ("fake-desktop-portal cancelled");
  return G_SOURCE_CONTINUE;
}

static gpointer
fake_desktop_portal_thread_cb (gpointer user_data)
{
  GFakeDesktopPortalThread *self = G_FAKE_DESKTOP_PORTAL_THREAD (user_data);
  GMainContext *context = NULL;
  GDBusConnection *connection = NULL;
  GSource *source = NULL;
  guint id;
  FakeOpenURI *interface_open_uri;
  GError *local_error = NULL;

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  connection = g_dbus_connection_new_for_address_sync (self->address,
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                       G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                       NULL,
                                                       self->cancellable,
                                                       &local_error);
  g_assert_no_error (local_error);

  /* Listen for cancellation. The source will wake up the context iteration
   * which can then re-check its exit condition below. */
  source = g_cancellable_source_new (self->cancellable);
  g_source_set_callback (source, G_SOURCE_FUNC (cancelled_cb), NULL, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  /* Set up the interface */
  g_test_message ("Acquired a message bus connection");

  interface_open_uri = fake_open_uri_skeleton_new ();

  g_signal_connect (interface_open_uri,
                    "handle-open-file",
                    G_CALLBACK (on_handle_open_file),
                    self);
  g_signal_connect (interface_open_uri,
                    "handle-open-uri",
                    G_CALLBACK (on_handle_open_uri),
                    self);

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (interface_open_uri),
                                    connection,
                                    "/org/freedesktop/portal/desktop",
                                    &local_error);
  g_assert_no_error (local_error);

  /* Own the portal name */
  id = g_bus_own_name_on_connection (connection,
                                     "org.freedesktop.portal.Desktop",
                                     G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                     G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                     on_name_acquired,
                                     on_name_lost,
                                     self,
                                     NULL);

  while (!g_cancellable_is_cancelled (self->cancellable))
    g_main_context_iteration (context, TRUE);

  g_bus_unown_name (id);
  g_clear_object (&connection);
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (interface_open_uri));
  g_object_unref (interface_open_uri);
  g_main_context_pop_thread_default (context);
  g_clear_pointer (&context, g_main_context_unref);

  return NULL;
}

/* Get the activation token given to the most recent OpenURI request
 *
 * Returns: (transfer none) (nullable: an activation token
 */
const gchar *
g_fake_desktop_portal_thread_get_last_request_activation_token (GFakeDesktopPortalThread *self)
{
  g_return_val_if_fail (G_IS_FAKE_DESKTOP_PORTAL_THREAD (self), NULL);

  return self->request_activation_token;
}

/* Get the file or URI given to the most recent OpenURI request
 *
 * Returns: (transfer none) (nullable): an URI
 */
const gchar *
g_fake_desktop_portal_thread_get_last_request_uri (GFakeDesktopPortalThread *self)
{
  g_return_val_if_fail (G_IS_FAKE_DESKTOP_PORTAL_THREAD (self), NULL);

  return self->request_uri;
}

/*
 * Create a new #GFakeDesktopPortalThread. The thread isn’t started until
 * g_fake_desktop_portal_thread_run() is called on it.
 *
 * Returns: (transfer full): the new fake desktop portal wrapper
 */
GFakeDesktopPortalThread *
g_fake_desktop_portal_thread_new (const char *address)
{
  GFakeDesktopPortalThread *self = g_object_new (G_TYPE_FAKE_DESKTOP_PORTAL_THREAD, NULL);
  self->address = g_strdup (address);
  return g_steal_pointer (&self);
}

/*
 * Start a worker thread which will run a fake
 * `org.freedesktop.portal.Desktops` portal on the bus at @address. This is
 * intended to be used with #GTestDBus to mock up a portal from within a unit
 * test process, rather than relying on D-Bus activation of a mock portal
 * subprocess.
 *
 * It blocks until the thread has owned its D-Bus name and is ready to handle
 * requests.
 */
void
g_fake_desktop_portal_thread_run (GFakeDesktopPortalThread *self)
{
  g_return_if_fail (G_IS_FAKE_DESKTOP_PORTAL_THREAD (self));
  g_return_if_fail (self->thread == NULL);

  self->thread = g_thread_new ("fake-desktop-portal", fake_desktop_portal_thread_cb, self);

  /* Block until the thread is ready. */
  g_mutex_lock (&self->mutex);
  while (!self->ready)
    g_cond_wait (&self->cond, &self->mutex);
  g_mutex_unlock (&self->mutex);
}

/* Stop and join a worker thread started with fake_desktop_portal_thread_run().
 * Blocks until the thread has stopped and joined.
 *
 * Once this has been called, it’s safe to drop the final reference on @self. */
void
g_fake_desktop_portal_thread_stop (GFakeDesktopPortalThread *self)
{
  g_return_if_fail (G_IS_FAKE_DESKTOP_PORTAL_THREAD (self));
  g_return_if_fail (self->thread != NULL);

  g_cancellable_cancel (self->cancellable);
  g_thread_join (g_steal_pointer (&self->thread));
}

/* Whether fake-desktop-portal is supported on this platform. This basically
 * means whether g_unix_fd_query_path() will work at runtime. */
gboolean
g_fake_desktop_portal_is_supported (void)
{
  enum {
    UNKNOWN,
    SUPPORTED,
    UNSUPPORTED,
  };

  static size_t checked = UNKNOWN;

  if g_once_init_enter (&checked)
    {
      char *fd_path;
      int fd;

      fd = g_open ("/dev/null", O_RDONLY);
      fd_path = g_unix_fd_query_path (fd, NULL);
      g_free (fd_path);
      g_clear_fd (&fd, NULL);

      g_once_init_leave (&checked, fd_path != NULL ? SUPPORTED : UNSUPPORTED);
    }

  return checked == SUPPORTED;
}
