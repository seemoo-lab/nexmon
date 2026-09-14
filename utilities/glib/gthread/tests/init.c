/* GLib testing framework examples and tests
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
 * Author: Philip Withnall <pwithnall@endlessos.org>
 */

#include <glib.h>
#include <locale.h>


/* All of GThread is deprecated, but that’s OK */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_thread_deprecated_init (void)
{
  const GThreadFunctions functions = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL
  };

  /* Should be a no-op. */
  g_thread_init (NULL);

  /* Should emit a warning. */
  g_test_expect_message ("GThread", G_LOG_LEVEL_WARNING,
                         "GThread system no longer supports custom thread implementations.");
  g_thread_init ((gpointer) &functions);
  g_test_assert_expected_messages ();
}

static void
test_thread_deprecated_init_with_errorcheck_mutexes (void)
{
  /* Should warn. */
  g_test_expect_message ("GThread", G_LOG_LEVEL_WARNING,
                         "GThread system no longer supports errorcheck mutexes.");
  g_thread_init_with_errorcheck_mutexes (NULL);
  g_test_assert_expected_messages ();
}

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/thread/deprecated/init", test_thread_deprecated_init);
  g_test_add_func ("/thread/deprecated/init-with-errorcheck-mutexes", test_thread_deprecated_init_with_errorcheck_mutexes);

  return g_test_run ();
}

G_GNUC_END_IGNORE_DEPRECATIONS
