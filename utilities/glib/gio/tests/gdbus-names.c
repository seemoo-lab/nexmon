/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 * Author: Frederic Martinsons <frederic.martinsons@gmail.com>
 */

#include <gio/gio.h>
#include <unistd.h>

#include "gdbusprivate.h"
#include "gdbus-tests.h"

/* ---------------------------------------------------------------------------------------------------- */
/* Test that g_bus_own_name() works correctly */
/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  gboolean expect_null_connection;
  guint num_bus_acquired;
  guint num_acquired;
  guint num_lost;
  guint num_free_func;
  GMainContext *main_context;  /* (unowned) */
} OwnNameData;

static void
own_name_data_free_func (OwnNameData *data)
{
  data->num_free_func++;
  g_main_context_wakeup (data->main_context);
}

static void
bus_acquired_handler (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
  OwnNameData *data = user_data;
  g_dbus_connection_set_exit_on_close (connection, FALSE);
  data->num_bus_acquired += 1;
  g_main_context_wakeup (data->main_context);
}

static void
name_acquired_handler (GDBusConnection *connection,
                       const gchar     *name,
                       gpointer         user_data)
{
  OwnNameData *data = user_data;
  data->num_acquired += 1;
  g_main_context_wakeup (data->main_context);
}

static void
name_lost_handler (GDBusConnection *connection,
                   const gchar     *name,
                   gpointer         user_data)
{
  OwnNameData *data = user_data;
  if (data->expect_null_connection)
    {
      g_assert (connection == NULL);
    }
  else
    {
      g_assert (connection != NULL);
      g_dbus_connection_set_exit_on_close (connection, FALSE);
    }
  data->num_lost += 1;
  g_main_context_wakeup (data->main_context);
}

