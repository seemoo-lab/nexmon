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
#include <unistd.h>
#include <string.h>

#include <sys/types.h>

#include "gdbusprivate.h"
#include "gdbus-tests.h"

/* all tests rely on a shared mainloop */
static GMainLoop *loop = NULL;

#if 0
G_GNUC_UNUSED static void
_log (const gchar *format, ...)
{
  GTimeVal now;
  time_t now_time;
  struct tm *now_tm;
  gchar time_buf[128];
  gchar *str;
  va_list var_args;

  va_start (var_args, format);
  str = g_strdup_vprintf (format, var_args);
  va_end (var_args);

  g_get_current_time (&now);
  now_time = (time_t) now.tv_sec;
  now_tm = localtime (&now_time);
  strftime (time_buf, sizeof time_buf, "%H:%M:%S", now_tm);

  g_printerr ("%s.%06d: %s\n",
           time_buf, (gint) now.tv_usec / 1000,
           str);
  g_free (str);
}
#else
#define _log(...)
#endif

static gboolean
test_connection_quit_mainloop (gpointer user_data)
{
  gboolean *quit_mainloop_fired = user_data;  /* (atomic) */
  _log ("quit_mainloop_fired");
  g_atomic_int_set (quit_mainloop_fired, TRUE);
  g_main_loop_quit (loop);
  return G_SOURCE_CONTINUE;
}

/* ---------------------------------------------------------------------------------------------------- */
/* Connection life-cycle testing */
/* ---------------------------------------------------------------------------------------------------- */

static const GDBusInterfaceInfo boo_interface_info =
{
  -1,
  "org.example.Boo",
  (GDBusMethodInfo **) NULL,
  (GDBusSignalInfo **) NULL,
  (GDBusPropertyInfo **) NULL,
  NULL,
};

static const GDBusInterfaceVTable boo_vtable =
{
  NULL, /* _method_call */
  NULL, /* _get_property */
  NULL,  /* _set_property */
  { 0 }
};

/* Runs in a worker thread. */
static GDBusMessage *
some_filter_func (GDBusConnection *connection,
                  GDBusMessage    *message,
                  gboolean         incoming,
                  gpointer         user_data)
{
  return message;
}

static void
on_name_owner_changed (GDBusConnection *connection,
                       const gchar     *sender_name,
                       const gchar     *object_path,
                       const gchar     *interface_name,
                       const gchar     *signal_name,
                       GVariant        *parameters,
                       gpointer         user_data)
{
}

static void
a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop (gpointer user_data)
{
  gboolean *val = user_data;  /* (atomic) */
  g_atomic_int_set (val, TRUE);
  _log ("destroynotify fired for %p", val);
  g_main_loop_quit (loop);
}

static void
test_connection_bus_failure (void)
{
  GDBusConnection *c;
  GError *error = NULL;

  /*
   * Check for correct behavior when no bus is present
   *
   */
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_nonnull (error);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_assert_null (c);
  g_error_free (error);
}

