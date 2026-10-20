/* GLib testing framework examples and tests
 * Copyright (C) 2008 Red Hat, Inc
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

#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <glib.h>
#include <glib/glib-unix.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* sizeof(DATA) will give the number of bytes in the array, plus the terminating nul */
static const gchar DATA[] = "abcdefghijklmnopqrstuvwxyz";

int writer_pipe[2], reader_pipe[2];
GCancellable *writer_cancel, *reader_cancel, *main_cancel;
GMainLoop *loop;


static gpointer
writer_thread (gpointer user_data)
{
  GOutputStream *out;
  gssize nwrote, offset;
  GError *err = NULL;

  out = g_unix_output_stream_new (writer_pipe[1], TRUE);

  do
    {
      g_usleep (10);

      offset = 0;
      while (offset < (gssize) sizeof (DATA))
	{
	  nwrote = g_output_stream_write (out, DATA + offset,
					  sizeof (DATA) - offset,
					  writer_cancel, &err);
	  if (nwrote <= 0 || err != NULL)
	    break;
	  offset += nwrote;
	}

      g_assert_true (nwrote > 0 || err != NULL);
    }
  while (err == NULL);

  if (g_cancellable_is_cancelled (writer_cancel))
    {
      g_clear_error (&err);
      g_cancellable_cancel (main_cancel);
      g_object_unref (out);
      return NULL;
    }

  g_warning ("writer: %s", err->message);
  g_assert_not_reached ();
}

static gpointer
reader_thread (gpointer user_data)
{
  GInputStream *in;
  gssize nread = 0, total;
  GError *err = NULL;
  char buf[sizeof (DATA)];

  in = g_unix_input_stream_new (reader_pipe[0], TRUE);

  do
    {
      total = 0;
      while (total < (gssize) sizeof (DATA))
	{
	  nread = g_input_stream_read (in, buf + total, sizeof (buf) - total,
				       reader_cancel, &err);
	  if (nread <= 0 || err != NULL)
	    break;
	  total += nread;
	}

      if (err)
	break;

      if (nread == 0)
	{
	  g_assert_no_error (err);
	  /* pipe closed */
	  g_object_unref (in);
	  return NULL;
	}

      g_assert_cmpstr (buf, ==, DATA);
      g_assert_false (g_cancellable_is_cancelled (reader_cancel));
    }
  while (err == NULL);

  g_warning ("reader: %s", err->message);
  g_assert_not_reached ();
}

static char main_buf[sizeof (DATA)];
static gssize main_len, main_offset;

static void main_thread_read (GObject *source, GAsyncResult *res, gpointer user_data);
static void main_thread_skipped (GObject *source, GAsyncResult *res, gpointer user_data);
static void main_thread_wrote (GObject *source, GAsyncResult *res, gpointer user_data);

static void
do_main_cancel (GOutputStream *out)
{
  g_output_stream_close (out, NULL, NULL);
  g_main_loop_quit (loop);
}

static void
main_thread_skipped (GObject *source, GAsyncResult *res, gpointer user_data)
{
  GInputStream *in = G_INPUT_STREAM (source);
  GOutputStream *out = user_data;
  GError *err = NULL;
  gssize nskipped;

  nskipped = g_input_stream_skip_finish (in, res, &err);

  if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_assert_true (g_cancellable_is_cancelled (main_cancel));
      do_main_cancel (out);
      g_clear_error (&err);
      return;
    }

  g_assert_no_error (err);

  main_offset += nskipped;
  if (main_offset == main_len)
    {
      main_offset = 0;
      g_output_stream_write_async (out, main_buf, main_len,
                                   G_PRIORITY_DEFAULT, main_cancel,
                                   main_thread_wrote, in);
    }
  else
    {
      g_input_stream_skip_async (in, main_len - main_offset,
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_skipped, out);
    }
}

static void
main_thread_read (GObject *source, GAsyncResult *res, gpointer user_data)
{
  GInputStream *in = G_INPUT_STREAM (source);
  GOutputStream *out = user_data;
  GError *err = NULL;
  gssize nread;

  nread = g_input_stream_read_finish (in, res, &err);

  if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_assert_true (g_cancellable_is_cancelled (main_cancel));
      do_main_cancel (out);
      g_clear_error (&err);
      return;
    }

  g_assert_no_error (err);

  main_offset += nread;
  if (main_offset == sizeof (DATA))
    {
      main_len = main_offset;
      main_offset = 0;
      /* Now skip the same amount */
      g_input_stream_skip_async (in, main_len,
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_skipped, out);
    }
  else
    {
      g_input_stream_read_async (in, main_buf, sizeof (main_buf),
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_read, out);
    }
}

