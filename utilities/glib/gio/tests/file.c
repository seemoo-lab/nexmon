#include "config.h"

#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <gio/gfiledescriptorbased.h>
#include <glib/gstdio.h>
#ifdef G_OS_UNIX
#include <sys/stat.h>
#endif

typedef struct
{
  GMainLoop *loop;
  GError **error;
} AsyncErrorData;

static void
test_basic_for_file (GFile       *file,
                     const gchar *suffix)
{
  gchar *s;

  s = g_file_get_basename (file);
  g_assert_cmpstr (s, ==, "testfile");
  g_free (s);

  s = g_file_get_uri (file);
  g_assert_true (g_str_has_prefix (s, "file://"));
  g_assert_true (g_str_has_suffix (s, suffix));
  g_free (s);

  g_assert_true (g_file_has_uri_scheme (file, "file"));
  s = g_file_get_uri_scheme (file);
  g_assert_cmpstr (s, ==, "file");
  g_free (s);
}

static void
test_basic (void)
{
  GFile *file;

  file = g_file_new_for_path ("./some/directory/testfile");
  test_basic_for_file (file, "/some/directory/testfile");
  g_object_unref (file);
}

static void
test_build_filename (void)
{
  GFile *file;

  file = g_file_new_build_filename (".", "some", "directory", "testfile", NULL);
  test_basic_for_file (file, "/some/directory/testfile");
  g_object_unref (file);

  file = g_file_new_build_filename ("testfile", NULL);
  test_basic_for_file (file, "/testfile");
  g_object_unref (file);
}

static void
test_build_filenamev (void)
{
  GFile *file;

  const gchar *args[] = { ".", "some", "directory", "testfile", NULL };
  file = g_file_new_build_filenamev (args);
  test_basic_for_file (file, "/some/directory/testfile");
  g_object_unref (file);

  const gchar *brgs[] = { "testfile", NULL };
  file = g_file_new_build_filenamev (brgs);
  test_basic_for_file (file, "/testfile");
  g_object_unref (file);
}

static void
test_parent (void)
{
  GFile *file;
  GFile *file2;
  GFile *parent;
  GFile *root;

  file = g_file_new_for_path ("./some/directory/testfile");
  file2 = g_file_new_for_path ("./some/directory");
  root = g_file_new_for_path ("/");

  g_assert_true (g_file_has_parent (file, file2));

  parent = g_file_get_parent (file);
  g_assert_true (g_file_equal (parent, file2));
  g_object_unref (parent);

  g_assert_null (g_file_get_parent (root));

  g_object_unref (file);
  g_object_unref (file2);
  g_object_unref (root);
}

static void
test_child (void)
{
  GFile *file;
  GFile *child;
  GFile *child2;

  file = g_file_new_for_path ("./some/directory");
  child = g_file_get_child (file, "child");
  g_assert_true (g_file_has_parent (child, file));

  child2 = g_file_get_child_for_display_name (file, "child2", NULL);
  g_assert_true (g_file_has_parent (child2, file));

  g_object_unref (child);
  g_object_unref (child2);
  g_object_unref (file);
}

static void
test_empty_path (void)
{
  GFile *file = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2328");
  g_test_summary ("Check that creating a file with an empty path results in errors");

  /* Creating the file must always succeed. */
  file = g_file_new_for_path ("");
  g_assert_nonnull (file);

  /* But then querying its path should indicate it’s invalid. */
  g_assert_null (g_file_get_path (file));
  g_assert_null (g_file_get_basename (file));
  g_assert_null (g_file_get_parent (file));

  g_object_unref (file);
}

static void
test_type (void)
{
  GFile *datapath_f;
  GFile *file;
  GFileType type;
  GError *error = NULL;

  datapath_f = g_file_new_for_path (g_test_get_dir (G_TEST_DIST));

  file = g_file_get_child (datapath_f, "g-icon.c");
  type = g_file_query_file_type (file, 0, NULL);
  g_assert_cmpint (type, ==, G_FILE_TYPE_REGULAR);
  g_object_unref (file);

  file = g_file_get_child (datapath_f, "cert-tests");
  type = g_file_query_file_type (file, 0, NULL);
  g_assert_cmpint (type, ==, G_FILE_TYPE_DIRECTORY);

  g_file_read (file, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY);
  g_error_free (error);
  g_object_unref (file);

  g_object_unref (datapath_f);
}

static void
test_parse_name (void)
{
  GFile *file;
  gchar *name;

  file = g_file_new_for_uri ("file://somewhere");
  name = g_file_get_parse_name (file);
  g_assert_cmpstr (name, ==, "file://somewhere");
  g_object_unref (file);
  g_free (name);

  file = g_file_parse_name ("~foo");
  name = g_file_get_parse_name (file);
  g_assert_nonnull (name);
  g_object_unref (file);
  g_free (name);
}

typedef struct
{
  GMainContext *context;
  GFile *file;
  GFileMonitor *monitor;
  GOutputStream *ostream;
  GInputStream *istream;
  gint buffersize;
  gint monitor_created;
  gint monitor_deleted;
  gint monitor_changed;
  gchar *monitor_path;
  gsize pos;
  const gchar *data;
  gchar *buffer;
  guint timeout;
  gboolean file_deleted;
  gboolean timed_out;
} CreateDeleteData;

static void
monitor_changed (GFileMonitor      *monitor,
                 GFile             *file,
                 GFile             *other_file,
                 GFileMonitorEvent  event_type,
                 gpointer           user_data)
{
  CreateDeleteData *data = user_data;
  gchar *path;
  const gchar *peeked_path;

  path = g_file_get_path (file);
  peeked_path = g_file_peek_path (file);
  g_assert_cmpstr (data->monitor_path, ==, path);
  g_assert_cmpstr (path, ==, peeked_path);
  g_free (path);

  if (event_type == G_FILE_MONITOR_EVENT_CREATED)
    data->monitor_created++;
  if (event_type == G_FILE_MONITOR_EVENT_DELETED)
    data->monitor_deleted++;
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED)
    data->monitor_changed++;

  g_main_context_wakeup (data->context);
}

static void
iclosed_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gboolean ret;

  error = NULL;
  ret = g_input_stream_close_finish (data->istream, res, &error);
  g_assert_no_error (error);
  g_assert_true (ret);

  g_assert_true (g_input_stream_is_closed (data->istream));

  ret = g_file_delete (data->file, NULL, &error);
  g_assert_true (ret);
  g_assert_no_error (error);

  data->file_deleted = TRUE;
  g_main_context_wakeup (data->context);
}

static void
read_cb (GObject      *source,
         GAsyncResult *res,
         gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gssize size;

  error = NULL;
  size = g_input_stream_read_finish (data->istream, res, &error);
  g_assert_no_error (error);

  data->pos += size;
  if (data->pos < strlen (data->data))
    {
      g_input_stream_read_async (data->istream,
                                 data->buffer + data->pos,
                                 strlen (data->data) - data->pos,
                                 0,
                                 NULL,
                                 read_cb,
                                 data);
    }
  else
    {
      g_assert_cmpstr (data->buffer, ==, data->data);
      g_assert_false (g_input_stream_is_closed (data->istream));
      g_input_stream_close_async (data->istream, 0, NULL, iclosed_cb, data);
    }
}

static void
ipending_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;

  error = NULL;
  g_input_stream_read_finish (data->istream, res, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  g_error_free (error);
}

static void
skipped_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gssize size;

  error = NULL;
  size = g_input_stream_skip_finish (data->istream, res, &error);
  g_assert_no_error (error);
  g_assert_cmpint (size, ==, data->pos);

  g_input_stream_read_async (data->istream,
                             data->buffer + data->pos,
                             strlen (data->data) - data->pos,
                             0,
                             NULL,
                             read_cb,
                             data);
  /* check that we get a pending error */
  g_input_stream_read_async (data->istream,
                             data->buffer + data->pos,
                             strlen (data->data) - data->pos,
                             0,
                             NULL,
                             ipending_cb,
                             data);
}

static void
opened_cb (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  GFileInputStream *base;
  CreateDeleteData *data = user_data;
  GError *error;

  error = NULL;
  base = g_file_read_finish (data->file, res, &error);
  g_assert_no_error (error);

  if (data->buffersize == 0)
    data->istream = G_INPUT_STREAM (g_object_ref (base));
  else
    data->istream = g_buffered_input_stream_new_sized (G_INPUT_STREAM (base), data->buffersize);
  g_object_unref (base);

  data->buffer = g_new0 (gchar, strlen (data->data) + 1);

  /* copy initial segment directly, then skip */
  memcpy (data->buffer, data->data, 10);
  data->pos = 10;

  g_input_stream_skip_async (data->istream,
                             10,
                             0,
                             NULL,
                             skipped_cb,
                             data);
}

static void
oclosed_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gboolean ret;

  error = NULL;
  ret = g_output_stream_close_finish (data->ostream, res, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  g_assert_true (g_output_stream_is_closed (data->ostream));

  g_file_read_async (data->file, 0, NULL, opened_cb, data);
}

static void
written_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  gssize size;
  GError *error;

  error = NULL;
  size = g_output_stream_write_finish (data->ostream, res, &error);
  g_assert_no_error (error);

  data->pos += size;
  if (data->pos < strlen (data->data))
    {
      g_output_stream_write_async (data->ostream,
                                   data->data + data->pos,
                                   strlen (data->data) - data->pos,
                                   0,
                                   NULL,
                                   written_cb,
                                   data);
    }
  else
    {
      g_assert_false (g_output_stream_is_closed (data->ostream));
      g_output_stream_close_async (data->ostream, 0, NULL, oclosed_cb, data);
    }
}

static void
opending_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;

  error = NULL;
  g_output_stream_write_finish (data->ostream, res, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  g_error_free (error);
}

static void
created_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  GFileOutputStream *base;
  CreateDeleteData *data = user_data;
  GError *error;

  error = NULL;
  base = g_file_create_finish (G_FILE (source), res, &error);
  g_assert_no_error (error);
  g_assert_true (g_file_query_exists (data->file, NULL));

  if (data->buffersize == 0)
    data->ostream = G_OUTPUT_STREAM (g_object_ref (base));
  else
    data->ostream = g_buffered_output_stream_new_sized (G_OUTPUT_STREAM (base), data->buffersize);
  g_object_unref (base);

  g_output_stream_write_async (data->ostream,
                               data->data,
                               strlen (data->data),
                               0,
                               NULL,
                               written_cb,
                               data);
  /* check that we get a pending error */
  g_output_stream_write_async (data->ostream,
                               data->data,
                               strlen (data->data),
                               0,
                               NULL,
                               opending_cb,
                               data);
}

static void
stop_timeout (gpointer user_data)
{
  CreateDeleteData *data = user_data;

  data->timed_out = TRUE;
  g_main_context_wakeup (data->context);
}

/*
 * This test does a fully async create-write-read-delete.
 * Callbackistan.
 */
static void
test_create_delete (gconstpointer d)
{
  GError *error;
  CreateDeleteData *data;
  GFileIOStream *iostream;

  data = g_new0 (CreateDeleteData, 1);

  data->buffersize = GPOINTER_TO_INT (d);
  data->data = "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ0123456789";
  data->pos = 0;

  data->file = g_file_new_tmp ("g_file_create_delete_XXXXXX",
			       &iostream, NULL);
  g_assert_nonnull (data->file);
  g_object_unref (iostream);

  data->monitor_path = g_file_get_path (data->file);
  remove (data->monitor_path);

  g_assert_false (g_file_query_exists (data->file, NULL));

  error = NULL;
  data->monitor = g_file_monitor_file (data->file, 0, NULL, &error);
  g_assert_no_error (error);

  /* This test doesn't work with GPollFileMonitor, because it assumes
   * that the monitor will notice a create immediately followed by a
   * delete, rather than coalescing them into nothing.
   */
  /* This test also doesn't work with GKqueueFileMonitor because of
   * the same reason. Kqueue is able to return a kevent when a file is
   * created or deleted in a directory. However, the kernel doesn't tell
   * the program file names, so GKqueueFileMonitor has to calculate the
   * difference itself. This is usually too slow for rapid file creation
   * and deletion tests.
   */
  if (strcmp (G_OBJECT_TYPE_NAME (data->monitor), "GPollFileMonitor") == 0 ||
      strcmp (G_OBJECT_TYPE_NAME (data->monitor), "GKqueueFileMonitor") == 0)
    {
      g_test_skip ("skipping test for this GFileMonitor implementation");
      goto skip;
    }

  g_file_monitor_set_rate_limit (data->monitor, 100);

  g_signal_connect (data->monitor, "changed", G_CALLBACK (monitor_changed), data);

  /* Use the global default main context */
  data->context = NULL;
  data->timeout = g_timeout_add_seconds_once (10, stop_timeout, data);

  g_file_create_async (data->file, 0, 0, NULL, created_cb, data);

  while (!data->timed_out &&
         (data->monitor_created == 0 ||
          data->monitor_deleted == 0 ||
          data->monitor_changed == 0 ||
          !data->file_deleted))
    g_main_context_iteration (data->context, TRUE);

  g_source_remove (data->timeout);

  g_assert_false (data->timed_out);
  g_assert_true (data->file_deleted);
  g_assert_cmpint (data->monitor_created, ==, 1);
  g_assert_cmpint (data->monitor_deleted, ==, 1);
  g_assert_cmpint (data->monitor_changed, >, 0);

  g_assert_false (g_file_monitor_is_cancelled (data->monitor));
  g_file_monitor_cancel (data->monitor);
  g_assert_true (g_file_monitor_is_cancelled (data->monitor));

  g_clear_pointer (&data->context, g_main_context_unref);
  g_object_unref (data->ostream);
  g_object_unref (data->istream);

 skip:
  g_object_unref (data->monitor);
  g_object_unref (data->file);
  g_free (data->monitor_path);
  g_free (data->buffer);
  g_free (data);
}

static const gchar *original_data =
    "/**\n"
    " * g_file_replace_contents_async:\n"
    "**/\n";