static void
test_connection_life_cycle (void)
{
  gboolean ret;
  GDBusConnection *c;
  GDBusConnection *c2;
  GError *error;
  gboolean on_signal_registration_freed_called;  /* (atomic) */
  gboolean on_filter_freed_called;  /* (atomic) */
  gboolean on_register_object_freed_called;  /* (atomic) */
  gboolean quit_mainloop_fired;  /* (atomic) */
  guint quit_mainloop_id;
  guint registration_id;

  error = NULL;

  /*
   *  Check for correct behavior when a bus is present
   */
  session_bus_up ();
  /* case 1 */
  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);
  g_assert_false (g_dbus_connection_is_closed (c));

  /*
   * Check that singleton handling work
   */
  error = NULL;
  c2 = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  g_assert_true (c == c2);
  g_object_unref (c2);

  /*
   * Check that private connections work
   */
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  g_assert_true (c != c2);
  g_object_unref (c2);

  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  g_assert_false (g_dbus_connection_is_closed (c2));
  ret = g_dbus_connection_close_sync (c2, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  _g_assert_signal_received (c2, "closed");
  g_assert_true (g_dbus_connection_is_closed (c2));
  ret = g_dbus_connection_close_sync (c2, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  g_error_free (error);
  g_assert_false (ret);
  g_object_unref (c2);

  /*
   * Check that the finalization code works
   *
   * (and that the GDestroyNotify for filters and objects and signal
   * registrations are run as expected)
   */
  error = NULL;
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  /* signal registration */
  g_atomic_int_set (&on_signal_registration_freed_called, FALSE);
  g_dbus_connection_signal_subscribe (c2,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_INTERFACE_DBUS,
                                      "NameOwnerChanged",     /* member */
                                      DBUS_PATH_DBUS,
                                      NULL,                   /* arg0 */
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      on_name_owner_changed,
                                      (gpointer) &on_signal_registration_freed_called,
                                      a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop);
  /* filter func */
  g_atomic_int_set (&on_filter_freed_called, FALSE);
  g_dbus_connection_add_filter (c2,
                                some_filter_func,
                                (gpointer) &on_filter_freed_called,
                                a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop);
  /* object registration */
  g_atomic_int_set (&on_register_object_freed_called, FALSE);
  error = NULL;
  registration_id = g_dbus_connection_register_object (c2,
                                                       "/foo",
                                                       (GDBusInterfaceInfo *) &boo_interface_info,
                                                       &boo_vtable,
                                                       (gpointer) &on_register_object_freed_called,
                                                       a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop,
                                                       &error);
  g_assert_no_error (error);
  g_assert_cmpuint (registration_id, >, 0);
  /* ok, finalize the connection and check that all the GDestroyNotify functions are invoked as expected */
  g_object_unref (c2);
  g_atomic_int_set (&quit_mainloop_fired, FALSE);
  quit_mainloop_id = g_timeout_add (30000, test_connection_quit_mainloop, (gpointer) &quit_mainloop_fired);
  _log ("destroynotifies for\n"
        " register_object %p\n"
        " filter          %p\n"
        " signal          %p",
        &on_register_object_freed_called,
        &on_filter_freed_called,
        &on_signal_registration_freed_called);
  while (TRUE)
    {
      if (g_atomic_int_get (&on_signal_registration_freed_called) &&
          g_atomic_int_get (&on_filter_freed_called) &&
          g_atomic_int_get (&on_register_object_freed_called))
        break;
      if (g_atomic_int_get (&quit_mainloop_fired))
        break;
      _log ("entering loop");
      g_main_loop_run (loop);
      _log ("exiting loop");
    }
  g_source_remove (quit_mainloop_id);
  g_assert_true (g_atomic_int_get (&on_signal_registration_freed_called));
  g_assert_true (g_atomic_int_get (&on_filter_freed_called));
  g_assert_true (g_atomic_int_get (&on_register_object_freed_called));
  g_assert_false (g_atomic_int_get (&quit_mainloop_fired));

  /*
   *  Check for correct behavior when the bus goes away
   *
   */
  g_assert_false (g_dbus_connection_is_closed (c));
  g_dbus_connection_set_exit_on_close (c, FALSE);
  session_bus_stop ();
  _g_assert_signal_received (c, "closed");
  g_assert_true (g_dbus_connection_is_closed (c));
  g_object_unref (c);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */
/* Test that sending and receiving messages work as expected */
/* ---------------------------------------------------------------------------------------------------- */

static void
msg_cb_expect_error_disconnected (GDBusConnection *connection,
                                  GAsyncResult    *res,
                                  gpointer         user_data)
{
  GError *error;
  GVariant *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_error_free (error);
  g_assert_null (result);

  g_main_loop_quit (loop);
}

static void
msg_cb_expect_error_unknown_method (GDBusConnection *connection,
                                    GAsyncResult    *res,
                                    gpointer         user_data)
{
  GError *error;
  GVariant *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
  g_assert_true (g_dbus_error_is_remote_error (error));
  g_error_free (error);
  g_assert_null (result);

  g_main_loop_quit (loop);
}

static void
msg_cb_expect_success (GDBusConnection *connection,
                       GAsyncResult    *res,
                       gpointer         user_data)
{
  GError *error;
  GVariant *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_variant_unref (result);

  g_main_loop_quit (loop);
}

static void
msg_cb_expect_error_cancelled (GDBusConnection *connection,
                               GAsyncResult    *res,
                               gpointer         user_data)
{
  GError *error;
  GVariant *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_error_free (error);
  g_assert_null (result);

  g_main_loop_quit (loop);
}

static void
msg_cb_expect_error_cancelled_2 (GDBusConnection *connection,
                                 GAsyncResult    *res,
                                 gpointer         user_data)
{
  GError *error;
  GVariant *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_error_free (error);
  g_assert_null (result);

  g_main_loop_quit (loop);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_send (void)
{
  GDBusConnection *c;
  GCancellable *ca;

  session_bus_up ();

  /* First, get an unopened connection */
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c);
  g_assert_false (g_dbus_connection_is_closed (c));

  /*
   * Check that we never actually send a message if the GCancellable
   * is already cancelled - i.e.  we should get G_IO_ERROR_CANCELLED
   * when the actual connection is not up.
   */
  ca = g_cancellable_new ();
  g_cancellable_cancel (ca);
  g_dbus_connection_call (c,
                          DBUS_SERVICE_DBUS,
                          DBUS_PATH_DBUS,
                          DBUS_INTERFACE_DBUS,
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (GAsyncReadyCallback) msg_cb_expect_error_cancelled,
                          NULL);
  g_main_loop_run (loop);
  g_object_unref (ca);

  /*
   * Check that we get a reply to the GetId() method call.
   */
  g_dbus_connection_call (c,
                          DBUS_SERVICE_DBUS,
                          DBUS_PATH_DBUS,
                          DBUS_INTERFACE_DBUS,
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) msg_cb_expect_success,
                          NULL);
  g_main_loop_run (loop);

  /*
   * Check that we get an error reply to the NonExistantMethod() method call.
   */
  g_dbus_connection_call (c,
                          DBUS_SERVICE_DBUS,
                          DBUS_PATH_DBUS,
                          DBUS_INTERFACE_DBUS,
                          "NonExistantMethod",     /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) msg_cb_expect_error_unknown_method,
                          NULL);
  g_main_loop_run (loop);

  /*
   * Check that cancellation works when the message is already in flight.
   */
  ca = g_cancellable_new ();
  g_dbus_connection_call (c,
                          DBUS_SERVICE_DBUS,
                          DBUS_PATH_DBUS,
                          DBUS_INTERFACE_DBUS,
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (GAsyncReadyCallback) msg_cb_expect_error_cancelled_2,
                          NULL);
  g_cancellable_cancel (ca);
  g_main_loop_run (loop);
  g_object_unref (ca);

  /*
   * Check that we get an error when sending to a connection that is disconnected.
   */
  g_dbus_connection_set_exit_on_close (c, FALSE);
  session_bus_stop ();
  _g_assert_signal_received (c, "closed");
  g_assert_true (g_dbus_connection_is_closed (c));

  g_dbus_connection_call (c,
                          DBUS_SERVICE_DBUS,
                          DBUS_PATH_DBUS,
                          DBUS_INTERFACE_DBUS,
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) msg_cb_expect_error_disconnected,
                          NULL);
  g_main_loop_run (loop);

  g_object_unref (c);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */
/* Connection signal tests */
/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_signal_handler (GDBusConnection  *connection,
                                const gchar      *sender_name,
                                const gchar      *object_path,
                                const gchar      *interface_name,
                                const gchar      *signal_name,
                                GVariant         *parameters,
                                gpointer         user_data)
{
  gint *counter = user_data;
  *counter += 1;

  /*g_debug ("in test_connection_signal_handler (sender=%s path=%s interface=%s member=%s)",
           sender_name,
           object_path,
           interface_name,
           signal_name);*/

  /* We defer quitting to a G_PRIORITY_DEFAULT_IDLE function so other queued signal
   * callbacks have a chance to run first. They get dispatched with a higher priority
   * of G_PRIORITY_DEFAULT, so as long as the queue is non-empty g_main_loop_quit won't
   * run
   */
  g_idle_add_once ((GSourceOnceFunc) g_main_loop_quit, loop);
}

static void
test_connection_signals (void)
{
  GDBusConnection *c1;
  GDBusConnection *c2;
  GDBusConnection *c3;
  guint s1;
  guint s1b;
  guint s2;
  guint s3;
  guint s4;
  guint s5;
  gint count_s1;
  gint count_s1b;
  gint count_s2;
  gint count_s4;
  gint count_s5;
  gint count_name_owner_changed;
  GError *error;
  gboolean ret;
  GVariant *result;
  gboolean quit_mainloop_fired;
  guint quit_mainloop_id;

  error = NULL;

  /*
   * Bring up first separate connections
   */
  session_bus_up ();
  /* if running with dbus-monitor, it claims the name :1.0 - so if we don't run with the monitor
   * emulate this
   */
  if (g_getenv ("G_DBUS_MONITOR") == NULL)
    {
      c1 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
      g_assert_nonnull (c1);
      g_assert_false (g_dbus_connection_is_closed (c1));
      g_object_unref (c1);
    }
  c1 = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c1);
  g_assert_false (g_dbus_connection_is_closed (c1));
  g_assert_cmpstr (g_dbus_connection_get_unique_name (c1), ==, ":1.1");

  /*
   * Install two signal handlers for the first connection
   *
   *  - Listen to the signal "Foo" from :1.2 (e.g. c2)
   *  - Listen to the signal "Foo" from anyone (e.g. both c2 and c3)
   *
   * and then count how many times this signal handler was invoked.
   */
  s1 = g_dbus_connection_signal_subscribe (c1,
                                           ":1.2",
                                           "org.gtk.GDBus.ExampleInterface",
                                           "Foo",
                                           "/org/gtk/GDBus/ExampleInterface",
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_s1,
                                           NULL);
  s2 = g_dbus_connection_signal_subscribe (c1,
                                           NULL, /* match any sender */
                                           "org.gtk.GDBus.ExampleInterface",
                                           "Foo",
                                           "/org/gtk/GDBus/ExampleInterface",
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_s2,
                                           NULL);
  s3 = g_dbus_connection_signal_subscribe (c1,
                                           DBUS_SERVICE_DBUS,
                                           DBUS_INTERFACE_DBUS,
                                           "NameOwnerChanged",      /* member */
                                           DBUS_PATH_DBUS,
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_name_owner_changed,
                                           NULL);
  s4 = g_dbus_connection_signal_subscribe (c1,
                                           ":1.2",  /* sender */
                                           "org.gtk.GDBus.ExampleInterface",  /* interface */
                                           "FooArg0",  /* member */
                                           "/org/gtk/GDBus/ExampleInterface",  /* path */
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_s4,
                                           NULL);
  s5 = g_dbus_connection_signal_subscribe (c1,
                                           ":1.2",  /* sender */
                                           "org.gtk.GDBus.ExampleInterface",  /* interface */
                                           "FooArg0",  /* member */
                                           "/org/gtk/GDBus/ExampleInterface",  /* path */
                                           "some-arg0",
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_s5,
                                           NULL);
  /* Note that s1b is *just like* s1 - this is to catch a bug where N
   * subscriptions of the same rule causes N calls to each of the N
   * subscriptions instead of just 1 call to each of the N subscriptions.
   */
  s1b = g_dbus_connection_signal_subscribe (c1,
                                            ":1.2",
                                            "org.gtk.GDBus.ExampleInterface",
                                            "Foo",
                                            "/org/gtk/GDBus/ExampleInterface",
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            test_connection_signal_handler,
                                            &count_s1b,
                                            NULL);
  g_assert_cmpuint (s1, !=, 0);
  g_assert_cmpuint (s1b, !=, 0);
  g_assert_cmpuint (s2, !=, 0);
  g_assert_cmpuint (s3, !=, 0);
  g_assert_cmpuint (s4, !=, 0);
  g_assert_cmpuint (s5, !=, 0);

  count_s1 = 0;
  count_s1b = 0;
  count_s2 = 0;
  count_s4 = 0;
  count_s5 = 0;
  count_name_owner_changed = 0;

  /*
   * Make c2 emit "Foo" - we should catch it twice
   *
   * Note that there is no way to be sure that the signal subscriptions
   * on c1 are effective yet - for all we know, the AddMatch() messages
   * could sit waiting in a buffer somewhere between this process and
   * the message bus. And emitting signals on c2 (a completely other
   * socket!) will not necessarily change this.
   *
   * To ensure this is not the case, do a synchronous call on c1.
   */
  result = g_dbus_connection_call_sync (c1,
                                        DBUS_SERVICE_DBUS,
                                        DBUS_PATH_DBUS,
                                        DBUS_INTERFACE_DBUS,
                                        "GetId",                 /* method name */
                                        NULL,                    /* parameters */
                                        NULL,                    /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_variant_unref (result);

  /*
   * Bring up two other connections
   */
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c2);
  g_assert_false (g_dbus_connection_is_closed (c2));
  g_assert_cmpstr (g_dbus_connection_get_unique_name (c2), ==, ":1.2");
  c3 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c3);
  g_assert_false (g_dbus_connection_is_closed (c3));
  g_assert_cmpstr (g_dbus_connection_get_unique_name (c3), ==, ":1.3");

  /* now, emit the signal on c2 */
  ret = g_dbus_connection_emit_signal (c2,
                                       NULL, /* destination bus name */
                                       "/org/gtk/GDBus/ExampleInterface",
                                       "org.gtk.GDBus.ExampleInterface",
                                       "Foo",
                                       NULL,
                                       &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  while (!(count_s1 >= 1 && count_s2 >= 1))
    g_main_loop_run (loop);
  g_assert_cmpint (count_s1, ==, 1);
  g_assert_cmpint (count_s2, ==, 1);

  /*
   * Make c3 emit "Foo" - we should catch it only once
   */
  ret = g_dbus_connection_emit_signal (c3,
                                       NULL, /* destination bus name */
                                       "/org/gtk/GDBus/ExampleInterface",
                                       "org.gtk.GDBus.ExampleInterface",
                                       "Foo",
                                       NULL,
                                       &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  while (!(count_s1 == 1 && count_s2 == 2))
    g_main_loop_run (loop);
  g_assert_cmpint (count_s1, ==, 1);
  g_assert_cmpint (count_s2, ==, 2);

  /* Emit another signal on c2 with and without arg0 set, to check matching on that.
   * Matching should fail on s5 when the signal is not emitted with an arg0. It
   * should succeed on s4 both times, as that doesn’t require an arg0 match. */
  ret = g_dbus_connection_emit_signal (c2,
                                       NULL, /* destination bus name */
                                       "/org/gtk/GDBus/ExampleInterface",
                                       "org.gtk.GDBus.ExampleInterface",
                                       "FooArg0",
                                       NULL,
                                       &error);
  g_assert_no_error (error);
  g_assert_true (ret);

  while (count_s4 < 1)
    g_main_loop_run (loop);
  g_assert_cmpint (count_s4, ==, 1);

  ret = g_dbus_connection_emit_signal (c2,
                                       NULL, /* destination bus name */
                                       "/org/gtk/GDBus/ExampleInterface",
                                       "org.gtk.GDBus.ExampleInterface",
                                       "FooArg0",
                                       g_variant_new_parsed ("('some-arg0',)"),
                                       &error);
  g_assert_no_error (error);
  g_assert_true (ret);

  while (count_s5 < 1)
    g_main_loop_run (loop);
  g_assert_cmpint (count_s4, ==, 2);
  g_assert_cmpint (count_s5, ==, 1);

  /*
   * Also to check the total amount of NameOwnerChanged signals - use a 5 second ceiling
   * to avoid spinning forever
   */
  quit_mainloop_fired = FALSE;
  quit_mainloop_id = g_timeout_add (30000, test_connection_quit_mainloop, &quit_mainloop_fired);
  while (count_name_owner_changed < 2 && !quit_mainloop_fired)
    g_main_loop_run (loop);
  g_source_remove (quit_mainloop_id);
  g_assert_cmpint (count_s1, ==, 1);
  g_assert_cmpint (count_s2, ==, 2);
  g_assert_cmpint (count_name_owner_changed, ==, 2);
  g_assert_cmpint (count_s4, ==, 2);
  g_assert_cmpint (count_s5, ==, 1);

  g_assert_cmpuint (s1, !=, 0);
  g_clear_dbus_signal_subscription (&s1, c1);
  g_assert_cmpuint (s1, ==, 0);
  /* g_clear_dbus_signal_subscription() is idempotent, with no warnings */
  g_clear_dbus_signal_subscription (&s1, c1);
  g_assert_cmpuint (s1, ==, 0);

  g_clear_dbus_signal_subscription (&s2, c1);
  g_clear_dbus_signal_subscription (&s3, c1);
  g_clear_dbus_signal_subscription (&s1b, c1);
  g_clear_dbus_signal_subscription (&s4, c1);
  g_clear_dbus_signal_subscription (&s5, c1);

  g_object_unref (c1);
  g_object_unref (c2);
  g_object_unref (c3);

  session_bus_down ();
}

static void
test_match_rule (GDBusConnection  *connection,
                 GDBusSignalFlags  flags,
                 gchar            *arg0_rule,
                 gchar            *arg0,
                 const gchar      *signal_type,
                 gboolean          should_match)
{
  guint subscription_ids[2];
  gint emissions = 0;
  gint matches = 0;
  GError *error = NULL;

  subscription_ids[0] = g_dbus_connection_signal_subscribe (connection,
                                                            NULL, "org.gtk.ExampleInterface", "Foo", "/",
                                                            NULL,
                                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                                            test_connection_signal_handler,
                                                            &emissions, NULL);
  subscription_ids[1] = g_dbus_connection_signal_subscribe (connection,
                                                            NULL, "org.gtk.ExampleInterface", "Foo", "/",
                                                            arg0_rule,
                                                            flags,
                                                            test_connection_signal_handler,
                                                            &matches, NULL);
  g_assert_cmpint (subscription_ids[0], !=, 0);
  g_assert_cmpint (subscription_ids[1], !=, 0);

  g_dbus_connection_emit_signal (connection,
                                 NULL, "/", "org.gtk.ExampleInterface",
                                 "Foo", g_variant_new (signal_type, arg0),
                                 &error);
  g_assert_no_error (error);

  /* synchronously ping a non-existent method to make sure the signals are dispatched */
  g_dbus_connection_call_sync (connection, "org.gtk.ExampleInterface", "/", "org.gtk.ExampleInterface",
                               "Bar", g_variant_new ("()"), G_VARIANT_TYPE_UNIT, G_DBUS_CALL_FLAGS_NONE,
                               -1, NULL, NULL);

  while (g_main_context_iteration (NULL, FALSE))
    ;

  g_assert_cmpint (emissions, ==, 1);
  g_assert_cmpint (matches, ==, should_match ? 1 : 0);

  g_clear_dbus_signal_subscription (&subscription_ids[0], connection);
  g_clear_dbus_signal_subscription (&subscription_ids[1], connection);
}

static void
test_connection_signal_match_rules (void)
{
  GDBusConnection *con;

  session_bus_up ();
  con = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_NONE, "foo", "foo", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_NONE, "foo", "bar", "(s)", FALSE);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "", "(s)", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org", "(s)", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org.gtk", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org.gtk.Example", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org.gtk+", "(s)", FALSE);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/", "/", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/", "", "(s)", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk/Example", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/", "/org/gtk/Example", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk/", "(s)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk", "(s)", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk+", "/org/gtk", "(s)", FALSE);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/", "/", "(o)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk/Example", "(o)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/", "/org/gtk/Example", "(o)", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk", "(o)", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk+", "/org/gtk", "(o)", FALSE);

  g_object_unref (con);
  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