static void
test_bus_own_name (void)
{
  guint id;
  guint id2;
  OwnNameData data;
  OwnNameData data2;
  const gchar *name;
  GDBusConnection *c;
  GError *error;
  gboolean name_has_owner_reply;
  GDBusConnection *c2;
  GVariant *result;
  GMainContext *main_context = NULL;  /* use the global default for now */

  error = NULL;
  name = "org.gtk.GDBus.Name1";

  /*
   * First check that name_lost_handler() is invoked if there is no bus.
   *
   * Also make sure name_lost_handler() isn't invoked when unowning the name.
   */
  data.num_bus_acquired = 0;
  data.num_free_func = 0;
  data.num_acquired = 0;
  data.num_lost = 0;
  data.expect_null_connection = TRUE;
  data.main_context = main_context;
  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       name,
                       G_BUS_NAME_OWNER_FLAGS_NONE,
                       bus_acquired_handler,
                       name_acquired_handler,
                       name_lost_handler,
                       &data,
                       (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data.num_bus_acquired, ==, 0);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_lost < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 0);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 1);
  g_bus_unown_name (id);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 1);
  g_assert_cmpint (data.num_free_func, ==, 1);

  /*
   * Bring up a bus, then own a name and check bus_acquired_handler() then name_acquired_handler() is invoked.
   */
  session_bus_up ();
  data.num_bus_acquired = 0;
  data.num_acquired = 0;
  data.num_lost = 0;
  data.expect_null_connection = FALSE;
  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       name,
                       G_BUS_NAME_OWNER_FLAGS_NONE,
                       bus_acquired_handler,
                       name_acquired_handler,
                       name_lost_handler,
                       &data,
                       (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data.num_bus_acquired, ==, 0);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_bus_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);

  /*
   * Check that the name was actually acquired.
   */
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert (c != NULL);
  g_assert (!g_dbus_connection_is_closed (c));
  result = g_dbus_connection_call_sync (c,
                                        DBUS_SERVICE_DBUS,
                                        DBUS_PATH_DBUS,
                                        DBUS_INTERFACE_DBUS,
                                        "NameHasOwner",          /* method name */
                                        g_variant_new ("(s)", name),
                                        G_VARIANT_TYPE ("(b)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_variant_get (result, "(b)", &name_has_owner_reply);
  g_assert (name_has_owner_reply);
  g_variant_unref (result);

  /*
   * Stop owning the name - this should invoke our free func
   */
  g_bus_unown_name (id);
  while (data.num_free_func < 2)
    g_main_context_iteration (main_context, TRUE);
  g_assert_cmpint (data.num_free_func, ==, 2);

  /*
   * Check that the name was actually released.
   */
  result = g_dbus_connection_call_sync (c,
                                        DBUS_SERVICE_DBUS,
                                        DBUS_PATH_DBUS,
                                        DBUS_INTERFACE_DBUS,
                                        "NameHasOwner",          /* method name */
                                        g_variant_new ("(s)", name),
                                        G_VARIANT_TYPE ("(b)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_variant_get (result, "(b)", &name_has_owner_reply);
  g_assert (!name_has_owner_reply);
  g_variant_unref (result);

  /* Now try owning the name and then immediately decide to unown the name */
  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);
  g_assert_cmpint (data.num_free_func, ==, 2);
  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       name,
                       G_BUS_NAME_OWNER_FLAGS_NONE,
                       bus_acquired_handler,
                       name_acquired_handler,
                       name_lost_handler,
                       &data,
                       (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);
  g_assert_cmpint (data.num_free_func, ==, 2);
  g_bus_unown_name (id);
  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);
  g_assert_cmpint (data.num_free_func, ==, 2);

  /* the GDestroyNotify is called in idle because the bus is acquired in idle */
  while (data.num_free_func < 3)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_free_func, ==, 3);

  /*
   * Own the name again.
   */
  data.num_bus_acquired = 0;
  data.num_acquired = 0;
  data.num_lost = 0;
  data.expect_null_connection = FALSE;
  id = g_bus_own_name_with_closures (G_BUS_TYPE_SESSION,
                                     name,
                                     G_BUS_NAME_OWNER_FLAGS_NONE,
                                     g_cclosure_new (G_CALLBACK (bus_acquired_handler),
                                                     &data,
                                                     NULL),
                                     g_cclosure_new (G_CALLBACK (name_acquired_handler),
                                                     &data,
                                                     NULL),
                                     g_cclosure_new (G_CALLBACK (name_lost_handler),
                                                     &data,
                                                     (GClosureNotify) own_name_data_free_func));
  g_assert_cmpint (data.num_bus_acquired, ==, 0);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_bus_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);

  /*
   * Try owning the name with another object on the same connection  - this should
   * fail because we already own the name.
   */
  data2.num_free_func = 0;
  data2.num_bus_acquired = 0;
  data2.num_acquired = 0;
  data2.num_lost = 0;
  data2.expect_null_connection = FALSE;
  data2.main_context = main_context;
  id2 = g_bus_own_name (G_BUS_TYPE_SESSION,
                        name,
                        G_BUS_NAME_OWNER_FLAGS_NONE,
                        bus_acquired_handler,
                        name_acquired_handler,
                        name_lost_handler,
                        &data2,
                        (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 0);

  while (data2.num_bus_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 1);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 0);

  while (data2.num_lost < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 1);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);

  g_bus_unown_name (id2);
  while (data2.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 1);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);
  g_assert_cmpint (data2.num_free_func, ==, 1);

  /*
   * Create a secondary (e.g. private) connection and try owning the name on that
   * connection. This should fail both with and without _REPLACE because we
   * didn't specify ALLOW_REPLACEMENT.
   */
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert (c2 != NULL);
  g_assert (!g_dbus_connection_is_closed (c2));
  /* first without _REPLACE */
  data2.num_bus_acquired = 0;
  data2.num_acquired = 0;
  data2.num_lost = 0;
  data2.expect_null_connection = FALSE;
  data2.num_free_func = 0;
  id2 = g_bus_own_name_on_connection (c2,
                                      name,
                                      G_BUS_NAME_OWNER_FLAGS_NONE,
                                      name_acquired_handler,
                                      name_lost_handler,
                                      &data2,
                                      (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 0);

  while (data2.num_lost < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);

  g_bus_unown_name (id2);
  while (data2.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);
  g_assert_cmpint (data2.num_free_func, ==, 1);
  /* then with _REPLACE */
  data2.num_bus_acquired = 0;
  data2.num_acquired = 0;
  data2.num_lost = 0;
  data2.expect_null_connection = FALSE;
  data2.num_free_func = 0;
  id2 = g_bus_own_name_on_connection (c2,
                                      name,
                                      G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                      name_acquired_handler,
                                      name_lost_handler,
                                      &data2,
                                      (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 0);

  while (data2.num_lost < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);

  g_bus_unown_name (id2);
  while (data2.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);
  g_assert_cmpint (data2.num_free_func, ==, 1);

  /*
   * Stop owning the name and grab it again with _ALLOW_REPLACEMENT.
   */
  data.expect_null_connection = FALSE;
  g_bus_unown_name (id);
  while (data.num_bus_acquired < 1 || data.num_free_func < 4)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_free_func, ==, 4);
  /* grab it again */
  data.num_bus_acquired = 0;
  data.num_acquired = 0;
  data.num_lost = 0;
  data.expect_null_connection = FALSE;
  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       name,
                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
                       bus_acquired_handler,
                       name_acquired_handler,
                       name_lost_handler,
                       &data,
                       (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data.num_bus_acquired, ==, 0);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_bus_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 0);
  g_assert_cmpint (data.num_lost,     ==, 0);

  while (data.num_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_bus_acquired, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);

  /*
   * Now try to grab the name from the secondary connection.
   *
   */
  /* first without _REPLACE - this won't make us acquire the name */
  data2.num_bus_acquired = 0;
  data2.num_acquired = 0;
  data2.num_lost = 0;
  data2.expect_null_connection = FALSE;
  data2.num_free_func = 0;
  id2 = g_bus_own_name_on_connection (c2,
                                      name,
                                      G_BUS_NAME_OWNER_FLAGS_NONE,
                                      name_acquired_handler,
                                      name_lost_handler,
                                      &data2,
                                      (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 0);

  while (data2.num_lost < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);

  g_bus_unown_name (id2);
  while (data2.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_bus_acquired, ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 1);
  g_assert_cmpint (data2.num_free_func, ==, 1);
  /* then with _REPLACE - here we should acquire the name - e.g. owner should lose it
   * and owner2 should acquire it  */
  data2.num_bus_acquired = 0;
  data2.num_acquired = 0;
  data2.num_lost = 0;
  data2.expect_null_connection = FALSE;
  data2.num_free_func = 0;
  id2 = g_bus_own_name_on_connection (c2,
                                      name,
                                      G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                      name_acquired_handler,
                                      name_lost_handler,
                                      &data2,
                                      (GDestroyNotify) own_name_data_free_func);
  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 0);
  g_assert_cmpint (data2.num_acquired, ==, 0);
  g_assert_cmpint (data2.num_lost,     ==, 0);

  /* wait for handlers for both owner and owner2 to fire */
  while (data.num_lost == 0 || data2.num_acquired == 0)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_acquired, ==, 1);
  g_assert_cmpint (data.num_lost,     ==, 1);
  g_assert_cmpint (data2.num_acquired, ==, 1);
  g_assert_cmpint (data2.num_lost,     ==, 0);
  g_assert_cmpint (data2.num_bus_acquired, ==, 0);

  /* ok, make owner2 release the name - then wait for owner to automagically reacquire it */
  g_bus_unown_name (id2);
  while (data.num_acquired < 2 || data2.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data2.num_free_func, ==, 1);
  g_assert_cmpint (data.num_acquired, ==, 2);
  g_assert_cmpint (data.num_lost,     ==, 1);

  /*
   * Finally, nuke the bus and check name_lost_handler() is invoked.
   *
   */
  data.expect_null_connection = TRUE;
  session_bus_stop ();
  while (data.num_lost != 2)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_acquired, ==, 2);
  g_assert_cmpint (data.num_lost,     ==, 2);

  g_bus_unown_name (id);
  while (data.num_free_func < 5)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_free_func, ==, 5);

  g_object_unref (c);
  g_object_unref (c2);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */
