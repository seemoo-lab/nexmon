/* GLib testing framework examples and tests
 *
 * Copyright © 2001 Hidetoshi Tajima
 * Copyright © 2001 Ron Steinke
 * Copyright © 2001 Owen Taylor
 * Copyright © 2002 Manish Singh
 * Copyright © 2011 Sjoerd Simons
 * Copyright © 2012 Simon McVittie
 * Copyright © 2013 Stef Walter
 * Copyright © 2005, 2006, 2008, 2012, 2013 Matthias Clasen
 * Copyright © 2020 Endless Mobile, Inc.
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
#include <glib/gstdio.h>

static void
test_small_writes (void)
{
  GIOChannel *io;
  GIOStatus status = G_IO_STATUS_ERROR;
  guint bytes_remaining;
  gchar tmp;
  GError *local_error = NULL;
  char *path = NULL;

  path = g_build_filename (g_get_tmp_dir (), "iochannel-test-outfile", NULL);
  io = g_io_channel_new_file (path, "w", &local_error);
  g_assert_no_error (local_error);

  g_io_channel_set_encoding (io, NULL, NULL);
  g_io_channel_set_buffer_size (io, 1022);

  bytes_remaining = 2 * g_io_channel_get_buffer_size (io);
  tmp = 0;

  while (bytes_remaining)
    {
      status = g_io_channel_write_chars (io, &tmp, 1, NULL, NULL);
      if (status == G_IO_STATUS_ERROR)
        break;
      if (status == G_IO_STATUS_NORMAL)
        bytes_remaining--;
    }

  g_assert_cmpint (status, ==, G_IO_STATUS_NORMAL);

  g_io_channel_unref (io);
  g_remove (path);
  g_free (path);
}

static void
test_read_write (void)
{
  GIOChannel *gio_r, *gio_w ;
  GError *local_error = NULL;
  GString *buffer;
  char *in_path = NULL, *out_path = NULL;
  gint rlength = 0;
  glong wlength = 0;
  gsize length_out;
  const gchar *encoding = "EUC-JP";
  GIOStatus status;
  const gsize buffer_size_bytes = 1024;

  in_path = g_test_build_filename (G_TEST_DIST, "iochannel-test-infile", NULL);

  setbuf (stdout, NULL); /* For debugging */

  gio_r = g_io_channel_new_file (in_path, "r", &local_error);
  g_assert_no_error (local_error);

  out_path = g_build_filename (g_get_tmp_dir (), "iochannel-test-outfile", NULL);
  gio_w = g_io_channel_new_file (out_path, "w", &local_error);
  g_assert_no_error (local_error);

  g_io_channel_set_encoding (gio_r, encoding, &local_error);
  g_assert_no_error (local_error);

  g_io_channel_set_buffer_size (gio_r, buffer_size_bytes);

  status = g_io_channel_set_flags (gio_r, G_IO_FLAG_NONBLOCK, &local_error);
  if (status == G_IO_STATUS_ERROR)
    {
#ifdef G_OS_WIN32
      g_test_message ("FIXME: not implemented on win32");
#else
      /* Errors should not happen */
      g_assert_no_error (local_error);
#endif
      g_clear_error (&local_error);
    }
  buffer = g_string_sized_new (buffer_size_bytes);

  while (TRUE)
    {
      do
        status = g_io_channel_read_line_string (gio_r, buffer, NULL, &local_error);
      while (status == G_IO_STATUS_AGAIN);
      if (status != G_IO_STATUS_NORMAL)
        break;

      rlength += buffer->len;

      do
        status = g_io_channel_write_chars (gio_w, buffer->str, buffer->len,
          &length_out, &local_error);
      while (status == G_IO_STATUS_AGAIN);
      if (status != G_IO_STATUS_NORMAL)
        break;

      wlength += length_out;

      /* Ensure the whole line was written */
      g_assert_cmpuint (length_out, ==, buffer->len);

      g_test_message ("%s", buffer->str);
      g_string_truncate (buffer, 0);
    }

  switch (status)
    {
      case G_IO_STATUS_EOF:
        break;
      case G_IO_STATUS_ERROR:
        /* Errors should not happen */
        g_assert_no_error (local_error);
        g_clear_error (&local_error);
        break;
      default:
        g_assert_not_reached ();
        break;
    }

  do
    status = g_io_channel_flush (gio_w, &local_error);
  while (status == G_IO_STATUS_AGAIN);

  if (status == G_IO_STATUS_ERROR)
    {
      /* Errors should not happen */
      g_assert_no_error (local_error);
      g_clear_error (&local_error);
    }

  g_test_message ("read %d bytes, wrote %ld bytes", rlength, wlength);

  g_io_channel_unref (gio_r);
  g_io_channel_unref (gio_w);

  test_small_writes ();

  g_free (in_path);
  g_free (out_path);
  g_string_free (buffer, TRUE);
}