static const gchar *replace_data =
    "/**\n"
    " * g_file_replace_contents_async:\n"
    " * @file: input #GFile.\n"
    " * @contents: string of contents to replace the file with.\n"
    " * @length: the length of @contents in bytes.\n"
    " * @etag: (nullable): a new <link linkend=\"gfile-etag\">entity tag</link> for the @file, or %NULL\n"
    " * @make_backup: %TRUE if a backup should be created.\n"
    " * @flags: a set of #GFileCreateFlags.\n"
    " * @cancellable: optional #GCancellable object, %NULL to ignore.\n"
    " * @callback: a #GAsyncReadyCallback to call when the request is satisfied\n"
    " * @user_data: the data to pass to callback function\n"
    " * \n"
    " * Starts an asynchronous replacement of @file with the given \n"
    " * @contents of @length bytes. @etag will replace the document's\n"
    " * current entity tag.\n"
    " * \n"
    " * When this operation has completed, @callback will be called with\n"
    " * @user_user data, and the operation can be finalized with \n"
    " * g_file_replace_contents_finish().\n"
    " * \n"
    " * If @cancellable is not %NULL, then the operation can be cancelled by\n"
    " * triggering the cancellable object from another thread. If the operation\n"
    " * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. \n"
    " * \n"
    " * If @make_backup is %TRUE, this function will attempt to \n"
    " * make a backup of @file.\n"
    " **/\n";

typedef struct
{
  GFile *file;
  const gchar *data;
  GMainLoop *loop;
  gboolean again;
} ReplaceLoadData;

static void replaced_cb (GObject      *source,
                         GAsyncResult *res,
                         gpointer      user_data);

static void
loaded_cb (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  ReplaceLoadData *data = user_data;
  gboolean ret;
  GError *error;
  gchar *contents;
  gsize length;

  error = NULL;
  ret = g_file_load_contents_finish (data->file, res, &contents, &length, NULL, &error);
  g_assert_true (ret);
  g_assert_no_error (error);
  g_assert_cmpint (length, ==, strlen (data->data));
  g_assert_cmpstr (contents, ==, data->data);

  g_free (contents);

  if (data->again)
    {
      data->again = FALSE;
      data->data = "pi pa po";

      g_file_replace_contents_async (data->file,
                                     data->data,
                                     strlen (data->data),
                                     NULL,
                                     FALSE,
                                     0,
                                     NULL,
                                     replaced_cb,
                                     data);
    }
  else
    {
       error = NULL;
       ret = g_file_delete (data->file, NULL, &error);
       g_assert_no_error (error);
       g_assert_true (ret);
       g_assert_false (g_file_query_exists (data->file, NULL));

       g_main_loop_quit (data->loop);
    }
}

static void
replaced_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  ReplaceLoadData *data = user_data;
  GError *error;

  error = NULL;
  g_file_replace_contents_finish (data->file, res, NULL, &error);
  g_assert_no_error (error);

  g_file_load_contents_async (data->file, NULL, loaded_cb, data);
}

static void
test_replace_load (void)
{
  ReplaceLoadData *data;
  const gchar *path;
  GFileIOStream *iostream;

  data = g_new0 (ReplaceLoadData, 1);
  data->again = TRUE;
  data->data = replace_data;

  data->file = g_file_new_tmp ("g_file_replace_load_XXXXXX",
			       &iostream, NULL);
  g_assert_nonnull (data->file);
  g_object_unref (iostream);

  path = g_file_peek_path (data->file);
  remove (path);

  g_assert_false (g_file_query_exists (data->file, NULL));

  data->loop = g_main_loop_new (NULL, FALSE);

  g_file_replace_contents_async (data->file,
                                 data->data,
                                 strlen (data->data),
                                 NULL,
                                 FALSE,
                                 0,
                                 NULL,
                                 replaced_cb,
                                 data);

  g_main_loop_run (data->loop);

  g_main_loop_unref (data->loop);
  g_object_unref (data->file);
  g_free (data);
}