/* Test that g_bus_watch_name() works correctly */
/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  gboolean expect_null_connection;
  guint num_acquired;
  guint num_lost;
  guint num_appeared;
  guint num_vanished;
  guint num_free_func;
  GMainContext *main_context;  /* (unowned), for the main test thread */
} WatchNameData;

typedef struct
{
  WatchNameData data;
  GDBusConnection *connection;
  GMutex cond_mutex;
  GCond cond;
  gboolean started;
  gboolean name_acquired;
  gboolean ended;
  gboolean unwatch_early;
  GMutex mutex;
  guint watch_id;
  GMainContext *thread_context;  /* (unowned), only accessed from watcher_thread() */
} WatchNameThreadData;

static void
watch_name_data_free_func (WatchNameData *data)
{
  data->num_free_func++;
  g_main_context_wakeup (data->main_context);
}

static void
w_bus_acquired_handler (GDBusConnection *connection,
                        const gchar     *name,
                        gpointer         user_data)
{
}

static void
w_name_acquired_handler (GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         user_data)
{
  OwnNameData *data = user_data;
  data->num_acquired += 1;
  g_main_context_wakeup (data->main_context);
}

static void
w_name_lost_handler (GDBusConnection *connection,
                     const gchar     *name,
                     gpointer         user_data)
{
  OwnNameData *data = user_data;
  data->num_lost += 1;
  g_main_context_wakeup (data->main_context);
}

static void
name_appeared_handler (GDBusConnection *connection,
                       const gchar     *name,
                       const gchar     *name_owner,
                       gpointer         user_data)
{
  WatchNameData *data = user_data;

  if (data->expect_null_connection)
    {
      g_assert (connection == NULL);
    }
  else
    {
      g_assert (connection != NULL);
      g_dbus_connection_set_exit_on_close (connection, FALSE);
    }
  data->num_appeared += 1;
  g_main_context_wakeup (data->main_context);
}

static void
name_vanished_handler (GDBusConnection *connection,
                       const gchar     *name,
                       gpointer         user_data)
{
  WatchNameData *data = user_data;

  if (data->expect_null_connection)
    {
      g_assert (connection == NULL);
    }
  else
    {
      g_assert (connection != NULL);
      g_dbus_connection_set_exit_on_close (connection, FALSE);
    }
  data->num_vanished += 1;
  g_main_context_wakeup (data->main_context);
}

