/* GLib testing framework examples and tests
 * Copyright (C) 2010-2012 Collabora Ltd.
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 *          Mike Ruprecht <mike.ruprecht@collabora.co.uk>
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

#include <glib/glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  GInputStream parent_instance;
  GBytes *bytes;
  gsize offset;
  gsize max_read;
  guint read_count;
  guint fail_read;
} TestInputStream;

typedef GInputStreamClass TestInputStreamClass;

static GType test_input_stream_get_type (void);

typedef struct
{
  void *buffer;
  gsize count;
} TestReadData;

G_DEFINE_TYPE (TestInputStream, test_input_stream, G_TYPE_INPUT_STREAM)

static gboolean
test_input_stream_complete_read (gpointer user_data)
{
  GTask *task = user_data;
  TestInputStream *stream = g_task_get_source_object (task);
  TestReadData *read_data = g_task_get_task_data (task);
  gsize size;
  gconstpointer data = g_bytes_get_data (stream->bytes, &size);
  gsize count;

  stream->read_count++;

  if (g_task_return_error_if_cancelled (task))
    return G_SOURCE_REMOVE;

  if (stream->fail_read == stream->read_count)
    {
      g_task_return_new_error_literal (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "Test input failure");
      return G_SOURCE_REMOVE;
    }

  count = MIN (read_data->count, stream->max_read);
  count = MIN (count, size - stream->offset);
  memcpy (read_data->buffer, (const guint8 *) data + stream->offset, count);
  stream->offset += count;
  g_task_return_int (task, count);

  return G_SOURCE_REMOVE;
}

static void
test_input_stream_read_async (GInputStream        *stream,
                              void                *buffer,
                              gsize                count,
                              int                  io_priority,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  GTask *task = g_task_new (stream, cancellable, callback, user_data);
  TestReadData *read_data = g_new (TestReadData, 1);

  read_data->buffer = buffer;
  read_data->count = count;
  g_task_set_priority (task, io_priority);
  g_task_set_task_data (task, read_data, g_free);
  g_idle_add_full (io_priority, test_input_stream_complete_read, task, g_object_unref);
}

static gssize
test_input_stream_read_finish (GInputStream  *stream,
                               GAsyncResult  *result,
                               GError       **error)
{
  g_assert (g_task_is_valid (result, stream));

  return g_task_propagate_int (G_TASK (result), error);
}

static void
test_input_stream_finalize (GObject *object)
{
  TestInputStream *stream = (TestInputStream *) object;

  g_clear_pointer (&stream->bytes, g_bytes_unref);
  G_OBJECT_CLASS (test_input_stream_parent_class)->finalize (object);
}

static void
test_input_stream_class_init (TestInputStreamClass *stream_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (stream_class);
  GInputStreamClass *input_stream_class = G_INPUT_STREAM_CLASS (stream_class);

  object_class->finalize = test_input_stream_finalize;
  input_stream_class->read_async = test_input_stream_read_async;
  input_stream_class->read_finish = test_input_stream_read_finish;
}

static void
test_input_stream_init (TestInputStream *stream)
{
  stream->max_read = G_MAXSIZE;
}

static TestInputStream *
test_input_stream_new (GBytes *bytes)
{
  TestInputStream *stream = g_object_new (test_input_stream_get_type (), NULL);

  stream->bytes = g_bytes_ref (bytes);

  return stream;
}

typedef struct
{
  GOutputStream parent_instance;
  TestInputStream *input;
  GByteArray *bytes;
  gsize max_write;
  guint write_count;
  guint fail_write;
  guint delay_ms;
  gboolean cancel_on_write;
  gboolean saw_overlap;
} TestOutputStream;

typedef GOutputStreamClass TestOutputStreamClass;

static GType test_output_stream_get_type (void);

G_DEFINE_TYPE (TestOutputStream, test_output_stream, G_TYPE_OUTPUT_STREAM)

static gboolean
test_output_stream_complete_write (gpointer user_data)
{
  GTask *task = user_data;
  TestOutputStream *stream = g_task_get_source_object (task);
  GBytes *bytes = g_task_get_task_data (task);
  gsize size;
  gconstpointer data = g_bytes_get_data (bytes, &size);

  if (stream->input->read_count >= 2)
    stream->saw_overlap = TRUE;

  if (g_task_return_error_if_cancelled (task))
    return G_SOURCE_REMOVE;

  if (stream->fail_write == stream->write_count)
    {
      g_task_return_new_error_literal (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "Test output failure");
      return G_SOURCE_REMOVE;
    }

  g_byte_array_append (stream->bytes, data, size);
  g_task_return_int (task, size);

  return G_SOURCE_REMOVE;
}

static void
test_output_stream_write_async (GOutputStream       *stream,
                                const void          *buffer,
                                gsize                count,
                                int                  io_priority,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  TestOutputStream *test_stream = (TestOutputStream *) stream;
  GTask *task = g_task_new (stream, cancellable, callback, user_data);
  GBytes *bytes;

  count = MIN (count, test_stream->max_write);
  bytes = g_bytes_new (buffer, count);
  test_stream->write_count++;
  g_task_set_priority (task, io_priority);
  g_task_set_task_data (task, bytes, (GDestroyNotify) g_bytes_unref);

  if (test_stream->cancel_on_write)
    g_cancellable_cancel (cancellable);

  if (test_stream->delay_ms > 0)
    g_timeout_add_full (io_priority, test_stream->delay_ms,
                        test_output_stream_complete_write, task, g_object_unref);
  else
    g_idle_add_full (io_priority, test_output_stream_complete_write, task, g_object_unref);
}

static gssize
test_output_stream_write_finish (GOutputStream  *stream,
                                 GAsyncResult   *result,
                                 GError        **error)
{
  g_assert (g_task_is_valid (result, stream));

  return g_task_propagate_int (G_TASK (result), error);
}

static void
test_output_stream_finalize (GObject *object)
{
  TestOutputStream *stream = (TestOutputStream *) object;

  g_clear_object (&stream->input);
  g_clear_pointer (&stream->bytes, g_byte_array_unref);
  G_OBJECT_CLASS (test_output_stream_parent_class)->finalize (object);
}

static void
test_output_stream_class_init (TestOutputStreamClass *stream_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (stream_class);
  GOutputStreamClass *output_stream_class = G_OUTPUT_STREAM_CLASS (stream_class);

  object_class->finalize = test_output_stream_finalize;
  output_stream_class->write_async = test_output_stream_write_async;
  output_stream_class->write_finish = test_output_stream_write_finish;
}

static void
test_output_stream_init (TestOutputStream *stream)
{
  stream->bytes = g_byte_array_new ();
  stream->max_write = G_MAXSIZE;
}

static TestOutputStream *
test_output_stream_new (TestInputStream *input)
{
  TestOutputStream *stream = g_object_new (test_output_stream_get_type (), NULL);

  stream->input = g_object_ref (input);

  return stream;
}

typedef enum
{
  TEST_THREADED_NONE    = 0,
  TEST_THREADED_ISTREAM = 1,
  TEST_THREADED_OSTREAM = 2,
  TEST_CANCEL           = 4,
  TEST_THREADED_BOTH    = TEST_THREADED_ISTREAM | TEST_THREADED_OSTREAM,
} TestThreadedFlags;

typedef struct
{
  GMainLoop *main_loop;
  gchar *data;
  gsize data_len;
  GInputStream *istream;
  GOutputStream *ostream;
  TestThreadedFlags flags;
  gchar *input_path;
  gchar *output_path;
} TestCopyChunksData;

static void
test_copy_chunks_splice_cb (GObject      *source,
                            GAsyncResult *res,
                            gpointer      user_data)
{
  TestCopyChunksData *data = user_data;
  gchar *received_data;
  GError *error = NULL;
  gssize bytes_spliced;

  bytes_spliced = g_output_stream_splice_finish (G_OUTPUT_STREAM (source),
                                                 res, &error);

  if (data->flags & TEST_CANCEL)
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
      g_error_free (error);
      g_main_loop_quit (data->main_loop);
      return;
    }

  g_assert_no_error (error);
  g_assert_cmpint (bytes_spliced, ==, data->data_len);

  if (data->flags & TEST_THREADED_OSTREAM)
    {
      gsize length = 0;

      g_file_get_contents (data->output_path, &received_data,
                           &length, &error);
      g_assert_no_error (error);
      g_assert_cmpuint (length, ==, data->data_len);
      g_assert_cmpmem (received_data, length, data->data, data->data_len);
      g_free (received_data);
    }
  else
    {
      received_data = g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (data->ostream));
      g_assert_cmpuint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (data->ostream)),
                        ==, data->data_len);
      g_assert_cmpmem (received_data, data->data_len, data->data, data->data_len);
    }

  g_assert (g_input_stream_is_closed (data->istream));
  g_assert (g_output_stream_is_closed (data->ostream));

  if (data->flags & TEST_THREADED_ISTREAM)
    {
      g_unlink (data->input_path);
      g_free (data->input_path);
    }

  if (data->flags & TEST_THREADED_OSTREAM)
    {
      g_unlink (data->output_path);
      g_free (data->output_path);
    }

  g_main_loop_quit (data->main_loop);
}

static void
test_copy_chunks_start (TestThreadedFlags flags,
                        gsize             data_len)
{
  TestCopyChunksData data;
  GError *error = NULL;
  GCancellable *cancellable = NULL;

  data.main_loop = g_main_loop_new (NULL, FALSE);
  data.data = g_malloc (data_len);
  data.data_len = data_len;
  data.flags = flags;

  for (gsize i = 0; i < data_len; i++)
    data.data[i] = 'a' + (i % 26);

  if (data.flags & TEST_CANCEL)
    {
      cancellable = g_cancellable_new ();
      g_cancellable_cancel (cancellable);
    }

  if (data.flags & TEST_THREADED_ISTREAM)
    {
      GFile *file;
      GFileIOStream *stream;

      file = g_file_new_tmp ("test-inputXXXXXX", &stream, &error);
      g_assert_no_error (error);
      g_object_unref (stream);
      data.input_path = g_file_get_path (file);
      g_file_set_contents (data.input_path,
                           data.data, data.data_len,
                           &error);
      g_assert_no_error (error);
      data.istream = G_INPUT_STREAM (g_file_read (file, NULL, &error));
      g_assert_no_error (error);
      g_object_unref (file);
    }
  else
    {
      data.istream = g_memory_input_stream_new_from_data (data.data, data.data_len, NULL);
    }

  if (data.flags & TEST_THREADED_OSTREAM)
    {
      GFile *file;
      GFileIOStream *stream;

      file = g_file_new_tmp ("test-outputXXXXXX", &stream, &error);
      g_assert_no_error (error);
      g_object_unref (stream);
      data.output_path = g_file_get_path (file);
      data.ostream = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE,
                                                      G_FILE_CREATE_NONE,
                                                      NULL, &error));
      g_assert_no_error (error);
      g_object_unref (file);
    }
  else
    {
      data.ostream = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
    }

  g_output_stream_splice_async (data.ostream, data.istream,
                                G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
                                G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT, cancellable,
                                test_copy_chunks_splice_cb, &data);

  /* We do not hold a ref in data struct, this is to make sure the operation
   * keeps the iostream objects alive until it finishes
   */
  g_object_unref (data.istream);
  g_object_unref (data.ostream);
  g_clear_object (&cancellable);

  g_main_loop_run (data.main_loop);
  g_main_loop_unref (data.main_loop);
  g_free (data.data);
}