/* Accessed both from the test code and the filter function (in a worker thread)
 * so all accesses must be atomic. */
typedef struct
{
  GAsyncQueue *incoming_queue;  /* (element-type GDBusMessage) */
  guint num_outgoing;  /* (atomic) */
} FilterData;

/* Runs in a worker thread. */
static GDBusMessage *
filter_func (GDBusConnection *connection,
             GDBusMessage    *message,
             gboolean         incoming,
             gpointer         user_data)
{
  FilterData *data = user_data;

  if (incoming)
    g_async_queue_push (data->incoming_queue, g_object_ref (message));
  else
    g_atomic_int_inc (&data->num_outgoing);

  return message;
}

static void
wait_for_filtered_reply (GAsyncQueue *incoming_queue,
                         guint32      expected_serial)
{
  GDBusMessage *popped_message = NULL;

  while ((popped_message = g_async_queue_pop (incoming_queue)) != NULL)
    {
      guint32 reply_serial = g_dbus_message_get_reply_serial (popped_message);
      g_object_unref (popped_message);
      if (reply_serial == expected_serial)
        return;
    }

  g_assert_not_reached ();
}

typedef struct
{
  gboolean alter_incoming;
  gboolean alter_outgoing;
} FilterEffects;

/* Runs in a worker thread. */
static GDBusMessage *
other_filter_func (GDBusConnection *connection,
                   GDBusMessage    *message,
                   gboolean         incoming,
                   gpointer         user_data)
{
  const FilterEffects *effects = user_data;
  GDBusMessage *ret;
  gboolean alter;

  if (incoming)
    alter = effects->alter_incoming;
  else
    alter = effects->alter_outgoing;

  if (alter)
    {
      GDBusMessage *copy;
      GVariant *body;
      gchar *s;
      gchar *s2;

      copy = g_dbus_message_copy (message, NULL);
      g_object_unref (message);

      body = g_dbus_message_get_body (copy);
      g_variant_get (body, "(s)", &s);
      s2 = g_strdup_printf ("MOD: %s", s);
      g_dbus_message_set_body (copy, g_variant_new ("(s)", s2));
      g_free (s2);
      g_free (s);

      ret = copy;
    }
  else
    {
      ret = message;
    }

  return ret;
}