typedef struct
{
  guint watcher_flags;
  gboolean watch_with_closures;
  gboolean existing_service;
} WatchNameTest;

static const WatchNameTest watch_no_closures_no_flags = {
  .watcher_flags = G_BUS_NAME_WATCHER_FLAGS_NONE,
  .watch_with_closures = FALSE,
  .existing_service = FALSE
};

static const WatchNameTest watch_no_closures_flags_auto_start = {
  .watcher_flags = G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
  .watch_with_closures = FALSE,
  .existing_service = FALSE
};

static const WatchNameTest watch_no_closures_flags_auto_start_service_exist = {
  .watcher_flags = G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
  .watch_with_closures = FALSE,
  .existing_service = TRUE
};

static const WatchNameTest watch_closures_no_flags = {
  .watcher_flags = G_BUS_NAME_WATCHER_FLAGS_NONE,
  .watch_with_closures = TRUE,
  .existing_service = FALSE
};

static const WatchNameTest watch_closures_flags_auto_start = {
  .watcher_flags = G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
  .watch_with_closures = TRUE,
  .existing_service = FALSE
};

static void
stop_service (GDBusConnection *connection,
              WatchNameData   *data)
{
  GError *error = NULL;
  GDBusProxy *proxy = NULL;
  GVariant *result = NULL;
  GMainContext *main_context = NULL;  /* use the global default for now */

  data->num_vanished = 0;

  proxy = g_dbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                 NULL,
                                 "org.gtk.GDBus.FakeService",
                                 "/org/gtk/GDBus/FakeService",
                                 "org.gtk.GDBus.FakeService",
                                 NULL,
                                 &error);
  g_assert_no_error (error);

  result = g_dbus_proxy_call_sync (proxy,
                                   "Quit",
                                   NULL,
                                   G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                   100,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_object_unref (proxy);
  if (result)
    g_variant_unref (result);
  while (data->num_vanished == 0)
    g_main_context_iteration (main_context, TRUE);
}

