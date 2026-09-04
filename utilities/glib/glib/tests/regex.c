/*
 * Copyright (C) 2005 - 2006, Marco Barisione <marco@barisione.org>
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2022, Marco Trevisan <marco.trevisan@canonical.com>
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
 */

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include "config.h"

#include <string.h>
#include <locale.h>
#include "glib.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

/* U+20AC EURO SIGN (symbol, currency) */
#define EURO "\xe2\x82\xac"
/* U+00E0 LATIN SMALL LETTER A WITH GRAVE (letter, lowercase) */
#define AGRAVE "\xc3\xa0"
/* U+00C0 LATIN CAPITAL LETTER A WITH GRAVE (letter, uppercase) */
#define AGRAVE_UPPER "\xc3\x80"
/* U+00E8 LATIN SMALL LETTER E WITH GRAVE (letter, lowercase) */
#define EGRAVE "\xc3\xa8"
/* U+00F2 LATIN SMALL LETTER O WITH GRAVE (letter, lowercase) */
#define OGRAVE "\xc3\xb2"
/* U+014B LATIN SMALL LETTER ENG (letter, lowercase) */
#define ENG "\xc5\x8b"
/* U+0127 LATIN SMALL LETTER H WITH STROKE (letter, lowercase) */
#define HSTROKE "\xc4\xa7"
/* U+0634 ARABIC LETTER SHEEN (letter, other) */
#define SHEEN "\xd8\xb4"
/* U+1374 ETHIOPIC NUMBER THIRTY (number, other) */
#define ETH30 "\xe1\x8d\xb4"

/* A random value use to mark untouched integer variables. */
#define UNTOUCHED -559038737

/* Lengths of test strings in JIT stack tests */
#define TEST_STRING_LEN 20000
#define LARGE_TEST_STRING_LEN 200000

static gint total;

typedef struct {
  const gchar *pattern;
  GRegexCompileFlags compile_opts;
  GRegexMatchFlags   match_opts;
  gint expected_error;
  gboolean check_flags;
  GRegexCompileFlags real_compile_opts;
  GRegexMatchFlags real_match_opts;
} TestNewData;

static void
test_new (gconstpointer d)
{
  const TestNewData *data = d;
  GRegex *regex;
  GError *error = NULL;

  regex = g_regex_new (data->pattern, data->compile_opts, data->match_opts, &error);
  g_assert (regex != NULL);
  g_assert_no_error (error);
  g_assert_cmpstr (data->pattern, ==, g_regex_get_pattern (regex));

  if (data->check_flags)
    {
      g_assert_cmphex (g_regex_get_compile_flags (regex), ==, data->real_compile_opts);
      g_assert_cmphex (g_regex_get_match_flags (regex), ==, data->real_match_opts);
    }

  g_regex_unref (regex);
}

#define TEST_NEW(_pattern, _compile_opts, _match_opts) {    \
  TestNewData *data;                                        \
  gchar *path;                                              \
  data = g_new0 (TestNewData, 1);                           \
  data->pattern = _pattern;                                 \
  data->compile_opts = _compile_opts;                       \
  data->match_opts = _match_opts;                           \
  data->expected_error = 0;                                 \
  data->check_flags = FALSE;                                \
  path = g_strdup_printf ("/regex/new/%d", ++total);        \
  g_test_add_data_func_full (path, data, test_new, g_free); \
  g_free (path);                                            \
}

#define TEST_NEW_CHECK_FLAGS(_pattern, _compile_opts, _match_opts, _real_compile_opts, _real_match_opts) { \
  TestNewData *data;                                             \
  gchar *path;                                                   \
  data = g_new0 (TestNewData, 1);                                \
  data->pattern = _pattern;                                      \
  data->compile_opts = _compile_opts;                            \
  data->match_opts = _match_opts;                                \
  data->expected_error = 0;                                      \
  data->check_flags = TRUE;                                      \
  data->real_compile_opts = _real_compile_opts;                  \
  data->real_match_opts = _real_match_opts;                      \
  path = g_strdup_printf ("/regex/new-check-flags/%d", ++total); \
  g_test_add_data_func_full (path, data, test_new, g_free);      \
  g_free (path);                                                 \
}

static void
test_new_fail (gconstpointer d)
{
  const TestNewData *data = d;
  GRegex *regex;
  GError *error = NULL;

  regex = g_regex_new (data->pattern, data->compile_opts, data->match_opts, &error);

  g_assert (regex == NULL);
  g_assert_error (error, G_REGEX_ERROR, data->expected_error);
  g_test_message ("Compiling pattern /%s/ failed with error: %s",
                  data->pattern, error->message);
  g_error_free (error);
}

#define TEST_NEW_FAIL(_pattern, _compile_opts, _expected_error) { \
  TestNewData *data;                                              \
  gchar *path;                                                    \
  data = g_new0 (TestNewData, 1);                                 \
  data->pattern = _pattern;                                       \
  data->compile_opts = _compile_opts;                             \
  data->match_opts = 0;                                           \
  data->expected_error = _expected_error;                         \
  path = g_strdup_printf ("/regex/new-fail/%d", ++total);         \
  g_test_add_data_func_full (path, data, test_new_fail, g_free);  \
  g_free (path);                                                  \
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  GRegexCompileFlags compile_opts;
  GRegexMatchFlags match_opts;
  gboolean expected;
  gssize string_len;
  gint start_position;
  GRegexMatchFlags match_opts2;
} TestMatchData;

static void
test_match_simple (gconstpointer d)
{
  const TestMatchData *data = d;
  gboolean match;

  match = g_regex_match_simple (data->pattern, data->string, data->compile_opts, data->match_opts);
  g_assert_cmpint (match, ==, data->expected);
}

#define TEST_MATCH_SIMPLE_NAMED(_name, _pattern, _string, _compile_opts, _match_opts, _expected) { \
  TestMatchData *data;                                                  \
  gchar *path;                                                          \
  data = g_new0 (TestMatchData, 1);                                     \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->compile_opts = _compile_opts;                                   \
  data->match_opts = _match_opts;                                       \
  data->expected = _expected;                                           \
  total++;                                                              \
  if (data->compile_opts & G_REGEX_OPTIMIZE)                            \
    path = g_strdup_printf ("/regex/match-%s-optimized/%d", _name, total); \
  else                                                                  \
    path = g_strdup_printf ("/regex/match-%s/%d", _name, total);        \
  g_test_add_data_func_full (path, data, test_match_simple, g_free);    \
  g_free (path);                                                        \
  data = g_memdup2 (data, sizeof (TestMatchData));                      \
  if (data->compile_opts & G_REGEX_OPTIMIZE)                            \
    {                                                                   \
      data->compile_opts &= ~G_REGEX_OPTIMIZE;                          \
      path = g_strdup_printf ("/regex/match-%s/%d", _name, total);      \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      data->compile_opts |= G_REGEX_OPTIMIZE;                           \
      path = g_strdup_printf ("/regex/match-%s-optimized/%d", _name, total); \
    }                                                                   \
  g_test_add_data_func_full (path, data, test_match_simple, g_free);    \
  g_free (path);                                                        \
}

#define TEST_MATCH_SIMPLE(_pattern, _string, _compile_opts, _match_opts, _expected) \
  TEST_MATCH_SIMPLE_NAMED("simple", _pattern, _string, _compile_opts, _match_opts, _expected)
#define TEST_MATCH_NOTEMPTY(_pattern, _string, _expected) \
  TEST_MATCH_SIMPLE_NAMED("notempty", _pattern, _string, 0, G_REGEX_MATCH_NOTEMPTY, _expected)
#define TEST_MATCH_NOTEMPTY_ATSTART(_pattern, _string, _expected) \
  TEST_MATCH_SIMPLE_NAMED("notempty-atstart", _pattern, _string, 0, G_REGEX_MATCH_NOTEMPTY_ATSTART, _expected)

static char *
compile_options_to_string (GRegexCompileFlags compile_flags)
{
  GStrvBuilder *builder = g_strv_builder_new();
  GStrv strv;
  char *ret;

  if (compile_flags & G_REGEX_DEFAULT)
    g_strv_builder_add (builder, "default");
  if (compile_flags & G_REGEX_CASELESS)
    g_strv_builder_add (builder, "caseless");
  if (compile_flags & G_REGEX_MULTILINE)
    g_strv_builder_add (builder, "multiline");
  if (compile_flags & G_REGEX_DOTALL)
    g_strv_builder_add (builder, "dotall");
  if (compile_flags & G_REGEX_EXTENDED)
    g_strv_builder_add (builder, "extended");
  if (compile_flags & G_REGEX_ANCHORED)
    g_strv_builder_add (builder, "anchored");
  if (compile_flags & G_REGEX_DOLLAR_ENDONLY)
    g_strv_builder_add (builder, "dollar-endonly");
  if (compile_flags & G_REGEX_UNGREEDY)
    g_strv_builder_add (builder, "ungreedy");
  if (compile_flags & G_REGEX_RAW)
    g_strv_builder_add (builder, "raw");
  if (compile_flags & G_REGEX_NO_AUTO_CAPTURE)
    g_strv_builder_add (builder, "no-auto-capture");
  if (compile_flags & G_REGEX_OPTIMIZE)
    g_strv_builder_add (builder, "optimize");
  if (compile_flags & G_REGEX_FIRSTLINE)
    g_strv_builder_add (builder, "firstline");
  if (compile_flags & G_REGEX_DUPNAMES)
    g_strv_builder_add (builder, "dupnames");
  if (compile_flags & G_REGEX_NEWLINE_CR)
    g_strv_builder_add (builder, "newline-cr");
  if (compile_flags & G_REGEX_NEWLINE_LF)
    g_strv_builder_add (builder, "newline-lf");
  if (compile_flags & G_REGEX_NEWLINE_CRLF)
    g_strv_builder_add (builder, "newline-crlf");
  if (compile_flags & G_REGEX_NEWLINE_ANYCRLF)
    g_strv_builder_add (builder, "newline-anycrlf");
  if (compile_flags & G_REGEX_BSR_ANYCRLF)
    g_strv_builder_add (builder, "bsr-anycrlf");

  strv = g_strv_builder_end (builder);
  ret = g_strjoinv ("|", strv);

  g_strfreev (strv);
  g_strv_builder_unref (builder);

  return ret;
}

static char *
match_options_to_string (GRegexMatchFlags match_flags)
{
  GStrvBuilder *builder = g_strv_builder_new();
  GStrv strv;
  char *ret;

  if (match_flags & G_REGEX_MATCH_DEFAULT)
    g_strv_builder_add (builder, "default");
  if (match_flags & G_REGEX_MATCH_ANCHORED)
    g_strv_builder_add (builder, "anchored");
  if (match_flags & G_REGEX_MATCH_NOTBOL)
    g_strv_builder_add (builder, "notbol");
  if (match_flags & G_REGEX_MATCH_NOTEOL)
    g_strv_builder_add (builder, "noteol");
  if (match_flags & G_REGEX_MATCH_NOTEMPTY)
    g_strv_builder_add (builder, "notempty");
  if (match_flags & G_REGEX_MATCH_PARTIAL)
    g_strv_builder_add (builder, "partial");
  if (match_flags & G_REGEX_MATCH_NEWLINE_CR)
    g_strv_builder_add (builder, "newline-cr");
  if (match_flags & G_REGEX_MATCH_NEWLINE_LF)
    g_strv_builder_add (builder, "newline-lf");
  if (match_flags & G_REGEX_MATCH_NEWLINE_CRLF)
    g_strv_builder_add (builder, "newline-crlf");
  if (match_flags & G_REGEX_MATCH_NEWLINE_ANY)
    g_strv_builder_add (builder, "newline-any");
  if (match_flags & G_REGEX_MATCH_NEWLINE_ANYCRLF)
    g_strv_builder_add (builder, "newline-anycrlf");
  if (match_flags & G_REGEX_MATCH_BSR_ANYCRLF)
    g_strv_builder_add (builder, "bsr-anycrlf");
  if (match_flags & G_REGEX_MATCH_BSR_ANY)
    g_strv_builder_add (builder, "bsr-any");
  if (match_flags & G_REGEX_MATCH_PARTIAL_SOFT)
    g_strv_builder_add (builder, "partial-soft");
  if (match_flags & G_REGEX_MATCH_PARTIAL_HARD)
    g_strv_builder_add (builder, "partial-hard");
  if (match_flags & G_REGEX_MATCH_NOTEMPTY_ATSTART)
    g_strv_builder_add (builder, "notempty-atstart");

  strv = g_strv_builder_end (builder);
  ret = g_strjoinv ("|", strv);

  g_strfreev (strv);
  g_strv_builder_unref (builder);

  return ret;
}

static void
test_match (gconstpointer d)
{
  const TestMatchData *data = d;
  GRegex *regex;
  gboolean match;
  GError *error = NULL;
  gchar *compile_opts_str;
  gchar *match_opts_str;
  gchar *match_opts2_str;

  regex = g_regex_new (data->pattern, data->compile_opts, data->match_opts, &error);
  g_assert (regex != NULL);
  g_assert_no_error (error);

  match = g_regex_match_full (regex, data->string, data->string_len,
                              data->start_position, data->match_opts2, NULL, NULL);

  compile_opts_str = compile_options_to_string (data->compile_opts);
  match_opts_str = match_options_to_string (data->match_opts);
  match_opts2_str = match_options_to_string (data->match_opts2);

  if (data->expected)
    {
      if (!match)
        g_error ("Regex '%s' (with compile options '%s' and "
            "match options '%s') should have matched '%.*s' "
            "(of length %d, at position %d, with match options '%s') but did not",
            data->pattern, compile_opts_str, match_opts_str,
            data->string_len == -1 ? (int) strlen (data->string) :
              (int) data->string_len,
            data->string, (int) data->string_len,
            data->start_position, match_opts2_str);

      g_assert_cmpint (match, ==, TRUE);
    }
  else
    {
      if (match)
        g_error ("Regex '%s' (with compile options '%s' and "
            "match options '%s') should not have matched '%.*s' "
            "(of length %d, at position %d, with match options '%s') but did",
            data->pattern, compile_opts_str, match_opts_str,
            data->string_len == -1 ? (int) strlen (data->string) :
              (int) data->string_len,
            data->string, (int) data->string_len,
            data->start_position, match_opts2_str);
    }

  if (data->string_len == -1 && data->start_position == 0)
    {
      match = g_regex_match (regex, data->string, data->match_opts2, NULL);
      g_assert_cmpint (match, ==, data->expected);
    }

  g_free (compile_opts_str);
  g_free (match_opts_str);
  g_free (match_opts2_str);
  g_regex_unref (regex);
}

#define TEST_MATCH(_pattern, _compile_opts, _match_opts, _string, \
                   _string_len, _start_position, _match_opts2, _expected) { \
  TestMatchData *data;                                                  \
  gchar *path;                                                          \
  data = g_new0 (TestMatchData, 1);                                     \
  data->pattern = _pattern;                                             \
  data->compile_opts = _compile_opts;                                   \
  data->match_opts = _match_opts;                                       \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  data->match_opts2 = _match_opts2;                                     \
  data->expected = _expected;                                           \
  total++;                                                              \
  if (data->compile_opts & G_REGEX_OPTIMIZE)                            \
    path = g_strdup_printf ("/regex/match-optimized/%d", total);        \
  else                                                                  \
    path = g_strdup_printf ("/regex/match/%d", total);                  \
  g_test_add_data_func_full (path, data, test_match, g_free);           \
  g_free (path);                                                        \
  data = g_memdup2 (data, sizeof (TestMatchData));                      \
  if (data->compile_opts & G_REGEX_OPTIMIZE)                            \
    {                                                                   \
      data->compile_opts &= ~G_REGEX_OPTIMIZE;                          \
      path = g_strdup_printf ("/regex/match/%d", total);                \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      data->compile_opts |= G_REGEX_OPTIMIZE;                           \
      path = g_strdup_printf ("/regex/match-optimized/%d", total);      \
    }                                                                   \
  g_test_add_data_func_full (path, data, test_match, g_free);           \
  g_free (path);                                                        \
}

struct _Match
{
  gchar *string;
  gint start, end;
};
typedef struct _Match Match;

static void
free_match (gpointer data)
{
  Match *match = data;
  if (match == NULL)
    return;
  g_free (match->string);
  g_free (match);
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  gssize string_len;
  gint start_position;
  GSList *expected;
} TestMatchNextData;