static void
test_connection_filter_name_owner_changed_signal_handler (GDBusConnection  *connection,
                                                          const gchar      *sender_name,
                                                          const gchar      *object_path,
                                                          const gchar      *interface_name,
                                                          const gchar      *signal_name,
                                                          GVariant         *parameters,
                                                          gpointer         user_data)
{
  const gchar *name;
  const gchar *old_owner;
  const gchar *new_owner;

  g_variant_get (parameters,
                 "(&s&s&s)",
                 &name,
                 &old_owner,
                 &new_owner);

  if (g_strcmp0 (name, "com.example.TestService") == 0 && strlen (new_owner) > 0)
    {
      g_main_loop_quit (loop);
    }
}

static gboolean
test_connection_filter_on_timeout (gpointer user_data)
{
  g_printerr ("Timeout waiting 30 sec on service\n");
  g_assert_not_reached ();
  return G_SOURCE_REMOVE;
}

static void
test_connection_filter (void)
{
  GDBusConnection *c;
  FilterData data = { NULL, 0 };
  GDBusMessage *m;
  GDBusMessage *m2;
  GDBusMessage *r;
  GError *error;
  guint filter_id;
  guint timeout_mainloop_id;
  guint signal_handler_id;
  FilterEffects effects;
  GVariant *result;
  const gchar *s;
  guint32 serial_temp;

  session_bus_up ();

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);

  data.incoming_queue = g_async_queue_new_full (g_object_unref);
  data.num_outgoing = 0;
  filter_id = g_dbus_connection_add_filter (c,
                                            filter_func,
                                            &data,
                                            NULL);

  m = g_dbus_message_new_method_call (DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS,
                                      "GetNameOwner");
  g_dbus_message_set_body (m, g_variant_new ("(s)", DBUS_SERVICE_DBUS));
  error = NULL;
  g_dbus_connection_send_message (c, m, G_DBUS_SEND_MESSAGE_FLAGS_NONE, &serial_temp, &error);
  g_assert_no_error (error);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);

  m2 = g_dbus_message_copy (m, &error);
  g_assert_no_error (error);
  g_dbus_connection_send_message (c, m2, G_DBUS_SEND_MESSAGE_FLAGS_NONE, &serial_temp, &error);
  g_object_unref (m2);
  g_assert_no_error (error);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);

  m2 = g_dbus_message_copy (m, &error);
  g_assert_no_error (error);
  g_dbus_message_set_serial (m2, serial_temp);
  /* lock the message to test PRESERVE_SERIAL flag. */
  g_dbus_message_lock (m2);
  g_dbus_connection_send_message (c, m2, G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL, &serial_temp, &error);
  g_object_unref (m2);
  g_assert_no_error (error);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);

  m2 = g_dbus_message_copy (m, &error);
  g_assert_no_error (error);
  r = g_dbus_connection_send_message_with_reply_sync (c,
                                                      m2,
                                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                      -1,
                                                      &serial_temp,
                                                      NULL, /* GCancellable */
                                                      &error);
  g_object_unref (m2);
  g_assert_no_error (error);
  g_assert_nonnull (r);
  g_object_unref (r);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);
  g_assert_cmpint (g_async_queue_length (data.incoming_queue), ==, 0);

  g_dbus_connection_remove_filter (c, filter_id);

  m2 = g_dbus_message_copy (m, &error);
  g_assert_no_error (error);
  r = g_dbus_connection_send_message_with_reply_sync (c,
                                                      m2,
                                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                      -1,
                                                      &serial_temp,
                                                      NULL, /* GCancellable */
                                                      &error);
  g_object_unref (m2);
  g_assert_no_error (error);
  g_assert_nonnull (r);
  g_object_unref (r);
  g_assert_cmpint (g_async_queue_length (data.incoming_queue), ==, 0);
  g_assert_cmpint (g_atomic_int_get (&data.num_outgoing), ==, 4);

  /* wait for service to be available */
  signal_handler_id = g_dbus_connection_signal_subscribe (c,
                                                          DBUS_SERVICE_DBUS,
                                                          DBUS_INTERFACE_DBUS,
                                                          "NameOwnerChanged",
                                                          DBUS_PATH_DBUS,
                                                          NULL, /* arg0 */
                                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                                          test_connection_filter_name_owner_changed_signal_handler,
                                                          NULL,
                                                          NULL);
  g_assert_cmpint (signal_handler_id, !=, 0);

  /* this is safe; testserver will exit once the bus goes away */
  g_assert_true (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  timeout_mainloop_id = g_timeout_add (30000, test_connection_filter_on_timeout, NULL);
  g_main_loop_run (loop);
  g_source_remove (timeout_mainloop_id);
  g_clear_dbus_signal_subscription (&signal_handler_id, c);

  /* now test some combinations... */
  filter_id = g_dbus_connection_add_filter (c,
                                            other_filter_func,
                                            &effects,
                                            NULL);
  /* -- */
  effects.alter_incoming = FALSE;
  effects.alter_outgoing = FALSE;
  error = NULL;
  result = g_dbus_connection_call_sync (c,
                                        "com.example.TestService",      /* bus name */
                                        "/com/example/TestObject",      /* object path */
                                        "com.example.Frob",             /* interface name */
                                        "HelloWorld",                   /* method name */
                                        g_variant_new ("(s)", "Cat"),   /* parameters */
                                        G_VARIANT_TYPE ("(s)"),         /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_variant_get (result, "(&s)", &s);
  g_assert_cmpstr (s, ==, "You greeted me with 'Cat'. Thanks!");
  g_variant_unref (result);
  /* -- */
  effects.alter_incoming = TRUE;
  effects.alter_outgoing = TRUE;
  error = NULL;
  result = g_dbus_connection_call_sync (c,
                                        "com.example.TestService",      /* bus name */
                                        "/com/example/TestObject",      /* object path */
                                        "com.example.Frob",             /* interface name */
                                        "HelloWorld",                   /* method name */
                                        g_variant_new ("(s)", "Cat"),   /* parameters */
                                        G_VARIANT_TYPE ("(s)"),         /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_variant_get (result, "(&s)", &s);
  g_assert_cmpstr (s, ==, "MOD: You greeted me with 'MOD: Cat'. Thanks!");
  g_variant_unref (result);


  g_dbus_connection_remove_filter (c, filter_id);

  g_object_unref (c);
  g_object_unref (m);
  g_async_queue_unref (data.incoming_queue);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

#define NUM_THREADS 50

static void
send_bogus_message (GDBusConnection *c, guint32 *out_serial)
{
  GDBusMessage *m;
  GError *error;

  m = g_dbus_message_new_method_call (DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS,
                                      "GetNameOwner");
  g_dbus_message_set_body (m, g_variant_new ("(s)", DBUS_SERVICE_DBUS));
  error = NULL;
  g_dbus_connection_send_message (c, m, G_DBUS_SEND_MESSAGE_FLAGS_NONE, out_serial, &error);
  g_assert_no_error (error);
  g_object_unref (m);
}

#define SLEEP_USEC (100 * 1000)

static gpointer
serials_thread_func (GDBusConnection *c)
{
  guint32 message_serial;
  guint i;

  /* No calls on this thread yet */
  g_assert_cmpint (g_dbus_connection_get_last_serial(c), ==, 0);

  /* Send a bogus message and store its serial */
  message_serial = 0;
  send_bogus_message (c, &message_serial);

  /* Give it some time to actually send the message out. 10 seconds
   * should be plenty, even on slow machines. */
  for (i = 0; i < 10 * G_USEC_PER_SEC / SLEEP_USEC; i++)
    {
      if (g_dbus_connection_get_last_serial(c) != 0)
        break;

      g_usleep (SLEEP_USEC);
    }

  g_assert_cmpint (g_dbus_connection_get_last_serial(c), !=, 0);
  g_assert_cmpint (g_dbus_connection_get_last_serial(c), ==, message_serial);

  return NULL;
}

static void
test_connection_serials (void)
{
  GDBusConnection *c;
  GError *error;
  GThread *pool[NUM_THREADS];
  int i;

  session_bus_up ();

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);

  /* Status after initialization */
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 1);

  /* Send a bogus message */
  send_bogus_message (c, NULL);
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 2);

  /* Start the threads */
  for (i = 0; i < NUM_THREADS; i++)
    pool[i] = g_thread_new (NULL, (GThreadFunc) serials_thread_func, c);

  /* Wait until threads are finished */
  for (i = 0; i < NUM_THREADS; i++)
      g_thread_join (pool[i]);

  /* No calls in between on this thread, should be the last value */
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 2);

  send_bogus_message (c, NULL);

  /* All above calls + calls in threads */
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 3 + NUM_THREADS);

  g_object_unref (c);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