static void
test_bus_watch_name (gconstpointer d)
{
  WatchNameData data;
  OwnNameData own_data;
  guint id;
  guint owner_id;
  GDBusConnection *connection;
  const WatchNameTest *watch_name_test;
  const gchar *name;
  GMainContext *main_context = NULL;  /* use the global default for now */

  watch_name_test = (WatchNameTest *) d;

  if (watch_name_test->existing_service)
    {
      name = "org.gtk.GDBus.FakeService";
    }
  else
    {
      name = "org.gtk.GDBus.Name1";
    }

  /*
   * First check that name_vanished_handler() is invoked if there is no bus.
   *
   * Also make sure name_vanished_handler() isn't invoked when unwatching the name.
   */
  data.num_free_func = 0;
  data.num_appeared = 0;
  data.num_vanished = 0;
  data.expect_null_connection = TRUE;
  data.main_context = main_context;
  id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                         name,
                         watch_name_test->watcher_flags,
                         name_appeared_handler,
                         name_vanished_handler,
                         &data,
                         (GDestroyNotify) watch_name_data_free_func);
  g_assert_cmpint (data.num_appeared, ==, 0);
  g_assert_cmpint (data.num_vanished, ==, 0);

  while (data.num_vanished < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_appeared, ==, 0);
  g_assert_cmpint (data.num_vanished, ==, 1);

  g_bus_unwatch_name (id);
  while (data.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_appeared, ==, 0);
  g_assert_cmpint (data.num_vanished, ==, 1);
  g_assert_cmpint (data.num_free_func, ==, 1);
  data.num_free_func = 0;

  /*
   * Now bring up a bus, own a name, and then start watching it.
   */
  session_bus_up ();
  /* own the name */
  own_data.num_free_func = 0;
  own_data.num_acquired = 0;
  own_data.num_lost = 0;
  data.expect_null_connection = FALSE;
  own_data.main_context = main_context;
  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             name,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             w_bus_acquired_handler,
                             w_name_acquired_handler,
                             w_name_lost_handler,
                             &own_data,
                             (GDestroyNotify) own_name_data_free_func);

  while (own_data.num_acquired < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (own_data.num_acquired, ==, 1);
  g_assert_cmpint (own_data.num_lost, ==, 0);

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert (connection != NULL);

  /* now watch the name */
  data.num_appeared = 0;
  data.num_vanished = 0;
  if (watch_name_test->watch_with_closures)
    {
      id = g_bus_watch_name_on_connection_with_closures (connection,
                                                         name,
                                                         watch_name_test->watcher_flags,
                                                         g_cclosure_new (G_CALLBACK (name_appeared_handler),
                                                                         &data,
                                                                         NULL),
                                                         g_cclosure_new (G_CALLBACK (name_vanished_handler),
                                                                         &data,
                                                                         (GClosureNotify) watch_name_data_free_func));
    }
  else
    {
      id = g_bus_watch_name_on_connection (connection,
                                           name,
                                           watch_name_test->watcher_flags,
                                           name_appeared_handler,
                                           name_vanished_handler,
                                           &data,
                                           (GDestroyNotify) watch_name_data_free_func);
    }
  g_assert_cmpint (data.num_appeared, ==, 0);
  g_assert_cmpint (data.num_vanished, ==, 0);

  while (data.num_appeared < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_appeared, ==, 1);
  g_assert_cmpint (data.num_vanished, ==, 0);

  /*
   * Unwatch the name.
   */
  g_bus_unwatch_name (id);
  while (data.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_free_func, ==, 1);

  /* unown the name */
  g_bus_unown_name (owner_id);
  while (own_data.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (own_data.num_acquired, ==, 1);
  g_assert_cmpint (own_data.num_free_func, ==, 1);
  own_data.num_free_func = 0;
  /*
   * Create a watcher and then make a name be owned.
   *
   * This should trigger name_appeared_handler() ...
   */
  /* watch the name */
  data.num_appeared = 0;
  data.num_vanished = 0;
  data.num_free_func = 0;
  if (watch_name_test->watch_with_closures)
    {
      id = g_bus_watch_name_with_closures (G_BUS_TYPE_SESSION,
                                           name,
                                           watch_name_test->watcher_flags,
                                           g_cclosure_new (G_CALLBACK (name_appeared_handler),
                                                           &data,
                                                           NULL),
                                           g_cclosure_new (G_CALLBACK (name_vanished_handler),
                                                           &data,
                                                           (GClosureNotify) watch_name_data_free_func));
    }
  else
    {
      id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                             name,
                             watch_name_test->watcher_flags,
                             name_appeared_handler,
                             name_vanished_handler,
                             &data,
                             (GDestroyNotify) watch_name_data_free_func);
    }

  g_assert_cmpint (data.num_appeared, ==, 0);
  g_assert_cmpint (data.num_vanished, ==, 0);

  while (data.num_appeared == 0 && data.num_vanished == 0)
    g_main_context_iteration (main_context, TRUE);

  if (watch_name_test->existing_service)
    {
      g_assert_cmpint (data.num_appeared, ==, 1);
      g_assert_cmpint (data.num_vanished, ==, 0);
    }
  else
    {
      g_assert_cmpint (data.num_appeared, ==, 0);
      g_assert_cmpint (data.num_vanished, ==, 1);
    }

  if (!watch_name_test->existing_service)
    {
      /* own the name */
      own_data.num_acquired = 0;
      own_data.num_lost = 0;
      own_data.expect_null_connection = FALSE;
      own_data.main_context = main_context;
      owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 name,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 w_bus_acquired_handler,
                                 w_name_acquired_handler,
                                 w_name_lost_handler,
                                 &own_data,
                                 (GDestroyNotify) own_name_data_free_func);

      while (own_data.num_acquired == 0 || data.num_appeared == 0)
        g_main_context_iteration (main_context, TRUE);

      g_assert_cmpint (own_data.num_acquired, ==, 1);
      g_assert_cmpint (own_data.num_lost, ==, 0);
      g_assert_cmpint (data.num_appeared, ==, 1);
      g_assert_cmpint (data.num_vanished, ==, 1);
    }

  data.expect_null_connection = TRUE;
  if (watch_name_test->existing_service)
    {
      data.expect_null_connection = FALSE;
      stop_service (connection, &data);
    }
  g_object_unref (connection);
  /*
   * Nuke the bus and check that the name vanishes and is lost.
   */
  session_bus_stop ();
  if (!watch_name_test->existing_service)
    {
      while (own_data.num_lost < 1 || data.num_vanished < 2)
        g_main_context_iteration (main_context, TRUE);
      g_assert_cmpint (own_data.num_lost, ==, 1);
      g_assert_cmpint (data.num_vanished, ==, 2);
    }
  else
    {
      g_assert_cmpint (own_data.num_lost, ==, 0);
      g_assert_cmpint (data.num_vanished, ==, 1);
    }

  g_bus_unwatch_name (id);
  while (data.num_free_func < 1)
    g_main_context_iteration (main_context, TRUE);

  g_assert_cmpint (data.num_free_func, ==, 1);

  if (!watch_name_test->existing_service)
    {
      g_bus_unown_name (owner_id);
      while (own_data.num_free_func < 1)
        g_main_context_iteration (main_context, TRUE);

      g_assert_cmpint (own_data.num_free_func, ==, 1);
    }
  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called in the same thread as watcher_thread() */