static void
test_read_line_embedded_nuls (void)
{
  const guint8 test_data[] = { 'H', 'i', '!', '\0', 'y', 'o', 'u', '\n', ':', ')', '\n' };
  gint fd;
  gchar *filename = NULL;
  GIOChannel *channel = NULL;
  GError *local_error = NULL;
  gchar *line = NULL;
  gsize line_length, terminator_pos;
  const gchar *line_term;
  gint line_term_length;
  GIOStatus status;

  g_test_summary ("Test that reading a line containing embedded nuls works "
                  "when using non-standard line terminators.");

  /* Write out a temporary file. */
  fd = g_file_open_tmp ("glib-test-io-channel-XXXXXX", &filename, &local_error);
  g_assert_no_error (local_error);
  g_close (fd, NULL);
  fd = -1;

  g_file_set_contents (filename, (const gchar *) test_data, sizeof (test_data), &local_error);
  g_assert_no_error (local_error);

  /* Create the channel. */
  channel = g_io_channel_new_file (filename, "r", &local_error);
  g_assert_no_error (local_error);

  /* Only break on newline characters, not nuls.
   * Use length -1 here to exercise glib#2323; the case where length > 0
   * is covered in glib/tests/protocol.c. */
  g_io_channel_set_line_term (channel, "\n", -1);

  line_term = g_io_channel_get_line_term (channel, &line_term_length);
  g_assert_cmpstr (line_term, ==, "\n");
  g_assert_cmpint (line_term_length, ==, 1);

  g_io_channel_set_encoding (channel, NULL, &local_error);
  g_assert_no_error (local_error);

  status = g_io_channel_read_line (channel, &line, &line_length,
                                   &terminator_pos, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (status, ==, G_IO_STATUS_NORMAL);
  g_assert_cmpuint (line_length, ==, 8);
  g_assert_cmpuint (terminator_pos, ==, 7);
  g_assert_cmpmem (line, line_length, test_data, 8);

  g_free (line);
  g_io_channel_unref (channel);
  g_free (filename);
}

static void
test_read_line_long_terminator (void)
{
  uint8_t *test_data = NULL;
  size_t test_data_len = 0;
  int fd;
  char *filename = NULL;
  GIOChannel *channel = NULL;
  GError *local_error = NULL;
  char *line = NULL;
  size_t line_length, terminator_pos;
  const char *line_term;
  int line_term_length;
  GIOStatus status;

  g_test_summary ("Test that reading a line when using a long terminator doesn’t over-read the buffer.");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/work_items/3925");

  /* Write out a temporary file containing 2047 bytes. This is enough to make it
   * near the length of the GString buffer when read back in. */
  fd = g_file_open_tmp ("glib-test-io-channel-XXXXXX", &filename, &local_error);
  g_assert_no_error (local_error);
  g_close (g_steal_fd (&fd), NULL);

  test_data_len = 2047;
  test_data = g_malloc (test_data_len);
  memset (test_data, 'M', test_data_len);
  g_file_set_contents (filename, (const gchar *) test_data, test_data_len, &local_error);
  g_assert_no_error (local_error);

  /* Create the channel. */
  channel = g_io_channel_new_file (filename, "r", &local_error);
  g_assert_no_error (local_error);

  /* Use a long line terminator so it could potentially over-read the end of the buffer. */
  g_io_channel_set_line_term (channel, "DEADBEEF", 8);

  line_term = g_io_channel_get_line_term (channel, &line_term_length);
  g_assert_cmpstr (line_term, ==, "DEADBEEF");
  g_assert_cmpint (line_term_length, ==, 8);

  g_io_channel_set_encoding (channel, "UTF-8", &local_error);
  g_assert_no_error (local_error);

  status = g_io_channel_read_line (channel, &line, &line_length,
                                   &terminator_pos, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (status, ==, G_IO_STATUS_NORMAL);
  g_assert_cmpuint (line_length, ==, 2047);
  g_assert_cmpuint (terminator_pos, ==, 2047);
  g_assert_cmpmem (line, line_length, test_data, test_data_len);

  g_free (line);
  g_io_channel_unref (channel);
  g_free (test_data);
  g_unlink (filename);
  g_free (filename);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/io-channel/read-write", test_read_write);
  g_test_add_func ("/io-channel/read-line/embedded-nuls", test_read_line_embedded_nuls);
  g_test_add_func ("/io-channel/read-line/long-terminator", test_read_line_long_terminator);

  return g_test_run ();
}
