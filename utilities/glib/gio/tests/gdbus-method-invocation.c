/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2022 Endless OS Foundation, LLC
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <string.h>
#include <unistd.h>

#include "gdbusprivate.h"
#include "gdbus-tests.h"

static const GDBusArgInfo foo_get_fds_in_args =
{
  -1,
  "type",
  "s",
  NULL
};
static const GDBusArgInfo * const foo_get_fds_in_arg_pointers[] = {&foo_get_fds_in_args, NULL};

static const GDBusArgInfo foo_get_fds_out_args =
{
  -1,
  "some_fd",
  "h",
  NULL
};
static const GDBusArgInfo * const foo_get_fds_out_arg_pointers[] = {&foo_get_fds_out_args, NULL};

static const GDBusMethodInfo foo_method_info_wrong_return_type =
{
  -1,
  "WrongReturnType",
  NULL,  /* in args */
  NULL,  /* out args */
  NULL  /* annotations */
};
static const GDBusMethodInfo foo_method_info_close_before_returning =
{
  -1,
  "CloseBeforeReturning",
  NULL,  /* in args */
  NULL,  /* out args */
  NULL  /* annotations */
};
static const GDBusMethodInfo foo_method_info_get_fds =
{
  -1,
  "GetFDs",
  (GDBusArgInfo **) foo_get_fds_in_arg_pointers,
  (GDBusArgInfo **) foo_get_fds_out_arg_pointers,
  NULL  /* annotations */
};
static const GDBusMethodInfo foo_method_info_return_error =
{
  -1,
  "ReturnError",
  NULL,  /* in args */
  NULL,  /* out args */
  NULL  /* annotations */
};
static const GDBusMethodInfo * const foo_method_info_pointers[] = {
  &foo_method_info_wrong_return_type,
  &foo_method_info_close_before_returning,
  &foo_method_info_get_fds,
  &foo_method_info_return_error,
  NULL
};

static const GDBusPropertyInfo foo_property_info[] =
{
  {
    -1,
    "InvalidType",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
    NULL
  },
  {
    -1,
    "InvalidTypeNull",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
    NULL
  },
  {
    -1,
    "InvalidValueType",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
    NULL
  },
};
static const GDBusPropertyInfo * const foo_property_info_pointers[] =
{
  &foo_property_info[0],
  &foo_property_info[1],
  &foo_property_info[2],
  NULL
};

static const GDBusInterfaceInfo foo_interface_info =
{
  -1,
  "org.example.Foo",
  (GDBusMethodInfo **) &foo_method_info_pointers,
  NULL,  /* signals */
  (GDBusPropertyInfo **) &foo_property_info_pointers,
  NULL,  /* annotations */
};

/* ---------------------------------------------------------------------------------------------------- */