static void
test_match_next (gconstpointer d)
{
   const TestMatchNextData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  GSList *matches;
  GSList *l_exp, *l_match;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  g_regex_match_full (regex, data->string, data->string_len,
                      data->start_position, 0, &match_info, NULL);
  matches = NULL;
  while (g_match_info_matches (match_info))
    {
      Match *match = g_new0 (Match, 1);
      match->string = g_match_info_fetch (match_info, 0);
      match->start = UNTOUCHED;
      match->end = UNTOUCHED;
      g_match_info_fetch_pos (match_info, 0, &match->start, &match->end);
      matches = g_slist_prepend (matches, match);
      g_match_info_next (match_info, NULL);
    }
  g_assert (regex == g_match_info_get_regex (match_info));
  g_assert_cmpstr (data->string, ==, g_match_info_get_string (match_info));
  g_match_info_free (match_info);
  matches = g_slist_reverse (matches);

  g_assert_cmpint (g_slist_length (matches), ==, g_slist_length (data->expected));

  l_exp = data->expected;
  l_match = matches;
  while (l_exp != NULL)
    {
      Match *exp = l_exp->data;
      Match *match = l_match->data;

      g_assert_cmpstr (exp->string, ==, match->string);
      g_assert_cmpint (exp->start, ==, match->start);
      g_assert_cmpint (exp->end, ==, match->end);

      l_exp = g_slist_next (l_exp);
      l_match = g_slist_next (l_match);
    }

  g_regex_unref (regex);
  g_slist_free_full (matches, free_match);
}

static void
free_match_next_data (gpointer _data)
{
  TestMatchNextData *data = _data;

  g_slist_free_full (data->expected, g_free);
  g_free (data);
}

