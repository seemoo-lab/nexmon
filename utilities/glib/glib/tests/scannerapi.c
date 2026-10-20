/* Gtk+ object tests
 * Copyright (C) 2007 Patrick Hulin
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
#include <string.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* GScanner fixture */
typedef struct {
  GScanner *scanner;
} ScannerFixture;
static void
scanner_fixture_setup (ScannerFixture *fix,
                       gconstpointer   test_data)
{
  fix->scanner = g_scanner_new (NULL);
  g_assert (fix->scanner != NULL);
}
static void
scanner_fixture_teardown (ScannerFixture *fix,
                          gconstpointer   test_data)
{
  g_assert (fix->scanner != NULL);
  g_scanner_destroy (fix->scanner);
}

static void
scanner_msg_func (GScanner *scanner,
                  gchar    *message,
                  gboolean  error)
{
  g_assert_cmpstr (message, ==, "test");
}

static void
test_scanner_warn (ScannerFixture *fix,
                   gconstpointer   test_data)
{
  fix->scanner->msg_handler = scanner_msg_func;
  g_scanner_warn (fix->scanner, "test");
}

static void
test_scanner_error (ScannerFixture *fix,
                    gconstpointer   test_data)
{
  if (g_test_subprocess ())
    {
      int pe = fix->scanner->parse_errors;
      g_scanner_error (fix->scanner, "scanner-error-message-test");
      g_assert_cmpint (fix->scanner->parse_errors, ==, pe + 1);
      exit (0);
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*scanner-error-message-test*");
}

static void
check_keys (gpointer key,
            gpointer value,
            gpointer user_data)
{
  g_assert_cmpint (GPOINTER_TO_INT (value), ==, g_ascii_strtoull (key, NULL, 0));
}

static void
test_scanner_symbols (ScannerFixture *fix,
                      gconstpointer   test_data)
{
  gint i;
  gchar buf[2];

  g_scanner_set_scope (fix->scanner, 1);

  for (i = 0; i < 10; i++)
    g_scanner_scope_add_symbol (fix->scanner,
                                1,
                                g_ascii_dtostr (buf, 2, (gdouble)i),
                                GINT_TO_POINTER (i));
  g_scanner_scope_foreach_symbol (fix->scanner, 1, check_keys, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (g_scanner_lookup_symbol (fix->scanner, "5")), ==, 5);
  g_scanner_scope_remove_symbol (fix->scanner, 1, "5");
  g_assert (g_scanner_lookup_symbol (fix->scanner, "5") == NULL);

  g_assert_cmpint (GPOINTER_TO_INT (g_scanner_scope_lookup_symbol (fix->scanner, 1, "4")), ==, 4);
  g_assert_cmpint (GPOINTER_TO_INT (g_scanner_scope_lookup_symbol (fix->scanner, 1, "5")), ==, 0);
}

static void
test_scanner_tokens (ScannerFixture *fix,
                     gconstpointer   test_data)
{
  gchar buf[] = "(\t\n\r\\){}";
  const size_t buflen = strlen (buf);
  gchar tokbuf[] = "(\\){}";
  const gsize tokbuflen = strlen (tokbuf);
  gsize i;

  g_scanner_input_text (fix->scanner, buf, buflen);

  g_assert_cmpint (g_scanner_cur_token (fix->scanner), ==, G_TOKEN_NONE);
  g_scanner_get_next_token (fix->scanner);
  g_assert_cmpint (g_scanner_cur_token (fix->scanner), ==, tokbuf[0]);
  g_assert_cmpint (g_scanner_cur_line (fix->scanner), ==, 1);

  for (i = 1; i < tokbuflen; i++)
    g_assert_cmpint (g_scanner_get_next_token (fix->scanner), ==, tokbuf[i]);
  g_assert_cmpint (g_scanner_get_next_token (fix->scanner), ==, G_TOKEN_EOF);
  return;
}

static void
test_scanner_multiline_comment (void)
{
  GScanner *scanner = NULL;
  const char buf[] = "/** this\n * is\n * multilined */";
  const size_t buflen = strlen (buf);

  scanner = g_scanner_new (NULL);
  scanner->config->skip_comment_multi = FALSE;

  g_scanner_input_text (scanner, buf, buflen);

  g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_NONE);
  g_scanner_get_next_token (scanner);
  g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_COMMENT_MULTI);
  g_assert_cmpint (g_scanner_cur_line (scanner), ==, 3);
  g_assert_cmpstr (g_scanner_cur_value (scanner).v_comment, ==, "* this\n * is\n * multilined ");
  g_assert_cmpint (g_scanner_get_next_token (scanner), ==, G_TOKEN_EOF);

  g_scanner_destroy (scanner);
}

static void
test_scanner_int_to_float (void)
{
  GScanner *scanner = NULL;
  const char buf[] = "4294967295";
  const size_t buflen = strlen (buf);

  scanner = g_scanner_new (NULL);
  scanner->config->int_2_float = TRUE;

  g_scanner_input_text (scanner, buf, buflen);

  g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_NONE);
  g_scanner_get_next_token (scanner);
  g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_FLOAT);
  g_assert_cmpint (g_scanner_cur_line (scanner), ==, 1);
  g_assert_cmpfloat (g_scanner_cur_value (scanner).v_float, ==, 4294967295.0);
  g_assert_cmpint (g_scanner_get_next_token (scanner), ==, G_TOKEN_EOF);

  g_scanner_destroy (scanner);
}

