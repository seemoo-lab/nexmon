/* Unit tests for gstring
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include "glib.h"


static void
test_string_chunks (void)
{
  GStringChunk *string_chunk;
  gchar *tmp_string, *tmp_string_2;
  gint i;

  string_chunk = g_string_chunk_new (1024);

  for (i = 0; i < 100000; i ++)
    {
      tmp_string = g_string_chunk_insert (string_chunk, "hi pete");
      g_assert_cmpstr ("hi pete", ==, tmp_string);
    }

  tmp_string_2 = g_string_chunk_insert_const (string_chunk, tmp_string);
  g_assert_true (tmp_string_2 != tmp_string);
  g_assert_cmpstr (tmp_string_2, ==, tmp_string);

  tmp_string = g_string_chunk_insert_const (string_chunk, tmp_string);
  g_assert_cmpstr (tmp_string_2, ==, tmp_string);

  g_string_chunk_clear (string_chunk);
  g_string_chunk_free (string_chunk);
}

static void
test_string_chunk_insert (void)
{
  const gchar s0[] = "Testing GStringChunk";
  const gchar s1[] = "a\0b\0c\0d\0";
  const gchar s2[] = "Hello, world";
  GStringChunk *chunk;
  gchar *str[3];

  chunk = g_string_chunk_new (512);

  str[0] = g_string_chunk_insert (chunk, s0);
  str[1] = g_string_chunk_insert_len (chunk, s1, 8);
  str[2] = g_string_chunk_insert (chunk, s2);

  g_assert_cmpmem (s0, sizeof s0, str[0], sizeof s0);
  g_assert_cmpmem (s1, sizeof s1, str[1], sizeof s1);
  g_assert_cmpmem (s2, sizeof s2, str[2], sizeof s2);

  g_string_chunk_free (chunk);
}

static void
test_string_new (void)
{
  GString *string1, *string2;

  string1 = g_string_new ("hi pete!");
  string2 = g_string_new (NULL);

  g_assert_nonnull (string1);
  g_assert_nonnull (string2);
  g_assert_cmpuint (strlen (string1->str), ==, string1->len);
  g_assert_cmpuint (strlen (string2->str), ==, string2->len);
  g_assert_cmpuint (string2->len, ==, 0);
  g_assert_cmpstr ("hi pete!", ==, string1->str);
  g_assert_cmpstr ("", ==, string2->str);

  g_string_free_deep (string1);
  g_string_free_deep (string2);

  string1 = g_string_new_len ("foo", -1);
  string2 = g_string_new_len ("foobar", 3);

  g_assert_cmpstr (string1->str, ==, "foo");
  g_assert_cmpuint (string1->len, ==, 3);
  g_assert_cmpstr (string2->str, ==, "foo");
  g_assert_cmpuint (string2->len, ==, 3);

  g_string_free_deep (string1);
  g_string_free_deep (string2);
}

G_GNUC_PRINTF(2, 3)
static void
my_string_printf (GString     *string,
                  const gchar *format,
                  ...)
{
  va_list args;

  va_start (args, format);
  g_string_vprintf (string, format, args);
  va_end (args);
}

static void
test_string_printf (void)
{
  GString *string;

  string = g_string_new (NULL);

#ifndef G_OS_WIN32
  /* MSVC and mingw32 use the same run-time C library, which doesn't like
     the %10000.10000f format... */
  g_string_printf (string, "%s|%0100d|%s|%0*d|%*.*f|%10000.10000f",
		   "this pete guy sure is a wuss, like he's the number ",
		   1,
		   " wuss.  everyone agrees.\n",
		   10, 666, 15, 15, 666.666666666, 666.666666666);
#else
  g_string_printf (string, "%s|%0100d|%s|%0*d|%*.*f|%100.100f",
		   "this pete guy sure is a wuss, like he's the number ",
		   1,
		   " wuss.  everyone agrees.\n",
		   10, 666, 15, 15, 666.666666666, 666.666666666);
#endif

  g_string_free (string, TRUE);

  string = g_string_new (NULL);
  g_string_printf (string, "bla %s %d", "foo", 99);
  g_assert_cmpstr (string->str, ==, "bla foo 99");
  my_string_printf (string, "%d,%s,%d", 1, "two", 3);
  g_assert_cmpstr (string->str, ==, "1,two,3");

  g_string_free (string, TRUE);
}