#define TEST_MATCH_NEXT0(_pattern, _string, _string_len, _start_position) { \
  TestMatchNextData *data;                                              \
  gchar *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  data->expected = NULL;                                                \
  path = g_strdup_printf ("/regex/match/next0/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT1(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1) {                                  \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  gchar *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = g_slist_append (NULL, match);                        \
  path = g_strdup_printf ("/regex/match/next1/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT2(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1, t2, s2, e2) {                      \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  gchar *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = g_slist_append (NULL, match);                        \
  match = g_new0 (Match, 1);                                            \
  match->string = t2;                                                   \
  match->start = s2;                                                    \
  match->end = e2;                                                      \
  data->expected = g_slist_append (data->expected, match);              \
  path = g_strdup_printf ("/regex/match/next2/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT3(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1, t2, s2, e2, t3, s3, e3) {          \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  gchar *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = g_slist_append (NULL, match);                        \
  match = g_new0 (Match, 1);                                            \
  match->string = t2;                                                   \
  match->start = s2;                                                    \
  match->end = e2;                                                      \
  data->expected = g_slist_append (data->expected, match);              \
  match = g_new0 (Match, 1);                                            \
  match->string = t3;                                                   \
  match->start = s3;                                                    \
  match->end = e3;                                                      \
  data->expected = g_slist_append (data->expected, match);              \
  path = g_strdup_printf ("/regex/match/next3/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT4(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1, t2, s2, e2, t3, s3, e3, t4, s4, e4) { \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  gchar *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = g_slist_append (NULL, match);                        \
  match = g_new0 (Match, 1);                                            \
  match->string = t2;                                                   \
  match->start = s2;                                                    \
  match->end = e2;                                                      \
  data->expected = g_slist_append (data->expected, match);              \
  match = g_new0 (Match, 1);                                            \
  match->string = t3;                                                   \
  match->start = s3;                                                    \
  match->end = e3;                                                      \
  data->expected = g_slist_append (data->expected, match);              \
  match = g_new0 (Match, 1);                                            \
  match->string = t4;                                                   \
  match->start = s4;                                                    \
  match->end = e4;                                                      \
  data->expected = g_slist_append (data->expected, match);              \
  path = g_strdup_printf ("/regex/match/next4/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  gint start_position;
  GRegexCompileFlags compile_flags;
  GRegexMatchFlags match_opts;
  gint expected_count;
} TestMatchCountData;

static void
test_match_count (gconstpointer d)
{
  const TestMatchCountData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  gint count;

  regex = g_regex_new (data->pattern, data->compile_flags,
                       G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  g_regex_match_full (regex, data->string, -1, data->start_position,
		      data->match_opts, &match_info, NULL);
  count = g_match_info_get_match_count (match_info);

  g_assert_cmpint (count, ==, data->expected_count);

  g_match_info_ref (match_info);
  g_match_info_unref (match_info);
  g_match_info_unref (match_info);
  g_regex_unref (regex);
}

#define TEST_MATCH_COUNT(_pattern, _string, _start_position, _match_opts, _expected_count) { \
  TestMatchCountData *data;                                             \
  gchar *path;                                                          \
  data = g_new0 (TestMatchCountData, 1);                                \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->match_opts = _match_opts;                                       \
  data->expected_count = _expected_count;                               \
  data->compile_flags = G_REGEX_DEFAULT;                                \
  total++;                                                              \
  path = g_strdup_printf ("/regex/match/count/%d", total);              \
  g_test_add_data_func_full (path, data, test_match_count, g_free);     \
  g_free (path);                                                        \
  data = g_memdup2 (data, sizeof (TestMatchCountData));                 \
  data->compile_flags |= G_REGEX_OPTIMIZE;                              \
  path = g_strdup_printf ("/regex/match/count-optimized/%d", total);    \
  g_test_add_data_func_full (path, data, test_match_count, g_free);     \
  g_free (path);                                                        \
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  GRegexCompileFlags compile_opts;
  GRegexMatchFlags match_opts;
  gssize string_len;
  gint start_position;
  const gchar *expected;
  gint         expected_start;
  gint         expected_end;
} TestPartialData;

static void
test_partial (gconstpointer d)
{
  const TestPartialData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;

  regex = g_regex_new (data->pattern, data->compile_opts, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  g_regex_match (regex, data->string, data->match_opts, &match_info);

  g_assert_cmpint ((data->expected != NULL), ==, (g_match_info_is_partial_match (match_info) != FALSE));

  if (data->expected)
    {
      gchar *expr;
      gint start, end;

      expr = g_match_info_fetch (match_info, 0);
      g_assert_cmpstr (expr, ==, data->expected);
      g_free (expr);

      g_assert (g_match_info_fetch_pos (match_info, 0, &start, &end));
      g_assert_cmpint (start, ==, data->expected_start);
      g_assert_cmpint (end, ==, data->expected_end);

      g_assert (!g_match_info_fetch_pos (match_info, 1, NULL, NULL));
    }

  g_match_info_free (match_info);
  g_regex_unref (regex);
}

#define TEST_PARTIAL_FULL(_pattern, _string, _compile_opts, _match_opts, \
                          _expected, _expected_start, _expected_end) { \
  TestPartialData *data;                                        \
  gchar *path;                                                  \
  data = g_new0 (TestPartialData, 1);                           \
  data->pattern = _pattern;                                     \
  data->string = _string;                                       \
  data->compile_opts = _compile_opts;                           \
  data->match_opts = _match_opts;                               \
  data->expected = _expected;                                   \
  data->expected_start = _expected_start;                       \
  data->expected_end = _expected_end;                           \
  total++;                                                      \
  if (data->compile_opts & G_REGEX_OPTIMIZE)                    \
    path = g_strdup_printf ("/regex/match/partial-optimized/%d", total); \
  else                                                          \
    path = g_strdup_printf ("/regex/match/partial%d", total);   \
  g_test_add_data_func_full (path, data, test_partial, g_free); \
  g_free (path);                                                \
  data = g_memdup2 (data, sizeof (TestPartialData));            \
  if (data->compile_opts & G_REGEX_OPTIMIZE)                    \
    {                                                           \
      data->compile_opts &= ~G_REGEX_OPTIMIZE;                  \
      path = g_strdup_printf ("/regex/match/partial%d", total); \
    }                                                           \
  else                                                          \
    {                                                           \
      data->compile_opts |= G_REGEX_OPTIMIZE;                   \
      path = g_strdup_printf ("/regex/match/partial-optimized/%d", total); \
    }                                                           \
  g_test_add_data_func_full (path, data, test_partial, g_free); \
  g_free (path);                                                \
}

#define TEST_PARTIAL(_pattern, _string, _compile_opts, \
                     _expected, _expected_start, _expected_end) \
        TEST_PARTIAL_FULL(_pattern, _string, _compile_opts, G_REGEX_MATCH_PARTIAL, _expected, _expected_start, _expected_end)

typedef struct {
  const gchar *pattern;
  const gchar *string;
  GRegexCompileFlags compile_flags;
  gint         start_position;
  gint         sub_n;
  const gchar *expected_sub;
  gint         expected_start;
  gint         expected_end;
} TestSubData;

static void
test_sub_pattern (gconstpointer d)
{
  const TestSubData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  gchar *sub_expr;
  gint start = UNTOUCHED, end = UNTOUCHED;

  regex = g_regex_new (data->pattern, data->compile_flags, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  g_regex_match_full (regex, data->string, -1, data->start_position, 0, &match_info, NULL);

  sub_expr = g_match_info_fetch (match_info, data->sub_n);
  g_assert_cmpstr (sub_expr, ==, data->expected_sub);
  g_free (sub_expr);

  g_match_info_fetch_pos (match_info, data->sub_n, &start, &end);
  g_assert_cmpint (start, ==, data->expected_start);
  g_assert_cmpint (end, ==, data->expected_end);

  g_match_info_free (match_info);
  g_regex_unref (regex);
}

#define TEST_SUB_PATTERN(_pattern, _string, _start_position, _sub_n, _expected_sub, \
			 _expected_start, _expected_end) {                       \
  TestSubData *data;                                                    \
  gchar *path;                                                          \
  data = g_new0 (TestSubData, 1);                                       \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->sub_n = _sub_n;                                                 \
  data->expected_sub = _expected_sub;                                   \
  data->expected_start = _expected_start;                               \
  data->expected_end = _expected_end;                                   \
  data->compile_flags = G_REGEX_DEFAULT;                                \
  total++;                                                              \
  path = g_strdup_printf ("/regex/match/subpattern/%d", total);         \
  g_test_add_data_func_full (path, data, test_sub_pattern, g_free);     \
  g_free (path);                                                        \
  data = g_memdup2 (data, sizeof (TestSubData));                        \
  data->compile_flags = G_REGEX_OPTIMIZE;                               \
  path = g_strdup_printf ("/regex/match/subpattern-optimized/%d", total); \
  g_test_add_data_func_full (path, data, test_sub_pattern, g_free);     \
  g_free (path);                                                        \
}

typedef struct {
  const gchar *pattern;
  GRegexCompileFlags flags;
  const gchar *string;
  gint         start_position;
  const gchar *sub_name;
  const gchar *expected_sub;
  gint         expected_start;
  gint         expected_end;
} TestNamedSubData;

static void
test_named_sub_pattern (gconstpointer d)
{
  const TestNamedSubData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  gint start = UNTOUCHED, end = UNTOUCHED;
  gchar *sub_expr;

  regex = g_regex_new (data->pattern, data->flags, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  g_regex_match_full (regex, data->string, -1, data->start_position, 0, &match_info, NULL);
  sub_expr = g_match_info_fetch_named (match_info, data->sub_name);
  g_assert_cmpstr (sub_expr, ==, data->expected_sub);
  g_free (sub_expr);

  g_match_info_fetch_named_pos (match_info, data->sub_name, &start, &end);
  g_assert_cmpint (start, ==, data->expected_start);
  g_assert_cmpint (end, ==, data->expected_end);

  g_match_info_free (match_info);
  g_regex_unref (regex);
}

#define TEST_NAMED_SUB_PATTERN(_pattern, _string, _start_position, _sub_name, \
			       _expected_sub, _expected_start, _expected_end) { \
  TestNamedSubData *data;                                                 \
  gchar *path;                                                            \
  data = g_new0 (TestNamedSubData, 1);                                    \
  data->pattern = _pattern;                                               \
  data->string = _string;                                                 \
  data->flags = 0;                                                        \
  data->start_position = _start_position;                                 \
  data->sub_name = _sub_name;                                             \
  data->expected_sub = _expected_sub;                                     \
  data->expected_start = _expected_start;                                 \
  data->expected_end = _expected_end;                                     \
  path = g_strdup_printf ("/regex/match/named/subpattern/%d", ++total);   \
  g_test_add_data_func_full (path, data, test_named_sub_pattern, g_free); \
  g_free (path);                                                          \
}

#define TEST_NAMED_SUB_PATTERN_DUPNAMES(_pattern, _string, _start_position, _sub_name, \
                                        _expected_sub, _expected_start, _expected_end) { \
  TestNamedSubData *data;                                                        \
  gchar *path;                                                                   \
  data = g_new0 (TestNamedSubData, 1);                                           \
  data->pattern = _pattern;                                                      \
  data->string = _string;                                                        \
  data->flags = G_REGEX_DUPNAMES;                                                \
  data->start_position = _start_position;                                        \
  data->sub_name = _sub_name;                                                    \
  data->expected_sub = _expected_sub;                                            \
  data->expected_start = _expected_start;                                        \
  data->expected_end = _expected_end;                                            \
  path = g_strdup_printf ("/regex/match/subpattern/named/dupnames/%d", ++total); \
  g_test_add_data_func_full (path, data, test_named_sub_pattern, g_free);        \
  g_free (path);                                                                 \
}

typedef struct {
  GRegexMatchFlags match_opts;
  const gchar *pattern;
  const gchar *string;
  GSList *expected;
  gint start_position;
  gint max_tokens;
} TestFetchAllData;

static void
test_fetch_all (gconstpointer d)
{
  const TestFetchAllData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  GSList *l_exp;
  gchar **matches;
  gint match_count;
  gint i;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, data->match_opts, NULL);

  g_assert (regex != NULL);

  g_regex_match (regex, data->string, 0, &match_info);
  matches = g_match_info_fetch_all (match_info);
  if (matches)
    match_count = g_strv_length (matches);
  else
    match_count = 0;

  g_assert_cmpint (match_count, ==, g_slist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = g_slist_next (l_exp))
    {
      g_assert_nonnull (matches);
      g_assert_cmpstr (l_exp->data, ==, matches[i]);
    }

  g_match_info_free (match_info);
  g_regex_unref (regex);
  g_strfreev (matches);
}

static void
free_fetch_all_data (gpointer _data)
{
  TestFetchAllData *data = _data;

  g_slist_free (data->expected);
  g_free (data);
}

#define TEST_FETCH_ALL0(_pattern, _string, _match_opts) {                      \
  TestFetchAllData *data;                                                      \
  gchar *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->match_opts = _match_opts;                                              \
  data->expected = NULL;                                                       \
  path = g_strdup_printf ("/regex/fetch-all0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

#define TEST_FETCH_ALL1(_pattern, _string, _match_opts, e1) {                  \
  TestFetchAllData *data;                                                      \
  gchar *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->match_opts = _match_opts;                                              \
  data->expected = g_slist_append (NULL, e1);                                  \
  path = g_strdup_printf ("/regex/fetch-all1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

#define TEST_FETCH_ALL2(_pattern, _string, _match_opts, e1, e2) {              \
  TestFetchAllData *data;                                                      \
  gchar *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->match_opts = _match_opts;                                              \
  data->expected = g_slist_append (NULL, e1);                                  \
  data->expected = g_slist_append (data->expected, e2);                        \
  path = g_strdup_printf ("/regex/fetch-all2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

#define TEST_FETCH_ALL3(_pattern, _string, _match_opts, e1, e2, e3) {          \
  TestFetchAllData *data;                                                      \
  gchar *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->match_opts = _match_opts;                                              \
  data->expected = g_slist_append (NULL, e1);                                  \
  data->expected = g_slist_append (data->expected, e2);                        \
  data->expected = g_slist_append (data->expected, e3);                        \
  path = g_strdup_printf ("/regex/fetch-all3/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

static void
test_split_simple (gconstpointer d)
{
  const TestFetchAllData *data = d;
  GSList *l_exp;
  gchar **tokens;
  gint token_count;
  gint i;

  tokens = g_regex_split_simple (data->pattern, data->string,
                                 G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  if (tokens)
    token_count = g_strv_length (tokens);
  else
    token_count = 0;

  g_assert_cmpint (token_count, ==, g_slist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = g_slist_next (l_exp))
    {
      g_assert_nonnull (tokens);
      g_assert_cmpstr (l_exp->data, ==, tokens[i]);
    }

  g_strfreev (tokens);
}

#define TEST_SPLIT_SIMPLE0(_pattern, _string) {                                   \
  TestFetchAllData *data;                                                         \
  gchar *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = NULL;                                                          \
  path = g_strdup_printf ("/regex/split/simple0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

#define TEST_SPLIT_SIMPLE1(_pattern, _string, e1) {                               \
  TestFetchAllData *data;                                                         \
  gchar *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = g_slist_append (NULL, e1);                                     \
  path = g_strdup_printf ("/regex/split/simple1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

#define TEST_SPLIT_SIMPLE2(_pattern, _string, e1, e2) {                           \
  TestFetchAllData *data;                                                         \
  gchar *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = g_slist_append (NULL, e1);                                     \
  data->expected = g_slist_append (data->expected, e2);                           \
  path = g_strdup_printf ("/regex/split/simple2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

#define TEST_SPLIT_SIMPLE3(_pattern, _string, e1, e2, e3) {                       \
  TestFetchAllData *data;                                                         \
  gchar *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = g_slist_append (NULL, e1);                                     \
  data->expected = g_slist_append (data->expected, e2);                           \
  data->expected = g_slist_append (data->expected, e3);                           \
  path = g_strdup_printf ("/regex/split/simple3/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

static void
test_split_full (gconstpointer d)
{
  const TestFetchAllData *data = d;
  GRegex *regex;
  GSList *l_exp;
  gchar **tokens;
  gint token_count;
  gint i;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  tokens = g_regex_split_full (regex, data->string, -1, data->start_position,
			       0, data->max_tokens, NULL);
  if (tokens)
    token_count = g_strv_length (tokens);
  else
    token_count = 0;

  g_assert_cmpint (token_count, ==, g_slist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = g_slist_next (l_exp))
    {
      g_assert_nonnull (tokens);
      g_assert_cmpstr (l_exp->data, ==, tokens[i]);
    }

  g_regex_unref (regex);
  g_strfreev (tokens);
}

static void
test_split (gconstpointer d)
{
  const TestFetchAllData *data = d;
  GRegex *regex;
  GSList *l_exp;
  gchar **tokens;
  gint token_count;
  gint i;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert (regex != NULL);

  tokens = g_regex_split (regex, data->string, 0);
  if (tokens)
    token_count = g_strv_length (tokens);
  else
    token_count = 0;

  g_assert_cmpint (token_count, ==, g_slist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = g_slist_next (l_exp))
    {
      g_assert_nonnull (tokens);
      g_assert_cmpstr (l_exp->data, ==, tokens[i]);
    }

  g_regex_unref (regex);
  g_strfreev (tokens);
}

#define TEST_SPLIT0(_pattern, _string, _start_position, _max_tokens) {          \
  TestFetchAllData *data;                                                       \
  gchar *path;                                                                  \
  data = g_new0 (TestFetchAllData, 1);                                          \
  data->pattern = _pattern;                                                     \
  data->string = _string;                                                       \
  data->start_position = _start_position;                                       \
  data->max_tokens = _max_tokens;                                               \
  data->expected = NULL;                                                        \
  if (_start_position == 0 && _max_tokens <= 0) {                               \
    path = g_strdup_printf ("/regex/split0/%d", ++total);                       \
    g_test_add_data_func (path, data, test_split);                              \
    g_free (path);                                                              \
  }                                                                             \
  path = g_strdup_printf ("/regex/full-split0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data); \
  g_free (path);                                                                \
}

#define TEST_SPLIT1(_pattern, _string, _start_position, _max_tokens, e1) {      \
  TestFetchAllData *data;                                                       \
  gchar *path;                                                                  \
  data = g_new0 (TestFetchAllData, 1);                                          \
  data->pattern = _pattern;                                                     \
  data->string = _string;                                                       \
  data->start_position = _start_position;                                       \
  data->max_tokens = _max_tokens;                                               \
  data->expected = NULL;                                                        \
  data->expected = g_slist_append (data->expected, e1);                         \
  if (_start_position == 0 && _max_tokens <= 0) {                               \
    path = g_strdup_printf ("/regex/split1/%d", ++total);                       \
    g_test_add_data_func (path, data, test_split);                              \
    g_free (path);                                                              \
  }                                                                             \
  path = g_strdup_printf ("/regex/full-split1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data); \
  g_free (path);                                                                \
}

#define TEST_SPLIT2(_pattern, _string, _start_position, _max_tokens, e1, e2) {  \
  TestFetchAllData *data;                                                       \
  gchar *path;                                                                  \
  data = g_new0 (TestFetchAllData, 1);                                          \
  data->pattern = _pattern;                                                     \
  data->string = _string;                                                       \
  data->start_position = _start_position;                                       \
  data->max_tokens = _max_tokens;                                               \
  data->expected = NULL;                                                        \
  data->expected = g_slist_append (data->expected, e1);                         \
  data->expected = g_slist_append (data->expected, e2);                         \
  if (_start_position == 0 && _max_tokens <= 0) {                               \
    path = g_strdup_printf ("/regex/split2/%d", ++total);                       \
    g_test_add_data_func (path, data, test_split);                              \
    g_free (path);                                                              \
  }                                                                             \
  path = g_strdup_printf ("/regex/full-split2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data); \
  g_free (path);                                                                \
}

#define TEST_SPLIT3(_pattern, _string, _start_position, _max_tokens, e1, e2, e3) { \
  TestFetchAllData *data;                                                          \
  gchar *path;                                                                     \
  data = g_new0 (TestFetchAllData, 1);                                             \
  data->pattern = _pattern;                                                        \
  data->string = _string;                                                          \
  data->start_position = _start_position;                                          \
  data->max_tokens = _max_tokens;                                                  \
  data->expected = NULL;                                                           \
  data->expected = g_slist_append (data->expected, e1);                            \
  data->expected = g_slist_append (data->expected, e2);                            \
  data->expected = g_slist_append (data->expected, e3);                            \
  if (_start_position == 0 && _max_tokens <= 0) {                                  \
    path = g_strdup_printf ("/regex/split3/%d", ++total);                          \
    g_test_add_data_func (path, data, test_split);                                 \
    g_free (path);                                                                 \
  }                                                                                \
  path = g_strdup_printf ("/regex/full-split3/%d", ++total);                       \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data);    \
  g_free (path);                                                                   \
}

typedef struct {
  const gchar *string_to_expand;
  gboolean expected;
  gboolean expected_refs;
} TestCheckReplacementData;

static void
test_check_replacement (gconstpointer d)
{
  const TestCheckReplacementData *data = d;
  gboolean has_refs;
  gboolean result;

  result = g_regex_check_replacement (data->string_to_expand, &has_refs, NULL);
  g_assert_cmpint (data->expected, ==, result);

  if (data->expected)
    g_assert_cmpint (data->expected_refs, ==, has_refs);
}

#define TEST_CHECK_REPLACEMENT(_string_to_expand, _expected, _expected_refs) { \
  TestCheckReplacementData *data;                                              \
  gchar *path;                                                                 \
  data = g_new0 (TestCheckReplacementData, 1);                                 \
  data->string_to_expand = _string_to_expand;                                  \
  data->expected = _expected;                                                  \
  data->expected_refs = _expected_refs;                                        \
  path = g_strdup_printf ("/regex/check-repacement/%d", ++total);              \
  g_test_add_data_func_full (path, data, test_check_replacement, g_free);      \
  g_free (path);                                                               \
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  const gchar *string_to_expand;
  gboolean     raw;
  const gchar *expected;
} TestExpandData;

static void
test_expand (gconstpointer d)
{
  const TestExpandData *data = d;
  GRegex *regex = NULL;
  GMatchInfo *match_info = NULL;
  gchar *res;
  GError *error = NULL;

  if (data->pattern)
    {
      regex = g_regex_new (data->pattern, data->raw ? G_REGEX_RAW : 0,
                           G_REGEX_MATCH_DEFAULT, &error);
      g_assert_no_error (error);
      g_regex_match (regex, data->string, 0, &match_info);
    }

  res = g_match_info_expand_references (match_info, data->string_to_expand, NULL);
  g_assert_cmpstr (res, ==, data->expected);
  g_free (res);
  g_match_info_free (match_info);
  if (regex)
    g_regex_unref (regex);
}

#define TEST_EXPAND(_pattern, _string, _string_to_expand, _raw, _expected) { \
  TestExpandData *data;                                                      \
  gchar *path;                                                               \
  data = g_new0 (TestExpandData, 1);                                         \
  data->pattern = _pattern;                                                  \
  data->string = _string;                                                    \
  data->string_to_expand = _string_to_expand;                                \
  data->raw = _raw;                                                          \
  data->expected = _expected;                                                \
  path = g_strdup_printf ("/regex/expand/%d", ++total);                      \
  g_test_add_data_func_full (path, data, test_expand, g_free);               \
  g_free (path);                                                             \
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  gint         start_position;
  const gchar *replacement;
  const gchar *expected;
  GRegexCompileFlags compile_flags;
  GRegexMatchFlags match_flags;
} TestReplaceData;

static void
test_replace (gconstpointer d)
{
  const TestReplaceData *data = d;
  GRegex *regex;
  gchar *res;
  GError *error = NULL;

  regex = g_regex_new (data->pattern, data->compile_flags, G_REGEX_MATCH_DEFAULT, &error);
  g_assert_no_error (error);

  res = g_regex_replace (regex, data->string, -1, data->start_position,
                         data->replacement, data->match_flags, &error);

  g_assert_cmpstr (res, ==, data->expected);

  if (data->expected)
    g_assert_no_error (error);

  g_free (res);
  g_regex_unref (regex);
  g_clear_error (&error);
}

#define TEST_REPLACE_OPTIONS(_pattern, _string, _start_position, _replacement, _expected, _compile_flags, _match_flags) { \
  TestReplaceData *data;                                                \
  gchar *path;                                                          \
  data = g_new0 (TestReplaceData, 1);                                   \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->replacement = _replacement;                                     \
  data->expected = _expected;                                           \
  data->compile_flags = _compile_flags;                                 \
  data->match_flags = _match_flags;                                     \
  total++;                                                              \
  if (data->compile_flags & G_REGEX_OPTIMIZE)                           \
    path = g_strdup_printf ("/regex/replace-optimized/%d", total);      \
  else                                                                  \
    path = g_strdup_printf ("/regex/replace/%d", total);                \
  g_test_add_data_func_full (path, data, test_replace, g_free);         \
  g_free (path);                                                        \
  data = g_memdup2 (data, sizeof (TestReplaceData));                    \
  if (data->compile_flags & G_REGEX_OPTIMIZE)                           \
    {                                                                   \
      data->compile_flags &= ~G_REGEX_OPTIMIZE;                         \
      path = g_strdup_printf ("/regex/replace/%d", total);              \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      data->compile_flags |= G_REGEX_OPTIMIZE;                          \
      path = g_strdup_printf ("/regex/replace-optimized/%d", total);    \
    }                                                                   \
  g_test_add_data_func_full (path, data, test_replace, g_free);         \
  g_free (path);                                                        \
}

#define TEST_REPLACE(_pattern, _string, _start_position, _replacement, _expected) \
  TEST_REPLACE_OPTIONS (_pattern, _string, _start_position, _replacement, _expected, 0, 0)

static void
test_replace_lit (gconstpointer d)
{
  const TestReplaceData *data = d;
  GRegex *regex;
  gchar *res;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  res = g_regex_replace_literal (regex, data->string, -1, data->start_position,
                                 data->replacement, 0, NULL);
  g_assert_cmpstr (res, ==, data->expected);

  g_free (res);
  g_regex_unref (regex);
}

#define TEST_REPLACE_LIT(_pattern, _string, _start_position, _replacement, _expected) { \
  TestReplaceData *data;                                                \
  gchar *path;                                                          \
  data = g_new0 (TestReplaceData, 1);                                   \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->replacement = _replacement;                                     \
  data->expected = _expected;                                           \
  path = g_strdup_printf ("/regex/replace-literally/%d", ++total);      \
  g_test_add_data_func_full (path, data, test_replace_lit, g_free);     \
  g_free (path);                                                        \
}

typedef struct {
  const gchar *pattern;
  const gchar *name;
  gint         expected_num;
} TestStringNumData;

static void
test_get_string_number (gconstpointer d)
{
  const TestStringNumData *data = d;
  GRegex *regex;
  gint num;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  num = g_regex_get_string_number (regex, data->name);

  g_assert_cmpint (num, ==, data->expected_num);
  g_regex_unref (regex);
}

#define TEST_GET_STRING_NUMBER(_pattern, _name, _expected_num) {          \
  TestStringNumData *data;                                                \
  gchar *path;                                                            \
  data = g_new0 (TestStringNumData, 1);                                   \
  data->pattern = _pattern;                                               \
  data->name = _name;                                                     \
  data->expected_num = _expected_num;                                     \
  path = g_strdup_printf ("/regex/string-number/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_get_string_number, g_free); \
  g_free (path);                                                          \
}

typedef struct {
  const gchar *string;
  gint         length;
  const gchar *expected;
} TestEscapeData;

static void
test_escape (gconstpointer d)
{
  const TestEscapeData *data = d;
  gchar *escaped;

  escaped = g_regex_escape_string (data->string, data->length);

  g_assert_cmpstr (escaped, ==, data->expected);

  g_free (escaped);
}

#define TEST_ESCAPE(_string, _length, _expected) {             \
  TestEscapeData *data;                                        \
  gchar *path;                                                 \
  data = g_new0 (TestEscapeData, 1);                           \
  data->string = _string;                                      \
  data->length = _length;                                      \
  data->expected = _expected;                                  \
  path = g_strdup_printf ("/regex/escape/%d", ++total);        \
  g_test_add_data_func_full (path, data, test_escape, g_free); \
  g_free (path);                                               \
}

static void
test_escape_nul (gconstpointer d)
{
  const TestEscapeData *data = d;
  gchar *escaped;

  escaped = g_regex_escape_nul (data->string, data->length);

  g_assert_cmpstr (escaped, ==, data->expected);

  g_free (escaped);
}

#define TEST_ESCAPE_NUL(_string, _length, _expected) {             \
  TestEscapeData *data;                                            \
  gchar *path;                                                     \
  data = g_new0 (TestEscapeData, 1);                               \
  data->string = _string;                                          \
  data->length = _length;                                          \
  data->expected = _expected;                                      \
  path = g_strdup_printf ("/regex/escape_nul/%d", ++total);        \
  g_test_add_data_func_full (path, data, test_escape_nul, g_free); \
  g_free (path);                                                   \
}

typedef struct {
  const gchar *pattern;
  const gchar *string;
  gssize       string_len;
  gint         start_position;
  GSList *expected;
} TestMatchAllData;

static void
test_match_all_full (gconstpointer d)
{
  const TestMatchAllData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  GSList *l_exp;
  gboolean match_ok;
  gint match_count;
  gint i;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  match_ok = g_regex_match_all_full (regex, data->string, data->string_len, data->start_position,
                                     0, &match_info, NULL);

  if (g_slist_length (data->expected) == 0)
    g_assert (!match_ok);
  else
    g_assert (match_ok);

  match_count = g_match_info_get_match_count (match_info);
  g_assert_cmpint (match_count, ==, g_slist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; i < match_count; i++)
    {
      gint start, end;
      gchar *matched_string;
      Match *exp = l_exp->data;

      matched_string = g_match_info_fetch (match_info, i);
      g_match_info_fetch_pos (match_info, i, &start, &end);

      g_assert_cmpstr (exp->string, ==, matched_string);
      g_assert_cmpint (exp->start, ==, start);
      g_assert_cmpint (exp->end, ==, end);

      g_free (matched_string);

      l_exp = g_slist_next (l_exp);
    }

  g_match_info_free (match_info);
  g_regex_unref (regex);
}

static void
test_match_all (gconstpointer d)
{
  const TestMatchAllData *data = d;
  GRegex *regex;
  GMatchInfo *match_info;
  GSList *l_exp;
  gboolean match_ok;
  guint i, match_count;

  regex = g_regex_new (data->pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  match_ok = g_regex_match_all (regex, data->string, 0, &match_info);

  if (g_slist_length (data->expected) == 0)
    g_assert (!match_ok);
  else
    g_assert (match_ok);

  match_count = g_match_info_get_match_count (match_info);
  g_assert_cmpint (match_count, >=, 0);

  if (match_count != g_slist_length (data->expected))
    {
      g_message ("regex: %s", data->pattern);
      g_message ("string: %s", data->string);
      g_message ("matched strings:");

      for (i = 0; i < match_count; i++)
        {
          gint start, end;
          gchar *matched_string;

          matched_string = g_match_info_fetch (match_info, i);
          g_match_info_fetch_pos (match_info, i, &start, &end);
          g_message ("%u. %d-%d '%s'", i, start, end, matched_string);
          g_free (matched_string);
        }

      g_message ("expected strings:");
      i = 0;

      for (l_exp = data->expected; l_exp != NULL; l_exp = l_exp->next)
        {
          Match *exp = l_exp->data;

          g_message ("%u. %d-%d '%s'", i, exp->start, exp->end, exp->string);
          i++;
        }

      g_error ("match_count not as expected: %u != %d",
          match_count, g_slist_length (data->expected));
    }

  l_exp = data->expected;
  for (i = 0; i < match_count; i++)
    {
      gint start, end;
      gchar *matched_string;
      Match *exp = l_exp->data;

      matched_string = g_match_info_fetch (match_info, i);
      g_match_info_fetch_pos (match_info, i, &start, &end);

      g_assert_cmpstr (exp->string, ==, matched_string);
      g_assert_cmpint (exp->start, ==, start);
      g_assert_cmpint (exp->end, ==, end);

      g_free (matched_string);

      l_exp = g_slist_next (l_exp);
    }

  g_match_info_free (match_info);
  g_regex_unref (regex);
}

static void
free_match_all_data (gpointer _data)
{
  TestMatchAllData *data = _data;

  g_slist_free_full (data->expected, g_free);
  g_free (data);
}

#define TEST_MATCH_ALL0(_pattern, _string, _string_len, _start_position) {          \
  TestMatchAllData *data;                                                           \
  gchar *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = g_strdup_printf ("/regex/match-all0/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = g_strdup_printf ("/regex/match-all-full0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

#define TEST_MATCH_ALL1(_pattern, _string, _string_len, _start_position,            \
                        t1, s1, e1) {                                               \
  TestMatchAllData *data;                                                           \
  Match *match;                                                                     \
  gchar *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  match = g_new0 (Match, 1);                                                        \
  match->string = t1;                                                               \
  match->start = s1;                                                                \
  match->end = e1;                                                                  \
  data->expected = g_slist_append (data->expected, match);                          \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = g_strdup_printf ("/regex/match-all1/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = g_strdup_printf ("/regex/match-all-full1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

#define TEST_MATCH_ALL2(_pattern, _string, _string_len, _start_position,            \
                        t1, s1, e1, t2, s2, e2) {                                   \
  TestMatchAllData *data;                                                           \
  Match *match;                                                                     \
  gchar *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  match = g_new0 (Match, 1);                                                        \
  match->string = t1;                                                               \
  match->start = s1;                                                                \
  match->end = e1;                                                                  \
  data->expected = g_slist_append (data->expected, match);                          \
  match = g_new0 (Match, 1);                                                        \
  match->string = t2;                                                               \
  match->start = s2;                                                                \
  match->end = e2;                                                                  \
  data->expected = g_slist_append (data->expected, match);                          \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = g_strdup_printf ("/regex/match-all2/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = g_strdup_printf ("/regex/match-all-full2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

#define TEST_MATCH_ALL3(_pattern, _string, _string_len, _start_position,            \
                        t1, s1, e1, t2, s2, e2, t3, s3, e3) {                       \
  TestMatchAllData *data;                                                           \
  Match *match;                                                                     \
  gchar *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  match = g_new0 (Match, 1);                                                        \
  match->string = t1;                                                               \
  match->start = s1;                                                                \
  match->end = e1;                                                                  \
  data->expected = g_slist_append (data->expected, match);                          \
  match = g_new0 (Match, 1);                                                        \
  match->string = t2;                                                               \
  match->start = s2;                                                                \
  match->end = e2;                                                                  \
  data->expected = g_slist_append (data->expected, match);                          \
  match = g_new0 (Match, 1);                                                        \
  match->string = t3;                                                               \
  match->start = s3;                                                                \
  match->end = e3;                                                                  \
  data->expected = g_slist_append (data->expected, match);                          \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = g_strdup_printf ("/regex/match-all3/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = g_strdup_printf ("/regex/match-all-full3/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

static void
test_properties (void)
{
  GRegex *regex;
  GError *error;
  gboolean res;
  GMatchInfo *match;
  gchar *str;

  error = NULL;
  regex = g_regex_new ("\\p{L}\\p{Ll}\\p{Lu}\\p{L&}\\p{N}\\p{Nd}", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  res = g_regex_match (regex, "ppPP01", 0, &match);
  g_assert (res);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "ppPP01");
  g_free (str);

  g_match_info_free (match);
  g_regex_unref (regex);
}

static void
test_class (void)
{
  GRegex *regex;
  GError *error;
  GMatchInfo *match;
  gboolean res;
  gchar *str;

  error = NULL;
  regex = g_regex_new ("[abc\\x{0B1E}\\p{Mn}\\x{0391}-\\x{03A9}]", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  res = g_regex_match (regex, "a:b:\340\254\236:\333\253:\316\240", 0, &match);
  g_assert (res);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "a");
  g_free (str);
  res = g_match_info_next (match, NULL);
  g_assert (res);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "b");
  g_free (str);
  res = g_match_info_next (match, NULL);
  g_assert (res);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "\340\254\236");
  g_free (str);
  res = g_match_info_next (match, NULL);
  g_assert (res);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "\333\253");
  g_free (str);
  res = g_match_info_next (match, NULL);
  g_assert (res);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "\316\240");
  g_free (str);

  res = g_match_info_next (match, NULL);
  g_assert (!res);

  /* Accessing match again should not crash */
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL,
                         "*match_info->pos_valid*");
  g_assert_false (g_match_info_next (match, NULL));
  g_test_assert_expected_messages ();

  g_match_info_free (match);
  g_regex_unref (regex);
}

/* examples for lookahead assertions taken from pcrepattern(3) */
static void
test_lookahead (void)
{
  GRegex *regex;
  GError *error;
  GMatchInfo *match;
  gboolean res;
  gchar *str;
  gint start, end;

  error = NULL;
  regex = g_regex_new ("\\w+(?=;)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "word1 word2: word3;", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 1);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "word3");
  g_free (str);
  g_match_info_free (match);
  g_regex_unref (regex);

  error = NULL;
  regex = g_regex_new ("foo(?!bar)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "foobar foobaz", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 1);
  res = g_match_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 7);
  g_assert_cmpint (end, ==, 10);
  g_match_info_free (match);
  g_regex_unref (regex);

  error = NULL;
  regex = g_regex_new ("(?!bar)foo", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "foobar foobaz", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 1);
  res = g_match_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 3);
  res = g_match_info_next (match, &error);
  g_assert (res);
  g_assert_no_error (error);
  res = g_match_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 7);
  g_assert_cmpint (end, ==, 10);
  g_match_info_free (match);
  g_regex_unref (regex);
}

/* examples for lookbehind assertions taken from pcrepattern(3) */
static void
test_lookbehind (void)
{
  GRegex *regex;
  GError *error;
  GMatchInfo *match;
  gboolean res;
  gint start, end;

  error = NULL;
  regex = g_regex_new ("(?<!foo)bar", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "foobar boobar", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 1);
  res = g_match_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 10);
  g_assert_cmpint (end, ==, 13);
  g_match_info_free (match);
  g_regex_unref (regex);

  error = NULL;
  regex = g_regex_new ("(?<=bullock|donkey) poo", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "don poo, and bullock poo", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 1);
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 20);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<=abc|abde)foo", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "abfoo, abdfoo, abcfoo", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 18);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^.*+(?<=abcd)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "abcabcabcabcabcabcabcabcabcd", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<=\\d{3})(?<!999)foo", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "999foo 123abcfoo 123foo", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 20);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<=\\d{3}...)(?<!999)foo", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "999foo 123abcfoo 123foo", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 13);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<=\\d{3}(?!999)...)foo", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "999foo 123abcfoo 123foo", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 13);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<=(?<!foo)bar)baz", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "foobarbaz barfoobaz barbarbaz", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 26);
  g_match_info_free (match);
  g_regex_unref (regex);
}