static void
get_connection_cb_expect_cancel (GObject       *source_object,
                                 GAsyncResult  *res,
                                 gpointer       user_data)
{
  GDBusConnection *c;
  GError *error;

  error = NULL;
  c = g_bus_get_finish (res, &error);

  /* unref here to avoid timeouts when the test fails */
  if (c)
    g_object_unref (c);

  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null (c);

  g_error_free (error);
}

static void
get_connection_cb_expect_success (GObject       *source_object,
                                  GAsyncResult  *res,
                                  gpointer       user_data)
{
  GDBusConnection *c;
  GError *error;

  error = NULL;
  c = g_bus_get_finish (res, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);

  g_main_loop_quit (loop);

  g_object_unref (c);
}

static void
test_connection_cancel (void)
{
  GCancellable *cancellable, *cancellable2;

  g_test_summary ("Test that cancelling one of two racing g_bus_get() calls does not cancel the other one");

  session_bus_up ();

  cancellable = g_cancellable_new ();
  cancellable2 = g_cancellable_new ();

  g_bus_get (G_BUS_TYPE_SESSION, cancellable, get_connection_cb_expect_cancel, NULL);
  g_bus_get (G_BUS_TYPE_SESSION, cancellable2, get_connection_cb_expect_success, NULL);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (loop);

  g_object_unref (cancellable);
  g_object_unref (cancellable2);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_basic (void)
{
  GDBusConnection *connection;
  GError *error;
  GDBusCapabilityFlags flags;
  GDBusConnectionFlags connection_flags;
  gchar *guid;
  gchar *name;
  gboolean closed;
  gboolean exit_on_close;
  GIOStream *stream;
  GCredentials *credentials;

  session_bus_up ();

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (connection);

  flags = g_dbus_connection_get_capabilities (connection);
  g_assert_true (flags == G_DBUS_CAPABILITY_FLAGS_NONE ||
                 flags == G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);

  connection_flags = g_dbus_connection_get_flags (connection);
  /* Ignore G_DBUS_CONNECTION_FLAGS_CROSS_NAMESPACE, it's an
   * implementation detail whether we set it */
  connection_flags &= ~G_DBUS_CONNECTION_FLAGS_CROSS_NAMESPACE;
  g_assert_cmpint (connection_flags, ==,
                   G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION);

  credentials = g_dbus_connection_get_peer_credentials (connection);
  g_assert_null (credentials);

  g_object_get (connection,
                "stream", &stream,
                "guid", &guid,
                "unique-name", &name,
                "closed", &closed,
                "exit-on-close", &exit_on_close,
                "capabilities", &flags,
                NULL);

  g_assert_true (G_IS_IO_STREAM (stream));
  g_assert_true (g_dbus_is_guid (guid));
  g_assert_true (g_dbus_is_unique_name (name));
  g_assert_false (closed);
  g_assert_true (exit_on_close);
  g_assert_true (flags == G_DBUS_CAPABILITY_FLAGS_NONE ||
                 flags == G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);
  g_object_unref (stream);
  g_free (name);
  g_free (guid);

  g_object_unref (connection);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  int ret;

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  /* all the tests rely on a shared main loop */
  loop = g_main_loop_new (NULL, FALSE);

  g_test_dbus_unset ();

  /* gdbus cleanup is pretty racy due to worker threads, so always do this test first */
  g_test_add_func ("/gdbus/connection/bus-failure", test_connection_bus_failure);

  g_test_add_func ("/gdbus/connection/basic", test_connection_basic);
  g_test_add_func ("/gdbus/connection/life-cycle", test_connection_life_cycle);
  g_test_add_func ("/gdbus/connection/send", test_connection_send);
  g_test_add_func ("/gdbus/connection/signals", test_connection_signals);
  g_test_add_func ("/gdbus/connection/signal-match-rules", test_connection_signal_match_rules);
  g_test_add_func ("/gdbus/connection/filter", test_connection_filter);
  g_test_add_func ("/gdbus/connection/serials", test_connection_serials);
  g_test_add_func ("/gdbus/connection/cancel", test_connection_cancel);
  ret = g_test_run();

  g_main_loop_unref (loop);
  return ret;
}