static void
t_watch_name_data_free_func (WatchNameThreadData *thread_data)
{
  thread_data->data.num_free_func++;

  g_assert_true (g_main_context_is_owner (thread_data->thread_context));
  g_main_context_wakeup (thread_data->thread_context);
}

/* Called in the same thread as watcher_thread() */
static void
t_name_appeared_handler (GDBusConnection *connection,
                         const gchar     *name,
                         const gchar     *name_owner,
                         gpointer         user_data)
{
  WatchNameThreadData *thread_data = user_data;
  thread_data->data.num_appeared += 1;

  g_assert_true (g_main_context_is_owner (thread_data->thread_context));
  g_main_context_wakeup (thread_data->thread_context);
}

/* Called in the same thread as watcher_thread() */
static void
t_name_vanished_handler (GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         user_data)
{
  WatchNameThreadData *thread_data = user_data;
  thread_data->data.num_vanished += 1;

  g_assert_true (g_main_context_is_owner (thread_data->thread_context));
  g_main_context_wakeup (thread_data->thread_context);
}

/* Called in the thread which constructed the GDBusConnection */
static void
connection_closed_cb (GDBusConnection *connection,
                      gboolean         remote_peer_vanished,
                      GError          *error,
                      gpointer         user_data)
{
  WatchNameThreadData *thread_data = (WatchNameThreadData *) user_data;
  if (thread_data->unwatch_early)
    {
      g_mutex_lock (&thread_data->mutex);
      g_bus_unwatch_name (g_atomic_int_get (&thread_data->watch_id));
      g_atomic_int_set (&thread_data->watch_id, 0);
      g_cond_signal (&thread_data->cond);
      g_mutex_unlock (&thread_data->mutex);
    }
}

static gpointer
watcher_thread (gpointer user_data)
{
  WatchNameThreadData *thread_data = user_data;
  GMainContext *thread_context;

  thread_context = g_main_context_new ();
  thread_data->thread_context = thread_context;
  g_main_context_push_thread_default (thread_context);

  // Notify that the thread has started
  g_mutex_lock (&thread_data->cond_mutex);
  g_atomic_int_set (&thread_data->started, TRUE);
  g_cond_signal (&thread_data->cond);
  g_mutex_unlock (&thread_data->cond_mutex);

  // Wait for the main thread to own the name before watching it
  g_mutex_lock (&thread_data->cond_mutex);
  while (!g_atomic_int_get (&thread_data->name_acquired))
    g_cond_wait (&thread_data->cond, &thread_data->cond_mutex);
  g_mutex_unlock (&thread_data->cond_mutex);

  thread_data->data.num_appeared = 0;
  thread_data->data.num_vanished = 0;
  thread_data->data.num_free_func = 0;
  // g_signal_connect_after is important to have default handler be called before our code
  g_signal_connect_after (thread_data->connection, "closed", G_CALLBACK (connection_closed_cb), thread_data);

  g_mutex_lock (&thread_data->mutex);
  thread_data->watch_id = g_bus_watch_name_on_connection (thread_data->connection,
                                                          "org.gtk.GDBus.Name1",
                                                          G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                          t_name_appeared_handler,
                                                          t_name_vanished_handler,
                                                          thread_data,
                                                          (GDestroyNotify) t_watch_name_data_free_func);
  g_mutex_unlock (&thread_data->mutex);

  g_assert_cmpint (thread_data->data.num_appeared, ==, 0);
  g_assert_cmpint (thread_data->data.num_vanished, ==, 0);
  while (thread_data->data.num_appeared == 0)
    g_main_context_iteration (thread_context, TRUE);
  g_assert_cmpint (thread_data->data.num_appeared, ==, 1);
  g_assert_cmpint (thread_data->data.num_vanished, ==, 0);
  thread_data->data.num_appeared = 0;

  /* Close the connection and:
   *  - check that we had received a vanished event even begin in different thread
   *  - or check that unwatching the bus when a vanished had been scheduled
   *    make it correctly unscheduled (unwatch_early condition)
   */
  g_dbus_connection_close_sync (thread_data->connection, NULL, NULL);
  if (thread_data->unwatch_early)
    {
      // Wait for the main thread to iterate in order to have close connection handled
      g_mutex_lock (&thread_data->mutex);
      while (g_atomic_int_get (&thread_data->watch_id) != 0)
        g_cond_wait (&thread_data->cond, &thread_data->mutex);
      g_mutex_unlock (&thread_data->mutex);

      while (thread_data->data.num_free_func == 0)
        g_main_context_iteration (thread_context, TRUE);
      g_assert_cmpint (thread_data->data.num_vanished, ==, 0);
      g_assert_cmpint (thread_data->data.num_appeared, ==, 0);
      g_assert_cmpint (thread_data->data.num_free_func, ==, 1);
    }
  else
    {
      while (thread_data->data.num_vanished == 0)
        {
          /*
           * Close of connection is treated in the context of the thread which
           * creates the connection. We must run iteration on it (to have the 'closed'
           * signal handled) and also run current thread loop to have name_vanished
           * callback handled.
           */
          g_main_context_iteration (thread_context, TRUE);
        }
      g_assert_cmpint (thread_data->data.num_vanished, ==, 1);
      g_assert_cmpint (thread_data->data.num_appeared, ==, 0);
      g_mutex_lock (&thread_data->mutex);
      g_bus_unwatch_name (g_atomic_int_get (&thread_data->watch_id));
      g_atomic_int_set (&thread_data->watch_id, 0);
      g_mutex_unlock (&thread_data->mutex);
      while (thread_data->data.num_free_func == 0)
        g_main_context_iteration (thread_context, TRUE);
      g_assert_cmpint (thread_data->data.num_free_func, ==, 1);
    }

  g_mutex_lock (&thread_data->cond_mutex);
  thread_data->ended = TRUE;
  g_main_context_wakeup (NULL);
  g_cond_signal (&thread_data->cond);
  g_mutex_unlock (&thread_data->cond_mutex);

  g_signal_handlers_disconnect_by_func (thread_data->connection, connection_closed_cb, thread_data);
  g_object_unref (thread_data->connection);
  g_main_context_pop_thread_default (thread_context);
  g_main_context_unref (thread_context);

  g_mutex_lock (&thread_data->mutex);
  g_assert_cmpint (thread_data->watch_id, ==, 0);
  g_mutex_unlock (&thread_data->mutex);
  return NULL;
}

