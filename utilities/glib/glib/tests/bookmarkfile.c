#include <glib.h>
#include <glib/gstdio.h>

#define TEST_URI_0 	"file:///abc/defgh/ijklmnopqrstuvwxyz"
#define TEST_URI_1 	"file:///test/uri/1"
#define TEST_URI_2 	"file:///test/uri/2"

#define TEST_MIME 	"text/plain"

#define TEST_APP_NAME 	"bookmarkfile-test"
#define TEST_APP_EXEC 	"bookmarkfile-test %f"

static void
test_load_from_data_dirs (void)
{
  GBookmarkFile *bookmark;
  gboolean res;
  gchar *path = NULL;
  GError *error = NULL;

  bookmark = g_bookmark_file_new ();

  res = g_bookmark_file_load_from_data_dirs (bookmark, "no-such-bookmark-file.xbel", &path, &error);

  g_assert_false (res);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
  g_assert_null (path);
  g_error_free (error);

  g_bookmark_file_free (bookmark);  
}

static void
test_to_file (void)
{
  GBookmarkFile *bookmark;
  const gchar *filename;
  gboolean res;
  GError *error = NULL;
  char *in, *mime, *out;
  gchar *tmp_filename = NULL;
  gint fd;

  fd = g_file_open_tmp ("bookmarkfile-test-XXXXXX.xbel", &tmp_filename, NULL);
  g_assert_cmpint (fd, >, -1);
  g_close (fd, NULL);

  bookmark = g_bookmark_file_new ();

  g_test_message ("Roundtrip from newly created bookmark file %s", tmp_filename);
  g_bookmark_file_set_title (bookmark, "file:///tmp/schedule.ps", "schedule.ps");
  g_bookmark_file_set_mime_type (bookmark, "file:///tmp/schedule.ps", "application/postscript");
  g_bookmark_file_set_icon (bookmark,
                            "file:///tmp/schedule.ps",
                            "application\">-x-postscript",
                            "image/\">png");
  g_bookmark_file_add_application (bookmark, "file:///tmp/schedule.ps", "ghostscript", "ghostscript %F");

  res = g_bookmark_file_to_file (bookmark, tmp_filename, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  res = g_bookmark_file_load_from_file (bookmark, tmp_filename, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  out = g_bookmark_file_get_title (bookmark, "file:///tmp/schedule.ps", &error);
  g_assert_no_error (error);
  g_assert_cmpstr (out, ==, "schedule.ps");
  g_free (out);

  out = g_bookmark_file_get_mime_type (bookmark, "file:///tmp/schedule.ps", &error);
  g_assert_no_error (error);
  g_assert_cmpstr (out, ==, "application/postscript");
  g_free (out);

  remove (tmp_filename);
  res = g_bookmark_file_get_icon (bookmark,
                                  "file:///tmp/schedule.ps",
                                  &out,
                                  &mime,
                                  &error);

  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpstr (out, ==, "application\">-x-postscript");
  g_free (out);
  g_assert_cmpstr (mime, ==, "image/\">png");
  g_free (mime);

  g_test_message ("Roundtrip from a valid bookmark file");
  filename = g_test_get_filename (G_TEST_DIST, "bookmarks", "valid-01.xbel", NULL);
  res = g_bookmark_file_load_from_file (bookmark, filename, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  res = g_bookmark_file_to_file (bookmark, tmp_filename, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  res = g_file_get_contents (filename, &in, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  res = g_file_get_contents (tmp_filename, &out, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  remove (tmp_filename);

  g_assert_cmpstr (in, ==, out);
  g_free (in);
  g_free (out);

  g_bookmark_file_free (bookmark);
  g_free (tmp_filename);
}

static void
test_move_item (void)
{
  GBookmarkFile *bookmark;
  const gchar *filename;
  gboolean res;
  GError *error = NULL;

  bookmark = g_bookmark_file_new ();

  filename = g_test_get_filename (G_TEST_DIST, "bookmarks", "valid-01.xbel", NULL);
  res = g_bookmark_file_load_from_file (bookmark, filename, &error);
  g_assert_true (res);
  g_assert_no_error (error);

  res = g_bookmark_file_move_item (bookmark,
                                   "file:///home/zefram/Documents/milan-stuttgart.ps",
                                   "file:///tmp/schedule.ps",
                                   &error);
  g_assert_true (res);
  g_assert_no_error (error);

  res = g_bookmark_file_move_item (bookmark,
                                   "file:///tmp/schedule.ps",
                                   "file:///tmp/schedule.ps",
                                   &error);
  g_assert_true (res);
  g_assert_no_error (error);

  res = g_bookmark_file_move_item (bookmark,
                                   "file:///no-such-file.xbel",
                                   "file:///tmp/schedule.ps",
                                   &error);
  g_assert_false (res);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  res = g_bookmark_file_move_item (bookmark,
                                   "file:///tmp/schedule.ps",
                                   NULL,
                                   &error);
  g_assert_true (res);
  g_assert_no_error (error);

  g_bookmark_file_free (bookmark);
}

static void
test_corner_cases (void)
{
  gsize size;
  gchar *message, **messages;
  GError *error = NULL;
  GBookmarkFile *bookmark;
  char *path = NULL;

  bookmark = g_bookmark_file_new ();

  if (g_test_undefined ())
    {
      /* g_bookmark_file_load_from_data() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_data (NULL, NULL, -1, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_load_from_file() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_file (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_file (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_load_from_data_dirs() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_data_dirs (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_data_dirs (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_to_data() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_to_data (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_to_file() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_to_file (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_to_file (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_remove_item() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_item (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_item (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_has_item() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_item (NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_item (bookmark, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_uris() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_uris (NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_title() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_title (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_title() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_title (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_description() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_description (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_description() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_get_description (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_mime_type() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_mime_type (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_mime_type (bookmark, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_mime_type (bookmark, "uri", NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_mime_type() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_mime_type (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_mime_type (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_is_private() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_is_private (NULL, NULL, TRUE);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_is_private (bookmark, NULL, TRUE);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_is_private() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_is_private (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_is_private (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_added_date_time() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_added_date_time (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_added_date_time (bookmark, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_added_date_time (bookmark, "a", NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_added_date_time() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_added_date_time (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_added_date_time (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_modified_date_time() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_modified_date_time (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_modified_date_time (bookmark, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_modified_date_time (bookmark, "a", NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_modified_date_time() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_modified_date_time (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_modified_date_time (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_visited_date_time() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_visited_date_time (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_visited_date_time (bookmark, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_visited_date_time (bookmark, "a", NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_visited_date_time() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_visited_date_time (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_visited_date_time (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_load_from_data_dirs() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_data_dirs (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_load_from_data_dirs (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_has_group() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_group (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_group (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_add_group() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_add_group (NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_add_group (bookmark, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_add_group (bookmark, "a", NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_add_group (bookmark, "a", "");
      g_test_assert_expected_messages ();

      /* g_bookmark_file_remove_group() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_group (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_group (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_group() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_groups (NULL, NULL, NULL, 0);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_groups (bookmark, NULL, NULL, 0);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_groups (bookmark, "a", NULL, 0);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_group() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_groups (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_groups (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_to_file() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_to_file (NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_to_file (bookmark, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_add_application() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_add_application (NULL, NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_add_application (bookmark, NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_remove_application() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_application (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_application (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_remove_application (bookmark, "a", NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_has_application() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_application (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_application (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_has_application (bookmark, "a", NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_application_info() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_set_application_info (NULL, NULL, NULL, NULL, 0, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_set_application_info (bookmark, NULL, NULL, NULL, 0, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_set_application_info (bookmark, "a", NULL, NULL, 0, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_set_application_info (bookmark, "a", "b", NULL, 0, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_set_application_info (bookmark, "a", "b", "c", 5, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_application_info() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_application_info (NULL, NULL, NULL, NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_application_info (bookmark, NULL, NULL, NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_application_info (bookmark, "a", NULL, NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_applications() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_applications (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_bookmark_file_get_applications (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_size() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_cmpint (g_bookmark_file_get_size (NULL), ==, 0);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_move_item() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_move_item (NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_move_item (bookmark, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      /* g_bookmark_file_set_icon() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_icon (NULL, NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_bookmark_file_set_icon (bookmark, NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      /* g_bookmark_file_get_icon() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_icon (NULL, NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_bookmark_file_get_icon (bookmark, NULL, NULL, NULL, NULL));
      g_test_assert_expected_messages ();
    }

  /* g_file_bookmark_free() */
  g_bookmark_file_free (NULL);

  /* g_bookmark_file_load_from_data() */
  g_assert_false (g_bookmark_file_load_from_data (bookmark, "data", -1, &error));
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE);
  g_clear_error (&error);

  /* g_bookmark_file_load_from_data_dirs() */
  g_assert_false (g_bookmark_file_load_from_data_dirs (bookmark, "a", NULL, NULL));
  g_assert_false (g_bookmark_file_load_from_data_dirs (bookmark, "a", NULL, &error));
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
  g_clear_error (&error);

  /* g_bookmark_file_to_data() */
  message = g_bookmark_file_to_data (bookmark, &size, &error);
  g_assert_nonnull (message);
  g_assert_cmpstr (message, ==,
                   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<xbel version=\"1.0\"\n"
                   "      xmlns:bookmark=\"http://www.freedesktop.org/standards/desktop-bookmarks\"\n"
                   "      xmlns:mime=\"http://www.freedesktop.org/standards/shared-mime-info\"\n"
                   "></xbel>");
  g_free (message);

  /* g_bookmark_file_get_uris() */
  size = 10;
  messages = g_bookmark_file_get_uris (bookmark, &size);
  g_assert_nonnull (messages);
  g_assert_null (messages[0]);
  g_free (messages);

  /* g_bookmark_file_get_added_date_time() */
  g_assert_null (g_bookmark_file_get_added_date_time (bookmark, "a", NULL));
  g_assert_null (g_bookmark_file_get_added_date_time (bookmark, "a", &error));
  g_clear_error (&error);

  /* g_bookmark_file_get_modified_date_time() */
  g_assert_null (g_bookmark_file_get_modified_date_time (bookmark, "a", NULL));
  g_assert_null (g_bookmark_file_get_modified_date_time (bookmark, "a", &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  /* g_bookmark_file_get_visited_date_time() */
  g_assert_null (g_bookmark_file_get_visited_date_time (bookmark, "a", NULL));
  g_assert_null (g_bookmark_file_get_visited_date_time (bookmark, "a", &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  /* g_bookmark_file_get_groups() */
  g_assert_null (g_bookmark_file_get_groups (bookmark, "a", &size, NULL));
  g_assert_null (g_bookmark_file_get_groups (bookmark, "a", &size, &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  /* g_bookmark_file_to_file() */
  path = g_build_filename (g_get_tmp_dir (), "corner-case-bookmark-file.xbel", NULL);
  g_assert_true (g_bookmark_file_to_file (bookmark, path, &error));
  g_assert_no_error (error);
  g_free (path);

  /* g_bookmark_file_remove_group() */
  g_assert_false (g_bookmark_file_remove_group (bookmark, "a", NULL, NULL));
  g_assert_false (g_bookmark_file_remove_group (bookmark, "a", NULL, &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  /* g_bookmark_file_get_title() */
  g_assert_null (g_bookmark_file_get_title (bookmark, "a", &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  /* g_bookmark_file_add_application() */
  g_bookmark_file_add_application (bookmark, "a", NULL, NULL);
  g_bookmark_file_add_application (bookmark, "a", "b", NULL);
  g_bookmark_file_add_application (bookmark, "a", "b", "c");

  /* g_bookmark_file_remove_application() */
  g_assert_true (g_bookmark_file_remove_application (bookmark, "a", "b", NULL));
  g_assert_false (g_bookmark_file_remove_application (bookmark, "a", "b", &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED);
  g_clear_error (&error);

  /* g_bookmark_file_get_application_info() */
  g_assert_false (g_bookmark_file_get_application_info (bookmark, "a", "b", NULL, NULL, NULL, NULL));
  g_assert_false (g_bookmark_file_get_application_info (bookmark, "a", "b", NULL, NULL, NULL, &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED);
  g_clear_error (&error);

  /* g_bookmark_file_move_item() */
  g_assert_true (g_bookmark_file_move_item (bookmark, "a", NULL, NULL));
  g_assert_false (g_bookmark_file_move_item (bookmark, "a", NULL, &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  g_assert_false (g_bookmark_file_move_item (bookmark, "a", "b", &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  g_bookmark_file_free (bookmark);
  g_unlink ("a");
}

static void
test_misc (void)
{
  GBookmarkFile *bookmark;
  const gchar *filename;
  gboolean res;
  GError *error = NULL;
  gchar *s;
  GDateTime *before, *after, *t;
  gchar *cmd, *exec;
  guint count;

  bookmark = g_bookmark_file_new ();

  filename = g_test_get_filename (G_TEST_DIST, "bookmarks", "valid-01.xbel", NULL);
  res = g_bookmark_file_load_from_file (bookmark, filename, &error);
  g_assert_true (res);
  g_assert_no_error (error);

  res = g_bookmark_file_get_icon (bookmark,
                                   "file:///home/zefram/Documents/milan-stuttgart.ps",
                                  NULL,
                                  NULL,
                                  &error);
  g_assert_false (res);
  g_assert_no_error (error);

  res = g_bookmark_file_get_icon (bookmark,
                                  "file:///tmp/schedule.ps",
                                  NULL,
                                  NULL,
                                  &error);
  g_assert_false (res);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);

  g_bookmark_file_set_description (bookmark,
                                   "file:///tmp/schedule0.ps",
                                   "imaginary schedule");
  s = g_bookmark_file_get_description (bookmark,
                                       "file:///tmp/schedule0.ps",
                                       &error);
  g_assert_no_error (error);
  g_assert_cmpstr (s, ==, "imaginary schedule");
  g_free (s);
  s = g_bookmark_file_get_mime_type (bookmark,
                                     "file:///tmp/schedule0.ps",
                                     &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_INVALID_VALUE);
  g_assert_null (s);
  g_clear_error (&error);
  res = g_bookmark_file_get_is_private (bookmark,
                                        "file:///tmp/schedule0.ps",
                                        &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_INVALID_VALUE);
  g_clear_error (&error);
  g_assert_false (res);

  g_bookmark_file_set_mime_type (bookmark, 
                                 "file:///tmp/schedule1.ps",
                                 "image/png");
  s = g_bookmark_file_get_mime_type (bookmark,
                                     "file:///tmp/schedule1.ps",
                                     &error);
  g_assert_no_error (error);
  g_assert_cmpstr (s, ==, "image/png");
  g_free (s);
  
  g_bookmark_file_set_is_private (bookmark,
                                  "file:///tmp/schedule2.ps",
                                  TRUE);
  res = g_bookmark_file_get_is_private (bookmark,
                                        "file:///tmp/schedule2.ps",
                                        &error);
  g_assert_no_error (error);
  g_assert_true (res);

  before = g_date_time_new_now_utc ();

  g_bookmark_file_set_added_date_time (bookmark,
                                       "file:///tmp/schedule3.ps",
                                       before);
  t = g_bookmark_file_get_added_date_time (bookmark,
                                           "file:///tmp/schedule3.ps",
                                           &error);
  g_assert_no_error (error);

  after = g_date_time_new_now_utc ();
  g_assert_cmpint (g_date_time_compare (before, t), <=, 0);
  g_assert_cmpint (g_date_time_compare (t, after), <=, 0);

  g_date_time_unref (after);
  g_date_time_unref (before);

  before = g_date_time_new_now_utc ();

  g_bookmark_file_set_modified_date_time (bookmark,
                                          "file:///tmp/schedule4.ps",
                                          before);
  t = g_bookmark_file_get_modified_date_time (bookmark,
                                              "file:///tmp/schedule4.ps",
                                              &error);
  g_assert_no_error (error);

  after = g_date_time_new_now_utc ();
  g_assert_cmpint (g_date_time_compare (before, t), <=, 0);
  g_assert_cmpint (g_date_time_compare (t, after), <=, 0);

  g_date_time_unref (after);
  g_date_time_unref (before);

  before = g_date_time_new_now_utc ();

  g_bookmark_file_set_visited_date_time (bookmark,
                                         "file:///tmp/schedule5.ps",
                                         before);
  t = g_bookmark_file_get_visited_date_time (bookmark,
                                             "file:///tmp/schedule5.ps",
                                             &error);
  g_assert_no_error (error);

  after = g_date_time_new_now_utc ();
  g_assert_cmpint (g_date_time_compare (before, t), <=, 0);
  g_assert_cmpint (g_date_time_compare (t, after), <=, 0);
  g_date_time_unref (after);
  g_date_time_unref (before);

  g_bookmark_file_set_icon (bookmark,
                            "file:///tmp/schedule6.ps",
                            "application-x-postscript",
                            "image/png");
  res = g_bookmark_file_get_icon (bookmark,
                                  "file:///tmp/schedule6.ps",
                                  &s,
                                  NULL, 
                                  &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpstr (s, ==, "application-x-postscript");
  g_free (s);

  g_bookmark_file_set_icon (bookmark,
                            "file:///tmp/schedule6.ps",
                            NULL, NULL);
  res = g_bookmark_file_get_icon (bookmark,
                                  "file:///tmp/schedule6.ps",
                                  &s,
                                  NULL, 
                                  &error);
  g_assert_no_error (error);
  g_assert_false (res);

  res = g_bookmark_file_has_application (bookmark,
                                         "file:///tmp/schedule7.ps",
                                         "foo",
                                         &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_assert_false (res);
  g_clear_error (&error);

  before = g_date_time_new_now_utc ();

  g_bookmark_file_add_application (bookmark,
                                   "file:///tmp/schedule7.ps",
                                   NULL, NULL);
  res = g_bookmark_file_get_application_info (bookmark,
                                              "file:///tmp/schedule7.ps",
                                              g_get_application_name (),
                                              &exec, &count, &t,
                                              &error);
  g_assert_no_error (error);
  g_assert_true (res);
  cmd = g_strconcat (g_get_prgname (), " file:///tmp/schedule7.ps", NULL);
  g_assert_cmpstr (exec, ==, cmd);
  g_free (cmd);
  g_free (exec);
  g_assert_cmpuint (count, ==, 1);

  after = g_date_time_new_now_utc ();
  g_assert_cmpint (g_date_time_compare (before, t), <=, 0);
  g_assert_cmpint (g_date_time_compare (t, after), <=, 0);

  g_date_time_unref (after);
  g_date_time_unref (before);

  g_bookmark_file_free (bookmark);
}

static void
test_deprecated (void)
{
  GBookmarkFile *file = NULL;
  GError *local_error = NULL;
  time_t t, now;
  gboolean retval;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  now = g_get_real_time () / G_USEC_PER_SEC;
  file = g_bookmark_file_new ();

  /* added */
  g_bookmark_file_set_added (file, "file://test", -1);
  t = g_bookmark_file_get_added (file, "file://test", &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (t, >=, now);

  g_bookmark_file_set_added (file, "file://test", 1234);
  t = g_bookmark_file_get_added (file, "file://test", &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (t, ==, 1234);

  t = g_bookmark_file_get_added (file, "file://not-exist", &local_error);
  g_assert_error (local_error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_assert_cmpint (t, ==, (time_t) -1);
  g_clear_error (&local_error);

  /* modified */
  g_bookmark_file_set_modified (file, "file://test", -1);
  t = g_bookmark_file_get_modified (file, "file://test", &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (t, >=, now);

  g_bookmark_file_set_modified (file, "file://test", 1234);
  t = g_bookmark_file_get_modified (file, "file://test", &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (t, ==, 1234);

  t = g_bookmark_file_get_modified (file, "file://not-exist", &local_error);
  g_assert_error (local_error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_assert_cmpint (t, ==, (time_t) -1);
  g_clear_error (&local_error);

  /* visited */
  g_bookmark_file_set_visited (file, "file://test", -1);
  t = g_bookmark_file_get_visited (file, "file://test", &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (t, >=, now);

  g_bookmark_file_set_visited (file, "file://test", 1234);
  t = g_bookmark_file_get_visited (file, "file://test", &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (t, ==, 1234);

  t = g_bookmark_file_get_visited (file, "file://not-exist", &local_error);
  g_assert_error (local_error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_assert_cmpint (t, ==, (time_t) -1);
  g_clear_error (&local_error);

  /* set app info */
  retval = g_bookmark_file_set_app_info (file, "file://test", "app", "/path/to/app", 1, -1, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (retval);

  retval = g_bookmark_file_get_app_info (file, "file://test", "app", NULL, NULL, &t, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (retval);
  g_assert_cmpint (t, >=, now);

  retval = g_bookmark_file_set_app_info (file, "file://test", "app", "/path/to/app", 1, 1234, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (retval);

  retval = g_bookmark_file_get_app_info (file, "file://test", "app", NULL, NULL, &t, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (retval);
  g_assert_cmpint (t, ==, 1234);

  retval = g_bookmark_file_get_app_info (file, "file://test", "app", NULL, NULL, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (retval);

  retval = g_bookmark_file_get_app_info (file, "file://not-exist", "app", NULL, NULL, &t, &local_error);
  g_assert_error (local_error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_assert_false (retval);
  g_clear_error (&local_error);

  g_bookmark_file_free (file);

G_GNUC_END_IGNORE_DEPRECATIONS
}

static gboolean
test_load (GBookmarkFile *bookmark,
           const gchar   *filename)
{
  GError *error = NULL;
  gboolean res;
  
  res = g_bookmark_file_load_from_file (bookmark, filename, &error);
  if (error && g_test_verbose ())
    g_printerr ("Load error: %s\n", error->message);

  g_clear_error (&error);
  return res;
}

static void
test_query (GBookmarkFile *bookmark)
{
  gint size;
  gchar **uris;
  gsize uris_len, i;
  gchar *mime;
  GError *error;

  size = g_bookmark_file_get_size (bookmark);
  uris = g_bookmark_file_get_uris (bookmark, &uris_len);

  g_assert_cmpint (uris_len, ==, size);

  for (i = 0; i < uris_len; i++)
    {
      g_assert_true (g_bookmark_file_has_item (bookmark, uris[i]));
      error = NULL;
      mime = g_bookmark_file_get_mime_type (bookmark, uris[i], &error);
      g_assert_nonnull (mime);
      g_assert_no_error (error);
      g_free (mime);
    }
  g_strfreev (uris);

  g_assert_false (g_bookmark_file_has_item (bookmark, "file:///no/such/uri"));
  error = NULL;
  mime = g_bookmark_file_get_mime_type (bookmark, "file:///no/such/uri", &error);
  g_assert_null (mime);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_error_free (error);
  g_free (mime);
}

static gboolean
test_modify (GBookmarkFile *bookmark)
{
  gchar *text;
  guint count;
  GDateTime *stamp;
  GDateTime *now = NULL;
  GError *error = NULL;
  gchar **groups;
  gsize length;
  gchar **apps;
  gchar *icon;
  gchar *mime;

  if (g_test_verbose ())
    g_printerr ("\t=> check global title/description...");
  g_bookmark_file_set_title (bookmark, NULL, "a file");
  g_bookmark_file_set_description (bookmark, NULL, "a bookmark file");

  text = g_bookmark_file_get_title (bookmark, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "a file");
  g_free (text);

  text = g_bookmark_file_get_description (bookmark, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "a bookmark file");
  g_free (text);
  if (g_test_verbose ())
    g_printerr ("ok\n");

  if (g_test_verbose ())
    g_printerr ("\t=> check bookmark title/description...");
  g_bookmark_file_set_title (bookmark, TEST_URI_0, "a title");
  g_bookmark_file_set_description (bookmark, TEST_URI_0, "a description");
  g_bookmark_file_set_is_private (bookmark, TEST_URI_0, TRUE);
  now = g_date_time_new_now_utc ();
  g_bookmark_file_set_added_date_time (bookmark, TEST_URI_0, now);
  g_bookmark_file_set_visited_date_time (bookmark, TEST_URI_0, now);
  g_bookmark_file_set_icon (bookmark, TEST_URI_0, "testicon", "image/png");

  /* Check the modification date by itself, as it’s updated whenever we modify
   * other properties. */
  g_bookmark_file_set_modified_date_time (bookmark, TEST_URI_0, now);
  stamp = g_bookmark_file_get_modified_date_time (bookmark, TEST_URI_0, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_date_time_compare (stamp, now), ==, 0);

  text = g_bookmark_file_get_title (bookmark, TEST_URI_0, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "a title");
  g_free (text);
  text = g_bookmark_file_get_description (bookmark, TEST_URI_0, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "a description");
  g_free (text);
  g_assert_true (g_bookmark_file_get_is_private (bookmark, TEST_URI_0, &error));
  g_assert_no_error (error);
  stamp = g_bookmark_file_get_added_date_time (bookmark, TEST_URI_0, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_date_time_compare (stamp, now), ==, 0);
  stamp = g_bookmark_file_get_visited_date_time (bookmark, TEST_URI_0, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_date_time_compare (stamp, now), ==, 0);
  g_assert_true (g_bookmark_file_get_icon (bookmark, TEST_URI_0, &icon, &mime, &error));
  g_assert_no_error (error);
  g_assert_cmpstr (icon, ==, "testicon");
  g_assert_cmpstr (mime, ==, "image/png");
  g_free (icon);
  g_free (mime);
  if (g_test_verbose ())
    g_printerr ("ok\n");

  if (g_test_verbose ())
    g_printerr ("\t=> check non existing bookmark...");
  g_bookmark_file_get_description (bookmark, TEST_URI_1, &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  g_bookmark_file_get_is_private (bookmark, TEST_URI_1, &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  g_bookmark_file_get_added_date_time (bookmark, TEST_URI_1, &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  g_bookmark_file_get_modified_date_time (bookmark, TEST_URI_1, &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  g_bookmark_file_get_visited_date_time (bookmark, TEST_URI_1, &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  if (g_test_verbose ())
    g_printerr ("ok\n");

  if (g_test_verbose ())
    g_printerr ("\t=> check application...");
  g_bookmark_file_set_mime_type (bookmark, TEST_URI_0, TEST_MIME);
  g_assert_false (g_bookmark_file_has_application (bookmark, TEST_URI_0, TEST_APP_NAME, NULL));
  g_bookmark_file_add_application (bookmark, TEST_URI_0,
				   TEST_APP_NAME,
				   TEST_APP_EXEC);
  g_assert_true (g_bookmark_file_has_application (bookmark, TEST_URI_0, TEST_APP_NAME, NULL));
  g_bookmark_file_get_application_info (bookmark, TEST_URI_0, TEST_APP_NAME,
                                        &text,
                                        &count,
                                        &stamp,
                                        &error);
  g_assert_no_error (error);
  g_assert_cmpuint (count, ==, 1);
  g_assert_cmpint (g_date_time_compare (stamp, g_bookmark_file_get_modified_date_time (bookmark, TEST_URI_0, NULL)), <=, 0);
  g_free (text);
  g_assert_true (g_bookmark_file_remove_application (bookmark, TEST_URI_0, TEST_APP_NAME, &error));
  g_assert_no_error (error);
  g_bookmark_file_add_application (bookmark, TEST_URI_0, TEST_APP_NAME, TEST_APP_EXEC);
  apps = g_bookmark_file_get_applications (bookmark, TEST_URI_0, &length, &error);
  g_assert_no_error (error);
  g_assert_cmpint (length, ==, 1);
  g_assert_cmpstr (apps[0], ==, TEST_APP_NAME);
  g_strfreev (apps);

  g_bookmark_file_get_application_info (bookmark, TEST_URI_0, "fail",
                                        &text,
                                        &count,
                                        &stamp,
                                        &error);
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED);
  g_clear_error (&error);

  if (g_test_verbose ())
    g_printerr ("ok\n");

  if (g_test_verbose ())
    g_printerr ("\t=> check groups...");
  g_assert_false (g_bookmark_file_has_group (bookmark, TEST_URI_1, "Test", NULL));
  g_bookmark_file_add_group (bookmark, TEST_URI_1, "Test");
  g_assert_true (g_bookmark_file_has_group (bookmark, TEST_URI_1, "Test", NULL));
  g_assert_false (g_bookmark_file_has_group (bookmark, TEST_URI_1, "Fail", NULL));
  g_assert_true (g_bookmark_file_remove_group (bookmark, TEST_URI_1, "Test", &error));
  g_assert_no_error (error);
  groups = g_bookmark_file_get_groups (bookmark, TEST_URI_1, NULL, &error);
  g_assert_cmpint (g_strv_length (groups), ==, 0);
  g_strfreev (groups);
  groups = g_new0 (gchar *, 3);
  groups[0] = "Group1";
  groups[1] = "Group2";
  groups[2] = NULL;
  g_bookmark_file_set_groups (bookmark, TEST_URI_1, (const gchar **)groups, 2);
  g_free (groups);
  groups = g_bookmark_file_get_groups (bookmark, TEST_URI_1, &length, &error);
  g_assert_cmpint (length, ==, 2);
  g_strfreev (groups);
  g_assert_no_error (error);

  if (g_test_verbose ())
    g_printerr ("ok\n");

  if (g_test_verbose ())
    g_printerr ("\t=> check remove...");
  g_assert_true (g_bookmark_file_remove_item (bookmark, TEST_URI_1, &error));
  g_assert_no_error (error);
  g_assert_false (g_bookmark_file_remove_item (bookmark, TEST_URI_1, &error));
  g_assert_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND);
  g_clear_error (&error);
  if (g_test_verbose ())
    g_printerr ("ok\n");

  g_date_time_unref (now);

  return TRUE;
}

static void
test_file (gconstpointer d)
{
  const gchar *filename = d;
  GBookmarkFile *bookmark_file;
  gboolean success;
  gchar *data;
  GError *error;

  bookmark_file = g_bookmark_file_new ();
  g_assert_nonnull (bookmark_file);

  success = test_load (bookmark_file, filename);

  if (success)
    {
      test_query (bookmark_file);
      test_modify (bookmark_file);

      error = NULL;
      data = g_bookmark_file_to_data (bookmark_file, NULL, &error);
      g_assert_no_error (error);
      /* FIXME do some checks on data */
      g_free (data);
    }

  g_bookmark_file_free (bookmark_file);

  g_assert_true (success == (strstr (filename, "fail") == NULL));
}

static void
test_file_copy (gconstpointer d)
{
  const gchar *filename = d;
  GBookmarkFile *bookmark_file;
  GBookmarkFile *copy;
  gboolean success;
  gchar *data;
  gchar *copy_data;
  gsize length;
  gsize copy_length;
  GError *error = NULL;

  bookmark_file = g_bookmark_file_new ();
  g_assert_nonnull (bookmark_file);

  success = test_load (bookmark_file, filename);
  g_assert_true (success == (strstr (filename, "fail") == NULL));

  copy = g_bookmark_file_copy (bookmark_file);
  g_assert_nonnull (copy);

  if (g_str_has_suffix (filename, "fail-08.xbel") ||
      g_str_has_suffix (filename, "fail-06.xbel") ||
      g_str_has_suffix (filename, "fail-07.xbel") ||
      g_str_has_suffix (filename, "fail-09.xbel") ||
      g_str_has_suffix (filename, "fail-10.xbel") ||
      g_str_has_suffix (filename, "fail-11.xbel") ||
      g_str_has_suffix (filename, "fail-39.xbel"))
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                             "*no registered applications*skipping*");
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                             "*no registered applications*skipping*");
    }

  data = g_bookmark_file_to_data (bookmark_file, &length, &error);
  g_assert_no_error (error);

  copy_data = g_bookmark_file_to_data (copy, &copy_length, &error);
  g_assert_no_error (error);

  g_test_assert_expected_messages ();

  g_assert_cmpuint (length, ==, copy_length);
  g_assert_cmpstr (data, ==, copy_data);

  if (success)
    {
      GBookmarkFile *modified_copy;
      gchar *modified_data;
      gchar *modified_copy_data;
      gsize modified_length;
      gsize modified_copy_length;

      test_modify (bookmark_file);
      test_modify (copy);

      modified_data = g_bookmark_file_to_data (bookmark_file,
                                               &modified_length,
                                               &error);
      g_assert_no_error (error);

      modified_copy_data = g_bookmark_file_to_data (copy,
                                                    &modified_copy_length,
                                                    &error);
      g_assert_no_error (error);

      g_assert_cmpstr (data, !=, modified_data);
      g_assert_cmpstr (copy_data, !=, modified_copy_data);

      g_free (modified_copy_data);
      modified_copy = g_bookmark_file_copy (bookmark_file);
      modified_copy_data = g_bookmark_file_to_data (modified_copy,
                                                    &modified_copy_length,
                                                    &error);
      g_assert_no_error (error);

      g_assert_cmpuint (modified_length, ==, modified_copy_length);
      g_assert_cmpstr (modified_data, ==, modified_copy_data);

      g_free (modified_data);
      g_free (modified_copy_data);
      g_bookmark_file_free (modified_copy);
    }

  g_bookmark_file_free (bookmark_file);
  g_bookmark_file_free (copy);

  g_free (data);
  g_free (copy_data);
}

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error;
  const gchar *name;
  gchar *path;

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  if (argc > 1)
    {
      test_file (argv[1]);
      return 0;
    }

  g_test_add_func ("/bookmarks/load-from-data-dirs", test_load_from_data_dirs);
  g_test_add_func ("/bookmarks/to-file", test_to_file);
  g_test_add_func ("/bookmarks/move-item", test_move_item);
  g_test_add_func ("/bookmarks/corner-cases", test_corner_cases);
  g_test_add_func ("/bookmarks/misc", test_misc);
  g_test_add_func ("/bookmarks/deprecated", test_deprecated);

  error = NULL;
  path = g_test_build_filename (G_TEST_DIST, "bookmarks", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      gchar *filename;
      if (!g_str_has_suffix (name, ".xbel"))
        continue;

      filename = g_test_build_filename (G_TEST_DIST, "bookmarks", name, NULL);

      path = g_strdup_printf ("/bookmarks/parse/%s", name);
      g_test_add_data_func_full (path, filename, test_file, g_free);
      g_free (path);
      path = g_strdup_printf ("/bookmarks/copy/%s", name);
      g_test_add_data_func_full (path, g_strdup (filename), test_file_copy, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