static void
main_thread_wrote (GObject *source, GAsyncResult *res, gpointer user_data)
{
  GOutputStream *out = G_OUTPUT_STREAM (source);
  GInputStream *in = user_data;
  GError *err = NULL;
  gssize nwrote;

  nwrote = g_output_stream_write_finish (out, res, &err);

  if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_assert_true (g_cancellable_is_cancelled (main_cancel));
      do_main_cancel (out);
      g_clear_error (&err);
      return;
    }

  g_assert_no_error (err);
  g_assert_cmpint (nwrote, <=, main_len - main_offset);

  main_offset += nwrote;
  if (main_offset == main_len)
    {
      main_offset = 0;
      g_input_stream_read_async (in, main_buf, sizeof (main_buf),
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_read, out);
    }
  else
    {
      g_output_stream_write_async (out, main_buf + main_offset,
				   main_len - main_offset,
				   G_PRIORITY_DEFAULT, main_cancel,
				   main_thread_wrote, in);
    }
}

static gboolean
timeout (gpointer cancellable)
{
  g_cancellable_cancel (cancellable);
  return FALSE;
}

static void
test_pipe_io (gconstpointer nonblocking)
{
  GThread *writer, *reader;
  GInputStream *in;
  GOutputStream *out;

  /* Split off two (additional) threads, a reader and a writer. From
   * the writer thread, write data synchronously in small chunks,
   * which gets alternately read and skipped asynchronously by the
   * main thread and then (if not skipped) written asynchronously to
   * the reader thread, which reads it synchronously. Eventually a
   * timeout in the main thread will cause it to cancel the writer
   * thread, which will in turn cancel the read op in the main thread,
   * which will then close the pipe to the reader thread, causing the
   * read op to fail.
   */

  g_assert_true (pipe (writer_pipe) == 0 && pipe (reader_pipe) == 0);

  if (nonblocking)
    {
      GError *error = NULL;

      g_unix_set_fd_nonblocking (writer_pipe[0], TRUE, &error);
      g_assert_no_error (error);
      g_unix_set_fd_nonblocking (writer_pipe[1], TRUE, &error);
      g_assert_no_error (error);
      g_unix_set_fd_nonblocking (reader_pipe[0], TRUE, &error);
      g_assert_no_error (error);
      g_unix_set_fd_nonblocking (reader_pipe[1], TRUE, &error);
      g_assert_no_error (error);
    }

  writer_cancel = g_cancellable_new ();
  reader_cancel = g_cancellable_new ();
  main_cancel = g_cancellable_new ();

  writer = g_thread_new ("writer", writer_thread, NULL);
  reader = g_thread_new ("reader", reader_thread, NULL);

  in = g_unix_input_stream_new (writer_pipe[0], TRUE);
  out = g_unix_output_stream_new (reader_pipe[1], TRUE);

  g_input_stream_read_async (in, main_buf, sizeof (main_buf),
			     G_PRIORITY_DEFAULT, main_cancel,
			     main_thread_read, out);

  g_timeout_add (500, timeout, writer_cancel);

  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_thread_join (reader);
  g_thread_join (writer);

  g_object_unref (main_cancel);
  g_object_unref (reader_cancel);
  g_object_unref (writer_cancel);
  g_object_unref (in);
  g_object_unref (out);
}

static void
test_basic (void)
{
  GUnixInputStream *is;
  GUnixOutputStream *os;
  gint fd;
  gboolean close_fd;

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (0, TRUE));
  g_object_get (is,
                "fd", &fd,
                "close-fd", &close_fd,
                NULL);
  g_assert_cmpint (fd, ==, 0);
  g_assert_true (close_fd);

  g_unix_input_stream_set_close_fd (is, FALSE);
  g_assert_false (g_unix_input_stream_get_close_fd (is));
  g_assert_cmpint (g_unix_input_stream_get_fd (is), ==, 0);

  g_assert_false (g_input_stream_has_pending (G_INPUT_STREAM (is)));

  g_object_unref (is);

  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (1, TRUE));
  g_object_get (os,
                "fd", &fd,
                "close-fd", &close_fd,
                NULL);
  g_assert_cmpint (fd, ==, 1);
  g_assert_true (close_fd);

  g_unix_output_stream_set_close_fd (os, FALSE);
  g_assert_false (g_unix_output_stream_get_close_fd (os));
  g_assert_cmpint (g_unix_output_stream_get_fd (os), ==, 1);

  g_assert_false (g_output_stream_has_pending (G_OUTPUT_STREAM (os)));

  g_object_unref (os);
}