static void
watch_with_different_context (gboolean unwatch_early)
{
  OwnNameData own_data;
  WatchNameThreadData thread_data;
  GDBusConnection *connection;
  GThread *watcher;
  guint id;
  GMainContext *main_context = NULL;  /* use the global default for now */

  session_bus_up ();

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert (connection != NULL);

  g_mutex_init (&thread_data.mutex);
  g_mutex_init (&thread_data.cond_mutex);
  g_cond_init (&thread_data.cond);
  thread_data.started = FALSE;
  thread_data.name_acquired = FALSE;
  thread_data.ended = FALSE;
  thread_data.connection = g_object_ref (connection);
  thread_data.unwatch_early = unwatch_early;

  // Create a thread which will watch a name and wait for it to be ready
  g_mutex_lock (&thread_data.cond_mutex);
  watcher = g_thread_new ("watcher", watcher_thread, &thread_data);
  while (!g_atomic_int_get (&thread_data.started))
    g_cond_wait (&thread_data.cond, &thread_data.cond_mutex);
  g_mutex_unlock (&thread_data.cond_mutex);

  own_data.num_acquired = 0;
  own_data.num_lost = 0;
  own_data.num_free_func = 0;
  own_data.expect_null_connection = FALSE;
  own_data.main_context = main_context;
  // Own the name to avoid direct name vanished in watcher thread
  id = g_bus_own_name_on_connection (connection,
                                     "org.gtk.GDBus.Name1",
                                     G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                     w_name_acquired_handler,
                                     w_name_lost_handler,
                                     &own_data,
                                     (GDestroyNotify) own_name_data_free_func);
  while (own_data.num_acquired == 0)
    g_main_context_iteration (main_context, TRUE);
  g_assert_cmpint (own_data.num_acquired, ==, 1);
  g_assert_cmpint (own_data.num_lost, ==, 0);

  // Wake the thread for it to begin watch
  g_mutex_lock (&thread_data.cond_mutex);
  g_atomic_int_set (&thread_data.name_acquired, TRUE);
  g_cond_signal (&thread_data.cond);
  g_mutex_unlock (&thread_data.cond_mutex);

  // Iterate the loop until thread is waking us up
  while (!thread_data.ended)
    g_main_context_iteration (main_context, TRUE);

  g_thread_join (watcher);

  g_bus_unown_name (id);
  while (own_data.num_free_func == 0)
    g_main_context_iteration (main_context, TRUE);
  g_assert_cmpint (own_data.num_free_func, ==, 1);

  g_mutex_clear (&thread_data.mutex);
  g_mutex_clear (&thread_data.cond_mutex);
  g_cond_clear (&thread_data.cond);

  session_bus_stop ();
  g_assert_true (g_dbus_connection_is_closed (connection));
  g_object_unref (connection);
  session_bus_down ();
}