/* examples for subpatterns taken from pcrepattern(3) */
static void
test_subpattern (void)
{
  GRegex *regex;
  GError *error;
  GMatchInfo *match;
  gboolean res;
  gchar *str;
  gint start;

  error = NULL;
  regex = g_regex_new ("cat(aract|erpillar|)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (g_regex_get_capture_count (regex), ==, 1);
  g_assert_cmpint (g_regex_get_max_backref (regex), ==, 0);
  res = g_regex_match_all (regex, "caterpillar", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 2);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "caterpillar");
  g_free (str);
  str = g_match_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "cat");
  g_free (str);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("the ((red|white) (king|queen))", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (g_regex_get_capture_count (regex), ==, 3);
  g_assert_cmpint (g_regex_get_max_backref (regex), ==, 0);
  res = g_regex_match (regex, "the red king", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 4);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "the red king");
  g_free (str);
  str = g_match_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "red king");
  g_free (str);
  str = g_match_info_fetch (match, 2);
  g_assert_cmpstr (str, ==, "red");
  g_free (str);
  str = g_match_info_fetch (match, 3);
  g_assert_cmpstr (str, ==, "king");
  g_free (str);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("the ((?:red|white) (king|queen))", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "the white queen", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 3);
  g_assert_cmpint (g_regex_get_max_backref (regex), ==, 0);
  str = g_match_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "the white queen");
  g_free (str);
  str = g_match_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "white queen");
  g_free (str);
  str = g_match_info_fetch (match, 2);
  g_assert_cmpstr (str, ==, "queen");
  g_free (str);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?|(Sat)(ur)|(Sun))day (morning|afternoon)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (g_regex_get_capture_count (regex), ==, 3);
  res = g_regex_match (regex, "Saturday morning", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_assert_cmpint (g_match_info_get_match_count (match), ==, 4);
  str = g_match_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "Sat");
  g_free (str);
  str = g_match_info_fetch (match, 2);
  g_assert_cmpstr (str, ==, "ur");
  g_free (str);
  str = g_match_info_fetch (match, 3);
  g_assert_cmpstr (str, ==, "morning");
  g_free (str);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?|(abc)|(def))\\1", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (g_regex_get_max_backref (regex), ==, 1);
  res = g_regex_match (regex, "abcabc abcdef defabc defdef", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  res = g_match_info_next (match, &error);
  g_assert (res);
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 21);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?|(abc)|(def))(?1)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "abcabc abcdef defabc defdef", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  res = g_match_info_next (match, &error);
  g_assert (res);
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 14);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<DN>Mon|Fri|Sun)(?:day)?|(?<DN>Tue)(?:sday)?|(?<DN>Wed)(?:nesday)?|(?<DN>Thu)(?:rsday)?|(?<DN>Sat)(?:urday)?", G_REGEX_OPTIMIZE|G_REGEX_DUPNAMES, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "Mon Tuesday Wed Saturday", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  str = g_match_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Mon");
  g_free (str);
  res = g_match_info_next (match, &error);
  g_assert (res);
  str = g_match_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Tue");
  g_free (str);
  res = g_match_info_next (match, &error);
  g_assert (res);
  str = g_match_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Wed");
  g_free (str);
  res = g_match_info_next (match, &error);
  g_assert (res);
  str = g_match_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Sat");
  g_free (str);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^(a|b\\1)+$", G_REGEX_OPTIMIZE|G_REGEX_DUPNAMES, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "aaaaaaaaaaaaaaaa", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "ababbaa", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);
}

