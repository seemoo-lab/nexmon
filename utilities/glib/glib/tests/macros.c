/* GLib testing framework examples and tests
 *
 * Copyright © 2018 Endless Mobile, Inc.
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
 * Author: Philip Withnall <withnall@endlessm.com>
 */

#include <glib.h>

#ifdef G_CXX_STD_VERSION
#error G_CXX_STD_VERSION should be undefined in C programs
#endif

G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (98));
G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (89));
G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (90));

#if G_C_STD_VERSION >= 199000L
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (89));
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (90));
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (199000L));
#endif

#if G_C_STD_VERSION == 198900L
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (99));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (199901L));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (11));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (201112L));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (201710L));
#endif

#if G_C_STD_VERSION >= 199901L
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (99));
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (199901L));
#endif

#if G_C_STD_VERSION == 199901L
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (11));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (201112L));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (201710L));
#endif

#if G_C_STD_VERSION >= 201112L
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (11));
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (201112L));
#endif

#if G_C_STD_VERSION == 201112L
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (201710L));
#endif

#if G_C_STD_VERSION >= 201710L
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (201710L));
#endif

#if G_C_STD_VERSION == 201710L
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (23));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (202311L));
#endif

#if G_C_STD_VERSION >= 202311L
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (23));
  G_STATIC_ASSERT (G_C_STD_CHECK_VERSION (202311L));
#endif

#if G_C_STD_VERSION == 202311L
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (26));
  G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (202600L));
#endif

#ifdef _G_EXPECTED_C_STANDARD
static void
test_c_standard (void)
{
  guint64 std_version = 0;

  if (!g_ascii_string_to_unsigned (_G_EXPECTED_C_STANDARD, 10, 0, G_MAXUINT64,
                                   &std_version, NULL))
    {
      g_test_skip ("Expected standard value is non-numeric: "
                   _G_EXPECTED_C_STANDARD);
      return;
    }

  g_test_message ("G_C_STD_VERSION is %lu", G_C_STD_VERSION);

  g_assert_true (G_C_STD_CHECK_VERSION (std_version));

  if (std_version > 80 && std_version < 99)
    std_version = 90;

  if (std_version >= 90)
    g_assert_cmpuint (G_C_STD_VERSION, >=, (std_version + 1900) * 100);
  else if (std_version == 23)
    /* see comment in the definition of G_C_STD_CHECK_VERSION */
    g_assert_cmpuint (G_C_STD_VERSION, >=, 202000L);
  else
    g_assert_cmpuint (G_C_STD_VERSION, >=, (std_version + 2000) * 100);
}
#endif

/* Test that G_STATIC_ASSERT_EXPR can be used as an expression */
static void
test_assert_static (void)
{
  G_STATIC_ASSERT (4 == 4);
  if (G_STATIC_ASSERT_EXPR (1 == 1), sizeof (gchar) == 2)
    g_assert_not_reached ();
}

/* Test G_ALIGNOF() gives the same results as the G_STRUCT_OFFSET fallback. This
 * should be the minimal alignment for the given type.
 *
 * This is necessary because the implementation of G_ALIGNOF() varies depending
 * on the compiler in use. We want all implementations to be consistent.
 *
 * In the case that the compiler uses the G_STRUCT_OFFSET fallback, this test
 * is a no-op. */
static void
test_alignof_fallback (void)
{
#define check_alignof(type) \
  g_assert_cmpint (G_ALIGNOF (type), ==, G_STRUCT_OFFSET (struct { char a; type b; }, b))

  check_alignof (char);
  check_alignof (int);
  check_alignof (float);
  check_alignof (double);
  check_alignof (struct { char a; int b; });
}

static void
test_struct_sizeof_member (void)
{
    G_STATIC_ASSERT (G_SIZEOF_MEMBER (struct { char a; int b; }, a) == sizeof (char));
    g_assert_cmpint (G_SIZEOF_MEMBER (struct { char a; int b; }, b), ==, sizeof (int));
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#ifdef _G_EXPECTED_C_STANDARD
  g_test_add_func ("/C/standard-" _G_EXPECTED_C_STANDARD, test_c_standard);
#endif

  g_test_add_func ("/alignof/fallback", test_alignof_fallback);
  g_test_add_func ("/assert/static", test_assert_static);
  g_test_add_func ("/struct/sizeof_member", test_struct_sizeof_member);

  return g_test_run ();
}