static void
test_replace_cancel (void)
{
  GFile *tmpdir, *file;
  GFileOutputStream *ostream;
  GFileEnumerator *fenum;
  GFileInfo *info;
  GCancellable *cancellable;
  gchar *path;
  gchar *contents;
  gsize nwrote, length;
  guint count;
  GError *error = NULL;

  g_test_bug ("https://bugzilla.gnome.org/629301");

  path = g_dir_make_tmp ("g_file_replace_cancel_XXXXXX", &error);
  g_assert_no_error (error);
  tmpdir = g_file_new_for_path (path);
  g_free (path);

  file = g_file_get_child (tmpdir, "file");
  g_file_replace_contents (file,
                           original_data,
                           strlen (original_data),
                           NULL, FALSE, 0, NULL,
                           NULL, &error);
  g_assert_no_error (error);

  ostream = g_file_replace (file, NULL, TRUE, 0, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                             replace_data, strlen (replace_data),
                             &nwrote, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nwrote, ==, strlen (replace_data));

  /* At this point there should be two files; the original and the
   * temporary.
   */
  fenum = g_file_enumerate_children (tmpdir, NULL, 0, NULL, &error);
  g_assert_no_error (error);

  info = g_file_enumerator_next_file (fenum, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (info);
  g_object_unref (info);
  info = g_file_enumerator_next_file (fenum, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (info);
  g_object_unref (info);

  g_file_enumerator_close (fenum, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (fenum);

  /* Also test the g_file_enumerator_iterate() API */
  fenum = g_file_enumerate_children (tmpdir, NULL, 0, NULL, &error);
  g_assert_no_error (error);
  count = 0;

  while (TRUE)
    {
      gboolean ret = g_file_enumerator_iterate (fenum, &info, NULL, NULL, &error);
      g_assert_true (ret);
      g_assert_no_error (error);
      if (!info)
        break;
      count++;
    }
  g_assert_cmpint (count, ==, 2);

  g_file_enumerator_close (fenum, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (fenum);

  /* Now test just getting child from the g_file_enumerator_iterate() API */
  fenum = g_file_enumerate_children (tmpdir, "standard::name", 0, NULL, &error);
  g_assert_no_error (error);
  count = 0;

  while (TRUE)
    {
      GFile *child;
      gboolean ret = g_file_enumerator_iterate (fenum, NULL, &child, NULL, &error);

      g_assert_true (ret);
      g_assert_no_error (error);

      if (!child)
        break;

      g_assert_true (G_IS_FILE (child));
      count++;
    }
  g_assert_cmpint (count, ==, 2);

  g_file_enumerator_close (fenum, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (fenum);

  /* Make sure the temporary gets deleted even if we cancel. */
  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (G_OUTPUT_STREAM (ostream), cancellable, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);

  g_object_unref (cancellable);
  g_object_unref (ostream);

  /* Make sure that file contents wasn't actually replaced. */
  g_file_load_contents (file,
                        NULL,
                        &contents,
                        &length,
                        NULL,
                        &error);
  g_assert_no_error (error);
  g_assert_cmpstr (contents, ==, original_data);
  g_free (contents);

  g_file_delete (file, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (file);

  /* This will only succeed if the temp file was deleted. */
  g_file_delete (tmpdir, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (tmpdir);
}

static void
test_replace_symlink (void)
{
#ifdef G_OS_UNIX
  gchar *tmpdir_path = NULL;
  GFile *tmpdir = NULL, *source_file = NULL, *target_file = NULL;
  GFileOutputStream *stream = NULL;
  const gchar *new_contents = "this is a test message which should be written to source and not target";
  gsize n_written;
  GFileEnumerator *enumerator = NULL;
  GFileInfo *info = NULL;
  gchar *contents = NULL;
  gsize length = 0;
  GError *local_error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2325");
  g_test_summary ("Test that G_FILE_CREATE_REPLACE_DESTINATION doesn’t follow symlinks");

  /* Create a fresh, empty working directory. */
  tmpdir_path = g_dir_make_tmp ("g_file_replace_symlink_XXXXXX", &local_error);
  g_assert_no_error (local_error);
  tmpdir = g_file_new_for_path (tmpdir_path);

  g_test_message ("Using temporary directory %s", tmpdir_path);
  g_free (tmpdir_path);

  source_file = g_file_get_child (tmpdir, "source");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*symlink_value*failed*");
  g_assert_false (g_file_make_symbolic_link (source_file, NULL, NULL, &local_error));
  g_assert_no_error (local_error);
  g_test_assert_expected_messages ();

  g_assert_false (g_file_make_symbolic_link (source_file, "", NULL, &local_error));
  g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_clear_object (&source_file);
  g_clear_error (&local_error);

  /* Create symlink `source` which points to `target`. */
  source_file = g_file_get_child (tmpdir, "source");
  target_file = g_file_get_child (tmpdir, "target");
  g_file_make_symbolic_link (source_file, "target", NULL, &local_error);
  g_assert_no_error (local_error);

  /* Ensure that `target` doesn’t exist */
  g_assert_false (g_file_query_exists (target_file, NULL));

  /* Replace the `source` symlink with a regular file using
   * %G_FILE_CREATE_REPLACE_DESTINATION, which should replace it *without*
   * following the symlink */
  stream = g_file_replace (source_file, NULL, FALSE  /* no backup */,
                           G_FILE_CREATE_REPLACE_DESTINATION, NULL, &local_error);
  g_assert_no_error (local_error);

  g_output_stream_write_all (G_OUTPUT_STREAM (stream), new_contents, strlen (new_contents),
                             &n_written, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (n_written, ==, strlen (new_contents));

  g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&stream);

  /* At this point, there should still only be one file: `source`. It should
   * now be a regular file. `target` should not exist. */
  enumerator = g_file_enumerate_children (tmpdir,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
  g_assert_no_error (local_error);

  info = g_file_enumerator_next_file (enumerator, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (info);

  g_assert_cmpstr (g_file_info_get_name (info), ==, "source");
  g_assert_cmpint (g_file_info_get_file_type (info), ==, G_FILE_TYPE_REGULAR);

  g_clear_object (&info);

  info = g_file_enumerator_next_file (enumerator, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_null (info);

  g_file_enumerator_close (enumerator, NULL, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&enumerator);

  /* Double-check that `target` doesn’t exist */
  g_assert_false (g_file_query_exists (target_file, NULL));

  /* Check the content of `source`. */
  g_file_load_contents (source_file,
                        NULL,
                        &contents,
                        &length,
                        NULL,
                        &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (contents, ==, new_contents);
  g_assert_cmpuint (length, ==, strlen (new_contents));
  g_free (contents);

  /* Tidy up. */
  g_file_delete (source_file, NULL, &local_error);
  g_assert_no_error (local_error);

  g_file_delete (tmpdir, NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&target_file);
  g_clear_object (&source_file);
  g_clear_object (&tmpdir);
#else  /* if !G_OS_UNIX */
  g_test_skip ("Symlink replacement tests can only be run on Unix")
#endif
}

static void
test_replace_symlink_using_etag (void)
{
#ifdef G_OS_UNIX
  gchar *tmpdir_path = NULL;
  GFile *tmpdir = NULL, *source_file = NULL, *target_file = NULL;
  GFileOutputStream *stream = NULL;
  const gchar *old_contents = "this is a test message which should be written to target and then overwritten";
  gchar *old_etag = NULL;
  const gchar *new_contents = "this is an updated message";
  gsize n_written;
  gchar *contents = NULL;
  gsize length = 0;
  GFileInfo *info = NULL;
  GError *local_error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2417");
  g_test_summary ("Test that ETag checks work when replacing a file through a symlink");

  /* Create a fresh, empty working directory. */
  tmpdir_path = g_dir_make_tmp ("g_file_replace_symlink_using_etag_XXXXXX", &local_error);
  g_assert_no_error (local_error);
  tmpdir = g_file_new_for_path (tmpdir_path);

  g_test_message ("Using temporary directory %s", tmpdir_path);
  g_free (tmpdir_path);

  /* Create symlink `source` which points to `target`. */
  source_file = g_file_get_child (tmpdir, "source");
  target_file = g_file_get_child (tmpdir, "target");
  g_file_make_symbolic_link (source_file, "target", NULL, &local_error);
  g_assert_no_error (local_error);

  /* Sleep for at least 1s to ensure the mtimes of `source` and `target` differ,
   * as that’s what _g_local_file_info_create_etag() uses to create the ETag,
   * and one failure mode we’re testing for is that the ETags of `source` and
   * `target` are conflated. */
  sleep (1);

  /* Create `target` with some arbitrary content. */
  stream = g_file_create (target_file, G_FILE_CREATE_NONE, NULL, &local_error);
  g_assert_no_error (local_error);
  g_output_stream_write_all (G_OUTPUT_STREAM (stream), old_contents, strlen (old_contents),
                             &n_written, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (n_written, ==, strlen (old_contents));

  g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, &local_error);
  g_assert_no_error (local_error);

  old_etag = g_file_output_stream_get_etag (stream);
  g_assert_nonnull (old_etag);
  g_assert_cmpstr (old_etag, !=, "");

  g_clear_object (&stream);

  /* Sleep again to ensure the ETag changes again. */
  sleep (1);

  /* Write out a new copy of the `target`, checking its ETag first. This should
   * replace `target` by following the symlink. */
  stream = g_file_replace (source_file, old_etag, FALSE  /* no backup */,
                           G_FILE_CREATE_NONE, NULL, &local_error);
  g_assert_no_error (local_error);

  g_output_stream_write_all (G_OUTPUT_STREAM (stream), new_contents, strlen (new_contents),
                             &n_written, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (n_written, ==, strlen (new_contents));

  g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&stream);

  /* At this point, there should be a regular file, `target`, containing
   * @new_contents; and a symlink `source` which points to `target`. */
  g_assert_cmpint (g_file_query_file_type (source_file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL), ==, G_FILE_TYPE_SYMBOLIC_LINK);
  g_assert_cmpint (g_file_query_file_type (target_file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL), ==, G_FILE_TYPE_REGULAR);

  /* Check the content of `target`. */
  g_file_load_contents (target_file,
                        NULL,
                        &contents,
                        &length,
                        NULL,
                        &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (contents, ==, new_contents);
  g_assert_cmpuint (length, ==, strlen (new_contents));
  g_free (contents);

  /* And check its ETag value has changed. */
  info = g_file_query_info (target_file, G_FILE_ATTRIBUTE_ETAG_VALUE,
                            G_FILE_QUERY_INFO_NONE, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (g_file_info_get_etag (info), !=, old_etag);

  g_clear_object (&info);
  g_free (old_etag);

  /* Tidy up. */
  g_file_delete (target_file, NULL, &local_error);
  g_assert_no_error (local_error);

  g_file_delete (source_file, NULL, &local_error);
  g_assert_no_error (local_error);

  g_file_delete (tmpdir, NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&target_file);
  g_clear_object (&source_file);
  g_clear_object (&tmpdir);
#else  /* if !G_OS_UNIX */
  g_test_skip ("Symlink replacement tests can only be run on Unix")
#endif
}

/* FIXME: These tests have only been checked on Linux. Most of them are probably
 * applicable on Windows, too, but that has not been tested yet.
 * See https://gitlab.gnome.org/GNOME/glib/-/issues/2325 */
#ifdef __linux__

/* Different arrangements of parent tmp directory. */
typedef enum
{
  FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
  FILE_TEST_DIRECTORY_SETUP_TYPE_READ_ONLY,
} FileTestDirectorySetupType;

/* Different kinds of file which create_test_file() can create. */
typedef enum
{
  FILE_TEST_SETUP_TYPE_NONEXISTENT,
  FILE_TEST_SETUP_TYPE_REGULAR_EMPTY,
  FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY,
  FILE_TEST_SETUP_TYPE_DIRECTORY,
  FILE_TEST_SETUP_TYPE_SOCKET,
  FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING,
  FILE_TEST_SETUP_TYPE_SYMLINK_VALID,
} FileTestSetupType;

/* Create file `tmpdir/basename`, of type @setup_type, and chmod it to
 * @setup_mode. Return the #GFile representing it. Abort on any errors. */
static GFile *
create_test_file (GFile             *tmpdir,
                  const gchar       *basename,
                  FileTestSetupType  setup_type,
                  guint              setup_mode)
{
  GFile *test_file = g_file_get_child (tmpdir, basename);
  gchar *target_basename = g_strdup_printf ("%s-target", basename);  /* for symlinks */
  GFile *target_file = g_file_get_child (tmpdir, target_basename);
  GError *local_error = NULL;

  switch (setup_type)
    {
    case FILE_TEST_SETUP_TYPE_NONEXISTENT:
      /* Nothing to do here. */
      g_assert (setup_mode == 0);
      break;
    case FILE_TEST_SETUP_TYPE_REGULAR_EMPTY:
    case FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY:
        {
          gchar *contents = NULL;

          if (setup_type == FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY)
            contents = g_strdup_printf ("this is some test content in %s", basename);
          else
            contents = g_strdup ("");

          g_file_set_contents (g_file_peek_path (test_file), contents, -1, &local_error);
          g_assert_no_error (local_error);

          g_file_set_attribute_uint32 (test_file, G_FILE_ATTRIBUTE_UNIX_MODE,
                                       setup_mode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                       NULL, &local_error);
          g_assert_no_error (local_error);

          g_free (contents);
          break;
        }
    case FILE_TEST_SETUP_TYPE_DIRECTORY:
      g_assert (setup_mode == 0);

      g_file_make_directory (test_file, NULL, &local_error);
      g_assert_no_error (local_error);
      break;
    case FILE_TEST_SETUP_TYPE_SOCKET:
      g_assert_no_errno (mknod (g_file_peek_path (test_file), S_IFSOCK | setup_mode, 0));
      break;
    case FILE_TEST_SETUP_TYPE_SYMLINK_VALID:
      g_file_set_contents (g_file_peek_path (target_file), "target file", -1, &local_error);
      g_assert_no_error (local_error);
      G_GNUC_FALLTHROUGH;
    case FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING:
      /* Permissions on a symlink are not used by the kernel, so are only
       * applicable if the symlink is valid (and are applied to the target) */
      g_assert (setup_type != FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING || setup_mode == 0);

      g_file_make_symbolic_link (test_file, target_basename, NULL, &local_error);
      g_assert_no_error (local_error);

      if (setup_type == FILE_TEST_SETUP_TYPE_SYMLINK_VALID)
        {
          g_file_set_attribute_uint32 (test_file, G_FILE_ATTRIBUTE_UNIX_MODE,
                                       setup_mode, G_FILE_QUERY_INFO_NONE,
                                       NULL, &local_error);
          g_assert_no_error (local_error);
        }

      if (setup_type == FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING)
        {
          /* Ensure that the target doesn’t exist */
          g_assert_false (g_file_query_exists (target_file, NULL));
        }
      break;
    default:
      g_assert_not_reached ();
    }

  g_clear_object (&target_file);
  g_free (target_basename);

  return g_steal_pointer (&test_file);
}

/* Check that @test_file is of the @expected_type, has the @expected_mode, and
 * (if it’s a regular file) has the @expected_contents or (if it’s a symlink)
 * has the symlink target given by @expected_contents.
 *
 * @test_file must point to the file `tmpdir/basename`.
 *
 * Aborts on any errors or mismatches against the expectations.
 */
static void
check_test_file (GFile             *test_file,
                 GFile             *tmpdir,
                 const gchar       *basename,
                 FileTestSetupType  expected_type,
                 guint              expected_mode,
                 const gchar       *expected_contents)
{
  gchar *target_basename = g_strdup_printf ("%s-target", basename);  /* for symlinks */
  GFile *target_file = g_file_get_child (tmpdir, target_basename);
  GFileType test_file_type = g_file_query_file_type (test_file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
  GFileInfo *info = NULL;
  GError *local_error = NULL;

  switch (expected_type)
    {
    case FILE_TEST_SETUP_TYPE_NONEXISTENT:
      g_assert (expected_mode == 0);
      g_assert (expected_contents == NULL);

      g_assert_false (g_file_query_exists (test_file, NULL));
      g_assert_cmpint (test_file_type, ==, G_FILE_TYPE_UNKNOWN);
      break;
    case FILE_TEST_SETUP_TYPE_REGULAR_EMPTY:
    case FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY:
      g_assert (expected_type != FILE_TEST_SETUP_TYPE_REGULAR_EMPTY || expected_contents == NULL);
      g_assert (expected_type != FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY || expected_contents != NULL);

      g_assert_cmpint (test_file_type, ==, G_FILE_TYPE_REGULAR);

      info = g_file_query_info (test_file,
                                G_FILE_ATTRIBUTE_STANDARD_SIZE ","
                                G_FILE_ATTRIBUTE_UNIX_MODE,
                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
      g_assert_no_error (local_error);

      if (expected_type == FILE_TEST_SETUP_TYPE_REGULAR_EMPTY)
        g_assert_cmpint (g_file_info_get_size (info), ==, 0);
      else
        g_assert_cmpint (g_file_info_get_size (info), >, 0);

      if (expected_contents != NULL)
        {
          gchar *contents = NULL;
          gsize length = 0;

          g_file_get_contents (g_file_peek_path (test_file), &contents, &length, &local_error);
          g_assert_no_error (local_error);

          g_assert_cmpstr (contents, ==, expected_contents);
          g_free (contents);
        }

      g_assert_cmpuint (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_UNIX_MODE) & 0777, ==, expected_mode);

      break;
    case FILE_TEST_SETUP_TYPE_DIRECTORY:
      g_assert (expected_mode == 0);
      g_assert (expected_contents == NULL);

      g_assert_cmpint (test_file_type, ==, G_FILE_TYPE_DIRECTORY);
      break;
    case FILE_TEST_SETUP_TYPE_SOCKET:
      g_assert (expected_contents == NULL);

      g_assert_cmpint (test_file_type, ==, G_FILE_TYPE_SPECIAL);

      info = g_file_query_info (test_file,
                                G_FILE_ATTRIBUTE_UNIX_MODE,
                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
      g_assert_no_error (local_error);

      g_assert_cmpuint (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_UNIX_MODE) & 0777, ==, expected_mode);
      break;
    case FILE_TEST_SETUP_TYPE_SYMLINK_VALID:
    case FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING:
        {
          GFile *symlink_target_file = NULL;

          /* Permissions on a symlink are not used by the kernel, so are only
           * applicable if the symlink is valid (and are applied to the target) */
          g_assert (expected_type != FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING || expected_mode == 0);
          g_assert (expected_contents != NULL);

          g_assert_cmpint (test_file_type, ==, G_FILE_TYPE_SYMBOLIC_LINK);

          info = g_file_query_info (test_file,
                                    G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
          g_assert_no_error (local_error);

          g_assert_cmpstr (g_file_info_get_symlink_target (info), ==, expected_contents);

          symlink_target_file = g_file_get_child (tmpdir, g_file_info_get_symlink_target (info));
          if (expected_type == FILE_TEST_SETUP_TYPE_SYMLINK_VALID)
            g_assert_true (g_file_query_exists (symlink_target_file, NULL));
          else
            g_assert_false (g_file_query_exists (symlink_target_file, NULL));

          if (expected_type == FILE_TEST_SETUP_TYPE_SYMLINK_VALID)
            {
              GFileInfo *target_info = NULL;

              /* Need to re-query the info so we follow symlinks */
              target_info = g_file_query_info (test_file,
                                               G_FILE_ATTRIBUTE_UNIX_MODE,
                                               G_FILE_QUERY_INFO_NONE, NULL, &local_error);
              g_assert_no_error (local_error);

              g_assert_cmpuint (g_file_info_get_attribute_uint32 (target_info, G_FILE_ATTRIBUTE_UNIX_MODE) & 0777, ==, expected_mode);

              g_clear_object (&target_info);
            }

          g_clear_object (&symlink_target_file);
          break;
        }
    default:
      g_assert_not_reached ();
    }

  g_clear_object (&info);
  g_clear_object (&target_file);
  g_free (target_basename);
}

#endif  /* __linux__ */

#ifdef __linux__
/*
 * check_cap_dac_override:
 * @tmpdir: A temporary directory in which we can create and delete files
 *
 * Check whether the current process can bypass DAC permissions.
 *
 * Traditionally, "privileged" processes (those with effective uid 0)
 * could do this (and bypass many other checks), and "unprivileged"
 * processes could not.
 *
 * In Linux, the special powers of euid 0 are divided into many
 * capabilities: see `capabilities(7)`. The one we are interested in
 * here is `CAP_DAC_OVERRIDE`.
 *
 * We do this generically instead of actually looking at the capability
 * bits, so that the right thing will happen on non-Linux Unix
 * implementations, in particular if they have something equivalent to
 * but not identical to Linux permissions.
 *
 * Returns: %TRUE if we have Linux `CAP_DAC_OVERRIDE` or equivalent
 *  privileges
 */
static gboolean
check_cap_dac_override (const char *tmpdir)
{
  gchar *dac_denies_write;
  gchar *inside;
  gboolean have_cap;

  dac_denies_write = g_build_filename (tmpdir, "dac-denies-write", NULL);
  inside = g_build_filename (dac_denies_write, "inside", NULL);

  g_assert_no_errno (mkdir (dac_denies_write, S_IRWXU));
  g_assert_no_errno (chmod (dac_denies_write, 0));

  if (mkdir (inside, S_IRWXU) == 0)
    {
      g_test_message ("Looks like we have CAP_DAC_OVERRIDE or equivalent");
      g_assert_no_errno (rmdir (inside));
      have_cap = TRUE;
    }
  else
    {
      int saved_errno = errno;

      g_test_message ("We do not have CAP_DAC_OVERRIDE or equivalent");
      g_assert_cmpint (saved_errno, ==, EACCES);
      have_cap = FALSE;
    }

  g_assert_no_errno (chmod (dac_denies_write, S_IRWXU));
  g_assert_no_errno (rmdir (dac_denies_write));
  g_free (dac_denies_write);
  g_free (inside);
  return have_cap;
}
#endif

/* A big test for g_file_replace() and g_file_replace_readwrite(). The
 * @test_data is a boolean: %TRUE to test g_file_replace_readwrite(), %FALSE to
 * test g_file_replace(). The test setup and checks are identical for both
 * functions; in the case of testing g_file_replace_readwrite(), only the output
 * stream side of the returned #GIOStream is tested. i.e. We test the write
 * behaviour of both functions is identical.
 *
 * This is intended to test all static behaviour of the function: for each test
 * scenario, a temporary directory is set up with a source file (and maybe some
 * other files) in a set configuration, g_file_replace{,_readwrite}() is called,
 * and the final state of the directory is checked.
 *
 * This test does not check dynamic behaviour or race conditions. For example,
 * it does not test what happens if the source file is deleted from another
 * process half-way through a call to g_file_replace().
 */
static void
test_replace (gconstpointer test_data)
{
#ifdef __linux__
  gboolean read_write = GPOINTER_TO_UINT (test_data);
  const gchar *new_contents = "this is a new test message which should be written to source";
  const gchar *original_source_contents = "this is some test content in source";
  const gchar *original_backup_contents = "this is some test content in source~";
  mode_t current_umask = umask (0);
  guint32 default_public_mode = 0666 & ~current_umask;
  guint32 default_private_mode = 0600;

  const struct
    {
      /* Arguments to pass to g_file_replace(). */
      gboolean replace_make_backup;
      GFileCreateFlags replace_flags;
      const gchar *replace_etag;  /* (nullable) */

      /* File system setup. */
      FileTestDirectorySetupType setup_directory_type;
      FileTestSetupType setup_source_type;
      guint setup_source_mode;
      FileTestSetupType setup_backup_type;
      guint setup_backup_mode;
      gboolean skip_if_cap_dac_override;

      /* Expected results. */
      gboolean expected_success;
      GQuark expected_error_domain;
      gint expected_error_code;

      /* Expected final file system state. */
      guint expected_n_files;
      FileTestSetupType expected_source_type;
      guint expected_source_mode;
      const gchar *expected_source_contents;  /* content for a regular file, or target for a symlink; NULL otherwise */
      FileTestSetupType expected_backup_type;
      guint expected_backup_mode;
      const gchar *expected_backup_contents;  /* content for a regular file, or target for a symlink; NULL otherwise */
    }
  tests[] =
    {
      /* replace_make_backup == FALSE, replace_flags == NONE, replace_etag == NULL,
       * all the different values of setup_source_type, mostly with a backup
       * file created to check it’s not modified */
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_etag set to an invalid value, with setup_source_type as a
       * regular non-empty file; replacement should fail */
      {
        FALSE, G_FILE_CREATE_NONE, "incorrect etag",
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_WRONG_ETAG,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_make_backup == TRUE, replace_flags == NONE, replace_etag == NULL,
       * all the different values of setup_source_type, with a backup
       * file created to check it’s either replaced or the operation fails */
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode, NULL,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        /* The final situation here is a bit odd; the backup file is a bit
         * pointless as the original source file was a dangling symlink.
         * Theoretically the backup file should be that symlink, pointing to
         * `source-target`, and hence no longer dangling, as that file has now
         * been created as the new source content, since REPLACE_DESTINATION was
         * not specified. However, the code instead creates an empty regular
         * file as the backup. FIXME: This seems acceptable for now, but not
         * entirely ideal and would be good to fix at some point. */
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, 0777 & ~current_umask, NULL,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        /* FIXME: The permissions for the backup file are just the default umask,
         * but should probably be the same as the permissions for the source
         * file (`default_public_mode`). This probably arises from the fact that
         * symlinks don’t have permissions. */
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, 0777 & ~current_umask, "target file",
      },

      /* replace_make_backup == TRUE, replace_flags == NONE, replace_etag == NULL,
       * setup_source_type is a regular file, with a backup file of every type
       * created to check it’s either replaced or the operation fails */
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, FALSE,
        TRUE, 0, 0,
        /* the third file is `source~-target`, the original target of the old
         * backup symlink */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },

      /* replace_make_backup == FALSE, replace_flags == REPLACE_DESTINATION,
       * replace_etag == NULL, all the different values of setup_source_type,
       * mostly with a backup file created to check it’s not modified */
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        /* the third file is `source-target`, the original target of the old
         * source file */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_flags == REPLACE_DESTINATION, replace_etag set to an invalid
       * value, with setup_source_type as a regular non-empty file; replacement
       * should fail */
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, "incorrect etag",
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_WRONG_ETAG,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_make_backup == TRUE, replace_flags == REPLACE_DESTINATION,
       * replace_etag == NULL, all the different values of setup_source_type,
       * with a backup file created to check it’s either replaced or the
       * operation fails */
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode, NULL,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0, "source-target",
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        /* the third file is `source-target`, the original target of the old
         * source file */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
      },

      /* replace_make_backup == TRUE, replace_flags == REPLACE_DESTINATION,
       * replace_etag == NULL, setup_source_type is a regular file, with a
       * backup file of every type created to check it’s either replaced or the
       * operation fails */
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0, FALSE,
        FALSE, G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0, FALSE,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, FALSE,
        TRUE, 0, 0,
        /* the third file is `source~-target`, the original target of the old
         * backup symlink */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },

      /* several different setups with replace_flags == PRIVATE */
      {
        FALSE, G_FILE_CREATE_PRIVATE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_private_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, G_FILE_CREATE_PRIVATE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        /* the file isn’t being replaced, so it should keep its existing permissions */
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_private_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, FALSE,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_private_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },

      /* make the initial source file unreadable, so the replace operation
       * should fail
       *
       * Permissions are ignored if we have CAP_DAC_OVERRIDE or equivalent,
       * and in particular if we're root. In this scenario,we need to skip it */
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_NORMAL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, 0  /* most restrictive permissions */,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, TRUE,
        FALSE, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
        1, FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, 0, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },

      /* A symlink in an unwriteable directory, with the need to replace the
       * destination and make a backup, tests the fallback backup code path. It
       * can’t succeed, because even if the file replace would work, the backup
       * file cannot be created in a read-only directory. */
      {
        TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_READ_ONLY,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, TRUE,
        FALSE, G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
        2, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },

      /* Same as above, but without trying to create a backup. We expect this to
       * fail because replacing the destination requires deleting and
       * re-creating the file, which can’t happen in a read-only directory. */
      {
        FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_READ_ONLY,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, TRUE,
        FALSE, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
        2, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },

      /* Same as above, but without trying to create a backup or trying to
       * replace the file. */
      {
        FALSE, G_FILE_CREATE_NONE, NULL,
        FILE_TEST_DIRECTORY_SETUP_TYPE_READ_ONLY,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, TRUE,
        TRUE, 0, 0,
        /* the second file is the source-target file, which should contain the new_contents */
        2, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
    };
  gsize i;

  g_test_summary ("Test various situations for g_file_replace()");

  /* Reset the umask after querying it above. There’s no way to query it without
   * changing it. */
  umask (current_umask);
  g_test_message ("Current umask: %u", current_umask);

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      gchar *tmpdir_path = NULL;
      GFile *tmpdir = NULL, *source_file = NULL, *backup_file = NULL;
      GFileOutputStream *output_stream = NULL;
      GFileIOStream *io_stream = NULL;
      GFileEnumerator *enumerator = NULL;
      GFileInfo *info = NULL;
      guint n_files;
      GError *local_error = NULL;

      /* Create a fresh, empty working directory. */
      tmpdir_path = g_dir_make_tmp ("g_file_replace_XXXXXX", &local_error);
      g_assert_no_error (local_error);
      tmpdir = g_file_new_for_path (tmpdir_path);

      g_test_message ("Test %" G_GSIZE_FORMAT ", using temporary directory %s", i, tmpdir_path);

      if (tests[i].skip_if_cap_dac_override && check_cap_dac_override (tmpdir_path))
        {
          g_test_message ("Skipping test as process has CAP_DAC_OVERRIDE capability and the test checks permissions");

          g_file_delete (tmpdir, NULL, &local_error);
          g_assert_no_error (local_error);
          g_clear_object (&tmpdir);
          g_free (tmpdir_path);

          continue;
        }

      g_free (tmpdir_path);

      /* Set up the test directory. */
      source_file = create_test_file (tmpdir, "source", tests[i].setup_source_type, tests[i].setup_source_mode);
      backup_file = create_test_file (tmpdir, "source~", tests[i].setup_backup_type, tests[i].setup_backup_mode);

      /* Make the tmpdir read-only if desired. */
      if (tests[i].setup_directory_type == FILE_TEST_DIRECTORY_SETUP_TYPE_READ_ONLY)
        {
          g_file_set_attribute_uint32 (tmpdir, G_FILE_ATTRIBUTE_UNIX_MODE,
                                       0500, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                       NULL, &local_error);
          g_assert_no_error (local_error);
        }

      /* Replace the source file. Check the error state only after finishing
       * writing, as the replace operation is split across g_file_replace() and
       * g_output_stream_close(). */
      if (read_write)
        io_stream = g_file_replace_readwrite (source_file,
                                              tests[i].replace_etag,
                                              tests[i].replace_make_backup,
                                              tests[i].replace_flags,
                                              NULL,
                                              &local_error);
      else
        output_stream = g_file_replace (source_file,
                                        tests[i].replace_etag,
                                        tests[i].replace_make_backup,
                                        tests[i].replace_flags,
                                        NULL,
                                        &local_error);

      if (tests[i].expected_success)
        {
          g_assert_no_error (local_error);
          if (read_write)
            g_assert_nonnull (io_stream);
          else
            g_assert_nonnull (output_stream);
        }

      /* Write new content to it. */
      if (io_stream != NULL)
        {
          GOutputStream *io_output_stream = g_io_stream_get_output_stream (G_IO_STREAM (io_stream));
          gsize n_written;

          g_output_stream_write_all (G_OUTPUT_STREAM (io_output_stream), new_contents, strlen (new_contents),
                                     &n_written, NULL, &local_error);

          if (tests[i].expected_success)
            {
              g_assert_no_error (local_error);
              g_assert_cmpint (n_written, ==, strlen (new_contents));
            }

          g_io_stream_close (G_IO_STREAM (io_stream), NULL, (local_error == NULL) ? &local_error : NULL);

          if (tests[i].expected_success)
            g_assert_no_error (local_error);
        }
      else if (output_stream != NULL)
        {
          gsize n_written;

          g_output_stream_write_all (G_OUTPUT_STREAM (output_stream), new_contents, strlen (new_contents),
                                     &n_written, NULL, &local_error);

          if (tests[i].expected_success)
            {
              g_assert_no_error (local_error);
              g_assert_cmpint (n_written, ==, strlen (new_contents));
            }

          g_output_stream_close (G_OUTPUT_STREAM (output_stream), NULL, (local_error == NULL) ? &local_error : NULL);

          if (tests[i].expected_success)
            g_assert_no_error (local_error);
        }

      if (tests[i].expected_success)
        g_assert_no_error (local_error);
      else
        g_assert_error (local_error, tests[i].expected_error_domain, tests[i].expected_error_code);

      g_clear_error (&local_error);
      g_clear_object (&io_stream);
      g_clear_object (&output_stream);

      /* Verify the final state of the directory. */
      enumerator = g_file_enumerate_children (tmpdir,
                                              G_FILE_ATTRIBUTE_STANDARD_NAME,
                                              G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
      g_assert_no_error (local_error);

      n_files = 0;
      do
        {
          g_file_enumerator_iterate (enumerator, &info, NULL, NULL, &local_error);
          g_assert_no_error (local_error);

          if (info != NULL)
            n_files++;
        }
      while (info != NULL);

      g_clear_object (&enumerator);

      g_assert_cmpuint (n_files, ==, tests[i].expected_n_files);

      check_test_file (source_file, tmpdir, "source", tests[i].expected_source_type, tests[i].expected_source_mode, tests[i].expected_source_contents);
      check_test_file (backup_file, tmpdir, "source~", tests[i].expected_backup_type, tests[i].expected_backup_mode, tests[i].expected_backup_contents);

      /* Tidy up. Ignore failure apart from when deleting the directory, which
       * should be empty. */
      if (tests[i].setup_directory_type == FILE_TEST_DIRECTORY_SETUP_TYPE_READ_ONLY)
        {
          g_file_set_attribute_uint32 (tmpdir, G_FILE_ATTRIBUTE_UNIX_MODE,
                                       0700, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                       NULL, &local_error);
          g_assert_no_error (local_error);
        }

      g_file_delete (source_file, NULL, NULL);
      g_file_delete (backup_file, NULL, NULL);

      /* Other files which are occasionally generated by the tests. */
        {
          GFile *backup_target_file = g_file_get_child (tmpdir, "source~-target");
          g_file_delete (backup_target_file, NULL, NULL);
          g_clear_object (&backup_target_file);
        }
        {
          GFile *backup_target_file = g_file_get_child (tmpdir, "source-target");
          g_file_delete (backup_target_file, NULL, NULL);
          g_clear_object (&backup_target_file);
        }

      g_file_delete (tmpdir, NULL, &local_error);
      g_assert_no_error (local_error);

      g_clear_object (&backup_file);
      g_clear_object (&source_file);
      g_clear_object (&tmpdir);
    }
#else  /* if !__linux__ */
  g_test_skip ("File replacement tests can only be run on Linux");
#endif
}

static void
on_new_tmp_done (GObject      *object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GFile *file;
  GFile *parent;
  GFileInfo *info;
  GFileIOStream *iostream;
  GError *error = NULL;
  GMainLoop *loop = user_data;
  gchar *basename;
  GFile *tmpdir = NULL;

  g_assert_null (object);

  file = g_file_new_tmp_finish (result, &iostream, &error);
  g_assert_no_error (error);

  g_assert_true (g_file_query_exists (file, NULL));

  basename = g_file_get_basename (file);
  g_assert_true (g_str_has_prefix (basename, "g_file_new_tmp_async_"));

  info = g_file_io_stream_query_info (iostream, G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                      NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpuint (g_file_info_get_file_type (info), ==, G_FILE_TYPE_REGULAR);
  g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);

  parent = g_file_get_parent (file);
  tmpdir = g_file_new_for_path (g_get_tmp_dir ());

  g_assert_true (g_file_equal (tmpdir, parent));

  g_main_loop_quit (loop);

  g_object_unref (file);
  g_object_unref (parent);
  g_object_unref (iostream);
  g_object_unref (info);
  g_free (basename);
  g_object_unref (tmpdir);
}

static void
on_new_tmp_error (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  GFileIOStream *iostream = (GFileIOStream*) &on_new_tmp_error;
  AsyncErrorData *error_data = user_data;

  g_assert_null (object);

  g_assert_null (g_file_new_tmp_finish (result, &iostream, error_data->error));
  g_assert_nonnull (error_data->error);
  g_assert_null (iostream);

  g_main_loop_quit (error_data->loop);
}

static void
test_async_new_tmp (void)
{
  GMainLoop *loop;
  GError *error = NULL;
  GCancellable *cancellable;
  AsyncErrorData error_data = { .error = &error };

  loop = g_main_loop_new (NULL, TRUE);
  error_data.loop = loop;

  g_file_new_tmp_async ("g_file_new_tmp_async_XXXXXX",
                        G_PRIORITY_DEFAULT, NULL,
                        on_new_tmp_done, loop);
  g_main_loop_run (loop);

  g_file_new_tmp_async ("g_file_new_tmp_async_invalid_template",
                        G_PRIORITY_DEFAULT, NULL,
                        on_new_tmp_error, &error_data);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_clear_error (&error);

  cancellable = g_cancellable_new ();
  g_file_new_tmp_async ("g_file_new_tmp_async_cancelled_XXXXXX",
                        G_PRIORITY_DEFAULT, cancellable,
                        on_new_tmp_error, &error_data);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_object (&cancellable);
  g_clear_error (&error);

  g_main_loop_unref (loop);
}

static void
on_new_tmp_dir_done (GObject      *object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  GFile *file;
  GFile *parent;
  GFileInfo *info;
  GError *error = NULL;
  GMainLoop *loop = user_data;
  gchar *basename;
  GFile *tmpdir = NULL;

  g_assert_null (object);

  file = g_file_new_tmp_dir_finish (result, &error);
  g_assert_no_error (error);

  g_assert_true (g_file_query_exists (file, NULL));

  basename = g_file_get_basename (file);
  g_assert_true (g_str_has_prefix (basename, "g_file_new_tmp_dir_async_"));

  info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE,
                            G_FILE_QUERY_INFO_NONE, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpuint (g_file_info_get_file_type (info), ==, G_FILE_TYPE_DIRECTORY);

  parent = g_file_get_parent (file);
  tmpdir = g_file_new_for_path (g_get_tmp_dir ());

  g_assert_true (g_file_equal (tmpdir, parent));

  g_main_loop_quit (loop);

  g_object_unref (file);
  g_object_unref (parent);
  g_object_unref (info);
  g_free (basename);
  g_object_unref (tmpdir);
}

static void
on_new_tmp_dir_error (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  AsyncErrorData *error_data = user_data;

  g_assert_null (object);

  g_assert_null (g_file_new_tmp_dir_finish (result, error_data->error));
  g_assert_nonnull (error_data->error);

  g_main_loop_quit (error_data->loop);
}

static void
test_async_new_tmp_dir (void)
{
  GMainLoop *loop;
  GError *error = NULL;
  GCancellable *cancellable;
  AsyncErrorData error_data = { .error = &error };

  loop = g_main_loop_new (NULL, TRUE);
  error_data.loop = loop;

  g_file_new_tmp_dir_async ("g_file_new_tmp_dir_async_XXXXXX",
                            G_PRIORITY_DEFAULT, NULL,
                            on_new_tmp_dir_done, loop);
  g_main_loop_run (loop);

  g_file_new_tmp_dir_async ("g_file_new_tmp_dir_async",
                            G_PRIORITY_DEFAULT, NULL,
                            on_new_tmp_dir_error, &error_data);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_clear_error (&error);

  cancellable = g_cancellable_new ();
  g_file_new_tmp_dir_async ("g_file_new_tmp_dir_async_cancelled_XXXXXX",
                            G_PRIORITY_DEFAULT, cancellable,
                            on_new_tmp_dir_error, &error_data);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_object (&cancellable);
  g_clear_error (&error);

  g_main_loop_unref (loop);
}

static void
on_file_deleted (GObject      *object,
		 GAsyncResult *result,
		 gpointer      user_data)
{
  GError *local_error = NULL;
  GMainLoop *loop = user_data;

  (void) g_file_delete_finish ((GFile*)object, result, &local_error);
  g_assert_no_error (local_error);

  g_main_loop_quit (loop);
}

static void
test_async_delete (void)
{
  GFile *file;
  GFileIOStream *iostream;
  GError *local_error = NULL;
  GError **error = &local_error;
  GMainLoop *loop;

  file = g_file_new_tmp ("g_file_delete_XXXXXX",
			 &iostream, error);
  g_assert_no_error (local_error);
  g_object_unref (iostream);

  g_assert_true (g_file_query_exists (file, NULL));

  loop = g_main_loop_new (NULL, TRUE);

  g_file_delete_async (file, G_PRIORITY_DEFAULT, NULL, on_file_deleted, loop);

  g_main_loop_run (loop);

  g_assert_false (g_file_query_exists (file, NULL));

  g_main_loop_unref (loop);
  g_object_unref (file);
}

static void
on_symlink_done (GObject      *object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GFile *file = (GFile *) object;
  GError *error = NULL;
  GMainLoop *loop = user_data;

  g_assert_true (g_file_make_symbolic_link_finish (file, result, &error));
  g_assert_no_error (error);

  g_main_loop_quit (loop);
}

static void
on_symlink_error (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  GFile *file = (GFile *) object;
  GError *error = NULL;
  AsyncErrorData *data = user_data;

  g_assert_false (g_file_make_symbolic_link_finish (file, result, &error));
  g_assert_nonnull (error);
  g_propagate_error (data->error, g_steal_pointer (&error));

  g_main_loop_quit (data->loop);
}

static void
test_async_make_symlink (void)
{
  GFile *link;
  GFile *parent_dir;
  GFile *target;
  GFileInfo *link_info;
  GFileIOStream *iostream;
  GError *error = NULL;
  GCancellable *cancellable;
  GMainLoop *loop;
  AsyncErrorData error_data = {0};
  gchar *tmpdir_path;
  gchar *target_path;

  target = g_file_new_tmp ("g_file_symlink_target_XXXXXX", &iostream, &error);
  g_assert_no_error (error);

  g_io_stream_close ((GIOStream *) iostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (iostream);

  g_assert_true (g_file_query_exists (target, NULL));

  loop = g_main_loop_new (NULL, TRUE);
  error_data.loop = loop;
  error_data.error = &error;

  tmpdir_path = g_dir_make_tmp ("g_file_symlink_XXXXXX", &error);
  g_assert_no_error (error);

  parent_dir = g_file_new_for_path (tmpdir_path);
  g_assert_true (g_file_query_exists (parent_dir, NULL));

  link = g_file_get_child (parent_dir, "symlink");
  g_assert_false (g_file_query_exists (link, NULL));

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*symlink_value*failed*");
  g_file_make_symbolic_link_async (link, NULL,
                                   G_PRIORITY_DEFAULT, NULL,
                                   on_symlink_done, loop);
  g_test_assert_expected_messages ();

  g_file_make_symbolic_link_async (link, "",
                                   G_PRIORITY_DEFAULT, NULL,
                                   on_symlink_error, &error_data);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_clear_error (&error);

  target_path = g_file_get_path (target);
  g_file_make_symbolic_link_async (link, target_path,
                                   G_PRIORITY_DEFAULT, NULL,
                                   on_symlink_done, loop);
  g_main_loop_run (loop);

  g_assert_true (g_file_query_exists (link, NULL));
  link_info = g_file_query_info (link,
                                 G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
                                 G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                 NULL,
                                 &error);
  g_assert_no_error (error);

  g_assert_true (g_file_info_get_is_symlink (link_info));
  g_assert_cmpstr (target_path, ==, g_file_info_get_symlink_target (link_info));

  /* Try creating it again, it fails */
  g_file_make_symbolic_link_async (link, target_path,
                                   G_PRIORITY_DEFAULT, NULL,
                                   on_symlink_error, &error_data);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
  g_clear_error (&error);

  cancellable = g_cancellable_new ();
  g_file_make_symbolic_link_async (link, target_path,
                                   G_PRIORITY_DEFAULT, cancellable,
                                   on_symlink_error, &error_data);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (loop);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_clear_object (&cancellable);

  g_main_loop_unref (loop);
  g_object_unref (target);
  g_object_unref (parent_dir);
  g_object_unref (link);
  g_object_unref (link_info);
  g_free (tmpdir_path);
  g_free (target_path);
}

static void
test_copy_preserve_mode (void)
{
#ifdef G_OS_UNIX
  mode_t current_umask = umask (0);
  const struct
    {
      guint32 source_mode;
      guint32 expected_destination_mode;
      gboolean create_destination_before_copy;
      GFileCopyFlags copy_flags;
    }
  vectors[] =
    {
      /* Overwriting the destination file should copy the permissions from the
       * source file, even if %G_FILE_COPY_ALL_METADATA is set: */
      { 0600, 0600, TRUE, G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA },
      { 0600, 0600, TRUE, G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS },
      /* The same behaviour should hold if the destination file is not being
       * overwritten because it doesn’t already exist: */
      { 0600, 0600, FALSE, G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME | G_FILE_COPY_ALL_METADATA },
      { 0600, 0600, FALSE, G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA },
      { 0600, 0600, FALSE, G_FILE_COPY_NOFOLLOW_SYMLINKS },
      /* Anything with %G_FILE_COPY_TARGET_DEFAULT_PERMS should use the current
       * umask for the destination file: */
      { 0600, 0666 & ~current_umask, TRUE, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA },
      { 0600, 0666 & ~current_umask, TRUE, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS },
      { 0600, 0666 & ~current_umask, FALSE, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME | G_FILE_COPY_ALL_METADATA },
      { 0600, 0666 & ~current_umask, FALSE, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME },
      { 0600, 0666 & ~current_umask, FALSE, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA },
      { 0600, 0666 & ~current_umask, FALSE, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_NOFOLLOW_SYMLINKS },
    };
  gsize i;

  /* Reset the umask after querying it above. There’s no way to query it without
   * changing it. */
  umask (current_umask);
  g_test_message ("Current umask: %u", (unsigned int) current_umask);

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      GFile *tmpfile;
      GFile *dest_tmpfile;
      GFileInfo *dest_info;
      GFileIOStream *iostream;
      GError *local_error = NULL;
      guint32 romode = vectors[i].source_mode;
      guint32 dest_mode;

      g_test_message ("Vector %" G_GSIZE_FORMAT, i);

      tmpfile = g_file_new_tmp ("tmp-copy-preserve-modeXXXXXX",
                                &iostream, &local_error);
      g_assert_no_error (local_error);
      g_io_stream_close ((GIOStream*)iostream, NULL, &local_error);
      g_assert_no_error (local_error);
      g_clear_object (&iostream);

      g_file_set_attribute (tmpfile, G_FILE_ATTRIBUTE_UNIX_MODE, G_FILE_ATTRIBUTE_TYPE_UINT32,
                            &romode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            NULL, &local_error);
      g_assert_no_error (local_error);

      dest_tmpfile = g_file_new_tmp ("tmp-copy-preserve-modeXXXXXX",
                                     &iostream, &local_error);
      g_assert_no_error (local_error);
      g_io_stream_close ((GIOStream*)iostream, NULL, &local_error);
      g_assert_no_error (local_error);
      g_clear_object (&iostream);

      if (!vectors[i].create_destination_before_copy)
        {
          g_file_delete (dest_tmpfile, NULL, &local_error);
          g_assert_no_error (local_error);
        }

      g_file_copy (tmpfile, dest_tmpfile, vectors[i].copy_flags,
                   NULL, NULL, NULL, &local_error);
      g_assert_no_error (local_error);

      dest_info = g_file_query_info (dest_tmpfile, G_FILE_ATTRIBUTE_UNIX_MODE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     NULL, &local_error);
      g_assert_no_error (local_error);

      dest_mode = g_file_info_get_attribute_uint32 (dest_info, G_FILE_ATTRIBUTE_UNIX_MODE);

      g_assert_cmpint (dest_mode & ~S_IFMT, ==, vectors[i].expected_destination_mode);
      g_assert_cmpint (dest_mode & S_IFMT, ==, S_IFREG);

      (void) g_file_delete (tmpfile, NULL, NULL);
      (void) g_file_delete (dest_tmpfile, NULL, NULL);

      g_clear_object (&tmpfile);
      g_clear_object (&dest_tmpfile);
      g_clear_object (&dest_info);
    }
#else  /* if !G_OS_UNIX */
  g_test_skip ("File permissions tests can only be run on Unix")
#endif
}

typedef struct
{
  goffset current_num_bytes;
  goffset total_num_bytes;
} CopyProgressData;

static void
file_copy_progress_cb (goffset  current_num_bytes,
                       goffset  total_num_bytes,
                       gpointer user_data)
{
  CopyProgressData *prev_data = user_data;

  g_assert_cmpuint (total_num_bytes, ==, prev_data->total_num_bytes);
  g_assert_cmpuint (current_num_bytes, >=, prev_data->current_num_bytes);

  /* Update it for the next callback. */
  prev_data->current_num_bytes = current_num_bytes;
}

static void
test_copy_progress (void)
{
  GFile *src_tmpfile = NULL;
  GFile *dest_tmpfile = NULL;
  GFileIOStream *iostream;
  GOutputStream *ostream;
  GError *local_error = NULL;
  const guint8 buffer[] = { 1, 2, 3, 4, 5 };
  CopyProgressData progress_data;

  src_tmpfile = g_file_new_tmp ("tmp-copy-progressXXXXXX",
                                &iostream, &local_error);
  g_assert_no_error (local_error);

  /* Write some content to the file for testing. */
  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write (ostream, buffer, sizeof (buffer), NULL, &local_error);
  g_assert_no_error (local_error);

  g_io_stream_close ((GIOStream *) iostream, NULL, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&iostream);

  /* Grab a unique destination filename. */
  dest_tmpfile = g_file_new_tmp ("tmp-copy-progressXXXXXX",
                                 &iostream, &local_error);
  g_assert_no_error (local_error);
  g_io_stream_close ((GIOStream *) iostream, NULL, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&iostream);

  /* Set the progress data to an initial offset of zero. The callback will
   * assert that progress is non-decreasing and reaches the total length of
   * the file. */
  progress_data.current_num_bytes = 0;
  progress_data.total_num_bytes = sizeof (buffer);

  /* Copy the file with progress reporting. */
  g_file_copy (src_tmpfile, dest_tmpfile, G_FILE_COPY_OVERWRITE,
               NULL, file_copy_progress_cb, &progress_data, &local_error);
  g_assert_no_error (local_error);

  g_assert_cmpuint (progress_data.current_num_bytes, ==, progress_data.total_num_bytes);
  g_assert_cmpuint (progress_data.total_num_bytes, ==, sizeof (buffer));

  /* Clean up. */
  (void) g_file_delete (src_tmpfile, NULL, NULL);
  (void) g_file_delete (dest_tmpfile, NULL, NULL);

  g_clear_object (&src_tmpfile);
  g_clear_object (&dest_tmpfile);
}

typedef struct
{
  GError *error;
  gboolean done;
  gboolean res;
} CopyAsyncData;

static void
test_copy_async_cb (GObject *object,
                    GAsyncResult *result,
                    void *user_data)
{
  GFile *file = G_FILE (object);
  CopyAsyncData *data = user_data;
  GError *error = NULL;

  data->res = g_file_move_finish (file, result, &error);
  data->error = g_steal_pointer (&error);
  data->done = TRUE;
}

typedef struct
{
  goffset total_num_bytes;
} CopyAsyncProgressData;

static void
test_copy_async_progress_cb (goffset current_num_bytes,
                             goffset total_num_bytes,
                             void *user_data)
{
  CopyAsyncProgressData *data = user_data;
  data->total_num_bytes = total_num_bytes;
}

/* Exercise copy_async_with_closures() */
static void
test_copy_async_with_closures (void)
{
  CopyAsyncData data = { 0 };
  CopyAsyncProgressData progress_data = { 0 };
  GFile *source;
  GFileIOStream *iostream;
  GOutputStream *ostream;
  GFile *destination;
  gchar *destination_path;
  GError *error = NULL;
  gboolean res;
  const guint8 buffer[] = { 1, 2, 3, 4, 5 };
  GClosure *progress_closure;
  GClosure *ready_closure;

  source = g_file_new_tmp ("g_file_copy_async_with_closures_XXXXXX", &iostream, NULL);

  destination_path = g_build_path (G_DIR_SEPARATOR_S, g_get_tmp_dir (), "g_file_copy_async_with_closures_target", NULL);
  destination = g_file_new_for_path (destination_path);

  g_assert_nonnull (source);
  g_assert_nonnull (iostream);

  res = g_file_query_exists (source, NULL);
  g_assert_true (res);
  res = g_file_query_exists (destination, NULL);
  g_assert_false (res);

  /* Write a known number of bytes to the file, so we can test the progress
   * callback against it */
  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write (ostream, buffer, sizeof (buffer), NULL, &error);
  g_assert_no_error (error);

  progress_closure = g_cclosure_new (G_CALLBACK (test_copy_async_progress_cb), &progress_data, NULL);
  ready_closure = g_cclosure_new (G_CALLBACK (test_copy_async_cb), &data, NULL);

  g_file_copy_async_with_closures (source,
                                   destination,
                                   G_FILE_COPY_NONE,
                                   0,
                                   NULL,
                                   progress_closure,
                                   ready_closure);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_no_error (data.error);
  g_assert_true (data.res);
  g_assert_cmpuint (progress_data.total_num_bytes, ==, sizeof (buffer));

  res = g_file_query_exists (source, NULL);
  g_assert_true (res);
  res = g_file_query_exists (destination, NULL);
  g_assert_true (res);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_delete (source, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  res = g_file_delete (destination, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_object_unref (source);
  g_object_unref (destination);

  g_free (destination_path);
}

static void
test_measure (void)
{
  GFile *file;
  guint64 num_bytes;
  guint64 num_dirs;
  guint64 num_files;
  GError *error = NULL;
  gboolean ok;
  gchar *path;

  path = g_test_build_filename (G_TEST_DIST, "desktop-files", NULL);
  file = g_file_new_for_path (path);

  ok = g_file_measure_disk_usage (file,
                                  G_FILE_MEASURE_APPARENT_SIZE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &num_bytes,
                                  &num_dirs,
                                  &num_files,
                                  &error);
  g_assert_true (ok);
  g_assert_no_error (error);

  g_assert_cmpuint (num_bytes, ==, 96702);
  g_assert_cmpuint (num_dirs, ==, 6);
  g_assert_cmpuint (num_files, ==, 34);

  g_object_unref (file);
  g_free (path);
}

typedef struct {
  guint64 expected_bytes;
  guint64 expected_dirs;
  guint64 expected_files;
  gint progress_count;
  guint64 progress_bytes;
  guint64 progress_dirs;
  guint64 progress_files;
} MeasureData;

static void
measure_progress (gboolean reporting,
                  guint64  current_size,
                  guint64  num_dirs,
                  guint64  num_files,
                  gpointer user_data)
{
  MeasureData *data = user_data;

  data->progress_count += 1;

  g_assert_cmpuint (current_size, >=, data->progress_bytes);
  g_assert_cmpuint (num_dirs, >=, data->progress_dirs);
  g_assert_cmpuint (num_files, >=, data->progress_files);

  data->progress_bytes = current_size;
  data->progress_dirs = num_dirs;
  data->progress_files = num_files;
}

static void
measure_done (GObject      *source,
              GAsyncResult *res,
              gpointer      user_data)
{
  MeasureData *data = user_data;
  guint64 num_bytes, num_dirs, num_files;
  GError *error = NULL;
  gboolean ok;

  ok = g_file_measure_disk_usage_finish (G_FILE (source), res, &num_bytes, &num_dirs, &num_files, &error);
  g_assert_true (ok);
  g_assert_no_error (error);

  g_assert_cmpuint (data->expected_bytes, ==, num_bytes);
  g_assert_cmpuint (data->expected_dirs, ==, num_dirs);
  g_assert_cmpuint (data->expected_files, ==, num_files);

  g_assert_cmpuint (data->progress_count, >, 0);
  g_assert_cmpuint (num_bytes, >=, data->progress_bytes);
  g_assert_cmpuint (num_dirs, >=, data->progress_dirs);
  g_assert_cmpuint (num_files, >=, data->progress_files);

  g_free (data);
  g_object_unref (source);
}

static void
test_measure_async (void)
{
  gchar *path;
  GFile *file;
  MeasureData *data;

  data = g_new (MeasureData, 1);

  data->progress_count = 0;
  data->progress_bytes = 0;
  data->progress_files = 0;
  data->progress_dirs = 0;

  path = g_test_build_filename (G_TEST_DIST, "desktop-files", NULL);
  file = g_file_new_for_path (path);
  g_free (path);

  data->expected_bytes = 96702;
  data->expected_dirs = 6;
  data->expected_files = 34;

  g_file_measure_disk_usage_async (file,
                                   G_FILE_MEASURE_APPARENT_SIZE,
                                   0, NULL,
                                   measure_progress, data,
                                   measure_done, data);
}

static void
test_load_bytes (void)
{
  char *filename = NULL;
  GError *error = NULL;
  GBytes *bytes;
  GFile *file;
  int len;
  int fd;
  int ret;

  filename = g_build_filename (g_get_tmp_dir (), "g_file_load_bytes_XXXXXX", NULL);
  fd = g_mkstemp (filename);
  g_assert_cmpint (fd, !=, -1);
  len = strlen ("test_load_bytes");
  ret = write (fd, "test_load_bytes", len);
  g_assert_cmpint (ret, ==, len);
  g_clear_fd (&fd, &error);
  g_assert_no_error (error);

  file = g_file_new_for_path (filename);
  bytes = g_file_load_bytes (file, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (bytes);
  g_assert_cmpint (len, ==, g_bytes_get_size (bytes));
  g_assert_cmpstr ("test_load_bytes", ==, (gchar *)g_bytes_get_data (bytes, NULL));

  g_file_delete (file, NULL, NULL);

  g_bytes_unref (bytes);
  g_object_unref (file);
  g_free (filename);
}

typedef struct
{
  GMainLoop *main_loop;
  GFile *file;
  GBytes *bytes;
} LoadBytesAsyncData;

static void
test_load_bytes_cb (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  GFile *file = G_FILE (object);
  LoadBytesAsyncData *data = user_data;
  GError *error = NULL;

  data->bytes = g_file_load_bytes_finish (file, result, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (data->bytes);

  g_main_loop_quit (data->main_loop);
}

static void
test_load_bytes_async (void)
{
  LoadBytesAsyncData data = { 0 };
  char *filename = NULL;
  GError *error = NULL;
  int len;
  int fd;
  int ret;

  filename = g_build_filename (g_get_tmp_dir (), "g_file_load_bytes_XXXXXX", NULL);
  fd = g_mkstemp (filename);
  g_assert_cmpint (fd, !=, -1);
  len = strlen ("test_load_bytes_async");
  ret = write (fd, "test_load_bytes_async", len);
  g_assert_cmpint (ret, ==, len);
  g_clear_fd (&fd, &error);
  g_assert_no_error (error);

  data.main_loop = g_main_loop_new (NULL, FALSE);
  data.file = g_file_new_for_path (filename);

  g_file_load_bytes_async (data.file, NULL, test_load_bytes_cb, &data);
  g_main_loop_run (data.main_loop);

  g_assert_cmpint (len, ==, g_bytes_get_size (data.bytes));
  g_assert_cmpstr ("test_load_bytes_async", ==, (gchar *)g_bytes_get_data (data.bytes, NULL));

  g_file_delete (data.file, NULL, NULL);
  g_object_unref (data.file);
  g_bytes_unref (data.bytes);
  g_main_loop_unref (data.main_loop);
  g_free (filename);
}

#if GLIB_SIZEOF_SIZE_T > 4
static const gsize testfile_4gb_size = ((gsize) 1 << 32) + (1 << 16); /* 4GB + a bit */
#else
/* Have to make do with something smaller on 32-bit platforms */
static const gsize testfile_4gb_size = G_MAXSIZE;
#endif

/* @filename will be modified as per g_mkstemp() */
static gboolean
create_testfile_4gb_or_skip (char *filename)
{
  GError *error = NULL;
  int fd;
  int ret;

  /* Reading each 4GB test file takes about 5s on a fast machine, and another 7s
   * to compare its contents once it’s been read. That’s too slow for a normal
   * test run, and there’s no way to speed it up. */
  if (!g_test_slow ())
    {
      g_test_skip ("Skipping slow >4GB file test");
      return FALSE;
    }

  fd = g_mkstemp (filename);
  g_assert_cmpint (fd, !=, -1);
  ret = ftruncate (fd, testfile_4gb_size);
  g_clear_fd (&fd, &error);
  g_assert_no_error (error);
  if (ret == 1)
    {
      g_test_skip ("Could not create testfile >4GB");
      g_assert_no_errno (g_unlink (filename));
      return FALSE;
    }

  return TRUE;
}

static void
check_testfile_4gb_contents (const char *data,
                             gsize       len)
{
  gsize i;

  g_assert_nonnull (data);
  g_assert_cmpuint (testfile_4gb_size, ==, len);

  for (i = 0; i < testfile_4gb_size; i++)
    {
      if (data[i] != 0)
        break;
    }
  g_assert_cmpint (i, ==, testfile_4gb_size);
}

static void
test_load_contents_4gb (void)
{
  char *filename = NULL;
  GError *error = NULL;
  gboolean result;
  char *data;
  gsize len;
  GFile *file;

  filename = g_build_filename (g_get_tmp_dir (), "g_file_load_contents_4gb_XXXXXX", NULL);
  if (!create_testfile_4gb_or_skip (filename))
    {
      g_free (filename);
      return;
    }

  file = g_file_new_for_path (filename);
  result = g_file_load_contents (file, NULL, &data, &len, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (result);

  check_testfile_4gb_contents (data, len);

  g_file_delete (file, NULL, NULL);

  g_free (data);
  g_object_unref (file);
  g_free (filename);
}

static void
load_contents_4gb_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert (*result_out == NULL);
  *result_out = g_object_ref (result);

  g_main_context_wakeup (NULL);
}

static void
test_load_contents_4gb_async (void)
{
  char *filename = NULL;
  GFile *file;
  GAsyncResult *async_result = NULL;
  GError *error = NULL;
  char *data;
  gsize len;
  gboolean ret;

  filename = g_build_filename (g_get_tmp_dir (), "g_file_load_contents_4gb_async_XXXXXX", NULL);
  if (!create_testfile_4gb_or_skip (filename))
    {
      g_free (filename);
      return;
    }

  file = g_file_new_for_path (filename);
  g_file_load_contents_async (file, NULL, load_contents_4gb_cb, &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  ret = g_file_load_contents_finish (file, async_result, &data, &len, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (ret);

  check_testfile_4gb_contents (data, len);

  g_file_delete (file, NULL, NULL);

  g_free (data);
  g_object_unref (async_result);
  g_object_unref (file);
  g_free (filename);
}

static void
test_load_bytes_4gb (void)
{
  char *filename = NULL;
  GError *error = NULL;
  GBytes *bytes;
  GFile *file;

  filename = g_build_filename (g_get_tmp_dir (), "g_file_load_bytes_4gb_XXXXXX", NULL);
  if (!create_testfile_4gb_or_skip (filename))
    {
      g_free (filename);
      return;
    }

  file = g_file_new_for_path (filename);
  bytes = g_file_load_bytes (file, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (bytes);

  check_testfile_4gb_contents (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));

  g_file_delete (file, NULL, NULL);

  g_bytes_unref (bytes);
  g_object_unref (file);
  g_free (filename);
}

static void
test_writev_helper (GOutputVector *vectors,
                    gsize          n_vectors,
                    gboolean       use_bytes_written,
                    const guint8  *expected_contents,
                    gsize          expected_length)
{
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputStream *ostream;
  GError *error = NULL;
  gsize bytes_written = 0;
  gboolean res;
  guint8 *contents;
  gsize length;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  res = g_output_stream_writev_all (ostream, vectors, n_vectors, use_bytes_written ? &bytes_written : NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  if (use_bytes_written)
    g_assert_cmpuint (bytes_written, ==, expected_length);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, expected_contents, expected_length);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

/* Test that writev() on local file output streams works on a non-empty vector */
static void
test_writev (void)
{
  GOutputVector vectors[3];
  const guint8 buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5 + 12;
  vectors[2].size = 3;

  test_writev_helper (vectors, G_N_ELEMENTS (vectors), TRUE, buffer, sizeof buffer);
}

/* Test that writev() on local file output streams works on a non-empty vector without returning bytes_written */
static void
test_writev_no_bytes_written (void)
{
  GOutputVector vectors[3];
  const guint8 buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5 + 12;
  vectors[2].size = 3;

  test_writev_helper (vectors, G_N_ELEMENTS (vectors), FALSE, buffer, sizeof buffer);
}

/* Test that writev() on local file output streams works on 0 vectors */
static void
test_writev_no_vectors (void)
{
  test_writev_helper (NULL, 0, TRUE, NULL, 0);
}

/* Test that writev() on local file output streams works on empty vectors */
static void
test_writev_empty_vectors (void)
{
  GOutputVector vectors[3];

  vectors[0].buffer = NULL;
  vectors[0].size = 0;
  vectors[1].buffer = NULL;
  vectors[1].size = 0;
  vectors[2].buffer = NULL;
  vectors[2].size = 0;

  test_writev_helper (vectors, G_N_ELEMENTS (vectors), TRUE, NULL, 0);
}

/* Test that writev() fails if the sum of sizes in the vector is too big */
static void
test_writev_too_big_vectors (void)
{
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputStream *ostream;
  GError *error = NULL;
  gsize bytes_written = 0;
  gboolean res;
  guint8 *contents;
  gsize length;
  GOutputVector vectors[3];

  vectors[0].buffer = (void*) 1;
  vectors[0].size = G_MAXSIZE / 2;

  vectors[1].buffer = (void*) 1;
  vectors[1].size = G_MAXSIZE / 2;

  vectors[2].buffer = (void*) 1;
  vectors[2].size = G_MAXSIZE / 2;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  res = g_output_stream_writev_all (ostream, vectors, G_N_ELEMENTS (vectors), &bytes_written, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpuint (bytes_written, ==, 0);
  g_assert_false (res);
  g_clear_error (&error);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, NULL, 0);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

typedef struct
{
  gsize bytes_written;
  GOutputVector *vectors;
  gsize n_vectors;
  GError *error;
  gboolean done;
} WritevAsyncData;

static void
test_writev_async_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  GOutputStream *ostream = G_OUTPUT_STREAM (object);
  WritevAsyncData *data = user_data;
  GError *error = NULL;
  gsize bytes_written;
  gboolean res;

  res = g_output_stream_writev_finish (ostream, result, &bytes_written, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  data->bytes_written += bytes_written;

  /* skip vectors that have been written in full */
  while (data->n_vectors > 0 && bytes_written >= data->vectors[0].size)
    {
      bytes_written -= data->vectors[0].size;
      ++data->vectors;
      --data->n_vectors;
    }
  /* skip partially written vector data */
  if (bytes_written > 0 && data->n_vectors > 0)
    {
      data->vectors[0].size -= bytes_written;
      data->vectors[0].buffer = ((guint8 *) data->vectors[0].buffer) + bytes_written;
    }

  if (data->n_vectors > 0)
    g_output_stream_writev_async (ostream, data->vectors, data->n_vectors, 0, NULL, test_writev_async_cb, &data);
}

/* Test that writev_async() on local file output streams works on a non-empty vector */
static void
test_writev_async (void)
{
  WritevAsyncData data = { 0 };
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputVector vectors[3];
  const guint8 buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};
  GOutputStream *ostream;
  GError *error = NULL;
  gboolean res;
  guint8 *contents;
  gsize length;

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5  + 12;
  vectors[2].size = 3;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  data.vectors = vectors;
  data.n_vectors = G_N_ELEMENTS (vectors);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  g_output_stream_writev_async (ostream, data.vectors, data.n_vectors, 0, NULL, test_writev_async_cb, &data);

  while (data.n_vectors > 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, sizeof buffer);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, buffer, sizeof buffer);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

static void
test_writev_all_cb (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  GOutputStream *ostream = G_OUTPUT_STREAM (object);
  WritevAsyncData *data = user_data;

  g_output_stream_writev_all_finish (ostream, result, &data->bytes_written, &data->error);
  data->done = TRUE;
}

/* Test that writev_async_all() on local file output streams works on a non-empty vector */
static void
test_writev_async_all (void)
{
  WritevAsyncData data = { 0 };
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputStream *ostream;
  GOutputVector vectors[3];
  const guint8 buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};
  GError *error = NULL;
  gboolean res;
  guint8 *contents;
  gsize length;

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5  + 12;
  vectors[2].size = 3;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  g_output_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, sizeof buffer);
  g_assert_no_error (data.error);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, buffer, sizeof buffer);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

/* Test that writev_async_all() on local file output streams handles cancellation correctly */
static void
test_writev_async_all_cancellation (void)
{
  WritevAsyncData data = { 0 };
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputVector vectors[3];
  const guint8 buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};
  GOutputStream *ostream;
  GError *error = NULL;
  gboolean res;
  guint8 *contents;
  gsize length;
  GCancellable *cancellable;

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5  + 12;
  vectors[2].size = 3;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);

  g_output_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, cancellable, test_writev_all_cb, &data);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
  g_object_unref (cancellable);
}

/* Test that writev_async_all() with empty vectors is handled correctly */
static void
test_writev_async_all_empty_vectors (void)
{
  WritevAsyncData data = { 0 };
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputVector vectors[3];
  GOutputStream *ostream;
  GError *error = NULL;
  gboolean res;
  guint8 *contents;
  gsize length;

  vectors[0].buffer = NULL;
  vectors[0].size = 0;

  vectors[1].buffer = NULL;
  vectors[1].size = 0;

  vectors[2].buffer = NULL;
  vectors[2].size = 0;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  g_output_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_no_error (data.error);
  g_clear_error (&data.error);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

/* Test that writev_async_all() with no vectors is handled correctly */
static void
test_writev_async_all_no_vectors (void)
{
  WritevAsyncData data = { 0 };
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputStream *ostream;
  GError *error = NULL;
  gboolean res;
  guint8 *contents;
  gsize length;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  g_output_stream_writev_all_async (ostream, NULL, 0, 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_no_error (data.error);
  g_clear_error (&data.error);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

/* Test that writev_async_all() with too big vectors is handled correctly */
static void
test_writev_async_all_too_big_vectors (void)
{
  WritevAsyncData data = { 0 };
  GFile *file;
  GFileIOStream *iostream = NULL;
  GOutputVector vectors[3];
  GOutputStream *ostream;
  GError *error = NULL;
  gboolean res;
  guint8 *contents;
  gsize length;

  vectors[0].buffer = (void*) 1;
  vectors[0].size = G_MAXSIZE / 2;

  vectors[1].buffer = (void*) 1;
  vectors[1].size = G_MAXSIZE / 2;

  vectors[2].buffer = (void*) 1;
  vectors[2].size = G_MAXSIZE / 2;

  file = g_file_new_tmp ("g_file_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

  g_output_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_clear_error (&data.error);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_load_contents (file, NULL, (gchar **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

static void
test_build_attribute_list_for_copy (void)
{
  GFile *tmpfile;
  GFileIOStream *iostream;
  GError *error = NULL;
  const GFileCopyFlags test_flags[] =
    {
      G_FILE_COPY_NONE,
      G_FILE_COPY_TARGET_DEFAULT_PERMS,
      G_FILE_COPY_ALL_METADATA,
      G_FILE_COPY_ALL_METADATA | G_FILE_COPY_TARGET_DEFAULT_PERMS,
      G_FILE_COPY_ALL_METADATA | G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME,
      G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME | G_FILE_COPY_TARGET_DEFAULT_PERMS,
      G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME,
    };
  gsize i;
  char *attrs;
  gchar *attrs_with_commas;

  tmpfile = g_file_new_tmp ("tmp-build-attribute-list-for-copyXXXXXX",
                            &iostream, &error);
  g_assert_no_error (error);
  g_io_stream_close ((GIOStream*)iostream, NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  for (i = 0; i < G_N_ELEMENTS (test_flags); i++)
    {
      GFileCopyFlags flags = test_flags[i];

      attrs = g_file_build_attribute_list_for_copy (tmpfile, flags, NULL, &error);
      g_test_message ("Attributes for copy: %s", attrs);
      g_assert_no_error (error);
      g_assert_nonnull (attrs);
      attrs_with_commas = g_strconcat (",", attrs, ",", NULL);
      g_free (attrs);

      /* See g_local_file_class_init for reference. */
      if (flags & G_FILE_COPY_TARGET_DEFAULT_PERMS)
        g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_UNIX_MODE ","));
      else
        g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_UNIX_MODE ","));
#ifdef G_OS_UNIX
      if (flags & G_FILE_COPY_ALL_METADATA)
        {
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_UNIX_UID ","));
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_UNIX_GID ","));
        }
      else
        {
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_UNIX_UID ","));
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_UNIX_GID ","));
        }
#endif
#ifdef HAVE_UTIMES
      if (flags & G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME)
        {
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED ","));
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC ","));
        }
      else
        {
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED ","));
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC ","));
        }
      if (flags & G_FILE_COPY_ALL_METADATA)
        {
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS ","));
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS_USEC ","));
        }
      else
        {
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS ","));
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS_USEC ","));
        }
