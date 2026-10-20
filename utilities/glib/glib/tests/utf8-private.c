/*
 * Copyright © 2025 Luca Bacci
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

#include "config.h"

#include <glib.h>
#include <glib/gunicodeprivate.h>

static void
test_utf8_to_utf16_make_valid (void)
{
  #define MAKE_ENTRY(string, string_is_valid_utf8) \
    {string, string_is_valid_utf8, NULL, 0}
  #define MAKE_VALID_UTF8_ENTRY(string) \
    MAKE_ENTRY(string, TRUE)
  #define MAKE_INVALID_UTF8_ENTRY(string) \
    MAKE_ENTRY(string, FALSE)
  struct {
    const char *utf8;
    gboolean    utf8_is_valid;
    gunichar2  *utf16;
    glong       utf16_length;
  } strings[] = {
    MAKE_VALID_UTF8_ENTRY ("hello"),
    MAKE_INVALID_UTF8_ENTRY ("\xf4\xf4\xf4"),
    MAKE_VALID_UTF8_ENTRY ("world"),
    MAKE_INVALID_UTF8_ENTRY ("\xf4\xf4\xf4"),
    MAKE_VALID_UTF8_ENTRY ("絵文字"),
    MAKE_VALID_UTF8_ENTRY (" 🚀"),
  };
  #undef MAKE_INVALID_UTF8_ENTRY
  #undef MAKE_VALID_UTF8_ENTRY
  #undef MAKE_ENTRY

  /* Set up structures */

  GString *utf8 = g_string_sized_new (100);

  for (size_t i = 0; i < G_N_ELEMENTS (strings); i++)
    g_string_append (utf8, strings[i].utf8);

  /* Could use UTF-16 string literals beginning with C23 */
  for (size_t i = 0; i < G_N_ELEMENTS (strings); i++)
    {
      if (strings[i].utf8_is_valid)
        {
          strings[i].utf16 = g_utf8_to_utf16 (strings[i].utf8, -1, NULL,
                                              &strings[i].utf16_length, NULL);
        }
    }

  /* Actual test */

{
  gunichar2 buffer[500];
  gunichar2 *utf16;
  size_t utf16_length = 0;
  const gunichar2 *iter = buffer;

  g_utf8_to_utf16_make_valid (utf8->str,
                              buffer, G_N_ELEMENTS (buffer),
                              &utf16, &utf16_length);

  /* The result fits in the passed staging buffer */
  g_assert_true (utf16 == buffer);

  for (size_t i = 0; i < G_N_ELEMENTS (strings); i++)
    {
      if (strings[i].utf8_is_valid)
        {
          size_t size = strings[i].utf16_length * sizeof (gunichar2);
          g_assert_true (memcmp (iter, strings[i].utf16, size) == 0);

          iter += strings[i].utf16_length;
        }
      else
        {
          g_assert_cmpint (*iter, ==, 0xFFFD);

          while (*iter == 0xFFFD)
            iter++;
        }
    }
  g_assert_cmpint (*iter, ==, (gunichar2) 0);

  /* Test small buffer */
  g_utf8_to_utf16_make_valid (utf8->str,
                              buffer, 10,
                              &utf16, &utf16_length);

  g_assert_true (utf16 != buffer);
  g_assert_cmpint (buffer[utf16_length], == , (gunichar2) 0);
  g_assert_cmpint (utf16[utf16_length], == , (gunichar2) 0);
  g_assert_cmpmem (utf16, utf16_length, buffer, utf16_length);

  g_free (utf16);
}

  /* Clean up structures */

  g_string_free (utf8, TRUE);

  for (size_t i = 0; i < G_N_ELEMENTS (strings); i++)
    {
      g_free (strings[i].utf16);
    }
}

static void
test_utf8_to_utf16_make_valid_backtrack (void)
{
  const char *utf8 =
    "a" /* 1 byte => 1 gunichar2 */
    "α" /* 2 bytes => 1 gunichar2 */
    "⍺" /* 3 bytes => 1 gunichar2 */
    "𝐀" /* 4 bytes => 2 gunichar2 */
    "\xf4\xf4"; /* invalid sequence of 2 bytes => 2 gunichar2 */
  gunichar2 buffer[50];
  gunichar2 *utf16;
  size_t utf8_offset = 0;
  size_t utf16_offset = 0;
  size_t i;

  g_utf8_to_utf16_make_valid (utf8, buffer, G_N_ELEMENTS (buffer), &utf16, NULL);

  for (i = 0; i <= 3; i++)
    {
      utf8_offset = g_utf8_to_utf16_make_valid_backtrack (utf8, utf16_offset++);
      g_assert_cmpint (utf8_offset, ==, i * (i + 1) / 2);
    }

  for (i = 0; i < 2; i++)
    {
      utf8_offset = g_utf8_to_utf16_make_valid_backtrack (utf8, utf16_offset++);
      g_assert_cmpint (utf8_offset, ==, 6 + 4);
    }

  for (i = 0; i < 2; i++)
    {
      utf8_offset = g_utf8_to_utf16_make_valid_backtrack (utf8, utf16_offset++);
      g_assert_cmpint (utf8_offset, ==, 6 + 4 + i + 1);
    }

  /* Passing overlong offsets is safe */
  utf8_offset = g_utf8_to_utf16_make_valid_backtrack (utf8, 1000);
  g_assert_cmpint (utf8_offset, ==, strlen (utf8));

  /* Edge case */
  g_assert_cmpint (g_utf8_to_utf16_make_valid_backtrack ("", 0), ==, 0);

  /* No need to call g_free */
  g_assert_true (buffer == utf16);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/utf8/utf8-to-utf16-make-valid", test_utf8_to_utf16_make_valid);
  g_test_add_func ("/utf8/utf8-to-utf16-make-valid-backtrack", test_utf8_to_utf16_make_valid_backtrack);

  return g_test_run();
}
