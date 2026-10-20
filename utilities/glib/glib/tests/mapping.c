/* GLIB - Library of useful routines for C programming
 * Copyright (C) 2005 Matthias Clasen
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

#include <glib.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/wait.h>
#endif
#ifdef G_OS_WIN32
#include <process.h>
#endif

static gboolean stop = FALSE;
static gint parent_pid;

/* Passing argc and argv through global variables */
static char **local_argv;

#ifndef G_OS_WIN32

static void
handle_usr1 (int signum)
{
  stop = TRUE;
}

#endif

static gboolean
check_stop (gpointer data)
{
  GMainLoop *loop = data;

#ifdef G_OS_WIN32
  char *stop_name = NULL;

  stop_name = g_build_filename (g_get_tmp_dir (), "maptest.stop", NULL);
  stop = g_file_test (stop_name, G_FILE_TEST_EXISTS);
  g_free (stop_name);
#endif

  if (stop)
    g_main_loop_quit (loop);

  return TRUE;
}

static void
write_or_die (const gchar *filename,
	      const gchar *contents,
	      gssize       length)
{
  GError *error = NULL;
  gboolean result;

  result = g_file_set_contents (filename, contents, length, &error);
  g_assert_no_error (error);
  g_assert_true (result);
}

static GMappedFile *
map_or_die (const gchar *filename,
	    gboolean     writable)
{
  GError *error = NULL;
  GMappedFile *map;

  map = g_mapped_file_new (filename, writable, &error);
  g_assert_no_error (error);
  g_assert_nonnull (map);

  return map;
}

static void
signal_parent (gpointer data)
{
#ifndef G_OS_WIN32
  kill (parent_pid, SIGUSR1);
#endif
}

static void
child_main (void)
{
  GMappedFile *map;
  GMainLoop *loop;
  const char *dir;
  char *global_filename = NULL, *childname = NULL;

  dir = g_get_tmp_dir ();
  global_filename = g_build_filename (dir, "maptest", NULL);
  childname = g_build_filename (dir, "mapchild", NULL);

  parent_pid = atoi (local_argv[2]);
  map = map_or_die (global_filename, FALSE);

#ifndef G_OS_WIN32
  signal (SIGUSR1, handle_usr1);
#endif
  loop = g_main_loop_new (NULL, FALSE);
  g_idle_add (check_stop, loop);
  g_idle_add_once (signal_parent, NULL);
  g_main_loop_run (loop);

  g_test_message ("test_child_private: received parent signal");

  write_or_die (childname,
                g_mapped_file_get_contents (map),
                g_mapped_file_get_length (map));

  g_free (childname);
  g_free (global_filename);
  g_mapped_file_unref (map);

  signal_parent (NULL);
}

static void
test_mapping_flags (void)
{
  GMappedFile *map;
  const char *dir;
  char *global_filename = NULL;

  dir = g_get_tmp_dir ();
  global_filename = g_build_filename (dir, "maptest", NULL);

  write_or_die (global_filename, "ABC", -1);

  map = map_or_die (global_filename, FALSE);
  g_assert_cmpint (g_mapped_file_get_length (map), ==, 3);
  g_mapped_file_unref (map);

  map = map_or_die (global_filename, TRUE);
  g_assert_cmpint (g_mapped_file_get_length (map), ==, 3);
  g_mapped_file_unref (map);
  g_test_message ("test_mapping: ok");

  /* Cleaning left over files */
  g_remove (global_filename);

  g_free (global_filename);
}

static void
test_private (void)
{
  GError *error = NULL;
  GMappedFile *map;
  gboolean result;
  gchar *buffer;
  gsize len;
  const char *dir;
  char *global_filename = NULL;

  dir = g_get_tmp_dir ();
  global_filename = g_build_filename (dir, "maptest", NULL);

  write_or_die (global_filename, "ABC", -1);
  map = map_or_die (global_filename, TRUE);

  buffer = (gchar *)g_mapped_file_get_contents (map);
  buffer[0] = '1';
  buffer[1] = '2';
  buffer[2] = '3';
  g_mapped_file_unref (map);

  result = g_file_get_contents (global_filename, &buffer, &len, &error);
  g_assert_no_error (error);
  g_assert_true (result);
  g_assert_cmpint (len, ==, 3);
  g_assert_cmpstr (buffer, ==, "ABC");
  g_free (buffer);

  /* Cleaning left over files */
  g_remove (global_filename);

  g_free (global_filename);
}