/* examples for conditions taken from pcrepattern(3) */
static void
test_condition (void)
{
  GRegex *regex;
  GError *error;
  GMatchInfo *match;
  gboolean res;

  error = NULL;
  regex = g_regex_new ("^(a+)(\\()?[^()]+(?(-1)\\))(b+)$", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "a(zzzzzz)b", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "aaazzzzzzbbb", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  error = NULL;
  regex = g_regex_new ("^(a+)(?<OPEN>\\()?[^()]+(?(<OPEN>)\\))(b+)$", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "a(zzzzzz)b", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "aaazzzzzzbbb", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^(a+)(?(+1)\\[|\\<)?[^()]+(\\])?(b+)$", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "a[zzzzzz]b", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "aaa<zzzzzzbbb", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("(?(DEFINE) (?<byte> 2[0-4]\\d | 25[0-5] | 1\\d\\d | [1-9]?\\d) )"
                       "\\b (?&byte) (\\.(?&byte)){3} \\b",
                       G_REGEX_OPTIMIZE|G_REGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "128.0.0.1", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "192.168.1.1", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "209.132.180.167", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^(?(?=[^a-z]*[a-z])"
                       "\\d{2}-[a-z]{3}-\\d{2} | \\d{2}-\\d{2}-\\d{2} )$",
                       G_REGEX_OPTIMIZE|G_REGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "01-abc-24", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "01-23-45", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "01-uv-45", 0, &match);
  g_assert (!res);
  g_assert (!g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "01-234-45", 0, &match);
  g_assert (!res);
  g_assert (!g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);
}

/* examples for recursion taken from pcrepattern(3) */
static void
test_recursion (void)
{
  GRegex *regex;
  GError *error;
  GMatchInfo *match;
  gboolean res;
  gint start;

  error = NULL;
  regex = g_regex_new ("\\( ( [^()]++ | (?R) )* \\)", G_REGEX_OPTIMIZE|G_REGEX_EXTENDED, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "(middle)", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "((((((((((((((((middle))))))))))))))))", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "(((xxx(((", 0, &match);
  g_assert (!res);
  g_assert (!g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^( \\( ( [^()]++ | (?1) )* \\) )$", G_REGEX_OPTIMIZE|G_REGEX_EXTENDED, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "((((((((((((((((middle))))))))))))))))", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "(((xxx((()", 0, &match);
  g_assert (!res);
  g_assert (!g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^(?<pn> \\( ( [^()]++ | (?&pn) )* \\) )$", G_REGEX_OPTIMIZE|G_REGEX_EXTENDED, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_regex_match (regex, "(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa()", 0, &match);
  g_assert (!res);
  g_assert (!g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("< (?: (?(R) \\d++ | [^<>]*+) | (?R)) * >", G_REGEX_OPTIMIZE|G_REGEX_EXTENDED, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "<ab<01<23<4>>>>", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  g_match_info_free (match);
  res = g_regex_match (regex, "<ab<01<xx<x>>>>", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  res = g_match_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, >, 0);
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^((.)(?1)\\2|.)$", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "abcdcba", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "abcddcba", 0, &match);
  g_assert (!res);
  g_assert (!g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^(?:((.)(?1)\\2|)|((.)(?3)\\4|.))$", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "abcdcba", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "abcddcba", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);

  regex = g_regex_new ("^\\W*+(?:((.)\\W*+(?1)\\W*+\\2|)|((.)\\W*+(?3)\\W*+\\4|\\W*+.\\W*+))\\W*+$", G_REGEX_OPTIMIZE|G_REGEX_CASELESS, G_REGEX_MATCH_DEFAULT, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = g_regex_match (regex, "abcdcba", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "A man, a plan, a canal: Panama!", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  res = g_regex_match (regex, "Oozy rat in a sanitary zoo", 0, &match);
  g_assert (res);
  g_assert (g_match_info_matches (match));
  g_match_info_free (match);
  g_regex_unref (regex);
}

static void
test_multiline (void)
{
  GRegex *regex;
  GMatchInfo *info;
  gint count;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=640489");

  regex = g_regex_new ("^a$", G_REGEX_MULTILINE|G_REGEX_DOTALL, G_REGEX_MATCH_DEFAULT, NULL);

  count = 0;
  g_regex_match (regex, "a\nb\na", 0, &info);
  while (g_match_info_matches (info))
    {
      count++;
      g_match_info_next (info, NULL);
    }
  g_match_info_free (info);
  g_regex_unref (regex);

  g_assert_cmpint (count, ==, 2);
}

static void
test_explicit_crlf (void)
{
  GRegex *regex;

  regex = g_regex_new ("[\r\n]a", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  g_assert_cmpint (g_regex_get_has_cr_or_lf (regex), ==, TRUE);
  g_regex_unref (regex);
}

static void
test_max_lookbehind (void)
{
  GRegex *regex;

  regex = g_regex_new ("abc", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  g_assert_cmpint (g_regex_get_max_lookbehind (regex), ==, 0);
  g_regex_unref (regex);

  regex = g_regex_new ("\\babc", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  g_assert_cmpint (g_regex_get_max_lookbehind (regex), ==, 1);
  g_regex_unref (regex);

  regex = g_regex_new ("(?<=123)abc", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
  g_assert_cmpint (g_regex_get_max_lookbehind (regex), ==, 3);
  g_regex_unref (regex);
}

static gboolean
pcre2_ge (guint64 major, guint64 minor)
{
    gchar version[32];
    const gchar *ptr;
    guint64 pcre2_major, pcre2_minor;

    /* e.g. 10.36 2020-12-04 */
    pcre2_config (PCRE2_CONFIG_VERSION, version);

    pcre2_major = g_ascii_strtoull (version, (gchar **) &ptr, 10);
    /* ptr points to ".MINOR (release date)" */
    g_assert (ptr[0] == '.');
    pcre2_minor = g_ascii_strtoull (ptr + 1, NULL, 10);

    return (pcre2_major > major) || (pcre2_major == major && pcre2_minor >= minor);
}

static void
test_compile_errors (void)
{
  GRegex *regex;
  GError *error = NULL;

  regex = g_regex_new ("\\o{999}", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, &error);
  g_assert_null (regex);
  g_assert_error (error, G_REGEX_ERROR, G_REGEX_ERROR_COMPILE);
  g_clear_error (&error);
}

static void
test_jit_unsupported_matching_options (void)
{
  GRegex *regex;
  GMatchInfo *info;
  gchar *substring;

  regex = g_regex_new ("(\\w+)#(\\w+)", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT, NULL);

  g_assert_true (g_regex_match (regex, "aa#bb cc#dd", G_REGEX_MATCH_DEFAULT, &info));
  g_assert_cmpint (g_match_info_get_match_count (info), ==, 3);
  substring = g_match_info_fetch (info, 1);
  g_assert_cmpstr (substring, ==, "aa");
  g_clear_pointer (&substring, g_free);
  substring = g_match_info_fetch (info, 2);
  g_assert_cmpstr (substring, ==, "bb");
  g_clear_pointer (&substring, g_free);
  g_assert_true (g_match_info_next (info, NULL));
  g_assert_cmpint (g_match_info_get_match_count (info), ==, 3);
  substring = g_match_info_fetch (info, 1);
  g_assert_cmpstr (substring, ==, "cc");
  g_clear_pointer (&substring, g_free);
  substring = g_match_info_fetch (info, 2);
  g_assert_cmpstr (substring, ==, "dd");
  g_clear_pointer (&substring, g_free);
  g_assert_false (g_match_info_next (info, NULL));
  g_match_info_free (info);

  g_assert_true (g_regex_match (regex, "aa#bb cc#dd", G_REGEX_MATCH_ANCHORED, &info));
  g_assert_cmpint (g_match_info_get_match_count (info), ==, 3);
  substring = g_match_info_fetch (info, 1);
  g_assert_cmpstr (substring, ==, "aa");
  g_clear_pointer (&substring, g_free);
  substring = g_match_info_fetch (info, 2);
  g_assert_cmpstr (substring, ==, "bb");
  g_clear_pointer (&substring, g_free);
  g_assert_false (g_match_info_next (info, NULL));
  g_match_info_free (info);

  g_assert_true (g_regex_match (regex, "aa#bb cc#dd", G_REGEX_MATCH_DEFAULT, &info));
  g_assert_cmpint (g_match_info_get_match_count (info), ==, 3);
  substring = g_match_info_fetch (info, 1);
  g_assert_cmpstr (substring, ==, "aa");
  g_clear_pointer (&substring, g_free);
  substring = g_match_info_fetch (info, 2);
  g_assert_cmpstr (substring, ==, "bb");
  g_clear_pointer (&substring, g_free);
  g_assert_true (g_match_info_next (info, NULL));
  g_assert_cmpint (g_match_info_get_match_count (info), ==, 3);
  substring = g_match_info_fetch (info, 1);
  g_assert_cmpstr (substring, ==, "cc");
  g_clear_pointer (&substring, g_free);
  substring = g_match_info_fetch (info, 2);
  g_assert_cmpstr (substring, ==, "dd");
  g_clear_pointer (&substring, g_free);
  g_assert_false (g_match_info_next (info, NULL));
  g_match_info_free (info);

  g_regex_unref (regex);
}

static void
test_unmatched_named_subpattern (void)
{
  GRegex *regex = NULL;
  GMatchInfo *match_info = NULL;
  const char *string = "Test";

  g_test_summary ("Test that unmatched subpatterns can still be queried");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2881");

  regex = g_regex_new ("((?<Key>[^\\s\"'\\=]+)|\"(?<Key>[^\"]*)\"|'(?<Key>[^']*)')"
                       "(?:\\=((?<Value>[^\\s\"']+)|\"(?<Value>[^\"]*)\"|'(?<Value>[^']*)'))?",
                       G_REGEX_DUPNAMES, 0, NULL);

  g_assert_true (g_regex_match (regex, string, 0, &match_info));

  while (g_match_info_matches (match_info))
    {
      char *key = g_match_info_fetch_named (match_info, "Key");
      char *value = g_match_info_fetch_named (match_info, "Value");

      g_assert_cmpstr (key, ==, "Test");
      g_assert_cmpstr (value, ==, "");

      g_free (key);
      g_free (value);
      g_match_info_next (match_info, NULL);
    }

  g_match_info_unref (match_info);
  g_regex_unref (regex);
}

static void
test_compiled_regex_after_jit_failure (void)
{
  GRegex *regex = NULL;
  char string[LARGE_TEST_STRING_LEN];

  g_test_summary ("Test that failed OPTIMIZE regex doesn't cause issues on subsequent matches");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2824");

  regex = g_regex_new ("^(?:[ \t\n]|[^[:cntrl:]])*$", G_REGEX_OPTIMIZE, 0, NULL);

  /* Generate large enough string to cause JIT failure on this regex. */
  memset (string, '*', LARGE_TEST_STRING_LEN);
  string[LARGE_TEST_STRING_LEN - 1] = '\0';

  g_assert_true (g_regex_match (regex, string, 0, NULL));
  /* Second assert here is the key - does the first JIT overflow mess up our state? */
  g_assert_true (g_regex_match (regex, string, 0, NULL));

  g_regex_unref (regex);
}

static void
test_replace_raw_change_case (void)
{
  GError *local_error = NULL;
  GRegex *regex = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3918");
  g_test_summary ("Test that case changes as part of a replacement are handled correctly in G_REGEX_RAW mode");

  /*
   * Match a multi-byte sequence in RAW mode. The pattern matches
   * exactly 2 bytes. The subject contains a 4-byte UTF-8 lead (0xF4)
   * followed by only one continuation byte, then NUL.
   *
   * The matched substring will be "\xf4\x80" (2 bytes, heap-allocated
   * as 3-byte buffer with NUL). If the code regresses and tries to handle
   * the replacement as UTF-8 then g_utf8_get_char() would see 0xF4 and try
   * to read 4 bytes, going 1 byte past the NUL into OOB territory.
   */
  regex = g_regex_new ("..", G_REGEX_RAW, 0, &local_error);
  g_assert_no_error (local_error);

  /*
   * Build a subject string with truncated UTF-8.
   * \xF4 = 4-byte UTF-8 lead byte
   * \x80 = continuation byte
   * No 3rd/4th continuation bytes — the match is only 2 bytes.
   *
   * \U\0 = uppercase the entire match → triggers string_append()
   * with case change on the 2-byte non-UTF-8 match.
   */
  char subject[] = "\xf4\x80";
  char *result = g_regex_replace (regex, subject, -1, 0, "\\U\\0", 0, &local_error);
  g_assert_no_error (local_error);

  g_clear_pointer (&result, g_free);
  g_clear_pointer (&regex, g_regex_unref);

  /*
   * Second variant: single-char case change \u with \0 backreference.
   */
  regex = g_regex_new (".", G_REGEX_RAW, 0, &local_error);
  g_assert_no_error (local_error);

  char subject2[] = "\xe6\xb0";  /* 3-byte UTF-8 lead, only 2 bytes */
  result = g_regex_replace (regex, subject2, -1, 0, "\\u\\0", 0, &local_error);
  g_assert_no_error (local_error);

  g_clear_pointer (&result, g_free);
  g_clear_pointer (&regex, g_regex_unref);
}

static void
test_split_raw (void)
{
  GError *local_error = NULL;
  GRegex *regex = NULL;
  char *subject = NULL;
  char **tokens = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3919");
  g_test_summary ("Test splitting a string in G_REGEX_RAW mode");

  /* Empty pattern in RAW mode — matches at every position */
  regex = g_regex_new ("", G_REGEX_RAW, 0, &local_error);
  g_assert_no_error (local_error);

  /*
   * Subject: single continuation byte 0x80, heap-allocated.
   * When split encounters empty match at position 0, if the code were to
   * regress then PREV_CHAR would call g_utf8_prev_char(&string[0]), which
   * would scan backwards past the allocation start.
   */
  subject = g_strdup ("\x80");

  tokens = g_regex_split_full (regex, subject, -1, 0, 0, 0, &local_error);
  g_assert_no_error (local_error);

  g_strfreev (tokens);
  g_free (subject);
  g_regex_unref (regex);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/regex/properties", test_properties);
  g_test_add_func ("/regex/class", test_class);
  g_test_add_func ("/regex/lookahead", test_lookahead);
  g_test_add_func ("/regex/lookbehind", test_lookbehind);
  g_test_add_func ("/regex/subpattern", test_subpattern);
  g_test_add_func ("/regex/condition", test_condition);
  g_test_add_func ("/regex/recursion", test_recursion);
  g_test_add_func ("/regex/multiline", test_multiline);
  g_test_add_func ("/regex/explicit-crlf", test_explicit_crlf);
  g_test_add_func ("/regex/max-lookbehind", test_max_lookbehind);
  g_test_add_func ("/regex/compile-errors", test_compile_errors);
  g_test_add_func ("/regex/jit-unsupported-matching", test_jit_unsupported_matching_options);
  g_test_add_func ("/regex/unmatched-named-subpattern", test_unmatched_named_subpattern);
  g_test_add_func ("/regex/compiled-regex-after-jit-failure", test_compiled_regex_after_jit_failure);
  g_test_add_func ("/regex/replace-raw-change-case", test_replace_raw_change_case);
  g_test_add_func ("/regex/split-raw", test_split_raw);

  /* TEST_NEW(pattern, compile_opts, match_opts) */
  TEST_NEW("[A-Z]+", G_REGEX_CASELESS | G_REGEX_EXTENDED | G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTBOL | G_REGEX_MATCH_PARTIAL);
  TEST_NEW("", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW(".*", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW(".*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT);
  TEST_NEW(".*", G_REGEX_MULTILINE, G_REGEX_MATCH_DEFAULT);
  TEST_NEW(".*", G_REGEX_DOTALL, G_REGEX_MATCH_DEFAULT);
  TEST_NEW(".*", G_REGEX_DOTALL, G_REGEX_MATCH_NOTBOL);
  TEST_NEW("(123\\d*)[a-zA-Z]+(?P<hello>.*)", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW("(123\\d*)[a-zA-Z]+(?P<hello>.*)", G_REGEX_CASELESS, G_REGEX_MATCH_DEFAULT);
  TEST_NEW("(123\\d*)[a-zA-Z]+(?P<hello>.*)", G_REGEX_CASELESS | G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT);
  TEST_NEW("(?P<A>x)|(?P<A>y)", G_REGEX_DUPNAMES, G_REGEX_MATCH_DEFAULT);
  TEST_NEW("(?P<A>x)|(?P<A>y)", G_REGEX_DUPNAMES | G_REGEX_OPTIMIZE, G_REGEX_MATCH_DEFAULT);
  /* This gives "internal error: code overflow" with pcre 6.0 */
  TEST_NEW("(?i)(?-i)", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW ("(?i)a", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW ("(?m)a", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW ("(?s)a", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW ("(?x)a", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW ("(?J)a", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);
  TEST_NEW ("(?U)[a-z]+", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT);

  /* TEST_NEW_CHECK_FLAGS(pattern, compile_opts, match_ops, real_compile_opts, real_match_opts) */
  TEST_NEW_CHECK_FLAGS ("a", G_REGEX_OPTIMIZE, 0, G_REGEX_OPTIMIZE, 0);
  TEST_NEW_CHECK_FLAGS ("a", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY,
                        G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY);
  TEST_NEW_CHECK_FLAGS ("a", 0, G_REGEX_MATCH_NEWLINE_ANYCRLF | G_REGEX_MATCH_BSR_ANYCRLF,
                        G_REGEX_NEWLINE_ANYCRLF | G_REGEX_BSR_ANYCRLF,
                        G_REGEX_MATCH_NEWLINE_ANYCRLF | G_REGEX_MATCH_BSR_ANYCRLF);
  TEST_NEW_CHECK_FLAGS ("a", G_REGEX_RAW, 0, G_REGEX_RAW, 0);
  TEST_NEW_CHECK_FLAGS ("(?J)a", 0, 0, G_REGEX_DUPNAMES, 0);
  TEST_NEW_CHECK_FLAGS ("^.*", 0, 0, G_REGEX_ANCHORED, 0);
  TEST_NEW_CHECK_FLAGS ("(*UTF8)a", 0, 0, 0 /* this is the default in GRegex */, 0);
  TEST_NEW_CHECK_FLAGS ("(*UCP)a", 0, 0, 0 /* this always on in GRegex */, 0);
  TEST_NEW_CHECK_FLAGS ("(*CR)a", 0, 0, G_REGEX_NEWLINE_CR, 0);
  TEST_NEW_CHECK_FLAGS ("(*LF)a", 0, 0, G_REGEX_NEWLINE_LF, 0);
  TEST_NEW_CHECK_FLAGS ("(*CRLF)a", 0, 0, G_REGEX_NEWLINE_CRLF, 0);
  TEST_NEW_CHECK_FLAGS ("(*ANY)a", 0, 0, 0 /* this is the default in GRegex */, 0);
  TEST_NEW_CHECK_FLAGS ("(*ANYCRLF)a", 0, 0, G_REGEX_NEWLINE_ANYCRLF, 0);
  TEST_NEW_CHECK_FLAGS ("(*BSR_ANYCRLF)a", 0, 0, G_REGEX_BSR_ANYCRLF, 0);
  TEST_NEW_CHECK_FLAGS ("(*BSR_UNICODE)a", 0, 0, 0 /* this is the default in GRegex */, 0);
  TEST_NEW_CHECK_FLAGS ("(*NO_START_OPT)a", 0, 0, 0 /* not exposed in GRegex */, 0);
  /* Make sure we ignore deprecated G_REGEX_JAVASCRIPT_COMPAT */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  TEST_NEW_CHECK_FLAGS ("a", G_REGEX_JAVASCRIPT_COMPAT, 0, 0, 0);
G_GNUC_END_IGNORE_DEPRECATIONS

  /* TEST_NEW_FAIL(pattern, compile_opts, expected_error) */
  TEST_NEW_FAIL("(", 0, G_REGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL(")", 0, G_REGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL("[", 0, G_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS);
  TEST_NEW_FAIL("*", 0, G_REGEX_ERROR_NOTHING_TO_REPEAT);
  TEST_NEW_FAIL("?", 0, G_REGEX_ERROR_NOTHING_TO_REPEAT);
  TEST_NEW_FAIL("(?P<A>x)|(?P<A>y)", 0, G_REGEX_ERROR_DUPLICATE_SUBPATTERN_NAME);

  /* Check all GRegexError codes */
  TEST_NEW_FAIL ("a\\", 0, G_REGEX_ERROR_STRAY_BACKSLASH);
  TEST_NEW_FAIL ("a\\c", 0, G_REGEX_ERROR_MISSING_CONTROL_CHAR);
  TEST_NEW_FAIL ("a\\l", 0, G_REGEX_ERROR_UNRECOGNIZED_ESCAPE);
  TEST_NEW_FAIL ("a{4,2}", 0, G_REGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER);
  TEST_NEW_FAIL ("a{999999,}", 0, G_REGEX_ERROR_QUANTIFIER_TOO_BIG);
  TEST_NEW_FAIL ("[a-z", 0, G_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS);
  TEST_NEW_FAIL ("[\\B]", 0, G_REGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS);
  TEST_NEW_FAIL ("[z-a]", 0, G_REGEX_ERROR_RANGE_OUT_OF_ORDER);
  TEST_NEW_FAIL ("^[[:alnum:]-_.]+$", 0, G_REGEX_ERROR_COMPILE);
  TEST_NEW_FAIL ("{2,4}", 0, G_REGEX_ERROR_NOTHING_TO_REPEAT);
  TEST_NEW_FAIL ("a(?u)", 0, G_REGEX_ERROR_UNRECOGNIZED_CHARACTER);
  TEST_NEW_FAIL ("a(?<$foo)bar", 0, G_REGEX_ERROR_MISSING_SUBPATTERN_NAME);
  TEST_NEW_FAIL ("a[:alpha:]b", 0, G_REGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS);
  TEST_NEW_FAIL ("a(b", 0, G_REGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("a)b", 0, G_REGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("a(?R", 0, G_REGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("a(?-54", 0, G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE);
  TEST_NEW_FAIL ("(ab\\2)", 0, G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE);
  TEST_NEW_FAIL ("a(?#abc", 0, G_REGEX_ERROR_UNTERMINATED_COMMENT);
  TEST_NEW_FAIL ("(?<=a+)b", 0, G_REGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND);
  TEST_NEW_FAIL ("(?(1?)a|b)", 0, G_REGEX_ERROR_MALFORMED_CONDITION);
  TEST_NEW_FAIL ("(a)(?(1)a|b|c)", 0, G_REGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES);
  TEST_NEW_FAIL ("(?(?i))", 0, G_REGEX_ERROR_ASSERTION_EXPECTED);
  TEST_NEW_FAIL ("a[[:fubar:]]b", 0, G_REGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME);
  TEST_NEW_FAIL ("[[.ch.]]", 0, G_REGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED);
  TEST_NEW_FAIL ("\\x{110000}", 0, G_REGEX_ERROR_HEX_CODE_TOO_LARGE);
  TEST_NEW_FAIL ("^(?(0)f|b)oo", 0, G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE);
  TEST_NEW_FAIL ("(?<=\\C)X", 0, G_REGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND);
  TEST_NEW ("(?!\\w)(?R)", 0, 0);
  TEST_NEW_FAIL ("(?(?<ab))", 0, G_REGEX_ERROR_ASSERTION_EXPECTED);
  TEST_NEW_FAIL ("(?P<sub>foo)\\g<sub", 0, G_REGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR);
  TEST_NEW_FAIL ("(?P<x>eks)(?P<x>eccs)", 0, G_REGEX_ERROR_DUPLICATE_SUBPATTERN_NAME);
  TEST_NEW_FAIL ("\\666", G_REGEX_RAW, G_REGEX_ERROR_INVALID_OCTAL_VALUE);
  TEST_NEW_FAIL ("^(?(DEFINE) abc | xyz ) ", 0, G_REGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE);
  TEST_NEW_FAIL ("a", G_REGEX_NEWLINE_CRLF | G_REGEX_NEWLINE_ANYCRLF, G_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS);
  TEST_NEW_FAIL ("^(a)\\g\"3", 0, G_REGEX_ERROR_MISSING_BACK_REFERENCE);
  TEST_NEW_FAIL ("^(a)\\g{3", 0, G_REGEX_ERROR_MISSING_BACK_REFERENCE);
  TEST_NEW_FAIL ("^(a)\\g{0}", 0, G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE);
  TEST_NEW ("abc(*FAIL:123)xyz", 0, 0);
  TEST_NEW_FAIL ("a(*FOOBAR)b", 0, G_REGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB);
  if (pcre2_ge (10, 37))
    {
      TEST_NEW ("(?i:A{1,}\\6666666666)", 0, 0);
    }
  TEST_NEW_FAIL ("(?<a>)(?&)", 0, G_REGEX_ERROR_MISSING_SUBPATTERN_NAME);
  TEST_NEW_FAIL ("(?+-a)", 0, G_REGEX_ERROR_INVALID_RELATIVE_REFERENCE);
  TEST_NEW_FAIL ("(?|(?<a>A)|(?<b>B))", 0, G_REGEX_ERROR_EXTRA_SUBPATTERN_NAME);
  TEST_NEW_FAIL ("a(*MARK)b", 0, G_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED);
  TEST_NEW_FAIL ("^\\c€", 0, G_REGEX_ERROR_INVALID_CONTROL_CHAR);
  TEST_NEW_FAIL ("\\k", 0, G_REGEX_ERROR_MISSING_NAME);
  TEST_NEW_FAIL ("a[\\NB]c", 0, G_REGEX_ERROR_NOT_SUPPORTED_IN_CLASS);
  TEST_NEW_FAIL ("(*:0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEFG)XX", 0, G_REGEX_ERROR_NAME_TOO_LONG);
  /* See https://gitlab.gnome.org/GNOME/gtksourceview/-/issues/278 */
  TEST_NEW_FAIL ("(?i-x)((?:(?i-x)[^\\x00\\t\\n\\f\\r \"'/<=>\\x{007F}-\\x{009F}" \
                 "\\x{FDD0}-\\x{FDEF}\\x{FFFE}\\x{FFFF}\\x{1FFFE}\\x{1FFFF}" \
                 "\\x{2FFFE}\\x{2FFFF}\\x{3FFFE}\\x{3FFFF}\\x{4FFFE}\\x{4FFFF}" \
                 "\\x{5FFFE}\\x{5FFFF}\\x{6FFFE}\\x{6FFFF}\\x{7FFFE}\\x{7FFFF}" \
                 "\\x{8FFFE}\\x{8FFFF}\\x{9FFFE}\\x{9FFFF}\\x{AFFFE}\\x{AFFFF}" \
                 "\\x{BFFFE}\\x{BFFFF}\\x{CFFFE}\\x{CFFFF}\\x{DFFFE}\\x{DFFFF}" \
                 "\\x{EFFFE}\\x{EFFFF}\\x{FFFFE}\\x{FFFFF}\\x{10FFFE}\\x{10FFFF}]+)" \
                 "\\s*=\\s*)(\\\")",
                 G_REGEX_RAW, G_REGEX_ERROR_HEX_CODE_TOO_LARGE);

  /* These errors can't really be tested easily:
   * G_REGEX_ERROR_EXPRESSION_TOO_LARGE
   * G_REGEX_ERROR_MEMORY_ERROR
   * G_REGEX_ERROR_SUBPATTERN_NAME_TOO_LONG
   * G_REGEX_ERROR_TOO_MANY_SUBPATTERNS
   * G_REGEX_ERROR_TOO_MANY_FORWARD_REFERENCES
   *
   * These errors are obsolete and never raised by PCRE:
   * G_REGEX_ERROR_DEFINE_REPETION
   */

  /* TEST_MATCH_SIMPLE(pattern, string, compile_opts, match_opts, expected) */
  TEST_MATCH_SIMPLE("a", "", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("a", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("a", "ba", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("^a", "ba", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("a", "ba", G_REGEX_ANCHORED, 0, FALSE);
  TEST_MATCH_SIMPLE("a", "ba", 0, G_REGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH_SIMPLE("a", "ab", G_REGEX_ANCHORED, 0, TRUE);
  TEST_MATCH_SIMPLE("a", "ab", 0, G_REGEX_MATCH_ANCHORED, TRUE);
  TEST_MATCH_SIMPLE("a", "a", G_REGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE("a", "A", G_REGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE("\\C\\C", "ab", G_REGEX_OPTIMIZE | G_REGEX_RAW, 0, TRUE);
  TEST_MATCH_SIMPLE("^[[:alnum:]\\-_.]+$", "admin-foo", 0, 0, TRUE);
  /* These are needed to test extended properties. */
  TEST_MATCH_SIMPLE(AGRAVE, AGRAVE, G_REGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE(AGRAVE, AGRAVE_UPPER, G_REGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{L}", AGRAVE, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", AGRAVE_UPPER, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", SHEEN, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ll}", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Ll}", AGRAVE, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Ll}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ll}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Sc}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Sc}", EURO, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Sc}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", "1", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{N}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", ETH30, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Nd}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", "1", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Nd}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", "%", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Common}", "1", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", SHEEN, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", "%", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Latin}", AGRAVE, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Latin}", AGRAVE_UPPER, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Latin}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", "%", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", ETH30, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", "%", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{L}(?<=\\p{Arabic})", SHEEN, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}(?<=\\p{Latin})", SHEEN, 0, 0, FALSE);
  /* Invalid patterns. */
  TEST_MATCH_SIMPLE("\\", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("[", "", 0, 0, FALSE);

  /* Test that JIT compiler has enough stack */
  char test_string[TEST_STRING_LEN];
  memset (test_string, '*', TEST_STRING_LEN);
  test_string[TEST_STRING_LEN - 1] = '\0';
  TEST_MATCH_SIMPLE ("^(?:[ \t\n]|[^[:cntrl:]])*$", test_string, 0, 0, TRUE);

  /* Test that gregex falls back to unoptimized matching when reaching the JIT
   * compiler stack limit */
  char large_test_string[LARGE_TEST_STRING_LEN];
  memset (large_test_string, '*', LARGE_TEST_STRING_LEN);
  large_test_string[LARGE_TEST_STRING_LEN - 1] = '\0';
  TEST_MATCH_SIMPLE ("^(?:[ \t\n]|[^[:cntrl:]])*$", large_test_string, 0, 0, TRUE);

  /* TEST_MATCH(pattern, compile_opts, match_opts, string,
   * 		string_len, start_position, match_opts2, expected) */
  TEST_MATCH("a", 0, 0, "a", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, 0, "A", -1, 0, 0, FALSE);
  TEST_MATCH("a", G_REGEX_CASELESS, 0, "A", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, 0, "ab", -1, 1, 0, FALSE);
  TEST_MATCH("a", 0, 0, "ba", 1, 0, 0, FALSE);
  TEST_MATCH("a", 0, 0, "bab", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, 0, "b", -1, 0, 0, FALSE);
  TEST_MATCH("a", 0, G_REGEX_MATCH_ANCHORED, "a", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, G_REGEX_MATCH_ANCHORED, "ab", -1, 1, 0, FALSE);
  TEST_MATCH("a", 0, G_REGEX_MATCH_ANCHORED, "ba", 1, 0, 0, FALSE);
  TEST_MATCH("a", 0, G_REGEX_MATCH_ANCHORED, "bab", -1, 0, 0, FALSE);
  TEST_MATCH("a", 0, G_REGEX_MATCH_ANCHORED, "b", -1, 0, 0, FALSE);
  TEST_MATCH("a", 0, 0, "a", -1, 0, G_REGEX_MATCH_ANCHORED, TRUE);
  TEST_MATCH("a", 0, 0, "ab", -1, 1, G_REGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a", 0, 0, "ba", 1, 0, G_REGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a", 0, 0, "bab", -1, 0, G_REGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a", 0, 0, "b", -1, 0, G_REGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a|b", 0, 0, "a", -1, 0, 0, TRUE);
  TEST_MATCH("\\d", 0, 0, EURO, -1, 0, 0, FALSE);
  TEST_MATCH("^.$", 0, 0, EURO, -1, 0, 0, TRUE);
  TEST_MATCH("^.{3}$", 0, 0, EURO, -1, 0, 0, FALSE);
  TEST_MATCH("^.$", G_REGEX_RAW, 0, EURO, -1, 0, 0, FALSE);
  TEST_MATCH("^.{3}$", G_REGEX_RAW, 0, EURO, -1, 0, 0, TRUE);
  TEST_MATCH(AGRAVE, G_REGEX_CASELESS, 0, AGRAVE_UPPER, -1, 0, 0, TRUE);

  /* New lines handling. */
  TEST_MATCH("^a\\Rb$", 0, 0, "a\r\nb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\Rb$", 0, 0, "a\nb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\Rb$", 0, 0, "a\rb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\Rb$", 0, 0, "a\n\rb", -1, 0, 0, FALSE);
  TEST_MATCH("^a\\R\\Rb$", 0, 0, "a\n\rb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\nb$", 0, 0, "a\r\nb", -1, 0, 0, FALSE);
  TEST_MATCH("^a\\r\\nb$", 0, 0, "a\r\nb", -1, 0, 0, TRUE);

  TEST_MATCH("^b$", 0, 0, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, 0, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, 0, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, 0, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, 0, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_LF, 0, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CRLF, 0, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, 0, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_LF, 0, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CRLF, 0, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, 0, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_LF, 0, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CRLF, 0, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_ANYCRLF, 0, "a\r\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_ANYCRLF, 0, "a\r\nb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CR, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_LF, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CRLF, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CR, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_LF, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CRLF, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CR, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_LF, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CRLF, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANYCRLF, "a\r\nb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANYCRLF, "a\r\nb\nc", -1, 0, 0, TRUE);

  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_ANY, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_ANY, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_ANY, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_LF, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_LF, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_CRLF, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", G_REGEX_MULTILINE | G_REGEX_NEWLINE_CR, G_REGEX_MATCH_NEWLINE_CRLF, "a\rb\rc", -1, 0, 0, FALSE);

  /* See https://gitlab.gnome.org/GNOME/glib/-/issues/2729#note_1544130 */
  TEST_MATCH("^a$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, "a", -1, 0, 0, TRUE);
  TEST_MATCH("^a$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_LF, "a", -1, 0, 0, TRUE);
  TEST_MATCH("^a$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CR, "a", -1, 0, 0, TRUE);
  TEST_MATCH("^a$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_CRLF, "a", -1, 0, 0, TRUE);
  TEST_MATCH("^a$", G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANYCRLF, "a", -1, 0, 0, TRUE);

  TEST_MATCH("a#\nb", G_REGEX_EXTENDED, 0, "a", -1, 0, 0, FALSE);
  TEST_MATCH("a#\r\nb", G_REGEX_EXTENDED, 0, "a", -1, 0, 0, FALSE);
  TEST_MATCH("a#\rb", G_REGEX_EXTENDED, 0, "a", -1, 0, 0, FALSE);
  /* Due to PCRE2 only supporting newline settings passed to pcre2_compile (and
   * not to pcre2_match also), we have to compile the pattern with the
   * effective (combined from compile and match options) newline setting.
   * However, this setting also affects how newlines are interpreted *inside*
   * the pattern. With G_REGEX_EXTENDED, this changes where the comment
   * (started with `#`) ends.
   */
  /* On PCRE1, this test expected no match; on PCRE2 it matches because of the above. */
  TEST_MATCH("a#\nb", G_REGEX_EXTENDED, G_REGEX_MATCH_NEWLINE_CR, "a", -1, 0, 0, TRUE /*FALSE*/);
  TEST_MATCH("a#\nb", G_REGEX_EXTENDED | G_REGEX_NEWLINE_CR, 0, "a", -1, 0, 0, TRUE);

  TEST_MATCH("line\nbreak", G_REGEX_MULTILINE, 0, "this is a line\nbreak", -1, 0, 0, TRUE);
  TEST_MATCH("line\nbreak", G_REGEX_MULTILINE | G_REGEX_FIRSTLINE, 0, "first line\na line\nbreak", -1, 0, 0, FALSE);

  /* This failed with PCRE 7.2 (gnome bug #455640) */
  TEST_MATCH(".*$", 0, 0, "\xe1\xbb\x85", -1, 0, 0, TRUE);

  /* Test that othercasing in our pcre/glib integration is bug-for-bug compatible
   * with pcre's internal tables. Bug #678273 */
  TEST_MATCH("[Ǆ]", G_REGEX_CASELESS, 0, "Ǆ", -1, 0, 0, TRUE);
  TEST_MATCH("[Ǆ]", G_REGEX_CASELESS, 0, "ǆ", -1, 0, 0, TRUE);
  TEST_MATCH("[Ǆ]", G_REGEX_CASELESS, 0, "ǅ", -1, 0, 0, TRUE);

  /* see https://gitlab.gnome.org/GNOME/glib/-/issues/2700 */
  TEST_MATCH("(\n.+)+", G_REGEX_DEFAULT, 0, "\n \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n  \n", -1, 0, 0, TRUE);
  TEST_MATCH("\n([\\-\\.a-zA-Z]+[\\-\\.0-9]*) +connected ([^(\n ]*)[^\n]*((\n +[0-9]+x[0-9]+[^\n]+)+)", G_REGEX_DEFAULT, 0, "Screen 0: minimum 1 x 1, current 3840 x 1080, maximum 8192 x 8192\nVirtual1 connected primary 1920x1080+0+0 (normal left inverted right x axis y axis) 0mm x 0mm\n   1920x1080     60.00*+  59.96  \n   3840x2400     59.97  \n   3840x2160     59.97  \n   2880x1800     59.95  \n   2560x1600     59.99  \n   2560x1440     59.95  \n   1920x1440     60.00  \n   1856x1392     60.00  \n   1792x1344     60.00  \n   1920x1200     59.88  \n   1600x1200     60.00  \n   1680x1050     59.95  \n   1400x1050     59.98  \n   1280x1024     60.02  \n   1440x900      59.89  \n   1280x960      60.00  \n   1360x768      60.02  \n   1280x800      59.81  \n   1152x864      75.00  \n   1280x768      59.87  \n   1280x720      59.86  \n   1024x768      60.00  \n   800x600       60.32  \n   640x480       59.94  \nVirtual2 connected 1920x1080+1920+0 (normal left inverted right x axis y axis) 0mm x 0mm\n   1920x1080     60.00*+  59.96  \n   3840x2400     59.97  \n   3840x2160     59.97  \n   2880x1800     59.95  \n   2560x1600     59.99  \n   2560x1440     59.95  \n   1920x1440     60.00  \n   1856x1392     60.00  \n   1792x1344     60.00  \n   1920x1200     59.88  \n   1600x1200     60.00  \n   1680x1050     59.95  \n   1400x1050     59.98  \n   1280x1024     60.02  \n   1440x900      59.89  \n   1280x960      60.00  \n   1360x768      60.02  \n   1280x800      59.81  \n   1152x864      75.00  \n   1280x768      59.87  \n   1280x720      59.86  \n   1024x768      60.00  \n   800x600       60.32  \n   640x480       59.94  \nVirtual3 disconnected (normal left inverted right x axis y axis)\nVirtual4 disconnected (normal left inverted right x axis y axis)\nVirtual5 disconnected (normal left inverted right x axis y axis)\nVirtual6 disconnected (normal left inverted right x axis y axis)\nVirtual7 disconnected (normal left inverted right x axis y axis)\nVirtual8 disconnected (normal left inverted right x axis y axis)\n", -1, 0, 0, TRUE);

  /* TEST_MATCH_NEXT#(pattern, string, string_len, start_position, ...) */
  TEST_MATCH_NEXT0("a", "x", -1, 0);
  TEST_MATCH_NEXT0("a", "ax", -1, 1);
  TEST_MATCH_NEXT0("a", "xa", 1, 0);
  TEST_MATCH_NEXT0("a", "axa", 1, 2);
  TEST_MATCH_NEXT1("", "", -1, 0, "", 0, 0);
  TEST_MATCH_NEXT1("a", "a", -1, 0, "a", 0, 1);
  TEST_MATCH_NEXT1("a", "xax", -1, 0, "a", 1, 2);
  TEST_MATCH_NEXT1(EURO, ENG EURO, -1, 0, EURO, 2, 5);
  TEST_MATCH_NEXT1("a*", "", -1, 0, "", 0, 0);
  TEST_MATCH_NEXT2("", "a", -1, 0, "", 0, 0, "", 1, 1);
  TEST_MATCH_NEXT2("a*", "aa", -1, 0, "aa", 0, 2, "", 2, 2);
  TEST_MATCH_NEXT2(EURO "*", EURO EURO, -1, 0, EURO EURO, 0, 6, "", 6, 6);
  TEST_MATCH_NEXT2("a", "axa", -1, 0, "a", 0, 1, "a", 2, 3);
  TEST_MATCH_NEXT2("a+", "aaxa", -1, 0, "aa", 0, 2, "a", 3, 4);
  TEST_MATCH_NEXT2("a", "aa", -1, 0, "a", 0, 1, "a", 1, 2);
  TEST_MATCH_NEXT2("a", "ababa", -1, 2, "a", 2, 3, "a", 4, 5);
  TEST_MATCH_NEXT2(EURO "+", EURO "-" EURO, -1, 0, EURO, 0, 3, EURO, 4, 7);
  TEST_MATCH_NEXT3("", "ab", -1, 0, "", 0, 0, "", 1, 1, "", 2, 2);
  TEST_MATCH_NEXT3("", AGRAVE "b", -1, 0, "", 0, 0, "", 2, 2, "", 3, 3);
  TEST_MATCH_NEXT3("a", "aaxa", -1, 0, "a", 0, 1, "a", 1, 2, "a", 3, 4);
  TEST_MATCH_NEXT3("a", "aa" OGRAVE "a", -1, 0, "a", 0, 1, "a", 1, 2, "a", 4, 5);
  TEST_MATCH_NEXT3("a*", "aax", -1, 0, "aa", 0, 2, "", 2, 2, "", 3, 3);
  TEST_MATCH_NEXT3("(?=[A-Z0-9])", "RegExTest", -1, 0, "", 0, 0, "", 3, 3, "", 5, 5);
  TEST_MATCH_NEXT4("a*", "aaxa", -1, 0, "aa", 0, 2, "", 2, 2, "a", 3, 4, "", 4, 4);

  /* TEST_MATCH_COUNT(pattern, string, start_position, match_opts, expected_count) */
  TEST_MATCH_COUNT("a", "", 0, 0, 0);
  TEST_MATCH_COUNT("a", "a", 0, 0, 1);
  TEST_MATCH_COUNT("a", "a", 1, 0, 0);
  TEST_MATCH_COUNT("(.)", "a", 0, 0, 2);
  TEST_MATCH_COUNT("(.)", EURO, 0, 0, 2);
  TEST_MATCH_COUNT("(?:.)", "a", 0, 0, 1);
  TEST_MATCH_COUNT("(?P<A>.)", "a", 0, 0, 2);
  TEST_MATCH_COUNT("a$", "a", 0, G_REGEX_MATCH_NOTEOL, 0);
  TEST_MATCH_COUNT("(a)?(b)", "b", 0, 0, 3);
  TEST_MATCH_COUNT("(a)?(b)", "ab", 0, 0, 3);

  /* TEST_PARTIAL(pattern, string, expected, expected_start, expected_end), no JIT */
  TEST_PARTIAL("^ab", "a", G_REGEX_DEFAULT, "a", 0, 1);
  TEST_PARTIAL("^ab", "xa", G_REGEX_DEFAULT, NULL, 0, 0);
  TEST_PARTIAL("ab", "xa", G_REGEX_DEFAULT, "a", 1, 2);
  TEST_PARTIAL("ab", "ab", G_REGEX_DEFAULT, NULL, 0, 0); /* normal match. */
  TEST_PARTIAL("a+b", "aa", G_REGEX_DEFAULT, "aa", 0, 2);
  TEST_PARTIAL("(a)+b", "aa", G_REGEX_DEFAULT, "aa", 0, 2);
  TEST_PARTIAL("a?b", "a", G_REGEX_DEFAULT, "a", 0, 1);

  /* TEST_PARTIAL(pattern, string, expected, expected_start, expected_end) with JIT */
  TEST_PARTIAL("^ab", "a", G_REGEX_OPTIMIZE, "a", 0, 1);
  TEST_PARTIAL("^ab", "xa", G_REGEX_OPTIMIZE, NULL, 0, 0);
  TEST_PARTIAL("ab", "xa", G_REGEX_OPTIMIZE, "a", 1, 2);
  TEST_PARTIAL("ab", "ab", G_REGEX_OPTIMIZE, NULL, 0, 0); /* normal match. */
  TEST_PARTIAL("a+b", "aa", G_REGEX_OPTIMIZE, "aa", 0, 2);
  TEST_PARTIAL("(a)+b", "aa", G_REGEX_OPTIMIZE, "aa", 0, 2);
  TEST_PARTIAL("a?b", "a", G_REGEX_OPTIMIZE, "a", 0, 1);

  /* Test soft vs. hard partial matching, no JIT */
  TEST_PARTIAL_FULL("cat(fish)?", "cat", G_REGEX_DEFAULT, G_REGEX_MATCH_PARTIAL_SOFT, NULL, 0, 0); /* normal match */
  TEST_PARTIAL_FULL("cat(fish)?", "cat", G_REGEX_DEFAULT, G_REGEX_MATCH_PARTIAL_HARD, "cat", 0, 3);
  TEST_PARTIAL_FULL("ab+", "ab", G_REGEX_DEFAULT, G_REGEX_MATCH_PARTIAL_SOFT, NULL, 0, 0); /* normal match */
  TEST_PARTIAL_FULL("ab+", "ab", G_REGEX_DEFAULT, G_REGEX_MATCH_PARTIAL_HARD, "ab", 0, 2);

  /* Test soft vs. hard partial matching with JIT */
  TEST_PARTIAL_FULL("cat(fish)?", "cat", G_REGEX_OPTIMIZE, G_REGEX_MATCH_PARTIAL_SOFT, NULL, 0, 0); /* normal match */
  TEST_PARTIAL_FULL("cat(fish)?", "cat", G_REGEX_OPTIMIZE, G_REGEX_MATCH_PARTIAL_HARD, "cat", 0, 3);
  TEST_PARTIAL_FULL("ab+", "ab", G_REGEX_OPTIMIZE, G_REGEX_MATCH_PARTIAL_SOFT, NULL, 0, 0); /* normal match */
  TEST_PARTIAL_FULL("ab+", "ab", G_REGEX_OPTIMIZE, G_REGEX_MATCH_PARTIAL_HARD, "ab", 0, 2);

  /* TEST_SUB_PATTERN(pattern, string, start_position, sub_n, expected_sub,
   * 		      expected_start, expected_end) */
  TEST_SUB_PATTERN("a", "a", 0, 0, "a", 0, 1);
  TEST_SUB_PATTERN("a(.)", "ab", 0, 1, "b", 1, 2);
  TEST_SUB_PATTERN("a(.)", "a" EURO, 0, 1, EURO, 1, 4);
  TEST_SUB_PATTERN("(?:.*)(a)(.)", "xxa" ENG, 0, 2, ENG, 3, 5);
  TEST_SUB_PATTERN("(" HSTROKE ")", "a" HSTROKE ENG, 0, 1, HSTROKE, 1, 3);
  TEST_SUB_PATTERN("a", "a", 0, 1, NULL, UNTOUCHED, UNTOUCHED);
  TEST_SUB_PATTERN("a", "a", 0, 1, NULL, UNTOUCHED, UNTOUCHED);
  TEST_SUB_PATTERN("(a)?(b)", "b", 0, 0, "b", 0, 1);
  TEST_SUB_PATTERN("(a)?(b)", "b", 0, 1, "", -1, -1);
  TEST_SUB_PATTERN("(a)?(b)", "b", 0, 2, "b", 0, 1);
  TEST_SUB_PATTERN("(a)?b", "b", 0, 0, "b", 0, 1);
  TEST_SUB_PATTERN("(a)?b", "b", 0, 1, "", -1, -1);
  TEST_SUB_PATTERN("(a)?b", "b", 0, 2, NULL, UNTOUCHED, UNTOUCHED);

  /* TEST_NAMED_SUB_PATTERN(pattern, string, start_position, sub_name,
   * 			    expected_sub, expected_start, expected_end) */
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "ab", 0, "A", "b", 1, 2);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "aab", 1, "A", "b", 2, 3);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", EURO "ab", 0, "A", "b", 4, 5);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", EURO "ab", 0, "B", "", -1, -1);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", EURO "ab", 0, "C", NULL, UNTOUCHED, UNTOUCHED);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "a" EGRAVE "x", 0, "A", EGRAVE, 1, 3);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "a" EGRAVE "x", 0, "B", "x", 3, 4);
  TEST_NAMED_SUB_PATTERN("(?P<A>a)?(?P<B>b)", "b", 0, "A", "", -1, -1);
  TEST_NAMED_SUB_PATTERN("(?P<A>a)?(?P<B>b)", "b", 0, "B", "b", 0, 1);

  /* TEST_NAMED_SUB_PATTERN_DUPNAMES(pattern, string, start_position, sub_name,
   *				     expected_sub, expected_start, expected_end) */
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>a)|(?P<N>b)", "ab", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>aa)|(?P<N>a)", "aa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>aa)(?P<N>a)", "aaa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>x)|(?P<N>a)", "a", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>x)y|(?P<N>a)b", "ab", 0, "N", "a", 0, 1);

  /* DUPNAMES option inside the pattern */
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>a)|(?P<N>b)", "ab", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>aa)|(?P<N>a)", "aa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>aa)(?P<N>a)", "aaa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>x)|(?P<N>a)", "a", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>x)y|(?P<N>a)b", "ab", 0, "N", "a", 0, 1);

  /* TEST_FETCH_ALL#(pattern, string, match_opts, ...) */
  TEST_FETCH_ALL0("a", "", 0);
  TEST_FETCH_ALL0("a", "b", 0);
  TEST_FETCH_ALL1("a", "a", 0, "a");
  TEST_FETCH_ALL1("a+", "aa", 0, "aa");
  TEST_FETCH_ALL1("(?:a)", "a", 0, "a");
  TEST_FETCH_ALL1("(a)bc", "ab", G_REGEX_MATCH_PARTIAL_SOFT, "ab");
  TEST_FETCH_ALL1("(a)bc", "ab", G_REGEX_MATCH_PARTIAL_HARD, "ab");
  TEST_FETCH_ALL1("(a)b+", "ab", G_REGEX_MATCH_PARTIAL_HARD, "ab");
  TEST_FETCH_ALL2("(a)b+", "ab", G_REGEX_MATCH_PARTIAL_SOFT, "ab", "a"); /* normal match */
  TEST_FETCH_ALL2("(a)", "a", 0, "a", "a");
  TEST_FETCH_ALL2("a(.)", "ab", 0, "ab", "b");
  TEST_FETCH_ALL2("a(.)", "a" HSTROKE, 0, "a" HSTROKE, HSTROKE);
  TEST_FETCH_ALL3("(?:.*)(a)(.)", "xyazk", 0, "xyaz", "a", "z");
  TEST_FETCH_ALL3("(?P<A>.)(a)", "xa", 0, "xa", "x", "a");
  TEST_FETCH_ALL3("(?P<A>.)(a)", ENG "a", 0, ENG "a", ENG, "a");
  TEST_FETCH_ALL3("(a)?(b)", "b", 0, "b", "", "b");
  TEST_FETCH_ALL3("(a)?(b)", "ab", 0, "ab", "a", "b");

  /* TEST_SPLIT_SIMPLE#(pattern, string, ...) */
  TEST_SPLIT_SIMPLE0("", "");
  TEST_SPLIT_SIMPLE0("a", "");
  TEST_SPLIT_SIMPLE1(",", "a", "a");
  TEST_SPLIT_SIMPLE1("(,)\\s*", "a", "a");
  TEST_SPLIT_SIMPLE2(",", "a,b", "a", "b");
  TEST_SPLIT_SIMPLE3(",", "a,b,c", "a", "b", "c");
  TEST_SPLIT_SIMPLE3(",\\s*", "a,b,c", "a", "b", "c");
  TEST_SPLIT_SIMPLE3(",\\s*", "a, b, c", "a", "b", "c");
  TEST_SPLIT_SIMPLE3("(,)\\s*", "a,b", "a", ",", "b");
  TEST_SPLIT_SIMPLE3("(,)\\s*", "a, b", "a", ",", "b");
  TEST_SPLIT_SIMPLE2("\\s", "ab c", "ab", "c");
  TEST_SPLIT_SIMPLE3("\\s*", "ab c", "a", "b", "c");
  /* Not matched sub-strings. */
  TEST_SPLIT_SIMPLE2("a|(b)", "xay", "x", "y");
  TEST_SPLIT_SIMPLE3("a|(b)", "xby", "x", "b", "y");
  /* Empty matches. */
  TEST_SPLIT_SIMPLE3("", "abc", "a", "b", "c");
  TEST_SPLIT_SIMPLE3(" *", "ab c", "a", "b", "c");
  /* Invalid patterns. */
  TEST_SPLIT_SIMPLE0("\\", "");
  TEST_SPLIT_SIMPLE0("[", "");

  /* TEST_SPLIT#(pattern, string, start_position, max_tokens, ...) */
  TEST_SPLIT0("", "", 0, 0);
  TEST_SPLIT0("a", "", 0, 0);
  TEST_SPLIT0("a", "", 0, 1);
  TEST_SPLIT0("a", "", 0, 2);
  TEST_SPLIT0("a", "a", 1, 0);
  TEST_SPLIT1(",", "a", 0, 0, "a");
  TEST_SPLIT1(",", "a,b", 0, 1, "a,b");
  TEST_SPLIT1("(,)\\s*", "a", 0, 0, "a");
  TEST_SPLIT1(",", "a,b", 2, 0, "b");
  TEST_SPLIT2(",", "a,b", 0, 0, "a", "b");
  TEST_SPLIT2(",", "a,b,c", 0, 2, "a", "b,c");
  TEST_SPLIT2(",", "a,b", 1, 0, "", "b");
  TEST_SPLIT2(",", "a,", 0, 0, "a", "");
  TEST_SPLIT3(",", "a,b,c", 0, 0, "a", "b", "c");
  TEST_SPLIT3(",\\s*", "a,b,c", 0, 0, "a", "b", "c");
  TEST_SPLIT3(",\\s*", "a, b, c", 0, 0, "a", "b", "c");
  TEST_SPLIT3("(,)\\s*", "a,b", 0, 0, "a", ",", "b");
  TEST_SPLIT3("(,)\\s*", "a, b", 0, 0, "a", ",", "b");
  /* Not matched sub-strings. */
  TEST_SPLIT2("a|(b)", "xay", 0, 0, "x", "y");
  TEST_SPLIT3("a|(b)", "xby", 0, -1, "x", "b", "y");
  /* Empty matches. */
  TEST_SPLIT2(" *", "ab c", 1, 0, "b", "c");
  TEST_SPLIT3("", "abc", 0, 0, "a", "b", "c");
  TEST_SPLIT3(" *", "ab c", 0, 0, "a", "b", "c");
  TEST_SPLIT1(" *", "ab c", 0, 1, "ab c");
  TEST_SPLIT2(" *", "ab c", 0, 2, "a", "b c");
  TEST_SPLIT3(" *", "ab c", 0, 3, "a", "b", "c");
  TEST_SPLIT3(" *", "ab c", 0, 4, "a", "b", "c");

  /* TEST_CHECK_REPLACEMENT(string_to_expand, expected, expected_refs) */
  TEST_CHECK_REPLACEMENT("", TRUE, FALSE);
  TEST_CHECK_REPLACEMENT("a", TRUE, FALSE);
  TEST_CHECK_REPLACEMENT("\\t\\n\\v\\r\\f\\a\\b\\\\\\x{61}", TRUE, FALSE);
  TEST_CHECK_REPLACEMENT("\\0", TRUE, TRUE);
  TEST_CHECK_REPLACEMENT("\\n\\2", TRUE, TRUE);
  TEST_CHECK_REPLACEMENT("\\g<foo>", TRUE, TRUE);
  /* Invalid strings */
  TEST_CHECK_REPLACEMENT("\\Q", FALSE, FALSE);
  TEST_CHECK_REPLACEMENT("x\\Ay", FALSE, FALSE);

  /* TEST_EXPAND(pattern, string, string_to_expand, raw, expected) */
  TEST_EXPAND("a", "a", "", FALSE, "");
  TEST_EXPAND("a", "a", "\\0", FALSE, "a");
  TEST_EXPAND("a", "a", "\\1", FALSE, "");
  TEST_EXPAND("(a)", "ab", "\\1", FALSE, "a");
  TEST_EXPAND("(a)", "a", "\\1", FALSE, "a");
  TEST_EXPAND("(a)", "a", "\\g<1>", FALSE, "a");
  TEST_EXPAND("a", "a", "\\0130", FALSE, "X");
  TEST_EXPAND("a", "a", "\\\\\\0", FALSE, "\\a");
  TEST_EXPAND("a(?P<G>.)c", "xabcy", "X\\g<G>X", FALSE, "XbX");
  TEST_EXPAND(".", EURO, "\\0", FALSE, EURO);
  TEST_EXPAND("(.)", EURO, "\\1", FALSE, EURO);
  TEST_EXPAND("(?P<G>.)", EURO, "\\g<G>", FALSE, EURO);
  TEST_EXPAND(".", "a", EURO, FALSE, EURO);
  TEST_EXPAND(".", "a", EURO "\\0", FALSE, EURO "a");
  TEST_EXPAND(".", "", "\\Lab\\Ec", FALSE, "abc");
  TEST_EXPAND(".", "", "\\LaB\\EC", FALSE, "abC");
  TEST_EXPAND(".", "", "\\Uab\\Ec", FALSE, "ABc");
  TEST_EXPAND(".", "", "a\\ubc", FALSE, "aBc");
  TEST_EXPAND(".", "", "a\\lbc", FALSE, "abc");
  TEST_EXPAND(".", "", "A\\uBC", FALSE, "ABC");
  TEST_EXPAND(".", "", "A\\lBC", FALSE, "AbC");
  TEST_EXPAND(".", "", "A\\l\\\\BC", FALSE, "A\\BC");
  TEST_EXPAND(".", "", "\\L" AGRAVE "\\E", FALSE, AGRAVE);
  TEST_EXPAND(".", "", "\\U" AGRAVE "\\E", FALSE, AGRAVE_UPPER);
  TEST_EXPAND(".", "", "\\u" AGRAVE "a", FALSE, AGRAVE_UPPER "a");
  TEST_EXPAND(".", "ab", "x\\U\\0y\\Ez", FALSE, "xAYz");
  TEST_EXPAND(".(.)", "AB", "x\\L\\1y\\Ez", FALSE, "xbyz");
  TEST_EXPAND(".", "ab", "x\\u\\0y\\Ez", FALSE, "xAyz");
  TEST_EXPAND(".(.)", "AB", "x\\l\\1y\\Ez", FALSE, "xbyz");
  TEST_EXPAND(".(.)", "a" AGRAVE_UPPER, "x\\l\\1y", FALSE, "x" AGRAVE "y");
  TEST_EXPAND("a", "bab", "\\x{61}", FALSE, "a");
  TEST_EXPAND("a", "bab", "\\x61", FALSE, "a");
  TEST_EXPAND("a", "bab", "\\x5a", FALSE, "Z");
  TEST_EXPAND("a", "bab", "\\0\\x5A", FALSE, "aZ");
  TEST_EXPAND("a", "bab", "\\1\\x{5A}", FALSE, "Z");
  TEST_EXPAND("a", "bab", "\\x{00E0}", FALSE, AGRAVE);
  TEST_EXPAND("", "bab", "\\x{0634}", FALSE, SHEEN);
  TEST_EXPAND("", "bab", "\\x{634}", FALSE, SHEEN);
  TEST_EXPAND("", "", "\\t", FALSE, "\t");
  TEST_EXPAND("", "", "\\v", FALSE, "\v");
  TEST_EXPAND("", "", "\\r", FALSE, "\r");
  TEST_EXPAND("", "", "\\n", FALSE, "\n");
  TEST_EXPAND("", "", "\\f", FALSE, "\f");
  TEST_EXPAND("", "", "\\a", FALSE, "\a");
  TEST_EXPAND("", "", "\\b", FALSE, "\b");
  TEST_EXPAND("a(.)", "abc", "\\0\\b\\1", FALSE, "ab\bb");
  TEST_EXPAND("a(.)", "abc", "\\0141", FALSE, "a");
  TEST_EXPAND("a(.)", "abc", "\\078", FALSE, "\a8");
  TEST_EXPAND("a(.)", "abc", "\\077", FALSE, "?");
  TEST_EXPAND("a(.)", "abc", "\\0778", FALSE, "?8");
  TEST_EXPAND("a(.)", "a" AGRAVE "b", "\\1", FALSE, AGRAVE);
  TEST_EXPAND("a(.)", "a" AGRAVE "b", "\\1", TRUE, "\xc3");
  TEST_EXPAND("a(.)", "a" AGRAVE "b", "\\0", TRUE, "a\xc3");
  /* Invalid strings. */
  TEST_EXPAND("", "", "\\Q", FALSE, NULL);
  TEST_EXPAND("", "", "x\\Ay", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<>", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<1a>", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<a$>", FALSE, NULL);
  TEST_EXPAND("", "", "\\", FALSE, NULL);
  TEST_EXPAND("a", "a", "\\x{61", FALSE, NULL);
  TEST_EXPAND("a", "a", "\\x6X", FALSE, NULL);
  /* Pattern-less. */
  TEST_EXPAND(NULL, NULL, "", FALSE, "");
  TEST_EXPAND(NULL, NULL, "\\n", FALSE, "\n");
  /* Invalid strings */
  TEST_EXPAND(NULL, NULL, "\\Q", FALSE, NULL);
  TEST_EXPAND(NULL, NULL, "x\\Ay", FALSE, NULL);

  /* TEST_REPLACE(pattern, string, start_position, replacement, expected) */
  TEST_REPLACE("a", "ababa", 0, "A", "AbAbA");
  TEST_REPLACE("a", "ababa", 1, "A", "abAbA");
  TEST_REPLACE("a", "ababa", 2, "A", "abAbA");
  TEST_REPLACE("a", "ababa", 3, "A", "ababA");
  TEST_REPLACE("a", "ababa", 4, "A", "ababA");
  TEST_REPLACE("a", "ababa", 5, "A", "ababa");
  TEST_REPLACE("a", "ababa", 6, "A", "ababa");
  TEST_REPLACE("a", "abababa", 2, "A", "abAbAbA");
  TEST_REPLACE("a", "abab", 0, "A", "AbAb");
  TEST_REPLACE("a", "baba", 0, "A", "bAbA");
  TEST_REPLACE("a", "bab", 0, "A", "bAb");
  TEST_REPLACE("$^", "abc", 0, "X", "abc");
  TEST_REPLACE("(.)a", "ciao", 0, "a\\1", "caio");
  TEST_REPLACE("a.", "abc", 0, "\\0\\0", "ababc");
  TEST_REPLACE("a", "asd", 0, "\\0101", "Asd");
  TEST_REPLACE("(a).\\1", "aba cda", 0, "\\1\\n", "a\n cda");
  TEST_REPLACE("a" AGRAVE "a", "a" AGRAVE "a", 0, "x", "x");
  TEST_REPLACE("a" AGRAVE "a", "a" AGRAVE "a", 0, OGRAVE, OGRAVE);
  TEST_REPLACE("[^-]", "-" EURO "-x-" HSTROKE, 0, "a", "-a-a-a");
  TEST_REPLACE("[^-]", "-" EURO "-" HSTROKE, 0, "a\\g<0>a", "-a" EURO "a-a" HSTROKE "a");
  TEST_REPLACE("-", "-" EURO "-" HSTROKE, 0, "", EURO HSTROKE);
  TEST_REPLACE(".*", "hello", 0, "\\U\\0\\E", "HELLO");
  TEST_REPLACE(".*", "hello", 0, "\\u\\0", "Hello");
  TEST_REPLACE("\\S+", "hello world", 0, "\\U-\\0-", "-HELLO- -WORLD-");
  TEST_REPLACE(".", "a", 0, "\\A", NULL);
  TEST_REPLACE(".", "a", 0, "\\g", NULL);
  TEST_REPLACE_OPTIONS("(\\w+)#(\\w+)", "aa#bb cc#dd", 0, "\\2#\\1", "bb#aa dd#cc",
                       G_REGEX_OPTIMIZE|G_REGEX_MULTILINE|G_REGEX_CASELESS,
                       0);
  TEST_REPLACE_OPTIONS("(\\w+)#(\\w+)", "aa#bb cc#dd", 0, "\\2#\\1", "bb#aa cc#dd",
                       G_REGEX_OPTIMIZE|G_REGEX_MULTILINE|G_REGEX_CASELESS,
                       G_REGEX_MATCH_ANCHORED);

  /* TEST_REPLACE_LIT(pattern, string, start_position, replacement, expected) */
  TEST_REPLACE_LIT("a", "ababa", 0, "A", "AbAbA");
  TEST_REPLACE_LIT("a", "ababa", 1, "A", "abAbA");
  TEST_REPLACE_LIT("a", "ababa", 2, "A", "abAbA");
  TEST_REPLACE_LIT("a", "ababa", 3, "A", "ababA");
  TEST_REPLACE_LIT("a", "ababa", 4, "A", "ababA");
  TEST_REPLACE_LIT("a", "ababa", 5, "A", "ababa");
  TEST_REPLACE_LIT("a", "ababa", 6, "A", "ababa");
  TEST_REPLACE_LIT("a", "abababa", 2, "A", "abAbAbA");
  TEST_REPLACE_LIT("a", "abcadaa", 0, "A", "AbcAdAA");
  TEST_REPLACE_LIT("$^", "abc", 0, "X", "abc");
  TEST_REPLACE_LIT("(.)a", "ciao", 0, "a\\1", "ca\\1o");
  TEST_REPLACE_LIT("a.", "abc", 0, "\\0\\0\\n", "\\0\\0\\nc");
  TEST_REPLACE_LIT("a" AGRAVE "a", "a" AGRAVE "a", 0, "x", "x");
  TEST_REPLACE_LIT("a" AGRAVE "a", "a" AGRAVE "a", 0, OGRAVE, OGRAVE);
  TEST_REPLACE_LIT(AGRAVE, "-" AGRAVE "-" HSTROKE, 0, "a" ENG "a", "-a" ENG "a-" HSTROKE);
  TEST_REPLACE_LIT("[^-]", "-" EURO "-" AGRAVE "-" HSTROKE, 0, "a", "-a-a-a");
  TEST_REPLACE_LIT("[^-]", "-" EURO "-" AGRAVE, 0, "a\\g<0>a", "-a\\g<0>a-a\\g<0>a");
  TEST_REPLACE_LIT("-", "-" EURO "-" AGRAVE "-" HSTROKE, 0, "", EURO AGRAVE HSTROKE);
  TEST_REPLACE_LIT("(?=[A-Z0-9])", "RegExTest", 0, "_", "_Reg_Ex_Test");
  TEST_REPLACE_LIT("(?=[A-Z0-9])", "RegExTest", 1, "_", "Reg_Ex_Test");

  /* TEST_GET_STRING_NUMBER(pattern, name, expected_num) */
  TEST_GET_STRING_NUMBER("", "A", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)", "B", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)", "B", 2);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)", "C", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)(?P<C>b)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)(?P<C>b)", "B", 2);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)(?P<C>b)", "C", 3);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)(?P<C>b)", "D", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(.)(?P<B>a)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(.)(?P<B>a)", "B", 3);
  TEST_GET_STRING_NUMBER("(?P<A>.)(.)(?P<B>a)", "C", -1);
  TEST_GET_STRING_NUMBER("(?:a)(?P<A>.)", "A", 1);
  TEST_GET_STRING_NUMBER("(?:a)(?P<A>.)", "B", -1);

  /* TEST_ESCAPE_NUL(string, length, expected) */
  TEST_ESCAPE_NUL("hello world", -1, "hello world");
  TEST_ESCAPE_NUL("hello\0world", -1, "hello");
  TEST_ESCAPE_NUL("\0world", -1, "");
  TEST_ESCAPE_NUL("hello world", 5, "hello");
  TEST_ESCAPE_NUL("hello.world", 11, "hello.world");
  TEST_ESCAPE_NUL("a(b\\b.$", 7, "a(b\\b.$");
  TEST_ESCAPE_NUL("hello\0", 6, "hello\\x00");
  TEST_ESCAPE_NUL("\0world", 6, "\\x00world");
  TEST_ESCAPE_NUL("\0\0", 2, "\\x00\\x00");
  TEST_ESCAPE_NUL("hello\0world", 11, "hello\\x00world");
  TEST_ESCAPE_NUL("hello\0world\0", 12, "hello\\x00world\\x00");
  TEST_ESCAPE_NUL("hello\\\0world", 12, "hello\\x00world");
  TEST_ESCAPE_NUL("hello\\\\\0world", 13, "hello\\\\\\x00world");
  TEST_ESCAPE_NUL("|()[]{}^$*+?.", 13, "|()[]{}^$*+?.");
  TEST_ESCAPE_NUL("|()[]{}^$*+?.\\\\", 15, "|()[]{}^$*+?.\\\\");

  /* TEST_ESCAPE(string, length, expected) */
  TEST_ESCAPE("hello world", -1, "hello world");
  TEST_ESCAPE("hello world", 5, "hello");
  TEST_ESCAPE("hello.world", -1, "hello\\.world");
  TEST_ESCAPE("a(b\\b.$", -1, "a\\(b\\\\b\\.\\$");
  TEST_ESCAPE("hello\0world", -1, "hello");
  TEST_ESCAPE("hello\0world", 11, "hello\\0world");
  TEST_ESCAPE(EURO "*" ENG, -1, EURO "\\*" ENG);
  TEST_ESCAPE("a$", -1, "a\\$");
  TEST_ESCAPE("$a", -1, "\\$a");
  TEST_ESCAPE("a$a", -1, "a\\$a");
  TEST_ESCAPE("$a$", -1, "\\$a\\$");
  TEST_ESCAPE("$a$", 0, "");
  TEST_ESCAPE("$a$", 1, "\\$");
  TEST_ESCAPE("$a$", 2, "\\$a");
  TEST_ESCAPE("$a$", 3, "\\$a\\$");
  TEST_ESCAPE("$a$", 4, "\\$a\\$\\0");
  TEST_ESCAPE("|()[]{}^$*+?.", -1, "\\|\\(\\)\\[\\]\\{\\}\\^\\$\\*\\+\\?\\.");
  TEST_ESCAPE("a|a(a)a[a]a{a}a^a$a*a+a?a.a", -1,
	      "a\\|a\\(a\\)a\\[a\\]a\\{a\\}a\\^a\\$a\\*a\\+a\\?a\\.a");

  /* TEST_MATCH_ALL#(pattern, string, string_len, start_position, ...) */
  TEST_MATCH_ALL0("<.*>", "", -1, 0);
  TEST_MATCH_ALL0("a+", "", -1, 0);
  TEST_MATCH_ALL0("a+", "a", 0, 0);
  TEST_MATCH_ALL0("a+", "a", -1, 1);
  TEST_MATCH_ALL1("<.*>", "<a>", -1, 0, "<a>", 0, 3);
  TEST_MATCH_ALL1("a+", "a", -1, 0, "a", 0, 1);
  TEST_MATCH_ALL1("a+", "aa", 1, 0, "a", 0, 1);
  TEST_MATCH_ALL1("a+", "aa", -1, 1, "a", 1, 2);
  TEST_MATCH_ALL1("a+", "aa", 2, 1, "a", 1, 2);
  TEST_MATCH_ALL1(".+", ENG, -1, 0, ENG, 0, 2);
  TEST_MATCH_ALL2("<.*>", "<a><b>", -1, 0, "<a><b>", 0, 6, "<a>", 0, 3);
  TEST_MATCH_ALL2("a+", "aa", -1, 0, "aa", 0, 2, "a", 0, 1);
  TEST_MATCH_ALL2(".+", ENG EURO, -1, 0, ENG EURO, 0, 5, ENG, 0, 2);
  TEST_MATCH_ALL3("<.*>", "<a><b><c>", -1, 0, "<a><b><c>", 0, 9,
		  "<a><b>", 0, 6, "<a>", 0, 3);
  TEST_MATCH_ALL3("a+", "aaa", -1, 0, "aaa", 0, 3, "aa", 0, 2, "a", 0, 1);

  /* NOTEMPTY matching */
  TEST_MATCH_NOTEMPTY("a?b?", "xyz", FALSE);
  TEST_MATCH_NOTEMPTY_ATSTART("a?b?", "xyz", TRUE);

  return g_test_run ();
}