static void
test_bus_watch_different_context (void)
{
  watch_with_different_context (FALSE);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_bus_unwatch_early (void)
{
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/604");
  watch_with_different_context (TRUE);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_validate_names (void)
{
  guint n;
  static const struct
  {
    gboolean name;
    gboolean unique;
    gboolean interface;
    const gchar *string;
  } names[] = {
    { 1, 0, 1, "valid.well_known.name"},
    { 1, 0, 0, "valid.well-known.name"},
    { 1, 1, 0, ":valid.unique.name"},
    { 0, 0, 0, "invalid.5well_known.name"},
    { 0, 0, 0, "4invalid.5well_known.name"},
    { 1, 1, 0, ":4valid.5unique.name"},
    { 0, 0, 0, ""},
    { 1, 0, 1, "very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.name1"}, /* 255 */
    { 0, 0, 0, "very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.very.long.name12"}, /* 256 - too long! */
    { 0, 0, 0, ".starts.with.a.dot"},
    { 0, 0, 0, "contains.invalid;.characters"},
    { 0, 0, 0, "contains.inva/lid.characters"},
    { 0, 0, 0, "contains.inva[lid.characters"},
    { 0, 0, 0, "contains.inva]lid.characters"},
    { 0, 0, 0, "contains.inva_æøå_lid.characters"},
    { 1, 1, 0, ":1.1"},
  };

  for (n = 0; n < G_N_ELEMENTS (names); n++)
    {
      if (names[n].name)
        g_assert (g_dbus_is_name (names[n].string));
      else
        g_assert (!g_dbus_is_name (names[n].string));

      if (names[n].unique)
        g_assert (g_dbus_is_unique_name (names[n].string));
      else
        g_assert (!g_dbus_is_unique_name (names[n].string));

      if (names[n].interface)
        {
          g_assert (g_dbus_is_interface_name (names[n].string));
          g_assert (g_dbus_is_error_name (names[n].string)); 
        }
      else
        {
          g_assert (!g_dbus_is_interface_name (names[n].string));
          g_assert (!g_dbus_is_error_name (names[n].string));
        }        
    }
}

static void
assert_cmp_escaped_object_path (const gchar *s,
                                const gchar *correct_escaped)
{
  gchar *escaped;
  guint8 *unescaped;

  escaped = g_dbus_escape_object_path (s);
  g_assert_cmpstr (escaped, ==, correct_escaped);

  g_free (escaped);
  escaped = g_dbus_escape_object_path_bytestring ((const guint8 *) s);
  g_assert_cmpstr (escaped, ==, correct_escaped);

  unescaped = g_dbus_unescape_object_path (escaped);
  g_assert_cmpstr ((const gchar *) unescaped, ==, s);

  g_free (escaped);
  g_free (unescaped);
}

static void
test_escape_object_path (void)
{
  assert_cmp_escaped_object_path ("Foo42", "Foo42");
  assert_cmp_escaped_object_path ("foo.bar.baz", "foo_2ebar_2ebaz");
  assert_cmp_escaped_object_path ("foo_bar_baz", "foo_5fbar_5fbaz");
  assert_cmp_escaped_object_path ("_", "_5f");
  assert_cmp_escaped_object_path ("__", "_5f_5f");
  assert_cmp_escaped_object_path ("", "_");
  assert_cmp_escaped_object_path (":1.42", "_3a1_2e42");
  assert_cmp_escaped_object_path ("a/b", "a_2fb");
  assert_cmp_escaped_object_path (" ", "_20");
  assert_cmp_escaped_object_path ("\n", "_0a");

  g_assert_null (g_dbus_unescape_object_path ("_ii"));
  g_assert_null (g_dbus_unescape_object_path ("döner"));
  g_assert_null (g_dbus_unescape_object_path ("_00"));
  g_assert_null (g_dbus_unescape_object_path ("_61"));
  g_assert_null (g_dbus_unescape_object_path ("_ga"));
  g_assert_null (g_dbus_unescape_object_path ("_ag"));
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  gint ret;

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_dbus_unset ();

  g_test_add_func ("/gdbus/validate-names", test_validate_names);
  g_test_add_func ("/gdbus/bus-own-name", test_bus_own_name);
  g_test_add_data_func ("/gdbus/bus-watch-name",
                        &watch_no_closures_no_flags,
                        test_bus_watch_name);
  g_test_add_data_func ("/gdbus/bus-watch-name-auto-start",
                        &watch_no_closures_flags_auto_start,
                        test_bus_watch_name);
  g_test_add_data_func ("/gdbus/bus-watch-name-auto-start-service-exist",
                        &watch_no_closures_flags_auto_start_service_exist,
                        test_bus_watch_name);
  g_test_add_data_func ("/gdbus/bus-watch-name-closures",
                        &watch_closures_no_flags,
                        test_bus_watch_name);
  g_test_add_data_func ("/gdbus/bus-watch-name-closures-auto-start",
                        &watch_closures_flags_auto_start,
                        test_bus_watch_name);
  g_test_add_func ("/gdbus/bus-watch-different-context", test_bus_watch_different_context);
  g_test_add_func ("/gdbus/bus-unwatch-early", test_bus_unwatch_early);
  g_test_add_func ("/gdbus/escape-object-path", test_escape_object_path);
  ret = g_test_run();

  return ret;
}