typedef struct {
  GInputStream *is;
  GOutputStream *os;
  const guint8 *write_data;
  guint8 *read_data;
} TestReadWriteData;

static gpointer
test_read_write_write_thread (gpointer user_data)
{
  TestReadWriteData *data = user_data;
  gsize bytes_written;
  GError *error = NULL;
  gboolean res;

  res = g_output_stream_write_all (data->os, data->write_data, 1024, &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_written, ==, 1024);

  return NULL;
}

static gpointer
test_read_write_read_thread (gpointer user_data)
{
  TestReadWriteData *data = user_data;
  gsize bytes_read;
  GError *error = NULL;
  gboolean res;

  res = g_input_stream_read_all (data->is, data->read_data, 1024, &bytes_read, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_read, ==, 1024);

  return NULL;
}

static gpointer
test_read_write_writev_thread (gpointer user_data)
{
  TestReadWriteData *data = user_data;
  gsize bytes_written;
  GError *error = NULL;
  gboolean res;
  GOutputVector vectors[3];

  vectors[0].buffer = data->write_data;
  vectors[0].size = 256;
  vectors[1].buffer = data->write_data + 256;
  vectors[1].size = 256;
  vectors[2].buffer = data->write_data + 512;
  vectors[2].size = 512;

  res = g_output_stream_writev_all (data->os, vectors, G_N_ELEMENTS (vectors), &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_written, ==, 1024);

  return NULL;
}

/* test if normal writing/reading from a pipe works */
static void
test_read_write (gconstpointer user_data)
{
  gboolean writev = GPOINTER_TO_INT (user_data);
  GUnixInputStream *is;
  GUnixOutputStream *os;
  gint fd[2];
  guint8 data_write[1024], data_read[1024];
  guint i;
  GThread *write_thread, *read_thread;
  TestReadWriteData data;

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  g_assert_cmpint (pipe (fd), ==, 0);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  data.is = G_INPUT_STREAM (is);
  data.os = G_OUTPUT_STREAM (os);
  data.read_data = data_read;
  data.write_data = data_write;

  if (writev)
    write_thread = g_thread_new ("writer", test_read_write_writev_thread, &data);
  else
    write_thread = g_thread_new ("writer", test_read_write_write_thread, &data);
  read_thread = g_thread_new ("reader", test_read_write_read_thread, &data);

  g_thread_join (write_thread);
  g_thread_join (read_thread);

  g_assert_cmpmem (data_write, sizeof data_write, data_read, sizeof data_read);

  g_object_unref (os);
  g_object_unref (is);
}

/* test if g_pollable_output_stream_write_nonblocking() and
 * g_pollable_output_stream_read_nonblocking() correctly return WOULD_BLOCK
 * and correctly reset their status afterwards again, and all data that is
 * written can also be read again.
 */
static void
test_write_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  gint fd[2];
  GError *err = NULL;
  guint8 data_write[1024], data_read[1024];
  gsize i;
  int retval;
  gsize pipe_capacity;

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  g_assert_cmpint (pipe (fd), ==, 0);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (gsize) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);
  g_assert_cmpint (pipe_capacity % 1024, >=, 0);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  /* Run the whole thing three times to make sure that the streams
   * reset the writability/readability state again */
  for (i = 0; i < 3; i++) {
    gssize written = 0, written_complete = 0;
    gssize read = 0, read_complete = 0;

    do
      {
        written_complete += written;
        written = g_pollable_output_stream_write_nonblocking (G_POLLABLE_OUTPUT_STREAM (os),
                                                              data_write,
                                                              sizeof (data_write),
                                                              NULL,
                                                              &err);
      }
    while (written > 0);

    g_assert_cmpuint (written_complete, >, 0);
    g_assert_nonnull (err);
    g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error (&err);

    do
      {
        read_complete += read;
        read = g_pollable_input_stream_read_nonblocking (G_POLLABLE_INPUT_STREAM (is),
                                                         data_read,
                                                         sizeof (data_read),
                                                         NULL,
                                                         &err);
        if (read > 0)
          g_assert_cmpmem (data_read, read, data_write, sizeof (data_write));
      }
    while (read > 0);

    g_assert_cmpuint (read_complete, ==, written_complete);
    g_assert_nonnull (err);
    g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error (&err);
  }

  g_object_unref (os);
  g_object_unref (is);
#endif  /* if F_GETPIPE_SZ */
}

/* test if g_pollable_output_stream_writev_nonblocking() and
 * g_pollable_output_stream_read_nonblocking() correctly return WOULD_BLOCK
 * and correctly reset their status afterwards again, and all data that is
 * written can also be read again.
 */