static void
test_method_invocation_return_method_call (GDBusConnection       *connection,
                                           const gchar           *sender,
                                           const gchar           *object_path,
                                           const gchar           *interface_name,
                                           const gchar           *method_name,
                                           GVariant              *parameters,
                                           GDBusMethodInvocation *invocation,
                                           gpointer               user_data)
{
  gboolean no_reply = g_dbus_message_get_flags (g_dbus_method_invocation_get_message (invocation)) & G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED;

  if (g_str_equal (interface_name, DBUS_INTERFACE_PROPERTIES) &&
      g_str_equal (method_name, "Get"))
    {
      const gchar *iface_name, *prop_name;

      g_variant_get (parameters, "(&s&s)", &iface_name, &prop_name);
      g_assert_cmpstr (iface_name, ==, "org.example.Foo");

      /* Do different things depending on the property name. */
      if (g_str_equal (prop_name, "InvalidType"))
        {
          if (!no_reply)
            g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_WARNING,
                                   "Type of return value for property 'Get' call should be '(v)' but got '(s)'");
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "this type is invalid"));
        }
      else if (g_str_equal (prop_name, "InvalidTypeNull"))
        {
          if (!no_reply)
            g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_WARNING,
                                   "Type of return value for property 'Get' call should be '(v)' but got '()'");
          g_dbus_method_invocation_return_value (invocation, NULL);
        }
      else if (g_str_equal (prop_name, "InvalidValueType"))
        {
          if (!no_reply)
            g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_WARNING,
                                   "Value returned from property 'Get' call for 'InvalidValueType' should be 's' but is 'u'");
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(v)", g_variant_new_uint32 (123)));
        }
      else
        {
          g_assert_not_reached ();
        }

      g_test_assert_expected_messages ();
    }
  else if (g_str_equal (interface_name, DBUS_INTERFACE_PROPERTIES) &&
           g_str_equal (method_name, "Set"))
    {
      const gchar *iface_name, *prop_name;
      GVariant *value;

      g_variant_get (parameters, "(&s&sv)", &iface_name, &prop_name, &value);
      g_assert_cmpstr (iface_name, ==, "org.example.Foo");

      if (g_str_equal (prop_name, "InvalidType"))
        {
          if (!no_reply)
            g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_WARNING,
                                   "Type of return value for property 'Set' call should be '()' but got '(s)'");
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "should be unit"));
        }
      else
        {
          g_assert_not_reached ();
        }

      g_test_assert_expected_messages ();
      g_variant_unref (value);
    }
  else if (g_str_equal (interface_name, DBUS_INTERFACE_PROPERTIES) &&
           g_str_equal (method_name, "GetAll"))
    {
      const gchar *iface_name;

      g_variant_get (parameters, "(&s)", &iface_name);
      g_assert_cmpstr (iface_name, ==, "org.example.Foo");

      if (!no_reply)
        g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_WARNING,
                               "Type of return value for property 'GetAll' call should be '(a{sv})' but got '(s)'");
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "should be a different type"));
    }
  else if (g_str_equal (interface_name, "org.example.Foo") &&
           g_str_equal (method_name, "WrongReturnType"))
    {
      if (!no_reply)
        g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_WARNING,
                               "Type of return value is incorrect: expected '()', got '(s)'");
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "should be a different type"));
    }
  else if (g_str_equal (interface_name, "org.example.Foo") &&
           g_str_equal (method_name, "CloseBeforeReturning"))
    {
      g_dbus_connection_close (connection, NULL, NULL, NULL);

      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else if (g_str_equal (interface_name, "org.example.Foo") &&
           g_str_equal (method_name, "GetFDs"))
    {
      const gchar *action;
      GUnixFDList *list = NULL;
      GError *local_error = NULL;

      g_variant_get (parameters, "(&s)", &action);

      list = g_unix_fd_list_new ();
      g_unix_fd_list_append (list, 1, &local_error);
      g_assert_no_error (local_error);

      if (g_str_equal (action, "WrongNumber"))
        {
          g_unix_fd_list_append (list, 1, &local_error);
          g_assert_no_error (local_error);
        }

      if (g_str_equal (action, "Valid") ||
          g_str_equal (action, "WrongNumber"))
        g_dbus_method_invocation_return_value_with_unix_fd_list (invocation, g_variant_new ("(h)", 0), list);
      else
        g_assert_not_reached ();

      g_object_unref (list);
    }
  else if (g_str_equal (interface_name, "org.example.Foo") &&
           g_str_equal (method_name, "ReturnError"))
    {
      g_dbus_method_invocation_return_dbus_error (invocation, "org.example.Foo", "SomeError");
    }
  else
    g_assert_not_reached ();
}

static void
ensure_result_cb (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (source);
  GVariant *reply;
  guint *n_outstanding_calls = user_data;

  reply = g_dbus_connection_call_finish (connection, result, NULL);

  /* We don’t care what the reply is. */
  g_clear_pointer (&reply, g_variant_unref);

  g_assert_cmpint (*n_outstanding_calls, >, 0);
  *n_outstanding_calls = *n_outstanding_calls - 1;
}