static void
test_child_private (void)
{
  GError *error = NULL;
  GMappedFile *map;
  gboolean result;
  gchar *buffer;
  gsize len;
  gchar *child_argv[4];
  GPid  child_pid;
  GSpawnFlags spawn_flags = G_SPAWN_DEFAULT;
#ifndef G_OS_WIN32
  GMainLoop *loop;
  int wait_status;
#endif
  gchar pid[100];
  const char *dir;
  char *global_filename = NULL, *childname = NULL, *stop_name = NULL;

  dir = g_get_tmp_dir ();
  global_filename = g_build_filename (dir, "maptest", NULL);
  childname = g_build_filename (dir, "mapchild", NULL);
  stop_name = g_build_filename (dir, "maptest.stop", NULL);

#ifdef G_OS_WIN32
  g_remove (stop_name);
  g_assert_false (g_file_test (stop_name, G_FILE_TEST_EXISTS));
#endif

  write_or_die (global_filename, "ABC", -1);
  map = map_or_die (global_filename, TRUE);

#ifndef G_OS_WIN32
  signal (SIGUSR1, handle_usr1);
  spawn_flags |= G_SPAWN_DO_NOT_REAP_CHILD;
#endif

  g_snprintf (pid, sizeof (pid), "%d", (int) getpid ());
  child_argv[0] = local_argv[0];
  child_argv[1] = "mapchild";
  child_argv[2] = pid;
  child_argv[3] = NULL;

  result = g_spawn_async (dir, child_argv, NULL,
                          spawn_flags, NULL, NULL, &child_pid, &error);
  g_assert_no_error (error);
  g_assert_true (result);
  g_test_message ("test_child_private: child spawned");

#ifndef G_OS_WIN32
  loop = g_main_loop_new (NULL, FALSE);
  g_idle_add (check_stop, loop);
  g_main_loop_run (loop);
  stop = FALSE;
#else
  g_usleep (2000000);
#endif

  g_test_message ("test_child_private: received first child signal");

  buffer = (gchar *)g_mapped_file_get_contents (map);
  buffer[0] = '1';
  buffer[1] = '2';
  buffer[2] = '3';
  g_mapped_file_unref (map);

#ifndef G_OS_WIN32
  kill (child_pid, SIGUSR1);
#else
  g_file_set_contents (stop_name, "Hey there\n", -1, NULL);
#endif

#ifndef G_OS_WIN32
  g_idle_add (check_stop, loop);
  g_main_loop_run (loop);
  waitpid (child_pid, &wait_status, 0);
  g_test_message ("Child exited with status %d", wait_status);
  g_spawn_check_wait_status (wait_status, &error);
  g_assert_no_error (error);
#else
  g_usleep (2000000);
#endif

  g_test_message ("test_child_private: received second child signal");

  result = g_file_get_contents (childname, &buffer, &len, &error);
  g_assert_no_error (error);
  g_assert_true (result);
  g_assert_cmpint (len, ==, 3);
  g_assert_cmpstr (buffer, ==, "ABC");
  g_free (buffer);

  /* Cleaning left over files */
  g_remove (childname);
  g_remove (global_filename);
  g_remove (stop_name);

  g_free (childname);
  g_free (global_filename);
  g_free (stop_name);
}

int
main (int argc,
      char *argv[])
{
#ifndef G_OS_WIN32
  sigset_t sig_mask, old_mask;

  sigemptyset (&sig_mask);
  sigaddset (&sig_mask, SIGUSR1);
  if (sigprocmask (SIG_UNBLOCK, &sig_mask, &old_mask) == 0)
    {
      if (sigismember (&old_mask, SIGUSR1))
        g_test_message ("SIGUSR1 was blocked, unblocking it");
    }
#endif

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
  local_argv = argv;

  if (argc > 1)
    {
      child_main ();
      return EXIT_SUCCESS;
    }

  g_test_add_func ("/mapping/flags", test_mapping_flags);
  g_test_add_func ("/mapping/private", test_private);
  g_test_add_func ("/mapping/private-child", test_child_private);

  return g_test_run ();
}