static void
test_writev_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  gint fd[2];
  GError *err = NULL;
  guint8 data_write[1024], data_read[1024];
  gsize i;
  int retval;
  gsize pipe_capacity;
  GOutputVector vectors[4];
  GPollableReturn res;

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  g_assert_cmpint (pipe (fd), ==, 0);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (gsize) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);
  g_assert_cmpint (pipe_capacity % 1024, >=, 0);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  /* Run the whole thing three times to make sure that the streams
   * reset the writability/readability state again */
  for (i = 0; i < 3; i++) {
    gsize written = 0, written_complete = 0;
    gssize read = 0, read_complete = 0;

    do
    {
        written_complete += written;

        vectors[0].buffer = data_write;
        vectors[0].size = 256;
        vectors[1].buffer = data_write + 256;
        vectors[1].size = 256;
        vectors[2].buffer = data_write + 512;
        vectors[2].size = 256;
        vectors[3].buffer = data_write + 768;
        vectors[3].size = 256;

        res = g_pollable_output_stream_writev_nonblocking (G_POLLABLE_OUTPUT_STREAM (os),
                                                           vectors,
                                                           G_N_ELEMENTS (vectors),
                                                           &written,
                                                           NULL,
                                                           &err);
      }
    while (res == G_POLLABLE_RETURN_OK);

    g_assert_cmpuint (written_complete, >, 0);
    g_assert_null (err);
    g_assert_cmpint (res, ==, G_POLLABLE_RETURN_WOULD_BLOCK);
    /* writev() on UNIX streams either succeeds fully or not at all */
    g_assert_cmpuint (written, ==, 0);

    do
      {
        read_complete += read;
        read = g_pollable_input_stream_read_nonblocking (G_POLLABLE_INPUT_STREAM (is),
                                                         data_read,
                                                         sizeof (data_read),
                                                         NULL,
                                                         &err);
        if (read > 0)
          g_assert_cmpmem (data_read, read, data_write, sizeof (data_write));
      }
    while (read > 0);

    g_assert_cmpuint (read_complete, ==, written_complete);
    g_assert_nonnull (err);
    g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error (&err);
  }

  g_object_unref (os);
  g_object_unref (is);
#endif  /* if F_GETPIPE_SZ */
}

#ifdef F_GETPIPE_SZ
static void
write_async_wouldblock_cb (GUnixOutputStream *os,
                           GAsyncResult      *result,
                           gpointer           user_data)
{
  gsize *bytes_written = user_data;
  GError *err = NULL;

  g_output_stream_write_all_finish (G_OUTPUT_STREAM (os), result, bytes_written, &err);
  g_assert_no_error (err);
}

static void
read_async_wouldblock_cb (GUnixInputStream  *is,
                          GAsyncResult      *result,
                          gpointer           user_data)
{
  gsize *bytes_read = user_data;
  GError *err = NULL;

  g_input_stream_read_all_finish (G_INPUT_STREAM (is), result, bytes_read, &err);
  g_assert_no_error (err);
}
#endif  /* if F_GETPIPE_SZ */

/* test if the async implementation of write_all() and read_all() in G*Stream
 * around the GPollable*Stream API is working correctly.
 */
static void
test_write_async_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  gint fd[2];
  guint8 *data, *data_read;
  gsize i;
  int retval;
  gsize pipe_capacity;
  gsize bytes_written = 0, bytes_read = 0;

  g_assert_cmpint (pipe (fd), ==, 0);

  /* FIXME: These should not be needed but otherwise
   * g_unix_output_stream_write() will block because
   *   a) the fd is writable
   *   b) writing 4x capacity will block because writes are atomic
   *   c) the fd is blocking
   *
   * See https://gitlab.gnome.org/GNOME/glib/issues/1654
   */
  g_unix_set_fd_nonblocking (fd[0], TRUE, NULL);
  g_unix_set_fd_nonblocking (fd[1], TRUE, NULL);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (gsize) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);

  data = g_new (guint8, 4 * pipe_capacity);
  for (i = 0; i < 4 * pipe_capacity; i++)
    data[i] = i;
  data_read = g_new (guint8, 4 * pipe_capacity);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  g_output_stream_write_all_async (G_OUTPUT_STREAM (os),
                                   data,
                                   4 * pipe_capacity,
                                   G_PRIORITY_DEFAULT,
                                   NULL,
                                   (GAsyncReadyCallback) write_async_wouldblock_cb,
                                   &bytes_written);

  g_input_stream_read_all_async (G_INPUT_STREAM (is),
                                 data_read,
                                 4 * pipe_capacity,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 (GAsyncReadyCallback) read_async_wouldblock_cb,
                                 &bytes_read);

  while (bytes_written == 0 && bytes_read == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (bytes_written, ==, 4 * pipe_capacity);
  g_assert_cmpuint (bytes_read, ==, 4 * pipe_capacity);
  g_assert_cmpmem (data_read, bytes_read, data, bytes_written);

  g_free (data);
  g_free (data_read);

  g_object_unref (os);
  g_object_unref (is);
#endif  /* if F_GETPIPE_SZ */
}