static void
test_method_invocation_return (void)
{
  GDBusConnection *connection = NULL;
  GError *local_error = NULL;
  guint registration_id;
  const GDBusInterfaceVTable vtable = {
    test_method_invocation_return_method_call, NULL, NULL, { 0 }
  };
  guint n_outstanding_calls = 0;

  g_test_summary ("Test calling g_dbus_method_invocation_return_*() in various ways");

  /* Connect to the bus. */
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (connection);

  /* Don’t exit the test when the server closes the connection in
   * CloseBeforeReturning(). */
  g_dbus_connection_set_exit_on_close (connection, FALSE);

  /* Register an object which we can call methods on. */
  registration_id = g_dbus_connection_register_object (connection,
                                                       "/foo",
                                                       (GDBusInterfaceInfo *) &foo_interface_info,
                                                       &vtable, NULL, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (registration_id, !=, 0);

  /* Test a variety of error cases */
    {
      const struct
        {
          const gchar *interface_name;
          const gchar *method_name;
          const gchar *parameters_string;
          gboolean tests_undefined_behaviour;
        }
      calls[] =
        {
          { DBUS_INTERFACE_PROPERTIES, "Get", "('org.example.Foo', 'InvalidType')", TRUE },
          { DBUS_INTERFACE_PROPERTIES, "Get", "('org.example.Foo', 'InvalidTypeNull')", TRUE },
          { DBUS_INTERFACE_PROPERTIES, "Get", "('org.example.Foo', 'InvalidValueType')", TRUE },
          { DBUS_INTERFACE_PROPERTIES, "Set", "('org.example.Foo', 'InvalidType', <'irrelevant'>)", TRUE },
          { DBUS_INTERFACE_PROPERTIES, "GetAll", "('org.example.Foo',)", TRUE },
          { "org.example.Foo", "WrongReturnType", "()", TRUE },
          { "org.example.Foo", "GetFDs", "('Valid',)", FALSE },
          { "org.example.Foo", "GetFDs", "('WrongNumber',)", TRUE },
          { "org.example.Foo", "ReturnError", "()", FALSE },
          { "org.example.Foo", "CloseBeforeReturning", "()", FALSE },
        };
      gsize i;

      for (i = 0; i < G_N_ELEMENTS (calls); i++)
        {
          if (calls[i].tests_undefined_behaviour && !g_test_undefined ())
            {
              g_test_message ("Skipping %s.%s", calls[i].interface_name, calls[i].method_name);
              continue;
            }
          else
            {
              g_test_message ("Calling %s.%s", calls[i].interface_name, calls[i].method_name);
            }

          /* Call twice, once expecting a result and once not. Do the call which
           * doesn’t expect a result first; message ordering should ensure that
           * it’s completed by the time the second call completes, so we don’t
           * have to account for it separately.
           *
           * That’s good, because the only way to get g_dbus_connection_call()
           * to set %G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED is to not provide
           * a callback function. */
          n_outstanding_calls++;

          g_dbus_connection_call (connection,
                                  g_dbus_connection_get_unique_name (connection),
                                  "/foo",
                                  calls[i].interface_name,
                                  calls[i].method_name,
                                  g_variant_new_parsed (calls[i].parameters_string),
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  NULL,  /* no callback */
                                  NULL);

          g_dbus_connection_call (connection,
                                  g_dbus_connection_get_unique_name (connection),
                                  "/foo",
                                  calls[i].interface_name,
                                  calls[i].method_name,
                                  g_variant_new_parsed (calls[i].parameters_string),
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  ensure_result_cb,
                                  &n_outstanding_calls);
        }
    }

  /* Wait until all the calls are complete. */
  while (n_outstanding_calls > 0)
    g_main_context_iteration (NULL, TRUE);

  g_dbus_connection_unregister_object (connection, registration_id);
  g_object_unref (connection);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gdbus/method-invocation/return", test_method_invocation_return);

  return session_bus_run ();
}