static void
test_copy_chunks (void)
{
  test_copy_chunks_start (TEST_THREADED_NONE, 26);
}

static void
test_copy_chunks_threaded_input (void)
{
  test_copy_chunks_start (TEST_THREADED_ISTREAM, 26);
}

static void
test_copy_chunks_threaded_output (void)
{
  test_copy_chunks_start (TEST_THREADED_OSTREAM, 26);
}

static void
test_copy_chunks_threaded (void)
{
  test_copy_chunks_start (TEST_THREADED_BOTH, 26);
}

static void
test_copy_chunks_large_threaded_output (void)
{
  test_copy_chunks_start (TEST_THREADED_OSTREAM, 2 * 1024 * 1024 + 17);
}

static void
test_cancelled (void)
{
  test_copy_chunks_start (TEST_THREADED_NONE | TEST_CANCEL, 26);
}

typedef struct
{
  GMainLoop *loop;
  GError *error;
  gssize bytes_spliced;
} TestPipelineResult;

static void
test_pipeline_splice_cb (GObject      *source,
                         GAsyncResult *result,
                         gpointer      user_data)
{
  TestPipelineResult *test_result = user_data;

  test_result->bytes_spliced =
    g_output_stream_splice_finish (G_OUTPUT_STREAM (source), result, &test_result->error);
  g_main_loop_quit (test_result->loop);
}