static void
test_string_assign (void)
{
  GString *string;

  string = g_string_new (NULL);
  g_string_assign (string, "boring text");
  g_assert_cmpstr (string->str, ==, "boring text");
  g_string_free (string, TRUE);

  /* assign with string overlap */
  string = g_string_new ("textbeforetextafter");
  g_string_assign (string, string->str + 10);
  g_assert_cmpstr (string->str, ==, "textafter");
  g_string_free (string, TRUE);

  string = g_string_new ("boring text");
  g_string_assign (string, string->str);
  g_assert_cmpstr (string->str, ==, "boring text");
  g_string_free (string, TRUE);
}

static void
test_string_append_c (void)
{
  GString *string;
  guint i;

  string = g_string_new ("hi pete!");

  for (i = 0; i < 10000; i++)
    if (i % 2)
      g_string_append_c (string, 'a'+(i%26));
    else
      (g_string_append_c) (string, 'a'+(i%26));

  g_assert_true ((strlen("hi pete!") + 10000) == string->len);
  g_assert_true ((strlen("hi pete!") + 10000) == strlen(string->str));

  for (i = 0; i < 10000; i++)
    g_assert_true (string->str[strlen ("Hi pete!") + i] == 'a' + (gchar) (i%26));

  g_string_free (string, TRUE);
}