static void
test_scanner_fd_input (void)
{
  /* GScanner does some internal buffering when reading from an FD, reading in
   * chunks of `READ_BUFFER_SIZE` (currently 4000B) to an internal buffer. The
   * parsing codepaths differ significantly depending on whether there’s data in
   * the buffer, so all parsing operations have to be tested with chunk
   * boundaries on each of the characters in that operation.
   *
   * Do that by prefixing the buffer we’re testing with a variable amount of
   * whitespace. Whitespace is (by default) ignored by the scanner, so won’t
   * affect the test other than by letting us choose where in the test string
   * the chunk boundaries fall. */
  const size_t whitespace_lens[] = { 0, 3998, 3999, 4000, 4001 };

  for (size_t i = 0; i < G_N_ELEMENTS (whitespace_lens); i++)
    {
      GScanner *scanner = NULL;
      char *filename = NULL;
      int fd = -1;
      char *buf = NULL;
      const size_t whitespace_len = whitespace_lens[i];
      size_t buflen;
      const char *buf_suffix = "/** this\n * is\n * multilined */";

      buflen = whitespace_len + strlen (buf_suffix) + 1;
      buf = g_malloc (buflen);
      memset (buf, ' ', whitespace_len);
      memcpy (buf + whitespace_len, buf_suffix, strlen (buf_suffix));
      buf[buflen - 1] = '\0';

      scanner = g_scanner_new (NULL);
      scanner->config->skip_comment_multi = FALSE;

      filename = g_strdup ("scanner-fd-input-XXXXXX");
      fd = g_mkstemp (filename);
      g_assert_cmpint (fd, >=, 0);

      g_assert_cmpint (write (fd, buf, buflen), ==, buflen);
      g_assert_no_errno (lseek (fd, 0, SEEK_SET));

      g_scanner_input_file (scanner, fd);

      g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_NONE);
      g_scanner_get_next_token (scanner);
      g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_COMMENT_MULTI);
      g_assert_cmpint (g_scanner_cur_line (scanner), ==, 3);
      g_assert_cmpstr (g_scanner_cur_value (scanner).v_comment, ==, "* this\n * is\n * multilined ");
      g_assert_cmpint (g_scanner_get_next_token (scanner), ==, G_TOKEN_EOF);

      g_close (fd, NULL);
      g_free (filename);
      g_free (buf);
      g_scanner_destroy (scanner);
    }
}

static void
test_scanner_fd_input_rewind (void)
{
  /* GScanner does some internal buffering when reading from an FD, reading in
   * chunks of `READ_BUFFER_SIZE` (currently 4000B) to an internal buffer. It
   * allows the input FD’s file offset to be positioned to match the current
   * parsing position. Test that works. */
  GScanner *scanner = NULL;
  char *filename = NULL;
  int fd = -1;
  char *buf = NULL;
  const size_t whitespace_len = 4000;
  size_t buflen;
  const char *buf_suffix = "({})";
  const struct
    {
      GTokenType expected_token;
      off_t expected_offset;  /* offset after consuming @expected_token */
    }
  tokens_and_offsets[] =
   {
     { G_TOKEN_LEFT_PAREN, whitespace_len + 1 },
     { G_TOKEN_LEFT_CURLY, whitespace_len + 2 },
     { G_TOKEN_RIGHT_CURLY, whitespace_len + 3 },
     { G_TOKEN_RIGHT_PAREN, whitespace_len + 4 },
   };

  buflen = whitespace_len + strlen (buf_suffix) + 1;
  buf = g_malloc (buflen);
  memset (buf, ' ', whitespace_len);
  memcpy (buf + whitespace_len, buf_suffix, strlen (buf_suffix));
  buf[buflen - 1] = '\0';

  scanner = g_scanner_new (NULL);

  filename = g_strdup ("scanner-fd-input-XXXXXX");
  fd = g_mkstemp (filename);
  g_assert_cmpint (fd, >=, 0);

  g_assert_cmpint (write (fd, buf, buflen), ==, buflen);
  g_assert_no_errno (lseek (fd, 0, SEEK_SET));

  g_scanner_input_file (scanner, fd);

  g_assert_cmpint (g_scanner_cur_token (scanner), ==, G_TOKEN_NONE);

  for (size_t i = 0; i < G_N_ELEMENTS (tokens_and_offsets); i++)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_sync_file_offset (scanner);
      g_assert_cmpint (g_scanner_cur_token (scanner), ==, tokens_and_offsets[i].expected_token);
      g_assert_cmpint (lseek (fd, 0, SEEK_CUR), ==, tokens_and_offsets[i].expected_offset);
    }

  g_assert_cmpint (g_scanner_get_next_token (scanner), ==, G_TOKEN_EOF);

  g_close (fd, NULL);
  g_free (filename);
  g_free (buf);
  g_scanner_destroy (scanner);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/scanner/warn", ScannerFixture, 0, scanner_fixture_setup, test_scanner_warn, scanner_fixture_teardown);
  g_test_add ("/scanner/error", ScannerFixture, 0, scanner_fixture_setup, test_scanner_error, scanner_fixture_teardown);
  g_test_add ("/scanner/symbols", ScannerFixture, 0, scanner_fixture_setup, test_scanner_symbols, scanner_fixture_teardown);
  g_test_add ("/scanner/tokens", ScannerFixture, 0, scanner_fixture_setup, test_scanner_tokens, scanner_fixture_teardown);
  g_test_add_func ("/scanner/multiline-comment", test_scanner_multiline_comment);
  g_test_add_func ("/scanner/int-to-float", test_scanner_int_to_float);
  g_test_add_func ("/scanner/fd-input", test_scanner_fd_input);
  g_test_add_func ("/scanner/fd-input/rewind", test_scanner_fd_input_rewind);

  return g_test_run();
}