#endif
#ifdef HAVE_UTIMENSAT
      if (flags & G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME)
        {
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED ","));
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED_NSEC ","));
        }
      else
        {
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED ","));
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_MODIFIED_NSEC ","));
        }
      if (flags & G_FILE_COPY_ALL_METADATA)
        {
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS ","));
          g_assert_nonnull (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS_NSEC ","));
        }
      else
        {
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS ","));
          g_assert_null (g_strstr_len (attrs_with_commas, -1, "," G_FILE_ATTRIBUTE_TIME_ACCESS_NSEC ","));
        }
#endif
      g_free (attrs_with_commas);
    }

  (void) g_file_delete (tmpfile, NULL, NULL);
  g_clear_object (&tmpfile);
}

typedef struct
{
  GError *error;
  gboolean done;
  gboolean res;
} MoveAsyncData;

static void
test_move_async_cb (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  GFile *file = G_FILE (object);
  MoveAsyncData *data = user_data;
  GError *error = NULL;

  data->res = g_file_move_finish (file, result, &error);
  data->error = error;
  data->done = TRUE;
}

typedef struct
{
  goffset total_num_bytes;
} MoveAsyncProgressData;

static void
test_move_async_progress_cb (goffset  current_num_bytes,
                             goffset  total_num_bytes,
                             gpointer user_data)
{
  MoveAsyncProgressData *data = user_data;
  data->total_num_bytes = total_num_bytes;
}

