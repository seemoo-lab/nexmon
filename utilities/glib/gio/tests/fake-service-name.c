/*
 * Copyright (C) 2021 Frederic Martinsons
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
 * Authors: Frederic Martinsons <frederic.martinsons@gmail.com>
 */

/* A dummy service which just own a dbus name and implement a method to quit*/

#include <gio/gio.h>
#include <glib.h>

static GDBusNodeInfo *introspection_data = NULL;
static GMainLoop *loop = NULL;
static const gchar introspection_xml[] =
    "<node>"
    "    <interface name='org.gtk.GDBus.FakeService'>"
    "        <method name='Quit'/>"
    "    </interface>"
    "</node>";

static void
incoming_method_call (GDBusConnection       *connection,
                      const gchar           *sender,
                      const gchar           *object_path,
                      const gchar           *interface_name,
                      const gchar           *method_name,
                      GVariant              *parameters,
                      GDBusMethodInvocation *invocation,
                      gpointer               user_data)
{
  if (g_strcmp0 (method_name, "Quit") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, NULL);
      g_main_loop_quit (loop);
    }
}

static const GDBusInterfaceVTable interface_vtable = {
  incoming_method_call,
  NULL,
  NULL,
  { 0 }
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  guint registration_id;
  GError *error = NULL;
  g_test_message ("Acquired a message bus connection");

  registration_id = g_dbus_connection_register_object (connection,
                                                       "/org/gtk/GDBus/FakeService",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL, /* user_data */
                                                       NULL, /* user_data_free_func */
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_test_message ("Acquired the name %s", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  g_test_message ("Lost the name %s", name);
}

gint
main (gint argc, gchar *argv[])
{
  guint id;

  g_log_writer_default_set_use_stderr (TRUE);

  loop = g_main_loop_new (NULL, FALSE);
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       "org.gtk.GDBus.FakeService",
                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                           G_BUS_NAME_OWNER_FLAGS_REPLACE,
                       on_bus_acquired,
                       on_name_acquired,
                       on_name_lost,
                       loop,
                       NULL);

  g_main_loop_run (loop);

  g_bus_unown_name (id);
  g_main_loop_unref (loop);
  g_dbus_node_info_unref (introspection_data);

  return 0;
}
