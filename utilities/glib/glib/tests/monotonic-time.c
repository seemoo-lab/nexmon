/* Unit tests for get_monotonic_time()
 * Copyright (C) 2026 Red Hat, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Otte
 */

#include "glib.h"

static void
test_increasing (void)
{
  int64_t first_time, last_time;

  first_time = last_time = g_get_monotonic_time ();

  /* This is an implementation detail we know about */
  g_assert_cmpint (first_time, >, 0);

  /* run for one second */
  while (last_time < first_time + G_USEC_PER_SEC)
    {
      int64_t cur_time = g_get_monotonic_time ();

      g_assert_cmpint (cur_time, >=, last_time);

      last_time = cur_time;
    }
}

static void
test_increasing_ns (void)
{
  uint64_t first_time, last_time;

  first_time = last_time = g_get_monotonic_time_ns ();

  /* We should be at least 1ns after boot by now */
  g_assert_cmpint (first_time, >, 0);

  /* run for one second */
  while (last_time < first_time + G_NSEC_PER_SEC)
    {
      uint64_t cur_time = g_get_monotonic_time_ns ();

      g_assert_cmpint (cur_time, >=, last_time);

      last_time = cur_time;
    }
}

static void
test_usleep (void)
{
  int64_t before, after, sleep_intended, sleep_actual;
  int i, n;

  n = 5;
  sleep_intended = G_USEC_PER_SEC / n;

  for (i = 0; i < n; i++)
    {
      before = g_get_monotonic_time ();
      g_usleep (sleep_intended);
      after = g_get_monotonic_time ();

      g_assert_cmpint (after, >=, before);
      sleep_actual = after - before;

      /* allow 5ms = 1/200s less. */
      g_assert_cmpint (sleep_actual, >, sleep_intended - G_USEC_PER_SEC / 200);
      /* just check that we slept less than 10s, with high load
       * sleeps can take ages */
      g_assert_cmpint (sleep_actual, <, 10 * G_USEC_PER_SEC);
    }
}

static void
test_usleep_ns (void)
{
  uint64_t before, after, sleep_intended, sleep_actual;
  int i, n;

  n = 5;
  sleep_intended = G_NSEC_PER_SEC / n;

  for (i = 0; i < n; i++)
    {
      before = g_get_monotonic_time_ns ();
      g_usleep (sleep_intended / 1000);
      after = g_get_monotonic_time_ns ();

      g_assert_cmpint (after, >=, before);
      sleep_actual = after - before;

      /* allow 5ms = 1/200s less.
       * Don't actually limit the increase, as sleeps can take ages
       * with high load */
      g_assert_cmpint (sleep_actual, >, sleep_intended - G_NSEC_PER_SEC / 200);
      /* just check that we slept less than 10s, with high load
       * sleeps can take ages */
      g_assert_cmpint (sleep_actual, <, 10 * G_NSEC_PER_SEC);
    }
}

static void
test_similar (void)
{
  int64_t us_before, us_after, us_elapsed;
  uint64_t ns1_before, ns1_after, ns1_elapsed;
  uint64_t ns2_before, ns2_after, ns2_elapsed;

  ns1_before = g_get_monotonic_time_ns ();
  us_before = g_get_monotonic_time ();
  ns2_before = g_get_monotonic_time_ns ();

  do
    {
      ns2_after = g_get_monotonic_time_ns ();
      us_after = g_get_monotonic_time ();
    }
  while (us_after == us_before);

  ns1_after = g_get_monotonic_time_ns ();

  g_assert_cmpint (us_after, >, us_before);
  g_assert_cmpint (ns1_after, >, ns1_before);
  /* need >= here, clocks can tick slowly */
  g_assert_cmpint (ns2_after, >=, ns2_before);
  us_elapsed = us_after - us_before;
  ns1_elapsed = ns1_after - ns1_before;
  ns2_elapsed = ns2_after - ns2_before;

  /* +1000 because of rounding errors */
  g_assert_cmpint (ns1_elapsed + 1000, >=, 1000 * us_elapsed);
  g_assert_cmpint (ns2_elapsed, <=, 1000 + 1000 * us_elapsed);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/monotonic-time/increasing", test_increasing);
  g_test_add_func ("/monotonic-time-ns/increasing", test_increasing_ns);
  g_test_add_func ("/monotonic-time/usleep", test_usleep);
  g_test_add_func ("/monotonic-time-ns/usleep", test_usleep_ns);
  g_test_add_func ("/monotonic-time-ns/similar", test_similar);

  return g_test_run ();
}