static GBytes *
test_pipeline_create_bytes (gsize size)
{
  guint8 *data = g_malloc (size);

  for (gsize i = 0; i < size; i++)
    data[i] = i % 251;

  return g_bytes_new_take (data, size);
}

static void
test_sync_splice_large (void)
{
  GBytes *bytes = test_pipeline_create_bytes (2 * 1024 * 1024 + 17);
  GInputStream *input = g_memory_input_stream_new_from_bytes (bytes);
  GOutputStream *output = g_memory_output_stream_new_resizable ();
  GError *error = NULL;
  gsize size;
  gconstpointer data = g_bytes_get_data (bytes, &size);
  gssize bytes_spliced;

  bytes_spliced = g_output_stream_splice (output, input,
                                          (G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
                                           G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET),
                                          NULL, &error);

  g_assert_no_error (error);
  g_assert_cmpint (bytes_spliced, ==, size);
  g_assert_cmpuint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (output)),
                    ==, size);
  g_assert_cmpmem (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (output)), size,
                   data, size);
  g_assert_true (g_input_stream_is_closed (input));
  g_assert_true (g_output_stream_is_closed (output));

  g_object_unref (output);
  g_object_unref (input);
  g_bytes_unref (bytes);
}

static TestPipelineResult
test_pipeline_run (TestInputStream         *input,
                   TestOutputStream        *output,
                   GOutputStreamSpliceFlags flags,
                   GCancellable            *cancellable)
{
  TestPipelineResult result = { 0, };

  result.loop = g_main_loop_new (NULL, FALSE);
  result.bytes_spliced = -1;
  g_output_stream_splice_async (G_OUTPUT_STREAM (output),
                                G_INPUT_STREAM (input),
                                flags,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                test_pipeline_splice_cb,
                                &result);
  g_main_loop_run (result.loop);
  g_main_loop_unref (result.loop);
  result.loop = NULL;

  return result;
}

