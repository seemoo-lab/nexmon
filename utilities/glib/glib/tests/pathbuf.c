/* Unit tests for GPathBuf
 *
 * SPDX-FileCopyrightText: 2023  Emmanuele Bassi
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include <string.h>
#include <errno.h>

#include <glib.h>

#ifndef g_assert_path_buf_equal
#define g_assert_path_buf_equal(p1,p2) \
  G_STMT_START { \
    if (g_path_buf_equal ((p1), (p2))) ; else { \
      char *__p1 = g_path_buf_to_path ((p1)); \
      char *__p2 = g_path_buf_to_path ((p2)); \
      g_assertion_message_cmpstr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                  #p1 " == " #p2, __p1, "==", __p2); \
      g_free (__p1); \
      g_free (__p2); \
    } \
  } G_STMT_END
#endif

static void
test_pathbuf_init (void)
{
#ifdef G_OS_UNIX
  GPathBuf buf, cmp;
  char *path;

  g_test_message ("Initializing empty path buf");
  g_path_buf_init (&buf);
  g_assert_null (g_path_buf_to_path (&buf));
  g_path_buf_clear (&buf);

  g_test_message ("Initializing with empty path");
  g_path_buf_init_from_path (&buf, NULL);
  g_assert_null (g_path_buf_to_path (&buf));
  g_path_buf_clear (&buf);

  g_test_message ("Initializing with full path");
  g_path_buf_init_from_path (&buf, "/usr/bin/echo");
  path = g_path_buf_clear_to_path (&buf);
  g_assert_nonnull (path);
  g_assert_cmpstr (path, ==, "/usr/bin/echo");
  g_free (path);

  g_test_message ("Initializing with no path");
  g_path_buf_init (&buf);
  g_assert_null (g_path_buf_to_path (&buf));
  g_path_buf_clear (&buf);

  g_test_message ("Allocating GPathBuf on the heap");
  GPathBuf *allocated = g_path_buf_new ();
  g_assert_null (g_path_buf_to_path (allocated));
  g_path_buf_clear (allocated);

  g_path_buf_init_from_path (allocated, "/bin/sh");
  path = g_path_buf_to_path (allocated);
  g_assert_cmpstr (path, ==, "/bin/sh");
  g_free (path);

  g_path_buf_clear (allocated);
  g_assert_null (g_path_buf_to_path (allocated));
  g_assert_null (g_path_buf_free_to_path (allocated));

  allocated = g_path_buf_new_from_path ("/bin/sh");
  g_path_buf_init_from_path (&cmp, "/bin/sh");
  g_assert_path_buf_equal (allocated, &cmp);
  g_path_buf_clear (&cmp);
  g_path_buf_free (allocated);

  g_path_buf_init_from_path (&buf, "/usr/bin/bash");
  allocated = g_path_buf_copy (&buf);
  g_assert_path_buf_equal (allocated, allocated);
  g_assert_path_buf_equal (allocated, &buf);
  g_path_buf_clear (&buf);

  g_path_buf_init_from_path (&cmp, "/usr/bin/bash");
  g_assert_path_buf_equal (allocated, &cmp);
  g_path_buf_clear (&cmp);

  g_path_buf_free (allocated);
#elif defined(G_OS_WIN32)
  GPathBuf buf;
  char *path;

  /* Forward slashes and backslashes are treated as interchangeable
   * on input... */
  g_path_buf_init_from_path (&buf, "C:\\windows/system32.dll");
  path = g_path_buf_clear_to_path (&buf);
  g_assert_nonnull (path);
  /* ... and normalized to backslashes on output */
  g_assert_cmpstr (path, ==, "C:\\windows\\system32.dll");
  g_free (path);

  g_path_buf_init (&buf);
  g_assert_null (g_path_buf_to_path (&buf));
  g_path_buf_clear (&buf);

  g_test_message ("Allocating GPathBuf on the heap");
  GPathBuf *allocated = g_path_buf_new ();
  g_assert_null (g_path_buf_to_path (allocated));
  g_path_buf_clear (allocated);

  g_path_buf_init_from_path (allocated, "C:\\does-not-exist.txt");
  path = g_path_buf_to_path (allocated);
  g_assert_cmpstr (path, ==, "C:\\does-not-exist.txt");
  g_free (path);

  g_path_buf_clear (allocated);
  g_assert_null (g_path_buf_to_path (allocated));
  g_assert_null (g_path_buf_free_to_path (allocated));
