/* Unit tests for gmessages on low-memory
 *
 * Copyright (C) 2022 Marco Trevisan
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
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"

#include <dlfcn.h>
#include <glib.h>

static gboolean malloc_eom = FALSE;
static gboolean our_malloc_called = FALSE;

#ifdef ENOMEM
/* Wrapper around malloc() which returns `ENOMEM` if the test variable
 * `malloc_eom` is set.
 * Otherwise passes through to the normal malloc() in libc.
 */

void *
malloc (size_t size)
{
  static void *(*real_malloc)(size_t);
  if (!real_malloc)
      real_malloc = dlsym (RTLD_NEXT, "malloc");

  if (malloc_eom)
    {
      our_malloc_called = TRUE;
      errno = ENOMEM;
      return NULL;
    }

  return real_malloc (size);
}
#endif

int
main (int   argc,
      char *argv[])
{
  /* We expect this test to abort, so try to avoid that creating a coredump */
  g_test_disable_crash_reporting ();

  g_setenv ("LC_ALL", "C", TRUE);

#ifndef ENOMEM
  g_message ("ENOMEM Not defined, test skipped");
  return 77;
#endif

  g_message ("Simulates a situation in which we were crashing because "
             "of low-memory, leading malloc to fail instead of aborting");
  g_message ("bug: https://gitlab.gnome.org/GNOME/glib/-/issues/2753");

  /* Setting `malloc_eom` to true should cause the override `malloc()`
   * in this file to fail on the allocation on the next line. */
  malloc_eom = TRUE;
  g_message ("Memory is exhausted, but we'll write anyway: %u", 123);

#ifndef __linux__
  if (!our_malloc_called)
    {
      /* For some reasons this doesn't work on darwin systems, so ignore the result
       * for non-linux, while we want to ensure the test is valid at least there
       */
      g_message ("Our malloc implementation has not been called, the test "
                 "has not been performed");
      return 77;
    }
#endif

  return 0;
}