#ifdef F_GETPIPE_SZ
static void
writev_async_wouldblock_cb (GUnixOutputStream *os,
                            GAsyncResult      *result,
                            gpointer           user_data)
{
  gsize *bytes_written = user_data;
  GError *err = NULL;

  g_output_stream_writev_all_finish (G_OUTPUT_STREAM (os), result, bytes_written, &err);
  g_assert_no_error (err);
}
#endif  /* if F_GETPIPE_SZ */

/* test if the async implementation of writev_all() and read_all() in G*Stream
 * around the GPollable*Stream API is working correctly.
 */
static void
test_writev_async_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  gint fd[2];
  guint8 *data, *data_read;
  gsize i;
  int retval;
  gsize pipe_capacity;
  gsize bytes_written = 0, bytes_read = 0;
  GOutputVector vectors[4];

  g_assert_cmpint (pipe (fd), ==, 0);

  /* FIXME: These should not be needed but otherwise
   * g_unix_output_stream_writev() will block because
   *   a) the fd is writable
   *   b) writing 4x capacity will block because writes are atomic
   *   c) the fd is blocking
   *
   * See https://gitlab.gnome.org/GNOME/glib/issues/1654
   */
  g_unix_set_fd_nonblocking (fd[0], TRUE, NULL);
  g_unix_set_fd_nonblocking (fd[1], TRUE, NULL);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (gsize) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);

  data = g_new (guint8, 4 * pipe_capacity);
  for (i = 0; i < 4 * pipe_capacity; i++)
    data[i] = i;
  data_read = g_new (guint8, 4 * pipe_capacity);

  vectors[0].buffer = data;
  vectors[0].size = 1024;
  vectors[1].buffer = data + 1024;
  vectors[1].size = 1024;
  vectors[2].buffer = data + 2048;
  vectors[2].size = 1024;
  vectors[3].buffer = data + 3072;
  vectors[3].size = 4 * pipe_capacity - 3072;

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  g_output_stream_writev_all_async (G_OUTPUT_STREAM (os),
                                    vectors,
                                    G_N_ELEMENTS (vectors),
                                    G_PRIORITY_DEFAULT,
                                    NULL,
                                    (GAsyncReadyCallback) writev_async_wouldblock_cb,
                                    &bytes_written);

  g_input_stream_read_all_async (G_INPUT_STREAM (is),
                                 data_read,
                                 4 * pipe_capacity,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 (GAsyncReadyCallback) read_async_wouldblock_cb,
                                 &bytes_read);

  while (bytes_written == 0 && bytes_read == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (bytes_written, ==, 4 * pipe_capacity);
  g_assert_cmpuint (bytes_read, ==, 4 * pipe_capacity);
  g_assert_cmpmem (data_read, bytes_read, data, bytes_written);

  g_free (data);
  g_free (data_read);

  g_object_unref (os);
  g_object_unref (is);
#endif  /* F_GETPIPE_SZ */
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix-streams/basic", test_basic);
  g_test_add_data_func ("/unix-streams/pipe-io-test",
			GINT_TO_POINTER (FALSE),
			test_pipe_io);
  g_test_add_data_func ("/unix-streams/nonblocking-io-test",
			GINT_TO_POINTER (TRUE),
			test_pipe_io);

  g_test_add_data_func ("/unix-streams/read_write",
                        GINT_TO_POINTER (FALSE),
                        test_read_write);

  g_test_add_data_func ("/unix-streams/read_writev",
                        GINT_TO_POINTER (TRUE),
                        test_read_write);

  g_test_add_func ("/unix-streams/write-wouldblock",
		   test_write_wouldblock);
  g_test_add_func ("/unix-streams/writev-wouldblock",
		   test_writev_wouldblock);

  g_test_add_func ("/unix-streams/write-async-wouldblock",
		   test_write_async_wouldblock);
  g_test_add_func ("/unix-streams/writev-async-wouldblock",
		   test_writev_async_wouldblock);

  return g_test_run();
}
