/*
 * Copyright 2018 Collabora Ltd.
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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <locale.h>
#include <stdio.h>
#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

static void
test_pass (void)
{
}

static void
test_skip (void)
{
  g_test_skip ("not enough tea");
}

static void
test_skip_printf (void)
{
  const char *beverage = "coffee";

  g_test_skip_printf ("not enough %s", beverage);
}

static void
test_fail (void)
{
  g_test_fail ();
}

static void
test_error (void)
{
  /* We expect this test to abort, so try to avoid that creating a coredump */
  g_test_disable_crash_reporting ();

  g_error ("This should error out\nBecause it's just\nwrong!");
}

static void
test_fail_printf (void)
{
  g_test_fail_printf ("this test intentionally left failing");
}

static void
test_incomplete (void)
{
  g_test_incomplete ("mind reading not implemented yet");
}

static void
test_incomplete_printf (void)
{
  const char *operation = "telekinesis";

  g_test_incomplete_printf ("%s not implemented yet", operation);
}

static void
test_summary (void)
{
  g_test_summary ("Tests that g_test_summary() works with TAP, by outputting a "
                  "known summary message in testing-helper, and checking for "
                  "it in the TAP output later.");
}

static void
test_message (void)
{
  g_test_message ("Tests that single line message works");
  g_test_message ("Tests that multi\n\nline\nmessage\nworks");
  g_test_message ("\nTests that multi\nline\nmessage\nworks with leading and trailing too\n");
}

static void
test_print (void)
{
  g_print ("Tests that single line message works\n");
  g_print ("test that multiple\nlines ");
  g_print ("can be ");
  g_print ("written ");
  g_print ("separately\n");
}

static void
test_subprocess_stdout (void)
{
  if (g_test_subprocess ())
    {
      printf ("Tests that single line message works\n");
      printf ("test that multiple\nlines ");
      printf ("can be ");
      printf ("written ");
      printf ("separately\n");

      puts ("And another line has been put");

      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT);
  g_test_trap_has_passed ();

  g_test_trap_assert_stdout ("/sub-stdout: Tests that single line message works\n*");
  g_test_trap_assert_stdout ("*\ntest that multiple\nlines can be written separately\n*");
  g_test_trap_assert_stdout ("*\nAnd another line has been put\n*");
}

static void
test_subprocess_stdout_no_nl (void)
{
  if (g_test_subprocess ())
    {
      printf ("A message without trailing new line");
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT);
  g_test_trap_has_passed ();

  g_test_trap_assert_stdout ("/sub-stdout-no-nl: A message without trailing new line");
}

int
main (int   argc,
      char *argv[])
{
  char *argv1;

  setlocale (LC_ALL, "");

#ifdef G_OS_WIN32
  /* Windows opens std streams in text mode, with \r\n EOLs.
   * Sometimes it's easier to force a switch to binary mode than
   * to account for extra \r in testcases.
   */
  setmode (fileno (stdout), O_BINARY);
#endif

  g_return_val_if_fail (argc > 1, 1);
  argv1 = argv[1];

  if (argc > 2)
    memmove (&argv[1], &argv[2], (argc - 2) * sizeof (char *));

  argc -= 1;
  argv[argc] = NULL;

  if (g_strcmp0 (argv1, "init-null-argv0") == 0)
    {
      int test_argc = 0;
      char *test_argva[1] = { NULL };
      char **test_argv = test_argva;

      /* Test that `g_test_init()` can handle being called with an empty argv
       * and argc == 0. While this isn’t recommended, it is possible for another
       * process to use execve() to call a gtest process this way, so we’d
       * better handle it gracefully.
       *
       * This test can’t be run after `g_test_init()` has been called normally,
       * as it isn’t allowed to be called more than once in a process. */
      g_test_init (&test_argc, &test_argv, NULL);

      return 0;
    }

  g_test_init (&argc, &argv, NULL);
  g_test_set_nonfatal_assertions ();

  if (g_strcmp0 (argv1, "pass") == 0)
    {
      g_test_add_func ("/pass", test_pass);
    }
  else if (g_strcmp0 (argv1, "skip") == 0)
    {
      g_test_add_func ("/skip", test_skip);
    }
  else if (g_strcmp0 (argv1, "skip-printf") == 0)
    {
      g_test_add_func ("/skip-printf", test_skip_printf);
    }
  else if (g_strcmp0 (argv1, "incomplete") == 0)
    {
      g_test_add_func ("/incomplete", test_incomplete);
    }
  else if (g_strcmp0 (argv1, "incomplete-printf") == 0)
    {
      g_test_add_func ("/incomplete-printf", test_incomplete_printf);
    }
  else if (g_strcmp0 (argv1, "fail") == 0)
    {
      g_test_add_func ("/fail", test_fail);
    }
  else if (g_strcmp0 (argv1, "error") == 0)
    {
      g_test_add_func ("/error", test_error);
    }
  else if (g_strcmp0 (argv1, "error-and-pass") == 0)
    {
      g_test_add_func ("/error", test_error);
      g_test_add_func ("/pass", test_pass);
    }
  else if (g_strcmp0 (argv1, "fail-printf") == 0)
    {
      g_test_add_func ("/fail-printf", test_fail_printf);
    }
  else if (g_strcmp0 (argv1, "all-non-failures") == 0)
    {
      g_test_add_func ("/pass", test_pass);
      g_test_add_func ("/skip", test_skip);
      g_test_add_func ("/incomplete", test_incomplete);
    }
  else if (g_strcmp0 (argv1, "all") == 0)
    {
      g_test_add_func ("/pass", test_pass);
      g_test_add_func ("/skip", test_skip);
      g_test_add_func ("/incomplete", test_incomplete);
      g_test_add_func ("/fail", test_fail);
    }
  else if (g_strcmp0 (argv1, "skip-options") == 0)
    {
      /* The caller is expected to skip some of these with
       * -p/-r, -s/-x and/or --GTestSkipCount */
      g_test_add_func ("/a", test_pass);
      g_test_add_func ("/b", test_pass);
      g_test_add_func ("/b/a", test_pass);
      g_test_add_func ("/b/b", test_pass);
      g_test_add_func ("/b/b/a", test_pass);
      g_test_add_func ("/prefix/a", test_pass);
      g_test_add_func ("/prefix/b/b", test_pass);
      g_test_add_func ("/prefix-long/a", test_pass);
      g_test_add_func ("/c/a", test_pass);
      g_test_add_func ("/d/a", test_pass);
    }
  else if (g_strcmp0 (argv1, "summary") == 0)
    {
      g_test_add_func ("/summary", test_summary);
    }
  else if (g_strcmp0 (argv1, "message") == 0)
    {
      g_test_add_func ("/message", test_message);
    }
  else if (g_strcmp0 (argv1, "print") == 0)
    {
      g_test_add_func ("/print", test_print);
    }
  else if (g_strcmp0 (argv1, "subprocess-stdout") == 0)
    {
      g_test_add_func ("/sub-stdout", test_subprocess_stdout);
    }
  else if (g_strcmp0 (argv1, "subprocess-stdout-no-nl") == 0)
    {
      g_test_add_func ("/sub-stdout-no-nl", test_subprocess_stdout_no_nl);
    }
  else
    {
      if (g_test_subprocess ())
        {
          g_test_add_func ("/sub-stdout", test_subprocess_stdout);
          g_test_add_func ("/sub-stdout-no-nl", test_subprocess_stdout_no_nl);
        }
      else
        {
          g_assert_not_reached ();
        }
    }

  return g_test_run ();
}