#else
  g_test_skip ("Unsupported platform"):
#endif
}

static void
test_pathbuf_push_pop (void)
{
#ifdef G_OS_UNIX
  GPathBuf buf, cmp;

  g_test_message ("Pushing relative path component");
  g_path_buf_init_from_path (&buf, "/tmp");
  g_path_buf_push (&buf, ".X11-unix/X0");

  g_path_buf_init_from_path (&cmp, "/tmp/.X11-unix/X0");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_test_message ("Pushing absolute path component");
  g_path_buf_push (&buf, "/etc/locale.conf");
  g_path_buf_init_from_path (&cmp, "/etc/locale.conf");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);
  g_path_buf_clear (&buf);

  g_test_message ("Popping a path component");
  g_path_buf_init_from_path (&buf, "/bin/sh");

  g_assert_true (g_path_buf_pop (&buf));
  g_path_buf_init_from_path (&cmp, "/bin");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_assert_true (g_path_buf_pop (&buf));
  g_path_buf_init_from_path (&cmp, "/");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_test_message ("Can't pop the last element of a path buffer");
  g_assert_false (g_path_buf_pop (&buf));

  g_path_buf_clear (&buf);
  g_path_buf_clear (&cmp);
#elif defined(G_OS_WIN32)
  GPathBuf buf, cmp;

  g_test_message ("Pushing relative path component");
  g_path_buf_init_from_path (&buf, "C:\\");
  g_path_buf_push (&buf, "windows");
  g_path_buf_push (&buf, "system32.dll");

  g_test_message ("Popping a path component");
  g_path_buf_init_from_path (&cmp, "C:\\windows/system32.dll");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_assert_true (g_path_buf_pop (&buf));
  g_path_buf_init_from_path (&cmp, "C:\\windows");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_assert_true (g_path_buf_pop (&buf));
  g_path_buf_init_from_path (&cmp, "C:");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_test_message ("Can't pop the last element of a path buffer");
  g_assert_false (g_path_buf_pop (&buf));

  g_path_buf_clear (&buf);
  g_path_buf_clear (&cmp);
#else
  g_test_skip ("Unsupported platform"):
#endif
}

static void
test_pathbuf_filename_extension (void)
{
#ifdef G_OS_UNIX
  GPathBuf buf, cmp;

  g_path_buf_init (&buf);
  g_assert_false (g_path_buf_set_filename (&buf, "foo"));
  g_assert_false (g_path_buf_set_extension (&buf, "txt"));
  g_assert_null (g_path_buf_to_path (&buf));
  g_path_buf_clear (&buf);

  g_path_buf_init_from_path (&buf, "/");
  g_path_buf_set_filename (&buf, "bar");

  g_path_buf_init_from_path (&cmp, "/bar");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_path_buf_set_filename (&buf, "baz.txt");
  g_path_buf_init_from_path (&cmp, "/baz.txt");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_path_buf_push (&buf, "/usr");
  g_path_buf_push (&buf, "lib64");
  g_path_buf_push (&buf, "libc");
  g_assert_true (g_path_buf_set_extension (&buf, "so.6"));

  g_path_buf_init_from_path (&cmp, "/usr/lib64/libc.so.6");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_path_buf_clear (&buf);
#elif defined(G_OS_WIN32)
  GPathBuf buf, cmp;

  g_path_buf_init_from_path (&buf, "C:\\");
  g_path_buf_push (&buf, "windows");
  g_path_buf_push (&buf, "system32");
  g_assert_true (g_path_buf_set_extension (&buf, "dll"));

  g_path_buf_init_from_path (&cmp, "C:\\windows\\system32.dll");
  g_assert_path_buf_equal (&buf, &cmp);
  g_path_buf_clear (&cmp);

  g_path_buf_clear (&buf);
#else
  g_test_skip ("Unsupported platform"):
#endif
}

int
main (int   argc,
      char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/pathbuf/init", test_pathbuf_init);
  g_test_add_func ("/pathbuf/push-pop", test_pathbuf_push_pop);
  g_test_add_func ("/pathbuf/filename-extension", test_pathbuf_filename_extension);

  return g_test_run ();
}