static void
test_string_append (void)
{
  GString *string;
  char *tmp;
  int i;

  tmp = g_strdup ("more");

  /* append */
  string = g_string_new ("firsthalf");
  g_string_append (string, "last");
  (g_string_append) (string, "half");

  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");

  i = 0;
  g_string_append (string, &tmp[i++]);
  (g_string_append) (string, &tmp[i++]);
  g_assert_true (i == 2);

  g_assert_cmpstr (string->str, ==, "firsthalflasthalfmoreore");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null (g_string_append (NULL, NULL));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null ((g_string_append) (NULL, NULL));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true (g_string_append (string, NULL) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true ((g_string_append) (string, NULL) == string);
  g_test_assert_expected_messages ();

  g_string_free (string, TRUE);
  g_free (tmp);

  /* append_len */
  string = g_string_new ("firsthalf");
  g_string_append_len (string, "lasthalfjunkjunk", strlen ("last"));
  (g_string_append_len) (string, "halfjunkjunk", strlen ("half"));
  g_string_append_len (string, "more", -1);
  (g_string_append_len) (string, "ore", -1);

  g_assert_true (g_string_append_len (string, NULL, 0) == string);
  g_assert_true ((g_string_append_len) (string, NULL, 0) == string);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null (g_string_append_len (NULL, NULL, -1));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null ((g_string_append_len) (NULL, NULL, -1));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true (g_string_append_len (string, NULL, -1) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true ((g_string_append_len) (string, NULL, -1) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true (g_string_append_len (string, NULL, 1) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true ((g_string_append_len) (string, NULL, 1) == string);
  g_test_assert_expected_messages ();

  g_assert_cmpstr (string->str, ==, "firsthalflasthalfmoreore");
  g_string_free (string, TRUE);
}

static void string_append_vprintf_va (GString     *string,
                                      const gchar *format,
                                      ...) G_GNUC_PRINTF (2, 3);

/* Wrapper around g_string_append_vprintf() which takes varargs */
static void
string_append_vprintf_va (GString     *string,
                          const gchar *format,
                          ...)
{
  va_list args;

  va_start (args, format);
  g_string_append_vprintf (string, format, args);
  va_end (args);
}

static void
test_string_append_vprintf (void)
{
  GString *string;

  /* append */
  string = g_string_new ("firsthalf");

  string_append_vprintf_va (string, "some %s placeholders", "format");

  /* vasprintf() placeholder checks on BSDs are less strict, so skip these checks if so */
#if !defined(__APPLE__) && !defined(__FreeBSD__)
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "Failed to append to string*");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
  string_append_vprintf_va (string, "%l", "invalid");
#pragma GCC diagnostic pop
      g_test_assert_expected_messages ();
    }
#endif

  g_assert_cmpstr (string->str, ==, "firsthalfsome format placeholders");

  g_string_free (string, TRUE);
}

static void
test_string_prepend_c (void)
{
  GString *string;
  gint i;

  string = g_string_new ("hi pete!");

  for (i = 0; i < 10000; i++)
    g_string_prepend_c (string, 'a'+(i%26));

  g_assert((strlen("hi pete!") + 10000) == string->len);
  g_assert((strlen("hi pete!") + 10000) == strlen(string->str));

  g_string_free (string, TRUE);
}

static void
test_string_prepend (void)
{
  GString *string;

  /* prepend */
  string = g_string_new ("lasthalf");
  g_string_prepend (string, "firsthalf");
  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");
  g_string_free (string, TRUE);

  /* prepend_len */
  string = g_string_new ("lasthalf");
  g_string_prepend_len (string, "firsthalfjunkjunk", strlen ("firsthalf"));
  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");
  g_string_free (string, TRUE);
}

static void
test_string_insert (void)
{
  GString *string;

  /* insert */
  string = g_string_new ("firstlast");
  g_string_insert (string, 5, "middle");
  g_assert_cmpstr (string->str, ==, "firstmiddlelast");
  g_string_free (string, TRUE);

  /* insert with pos == end of the string */
  string = g_string_new ("firstmiddle");
  g_string_insert (string, strlen ("firstmiddle"), "last");
  g_assert_cmpstr (string->str, ==, "firstmiddlelast");
  g_string_free (string, TRUE);
  
  /* insert_len */
  string = g_string_new ("firstlast");
  g_string_insert_len (string, 5, "middlejunkjunk", strlen ("middle"));
  g_assert_cmpstr (string->str, ==, "firstmiddlelast");
  g_string_free (string, TRUE);

  /* insert_len with magic -1 pos for append */
  string = g_string_new ("first");
  g_string_insert_len (string, -1, "lastjunkjunk", strlen ("last"));
  g_assert_cmpstr (string->str, ==, "firstlast");
  g_string_free (string, TRUE);
  
  /* insert_len with magic -1 len for strlen-the-string */
  string = g_string_new ("first");
  g_string_insert_len (string, 5, "last", -1);
  g_assert_cmpstr (string->str, ==, "firstlast");
  g_string_free (string, TRUE);

  /* insert_len with string overlap */
  string = g_string_new ("textbeforetextafter");
  g_string_insert_len (string, 10, string->str + 8, 5);
  g_assert_cmpstr (string->str, ==, "textbeforeretextextafter");
  g_string_free (string, TRUE);
}

static void
test_string_insert_unichar (void)
{
  GString *string;

  /* insert_unichar with insertion in middle */
  string = g_string_new ("firsthalf");
  g_string_insert_unichar (string, 5, 0x0041);
  g_assert_cmpstr (string->str, ==, "first\x41half");
  g_string_free (string, TRUE);

  string = g_string_new ("firsthalf");
  g_string_insert_unichar (string, 5, 0x0298);
  g_assert_cmpstr (string->str, ==, "first\xCA\x98half");
  g_string_free (string, TRUE);

  string = g_string_new ("firsthalf");
  g_string_insert_unichar (string, 5, 0xFFFD);
  g_assert_cmpstr (string->str, ==, "first\xEF\xBF\xBDhalf");
  g_string_free (string, TRUE);

  string = g_string_new ("firsthalf");
  g_string_insert_unichar (string, 5, 0x1D100);
  g_assert_cmpstr (string->str, ==, "first\xF0\x9D\x84\x80half");
  g_string_free (string, TRUE);

  /* insert_unichar with insertion at end */
  string = g_string_new ("start");
  g_string_insert_unichar (string, -1, 0x0041);
  g_assert_cmpstr (string->str, ==, "start\x41");
  g_string_free (string, TRUE);

  string = g_string_new ("start");
  g_string_insert_unichar (string, -1, 0x0298);
  g_assert_cmpstr (string->str, ==, "start\xCA\x98");
  g_string_free (string, TRUE);

  string = g_string_new ("start");
  g_string_insert_unichar (string, -1, 0xFFFD);
  g_assert_cmpstr (string->str, ==, "start\xEF\xBF\xBD");
  g_string_free (string, TRUE);

  string = g_string_new ("start");
  g_string_insert_unichar (string, -1, 0x1D100);
  g_assert_cmpstr (string->str, ==, "start\xF0\x9D\x84\x80");
  g_string_free (string, TRUE);

  string = g_string_new ("start");
  g_string_insert_unichar (string, -1, 0xFFD0);
  g_assert_cmpstr (string->str, ==, "start\xEF\xBF\x90");
  g_string_free (string, TRUE);

  string = g_string_new ("start");
  g_string_insert_unichar (string, -1, 0xFDD0);
  g_assert_cmpstr (string->str, ==, "start\xEF\xB7\x90");
  g_string_free (string, TRUE);
}

static void
test_string_equal (void)
{
  GString *string1, *string2;

  string1 = g_string_new ("test");
  string2 = g_string_new ("te");
  g_assert_false (g_string_equal (string1, string2));
  g_string_append (string2, "st");
  g_assert_true (g_string_equal (string1, string2));
  g_string_free (string1, TRUE);
  g_string_free (string2, TRUE);
}

static void
test_string_truncate (void)
{
  GString *string;

  string = g_string_new ("testing");

  g_string_truncate (string, 1000);
  g_assert_cmpuint (string->len, ==, strlen("testing"));
  g_assert_cmpstr (string->str, ==, "testing");

  (g_string_truncate) (string, 4);
  g_assert_cmpuint (string->len, ==, 4);
  g_assert_cmpstr (string->str, ==, "test");

  g_string_truncate (string, 0);
  g_assert_cmpuint (string->len, ==, 0);
  g_assert_cmpstr (string->str, ==, "");

  g_string_free (string, TRUE);
}

static void
test_string_overwrite (void)
{
  GString *string;

  /* overwriting functions */
  string = g_string_new ("testing");

  g_string_overwrite (string, 4, " and expand");
  g_assert_cmpuint (15, ==, string->len);
  g_assert_true ('\0' == string->str[15]);
  g_assert_true (g_str_equal ("test and expand", string->str));

  g_string_overwrite (string, 5, "NOT-");
  g_assert_cmpuint (15, ==, string->len);
  g_assert_true ('\0' == string->str[15]);
  g_assert_true (g_str_equal ("test NOT-expand", string->str));

  g_string_overwrite_len (string, 9, "blablabla", 6);
  g_assert_cmpuint (15, ==, string->len);
  g_assert_true ('\0' == string->str[15]);
  g_assert_true (g_str_equal ("test NOT-blabla", string->str));

  g_string_overwrite_len (string, 4, "BLABL", 0);
  g_assert_true (g_str_equal ("test NOT-blabla", string->str));
  g_string_overwrite_len (string, 4, "BLABL", -1);
  g_assert_true (g_str_equal ("testBLABLblabla", string->str));

  g_string_free (string, TRUE);
}

static void
test_string_nul_handling (void)
{
  GString *string1, *string2;

  /* Check handling of embedded ASCII 0 (NUL) characters in GString. */
  string1 = g_string_new ("fiddle");
  string2 = g_string_new ("fiddle");
  g_assert_true (g_string_equal (string1, string2));
  g_string_append_c (string1, '\0');
  g_assert_false (g_string_equal (string1, string2));
  g_string_append_c (string2, '\0');
  g_assert_true (g_string_equal (string1, string2));
  g_string_append_c (string1, 'x');
  g_string_append_c (string2, 'y');
  g_assert_false (g_string_equal (string1, string2));
  g_assert_cmpuint (string1->len, ==, 8);
  g_string_append (string1, "yzzy");
  g_assert_cmpmem (string1->str, string1->len + 1, "fiddle\0xyzzy", 13);
  g_string_insert (string1, 1, "QED");
  g_assert_cmpmem (string1->str, string1->len + 1, "fQEDiddle\0xyzzy", 16);
  g_string_printf (string1, "fiddle%cxyzzy", '\0');
  g_assert_cmpmem (string1->str, string1->len + 1, "fiddle\0xyzzy", 13);

  g_string_free (string1, TRUE);
  g_string_free (string2, TRUE);
}

static void
test_string_up_down (void)
{
  GString *s;

  s = g_string_new ("Mixed Case String !?");
  g_string_ascii_down (s);
  g_assert_cmpstr (s->str, ==, "mixed case string !?");

  g_string_assign (s, "Mixed Case String !?");
  g_string_down (s);
  g_assert_cmpstr (s->str, ==, "mixed case string !?");

  g_string_assign (s, "Mixed Case String !?");
  g_string_ascii_up (s);
  g_assert_cmpstr (s->str, ==, "MIXED CASE STRING !?");

  g_string_assign (s, "Mixed Case String !?");
  g_string_up (s);
  g_assert_cmpstr (s->str, ==, "MIXED CASE STRING !?");

  g_string_free (s, TRUE);
}

static void
test_string_set_size (void)
{
  GString *s;

  s = g_string_new ("foo");
  g_string_set_size (s, 30);

  g_assert_cmpstr (s->str, ==, "foo");
  g_assert_cmpuint (s->len, ==, 30);

  g_string_free (s, TRUE);
}

static void
test_string_to_bytes (void)
{
  GString *s;
  GBytes *bytes;
  gconstpointer byte_data;
  gsize byte_len;

  s = g_string_new ("foo");
  g_string_append (s, "-bar");

  bytes = g_string_free_to_bytes (s);

  byte_data = g_bytes_get_data (bytes, &byte_len);

  g_assert_cmpuint (byte_len, ==, 7);

  g_assert_cmpmem (byte_data, byte_len, "foo-bar", 7);

  g_bytes_unref (bytes);
}

static void
test_string_replace (void)
{
  static const struct
  {
    const char *string;
    const char *original;
    const char *replacement;
    guint limit;
    const char *expected;
    guint expected_n;
  }
  tests[] =
  {
    { "foo bar foo baz foo bar foobarbaz", "bar", "baz", 0,
      "foo baz foo baz foo baz foobazbaz", 3 },
    { "foo baz foo baz foo baz foobazbaz", "baz", "bar", 3,
      "foo bar foo bar foo bar foobazbaz", 3 },
    { "foo bar foo bar foo bar foobazbaz", "foobar", "bar", 1,
      "foo bar foo bar foo bar foobazbaz", 0 },
    { "aaaaaaaa", "a", "abcdefghijkl", 0,
      "abcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijkl",
      8 },
    { "/usr/$LIB/libMangoHud.so", "$LIB", "lib32", 0,
      "/usr/lib32/libMangoHud.so", 1 },
    { "food for foals", "o", "", 0,
      "fd fr fals", 4 },
    { "aaa", "a", "aaa", 0,
      "aaaaaaaaa", 3 },
    { "aaa", "a", "", 0,
      "", 3 },
    { "aaa", "aa", "bb", 0,
      "bba", 1 },
    { "foo", "", "bar", 0,
      "barfbarobarobar", 4 },
    { "foo", "", "bar", 1,
      "barfoo", 1 },
    { "foo", "", "bar", 2,
      "barfbaroo", 2 },
    { "foo", "", "bar", 3,
      "barfbarobaro", 3 },
    { "foo", "", "bar", 4,
      "barfbarobarobar", 4 },
    { "foo", "", "bar", 5,
      "barfbarobarobar", 4 },
    { "", "", "x", 0,
      "x", 1 },
    { "", "", "", 0,
      "", 1 },
    /* use find and replace strings long enough to trigger a reallocation in
     * the result string */
    { "bbbbbbbbb", "", "aaaaaaaaaaaa", 0,
      "aaaaaaaaaaaabaaaaaaaaaaaabaaaaaaaaaaaabaaaaaaaaaaaabaaaaaaaaaaaab"
      "aaaaaaaaaaaabaaaaaaaaaaaabaaaaaaaaaaaabaaaaaaaaaaaabaaaaaaaaaaaa", 10 },
  };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      GString *s;
      guint n;

      s = g_string_new (tests[i].string);
      g_test_message ("%" G_GSIZE_FORMAT ": Replacing \"%s\" with \"%s\" (limit %u) in \"%s\"",
                      i, tests[i].original, tests[i].replacement,
                      tests[i].limit, tests[i].string);
      n = g_string_replace (s, tests[i].original, tests[i].replacement,
                            tests[i].limit);
      g_test_message ("-> %u replacements, \"%s\"",
                      n, s->str);
      g_assert_cmpstr (tests[i].expected, ==, s->str);
      g_assert_cmpuint (strlen (tests[i].expected), ==, s->len);
      g_assert_cmpuint (strlen (tests[i].expected) + 1, <=, s->allocated_len);
      g_assert_cmpuint (tests[i].expected_n, ==, n);
      g_string_free (s, TRUE);
    }
}

static void
test_string_steal (void)
{
  GString *string;
  char *str;

  string = g_string_new ("One");
  g_string_append (string, ", two");
  g_string_append (string, ", three");
  g_string_append_c (string, '.');

  str = g_string_free (string, FALSE);

  g_assert_cmpstr (str, ==, "One, two, three.");
  g_free (str);

  string = g_string_new ("1");
  g_string_append (string, " 2");
  g_string_append (string, " 3");

  str = g_string_free_and_steal (string);

  g_assert_cmpstr (str, ==, "1 2 3");
  g_free (str);
}

static void
test_string_new_take (void)
{
  const char *test_str_const = "test_test";
  const char *replaced_str_const = "test__test";
  char *test_str = malloc (10 * sizeof (*test_str_const));
  GString *string;

  strcpy (test_str, test_str_const);
  g_assert_cmpstr (test_str, ==, test_str_const);

  string = g_string_new_take (g_steal_pointer (&test_str));
  g_assert_null (test_str);
  g_assert_nonnull (string);

  g_string_replace (string, "_", "__", 0);
  g_assert_cmpstr (string->str, ==, replaced_str_const);

  test_str = g_string_free_and_steal (g_steal_pointer (&string));
  g_assert_cmpstr (test_str, ==, replaced_str_const);

  g_free (test_str);
}

static void
test_string_new_take_null (void)
{
  GString *string = g_string_new_take (NULL);

  g_assert_cmpstr (string->str, ==, "");

  g_string_free (g_steal_pointer (&string), TRUE);
}

static void
test_string_copy (void)
{
  GString *string1 = NULL, *string2 = NULL;

  string1 = g_string_new ("hello");
  string2 = g_string_copy (string1);
  g_assert_cmpstr (string1->str, ==, string2->str);
  g_assert_true (string1->str != string2->str);
  g_assert_cmpuint (string1->len, ==, string2->len);
  g_assert_cmpuint (string2->allocated_len, ==, string2->allocated_len);
  g_string_free (string1, TRUE);
  g_string_free (string2, TRUE);

  string1 = g_string_sized_new (100);
  string2 = g_string_copy (string1);
  g_assert_cmpstr (string1->str, ==, string2->str);
  g_assert_true (string1->str != string2->str);
  g_assert_cmpuint (string1->len, ==, string2->len);
  g_assert_cmpuint (string2->allocated_len, ==, string2->allocated_len);
  g_string_free (string1, TRUE);
  g_string_free (string2, TRUE);

  string1 = g_string_sized_new (200);
  g_string_append_len (string1, "test with embedded\0nuls", sizeof ("test with embedded\0nuls"));
  string2 = g_string_copy (string1);
  g_assert_cmpmem (string1->str, string1->len, string2->str, string2->len);
  g_assert_true (string1->str != string2->str);
  g_assert_cmpuint (string1->len, ==, string2->len);
  g_assert_cmpuint (string2->allocated_len, ==, string2->allocated_len);
  g_string_free (string1, TRUE);
  g_string_free (string2, TRUE);
}

static void
test_string_sized_new (void)
{

  if (g_test_subprocess ())
    {
      GString *string = g_string_sized_new (G_MAXSIZE);
      g_string_free (string, TRUE);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*string would overflow*");
    }
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/string/chunks", test_string_chunks);
  g_test_add_func ("/string/chunk-insert", test_string_chunk_insert);
  g_test_add_func ("/string/new", test_string_new);
  g_test_add_func ("/string/printf", test_string_printf);
  g_test_add_func ("/string/assign", test_string_assign);
  g_test_add_func ("/string/append-c", test_string_append_c);
  g_test_add_func ("/string/append", test_string_append);
  g_test_add_func ("/string/append-vprintf", test_string_append_vprintf);
  g_test_add_func ("/string/prepend-c", test_string_prepend_c);
  g_test_add_func ("/string/prepend", test_string_prepend);
  g_test_add_func ("/string/insert", test_string_insert);
  g_test_add_func ("/string/insert-unichar", test_string_insert_unichar);
  g_test_add_func ("/string/equal", test_string_equal);
  g_test_add_func ("/string/truncate", test_string_truncate);
  g_test_add_func ("/string/overwrite", test_string_overwrite);
  g_test_add_func ("/string/nul-handling", test_string_nul_handling);
  g_test_add_func ("/string/up-down", test_string_up_down);
  g_test_add_func ("/string/set-size", test_string_set_size);
  g_test_add_func ("/string/to-bytes", test_string_to_bytes);
  g_test_add_func ("/string/replace", test_string_replace);
  g_test_add_func ("/string/steal", test_string_steal);
  g_test_add_func ("/string/new-take", test_string_new_take);
  g_test_add_func ("/string/new-take/null", test_string_new_take_null);
  g_test_add_func ("/string/copy", test_string_copy);
  g_test_add_func ("/string/sized-new", test_string_sized_new);

  return g_test_run();
}