static void
test_pipeline_overlap_partial_writes (void)
{
  GBytes *bytes = test_pipeline_create_bytes (768 * 1024 + 17);
  TestInputStream *input = test_input_stream_new (bytes);
  TestOutputStream *output = test_output_stream_new (input);
  TestPipelineResult result;
  gsize size;
  gconstpointer data = g_bytes_get_data (bytes, &size);

  input->max_read = 128 * 1024;
  output->max_write = 4096;
  output->delay_ms = 1;
  result = test_pipeline_run (input, output, G_OUTPUT_STREAM_SPLICE_NONE, NULL);

  g_assert_no_error (result.error);
  g_assert_cmpint (result.bytes_spliced, ==, size);
  g_assert_true (output->saw_overlap);
  g_assert_cmpmem (output->bytes->data, output->bytes->len, data, size);
  g_assert_false (g_input_stream_is_closed (G_INPUT_STREAM (input)));
  g_assert_false (g_output_stream_is_closed (G_OUTPUT_STREAM (output)));

  g_object_unref (output);
  g_object_unref (input);
  g_bytes_unref (bytes);
}

static void
test_pipeline_read_error_during_write (void)
{
  GBytes *bytes = test_pipeline_create_bytes (256 * 1024);
  TestInputStream *input = test_input_stream_new (bytes);
  TestOutputStream *output = test_output_stream_new (input);
  TestPipelineResult result;

  input->max_read = 64 * 1024;
  input->fail_read = 2;
  output->delay_ms = 10;
  result = test_pipeline_run (input, output,
                              (G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
                               G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET),
                              NULL);

  g_assert_error (result.error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_assert_cmpstr (result.error->message, ==, "Test input failure");
  g_assert_true (g_input_stream_is_closed (G_INPUT_STREAM (input)));
  g_assert_true (g_output_stream_is_closed (G_OUTPUT_STREAM (output)));
  g_clear_error (&result.error);

  g_object_unref (output);
  g_object_unref (input);
  g_bytes_unref (bytes);
}

static void
test_pipeline_write_error_during_read (void)
{
  GBytes *bytes = test_pipeline_create_bytes (256 * 1024);
  TestInputStream *input = test_input_stream_new (bytes);
  TestOutputStream *output = test_output_stream_new (input);
  TestPipelineResult result;

  input->max_read = 64 * 1024;
  output->fail_write = 1;
  output->delay_ms = 1;
  result = test_pipeline_run (input, output,
                              G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE,
                              NULL);

  g_assert_error (result.error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_assert_cmpstr (result.error->message, ==, "Test output failure");
  g_assert_true (g_input_stream_is_closed (G_INPUT_STREAM (input)));
  g_assert_false (g_output_stream_is_closed (G_OUTPUT_STREAM (output)));
  g_clear_error (&result.error);

  g_object_unref (output);
  g_object_unref (input);
  g_bytes_unref (bytes);
}

static void
test_pipeline_cancel_during_overlap (void)
{
  GBytes *bytes = test_pipeline_create_bytes (256 * 1024);
  TestInputStream *input = test_input_stream_new (bytes);
  TestOutputStream *output = test_output_stream_new (input);
  GCancellable *cancellable = g_cancellable_new ();
  TestPipelineResult result;

  input->max_read = 64 * 1024;
  output->cancel_on_write = TRUE;
  result = test_pipeline_run (input, output,
                              G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                              cancellable);

  g_assert_error (result.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_input_stream_is_closed (G_INPUT_STREAM (input)));
  g_assert_true (g_output_stream_is_closed (G_OUTPUT_STREAM (output)));
  g_clear_error (&result.error);

  g_object_unref (cancellable);
  g_object_unref (output);
  g_object_unref (input);
  g_bytes_unref (bytes);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/async-splice/copy-chunks", test_copy_chunks);
  g_test_add_func ("/async-splice/copy-chunks-threaded-input",
                   test_copy_chunks_threaded_input);
  g_test_add_func ("/async-splice/copy-chunks-threaded-output",
                   test_copy_chunks_threaded_output);
  g_test_add_func ("/async-splice/copy-chunks-threaded",
                   test_copy_chunks_threaded);
  g_test_add_func ("/async-splice/copy-chunks-large-threaded-output",
                   test_copy_chunks_large_threaded_output);
  g_test_add_func ("/async-splice/cancelled",
                   test_cancelled);
  g_test_add_func ("/async-splice/sync-large",
                   test_sync_splice_large);
  g_test_add_func ("/async-splice/pipeline/overlap-partial-writes",
                   test_pipeline_overlap_partial_writes);
  g_test_add_func ("/async-splice/pipeline/read-error-during-write",
                   test_pipeline_read_error_during_write);
  g_test_add_func ("/async-splice/pipeline/write-error-during-read",
                   test_pipeline_write_error_during_read);
  g_test_add_func ("/async-splice/pipeline/cancel-during-overlap",
                   test_pipeline_cancel_during_overlap);

  return g_test_run();
}