/* Test that move_async() moves the file correctly */
static void
test_move_async (void)
{
  MoveAsyncData data = { 0 };
  MoveAsyncProgressData progress_data = { 0 };
  GFile *source;
  GFileIOStream *iostream;
  GOutputStream *ostream;
  GFile *destination;
  gchar *destination_path;
  GError *error = NULL;
  gboolean res;
  const guint8 buffer[] = {1, 2, 3, 4, 5};

  source = g_file_new_tmp ("g_file_move_XXXXXX", &iostream, NULL);

  destination_path = g_build_path (G_DIR_SEPARATOR_S, g_get_tmp_dir (), "g_file_move_target", NULL);
  destination = g_file_new_for_path (destination_path);

  g_assert_nonnull (source);
  g_assert_nonnull (iostream);

  res = g_file_query_exists (source, NULL);
  g_assert_true (res);
  res = g_file_query_exists (destination, NULL);
  g_assert_false (res);

  // Write a known amount of bytes to the file, so we can test the progress callback against it
  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write (ostream, buffer, sizeof (buffer), NULL, &error);
  g_assert_no_error (error);

  g_file_move_async (source,
                     destination,
                     G_FILE_COPY_NONE,
                     0,
                     NULL,
                     test_move_async_progress_cb,
                     &progress_data,
                     test_move_async_cb,
                     &data);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_no_error (data.error);
  g_assert_true (data.res);
  g_assert_cmpuint (progress_data.total_num_bytes, ==, sizeof (buffer));

  res = g_file_query_exists (source, NULL);
  g_assert_false (res);
  res = g_file_query_exists (destination, NULL);
  g_assert_true (res);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_delete (destination, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_object_unref (source);
  g_object_unref (destination);

  g_free (destination_path);
}

/* Same test as for move_async(), but for move_async_with_closures() */
static void
test_move_async_with_closures (void)
{
  MoveAsyncData data = { 0 };
  MoveAsyncProgressData progress_data = { 0 };
  GFile *source;
  GFileIOStream *iostream;
  GOutputStream *ostream;
  GFile *destination;
  gchar *destination_path;
  GError *error = NULL;
  gboolean res;
  const guint8 buffer[] = { 1, 2, 3, 4, 5 };
  GClosure *progress_closure;
  GClosure *ready_closure;

  source = g_file_new_tmp ("g_file_move_async_with_closures_XXXXXX", &iostream, NULL);

  destination_path = g_build_path (G_DIR_SEPARATOR_S, g_get_tmp_dir (), "g_file_move_async_with_closures_target", NULL);
  destination = g_file_new_for_path (destination_path);

  g_assert_nonnull (source);
  g_assert_nonnull (iostream);

  res = g_file_query_exists (source, NULL);
  g_assert_true (res);
  res = g_file_query_exists (destination, NULL);
  g_assert_false (res);

  /* Write a known number of bytes to the file, so we can test the progress
   * callback against it */
  ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write (ostream, buffer, sizeof (buffer), NULL, &error);
  g_assert_no_error (error);

  progress_closure = g_cclosure_new (G_CALLBACK (test_move_async_progress_cb), &progress_data, NULL);
  ready_closure = g_cclosure_new (G_CALLBACK (test_move_async_cb), &data, NULL);

  g_file_move_async_with_closures (source,
                                   destination,
                                   G_FILE_COPY_NONE,
                                   0,
                                   NULL,
                                   progress_closure,
                                   ready_closure);

  while (!data.done)
    g_main_context_iteration (NULL, TRUE);

  g_assert_no_error (data.error);
  g_assert_true (data.res);
  g_assert_cmpuint (progress_data.total_num_bytes, ==, sizeof (buffer));

  res = g_file_query_exists (source, NULL);
  g_assert_false (res);
  res = g_file_query_exists (destination, NULL);
  g_assert_true (res);

  res = g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_object_unref (iostream);

  res = g_file_delete (destination, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_object_unref (source);
  g_object_unref (destination);

  g_free (destination_path);
}

static GAppInfo *
create_command_line_app_info (const char *name,
                              const char *command_line,
                              const char *default_for_type)
{
  GAppInfo *info;
  GError *error = NULL;

  info = g_app_info_create_from_commandline (command_line,
                                             name,
                                             G_APP_INFO_CREATE_NONE,
                                             &error);
  g_assert_no_error (error);

  g_app_info_set_as_default_for_type (info, default_for_type, &error);
  g_assert_no_error (error);

  return g_steal_pointer (&info);
}

static gboolean
skip_missing_update_desktop_database (void)
{
  gchar *path = g_find_program_in_path ("update-desktop-database");

  if (path == NULL)
    {
      g_test_skip ("update-desktop-database is required to run this test");
      return TRUE;
    }
  g_free (path);
  return FALSE;
}

static void
test_query_default_handler_uri (void)
{
  GError *error = NULL;
  GAppInfo *info;
  GAppInfo *default_info;
  GFile *file;
  GFile *invalid_file;

  if (skip_missing_update_desktop_database ())
    return;

  info = create_command_line_app_info ("Gio File Handler", "true",
                                       "x-scheme-handler/gio-file");
  g_assert_true (G_IS_APP_INFO (info));

  file = g_file_new_for_uri ("gio-file://hello-gio!");
  default_info = g_file_query_default_handler (file, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_app_info_equal (default_info, info));

  invalid_file = g_file_new_for_uri ("gio-file-INVALID://goodbye-gio!");
  g_assert_null (g_file_query_default_handler (invalid_file, NULL, &error));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  g_app_info_remove_supports_type (info, "x-scheme-handler/gio-file", &error);
  g_assert_no_error (error);
  g_app_info_reset_type_associations ("x-scheme-handler/gio-file");

  g_object_unref (default_info);
  g_object_unref (info);
  g_object_unref (file);
  g_object_unref (invalid_file);
}

static void
test_query_zero_length_content_type (void)
{
  GFile *empty_file;
  GFileInfo *file_info;
  GError *error = NULL;
  GFileIOStream *iostream;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=755795");
  /* Historically, GLib used to explicitly consider zero-size files as text/plain,
   * so they opened in a text editor. In 2.76, we changed that to application/x-zerosize,
   * because that’s what xdgmime uses:
   * - https://gitlab.gnome.org/GNOME/glib/-/blob/2.74.0/gio/glocalfileinfo.c#L1360-1369
   * - https://bugzilla.gnome.org/show_bug.cgi?id=755795
   * - https://gitlab.gnome.org/GNOME/glib/-/issues/2777
   */
  g_test_summary ("empty files should always be considered application/x-zerosize");

  empty_file = g_file_new_tmp ("empty-file-XXXXXX", &iostream, &error);
  g_assert_no_error (error);

  g_io_stream_close (G_IO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  file_info =
    g_file_query_info (empty_file,
                       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                       G_FILE_QUERY_INFO_NONE,
                       NULL, &error);
  g_assert_no_error (error);

#ifndef __APPLE__
  g_assert_cmpstr (g_file_info_get_content_type (file_info), ==, "application/x-zerosize");
#else
  g_assert_cmpstr (g_file_info_get_content_type (file_info), ==, "public.text");
#endif

  g_clear_object (&file_info);
  g_clear_object (&empty_file);
}

static void
test_query_default_handler_file (void)
{
  GError *error = NULL;
  GAppInfo *info;
  GAppInfo *default_info;
  GFile *text_file;
  GFile *binary_file;
  GFile *invalid_file;
  GFileIOStream *iostream;
  GOutputStream *output_stream;
  const char buffer[] = "Text file!\n";
  const guint8 binary_buffer[] = "\xde\xad\xbe\xff";

  if (skip_missing_update_desktop_database ())
    return;

  text_file = g_file_new_tmp ("query-default-handler-XXXXXX", &iostream, &error);
  g_assert_no_error (error);

  output_stream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write_all (output_stream, buffer, G_N_ELEMENTS (buffer) - 1,
                             NULL, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_flush (output_stream, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_close (output_stream, NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  info = create_command_line_app_info ("Text handler", "true", "text/plain");
  g_assert_true (G_IS_APP_INFO (info));

  default_info = g_file_query_default_handler (text_file, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_app_info_equal (default_info, info));

  invalid_file = g_file_new_for_path ("/hopefully/this-does-not-exists");
  g_assert_null (g_file_query_default_handler (invalid_file, NULL, &error));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
  g_clear_error (&error);

  binary_file = g_file_new_tmp ("query-default-handler-bin-XXXXXX", &iostream, &error);
  g_assert_no_error (error);

  output_stream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write_all (output_stream, binary_buffer,
                             G_N_ELEMENTS (binary_buffer),
                             NULL, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_flush (output_stream, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_close (output_stream, NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  g_assert_null (g_file_query_default_handler (binary_file, NULL, &error));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  g_app_info_remove_supports_type (info, "text/plain", &error);
  g_assert_no_error (error);
  g_app_info_reset_type_associations ("text/plain");

  g_object_unref (default_info);
  g_object_unref (info);
  g_object_unref (text_file);
  g_object_unref (binary_file);
  g_object_unref (invalid_file);
}

typedef struct {
  GMainLoop *loop;
  GAppInfo *info;
  GError *error;
} QueryDefaultHandlerData;

static void
on_query_default (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  QueryDefaultHandlerData *data = user_data;

  data->info = g_file_query_default_handler_finish (G_FILE (source), result,
                                                    &data->error);
  g_main_loop_quit (data->loop);
}

static void
test_query_default_handler_file_async (void)
{
  QueryDefaultHandlerData data = {0};
  GCancellable *cancellable;
  GAppInfo *info;
  GFile *text_file;
  GFile *binary_file;
  GFile *invalid_file;
  GFileIOStream *iostream;
  GOutputStream *output_stream;
  const char buffer[] = "Text file!\n";
  const guint8 binary_buffer[] = "\xde\xad\xbe\xff";
  GError *error = NULL;

  if (skip_missing_update_desktop_database ())
    return;

  data.loop = g_main_loop_new (NULL, FALSE);

  text_file = g_file_new_tmp ("query-default-handler-XXXXXX", &iostream, &error);
  g_assert_no_error (error);

  output_stream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write_all (output_stream, buffer, G_N_ELEMENTS (buffer) - 1,
                             NULL, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_close (output_stream, NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  info = create_command_line_app_info ("Text handler", "true", "text/plain");
  g_assert_true (G_IS_APP_INFO (info));

  g_file_query_default_handler_async (text_file, G_PRIORITY_DEFAULT,
                                      NULL, on_query_default,
                                      &data);
  g_main_loop_run (data.loop);
  g_assert_no_error (data.error);
  g_assert_true (g_app_info_equal (data.info, info));
  g_clear_object (&data.info);

  invalid_file = g_file_new_for_path ("/hopefully/this/.file/does-not-exists");
  g_file_query_default_handler_async (invalid_file, G_PRIORITY_DEFAULT,
                                      NULL, on_query_default,
                                      &data);
  g_main_loop_run (data.loop);
  g_assert_null (data.info);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
  g_clear_error (&data.error);

  cancellable = g_cancellable_new ();
  g_file_query_default_handler_async (text_file, G_PRIORITY_DEFAULT,
                                      cancellable, on_query_default,
                                      &data);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (data.loop);
  g_assert_null (data.info);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  binary_file = g_file_new_tmp ("query-default-handler-bin-XXXXXX", &iostream, &error);
  g_assert_no_error (error);

  output_stream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
  g_output_stream_write_all (output_stream, binary_buffer,
                             G_N_ELEMENTS (binary_buffer),
                             NULL, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_close (output_stream, NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  g_file_query_default_handler_async (binary_file, G_PRIORITY_DEFAULT,
                                      NULL, on_query_default,
                                      &data);
  g_main_loop_run (data.loop);
  g_assert_null (data.info);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&data.error);

  g_app_info_remove_supports_type (info, "text/plain", &error);
  g_assert_no_error (error);
  g_app_info_reset_type_associations ("text/plain");

  g_main_loop_unref (data.loop);
  g_object_unref (info);
  g_object_unref (text_file);
  g_object_unref (binary_file);
  g_object_unref (invalid_file);
}

static void
test_query_default_handler_uri_async (void)
{
  QueryDefaultHandlerData data = {0};
  GCancellable *cancellable;
  GAppInfo *info;
  GFile *file;
  GFile *invalid_file;

  if (skip_missing_update_desktop_database ())
    return;

  info = create_command_line_app_info ("Gio File Handler", "true",
                                       "x-scheme-handler/gio-file");
  g_assert_true (G_IS_APP_INFO (info));

  data.loop = g_main_loop_new (NULL, FALSE);

  file = g_file_new_for_uri ("gio-file://hello-gio!");
  g_file_query_default_handler_async (file, G_PRIORITY_DEFAULT,
                                      NULL, on_query_default,
                                      &data);
  g_main_loop_run (data.loop);
  g_assert_no_error (data.error);
  g_assert_true (g_app_info_equal (data.info, info));
  g_clear_object (&data.info);

  invalid_file = g_file_new_for_uri ("gio-file-INVALID://goodbye-gio!");
  g_file_query_default_handler_async (invalid_file, G_PRIORITY_DEFAULT,
                                      NULL, on_query_default,
                                      &data);
  g_main_loop_run (data.loop);
  g_assert_null (data.info);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&data.error);

  cancellable = g_cancellable_new ();
  g_file_query_default_handler_async (file, G_PRIORITY_DEFAULT,
                                      cancellable, on_query_default,
                                      &data);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (data.loop);
  g_assert_null (data.info);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  g_app_info_remove_supports_type (info, "x-scheme-handler/gio-file", &data.error);
  g_assert_no_error (data.error);
  g_app_info_reset_type_associations ("x-scheme-handler/gio-file");

  g_main_loop_unref (data.loop);
  g_object_unref (info);
  g_object_unref (file);
  g_object_unref (invalid_file);
}

static void
test_enumerator_cancellation (void)
{
  GCancellable *cancellable;
  GFileEnumerator *enumerator;
  GFileInfo *info;
  GFile *dir;
  GError *error = NULL;

  dir = g_file_new_for_path (g_get_tmp_dir ());
  g_assert_nonnull (dir);

  enumerator = g_file_enumerate_children (dir,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL,
                                          &error);
  g_assert_nonnull (enumerator);

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  info = g_file_enumerator_next_file (enumerator, cancellable, &error);
  g_assert_null (info);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);

  g_error_free (error);
  g_object_unref (cancellable);
  g_object_unref (enumerator);
  g_object_unref (dir);
}

static void
test_path_from_uri_helper (const gchar *uri,
			   const gchar *expected_path)
{
  GFile *file;
  gchar *path;
  gchar *expected_platform_path;

  expected_platform_path = g_strdup (expected_path);
#ifdef G_OS_WIN32
  for (gchar *p = expected_platform_path; *p; p++)
    {
      if (*p == '/')
	*p = '\\';
    }
#endif

  file = g_file_new_for_uri (uri);
  path = g_file_get_path (file);
  g_assert_cmpstr (path, ==, expected_platform_path);
  g_free (path);
  g_object_unref (file);
  g_free (expected_platform_path);
}

static void
test_from_uri_ignores_fragment (void)
{
  test_path_from_uri_helper ("file:///tmp/foo#bar", "/tmp/foo");
  test_path_from_uri_helper ("file:///tmp/foo#bar?baz", "/tmp/foo");
}

static void
test_from_uri_ignores_query_string (void)
{
  test_path_from_uri_helper ("file:///tmp/foo?bar", "/tmp/foo");
  test_path_from_uri_helper ("file:///tmp/foo?bar#baz", "/tmp/foo");
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/file/basic", test_basic);
  g_test_add_func ("/file/build-filename", test_build_filename);
  g_test_add_func ("/file/build-filenamev", test_build_filenamev);
  g_test_add_func ("/file/parent", test_parent);
  g_test_add_func ("/file/child", test_child);
  g_test_add_func ("/file/empty-path", test_empty_path);
  g_test_add_func ("/file/type", test_type);
  g_test_add_func ("/file/parse-name", test_parse_name);
  g_test_add_data_func ("/file/async-create-delete/0", GINT_TO_POINTER (0), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/1", GINT_TO_POINTER (1), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/10", GINT_TO_POINTER (10), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/25", GINT_TO_POINTER (25), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/4096", GINT_TO_POINTER (4096), test_create_delete);
  g_test_add_func ("/file/replace-load", test_replace_load);
  g_test_add_func ("/file/replace-cancel", test_replace_cancel);
  g_test_add_func ("/file/replace-symlink", test_replace_symlink);
  g_test_add_func ("/file/replace-symlink/using-etag", test_replace_symlink_using_etag);
  g_test_add_data_func ("/file/replace/write-only", GUINT_TO_POINTER (FALSE), test_replace);
  g_test_add_data_func ("/file/replace/read-write", GUINT_TO_POINTER (TRUE), test_replace);
  g_test_add_func ("/file/async-new-tmp", test_async_new_tmp);
  g_test_add_func ("/file/async-new-tmp-dir", test_async_new_tmp_dir);
  g_test_add_func ("/file/async-delete", test_async_delete);
  g_test_add_func ("/file/async-make-symlink", test_async_make_symlink);
  g_test_add_func ("/file/copy-preserve-mode", test_copy_preserve_mode);
  g_test_add_func ("/file/copy/progress", test_copy_progress);
  g_test_add_func ("/file/copy-async-with-closures", test_copy_async_with_closures);
  g_test_add_func ("/file/measure", test_measure);
  g_test_add_func ("/file/measure-async", test_measure_async);
  g_test_add_func ("/file/load-bytes", test_load_bytes);
  g_test_add_func ("/file/load-bytes-async", test_load_bytes_async);
  g_test_add_func ("/file/load-bytes-4gb", test_load_bytes_4gb);
  g_test_add_func ("/file/load-contents-4gb", test_load_contents_4gb);
  g_test_add_func ("/file/load-contents-4gb-async", test_load_contents_4gb_async);
  g_test_add_func ("/file/writev", test_writev);
  g_test_add_func ("/file/writev/no-bytes-written", test_writev_no_bytes_written);
  g_test_add_func ("/file/writev/no-vectors", test_writev_no_vectors);
  g_test_add_func ("/file/writev/empty-vectors", test_writev_empty_vectors);
  g_test_add_func ("/file/writev/too-big-vectors", test_writev_too_big_vectors);
  g_test_add_func ("/file/writev/async", test_writev_async);
  g_test_add_func ("/file/writev/async_all", test_writev_async_all);
  g_test_add_func ("/file/writev/async_all-empty-vectors", test_writev_async_all_empty_vectors);
  g_test_add_func ("/file/writev/async_all-no-vectors", test_writev_async_all_no_vectors);
  g_test_add_func ("/file/writev/async_all-to-big-vectors", test_writev_async_all_too_big_vectors);
  g_test_add_func ("/file/writev/async_all-cancellation", test_writev_async_all_cancellation);
  g_test_add_func ("/file/build-attribute-list-for-copy", test_build_attribute_list_for_copy);
  g_test_add_func ("/file/move_async", test_move_async);
  g_test_add_func ("/file/move-async-with-closures", test_move_async_with_closures);
  g_test_add_func ("/file/query-zero-length-content-type", test_query_zero_length_content_type);
  g_test_add_func ("/file/query-default-handler-file", test_query_default_handler_file);
  g_test_add_func ("/file/query-default-handler-file-async", test_query_default_handler_file_async);
  g_test_add_func ("/file/query-default-handler-uri", test_query_default_handler_uri);
  g_test_add_func ("/file/query-default-handler-uri-async", test_query_default_handler_uri_async);
  g_test_add_func ("/file/enumerator-cancellation", test_enumerator_cancellation);
  g_test_add_func ("/file/from-uri/ignores-query-string", test_from_uri_ignores_query_string);
  g_test_add_func ("/file/from-uri/ignores-fragment", test_from_uri_ignores_fragment);

  return g_test_run ();
}

