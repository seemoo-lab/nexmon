/* Unit tests for GFile thumbnails
 * GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2022 Marco Trevisan
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
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gio/gio.h>

#define THUMBNAIL_FAIL_SIZE "fail"

#define THUMBNAILS_ATTRIBS ( \
  G_FILE_ATTRIBUTE_THUMBNAIL_PATH "," \
  G_FILE_ATTRIBUTE_THUMBNAILING_FAILED "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_PATH_NORMAL "," \
  G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_NORMAL "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_NORMAL "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_PATH_LARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_LARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_LARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_PATH_XLARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_XLARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XLARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_PATH_XXLARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_XXLARGE "," \
  G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XXLARGE "," \
)

/* Must be kept in order, for priority */
static const char * SIZES_NAMES[] = {
  "normal",
  "large",
  "x-large",
  "xx-large",
};

static GFile *
get_thumbnail_src_file (const gchar *name)
{
  const gchar *thumbnail_path;
  thumbnail_path = g_test_get_filename (G_TEST_DIST, "thumbnails",
                                        name, NULL);

  g_assert_true (g_file_test (thumbnail_path, G_FILE_TEST_IS_REGULAR));

  return g_file_new_for_path (thumbnail_path);
}

static gchar *
get_thumbnail_basename (GFile *source)
{
  GChecksum *checksum;
  gchar *uri = g_file_get_uri (source);
  gchar *basename;

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  basename = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);

  g_checksum_free (checksum);
  g_free (uri);

  return basename;
}

static GFile *
get_expected_thumbnail_file (GFile       *source,
                             const gchar *size)
{
  GFile *file;
  gchar *basename;

  basename = get_thumbnail_basename (source);
  file = g_file_new_build_filename (g_get_user_cache_dir (),
                                    "thumbnails",
                                    size,
                                    basename,
                                    NULL);
  g_free (basename);
  return file;
}

static GFile *
get_failed_thumbnail_file (GFile *source)
{
  GFile *file;
  gchar *basename;

  basename = get_thumbnail_basename (source);
  file = g_file_new_build_filename (g_get_user_cache_dir (),
                                    "thumbnails", THUMBNAIL_FAIL_SIZE,
                                    "gnome-thumbnail-factory",
                                    basename,
                                    NULL);
  g_free (basename);
  return file;
}

static gboolean
check_thumbnail_exists (GFile       *source,
                         const gchar *size)
{
  GFile *thumbnail;
  gboolean ret;

  thumbnail = get_expected_thumbnail_file (source, size);
  g_assert_nonnull (thumbnail);

  ret = g_file_query_exists (thumbnail, NULL);
  g_clear_object (&thumbnail);

  return ret;
}

static gboolean
check_failed_thumbnail_exists (GFile *source)
{
  GFile *thumbnail;
  gboolean ret;

  thumbnail = get_failed_thumbnail_file (source);
  g_assert_nonnull (thumbnail);

  ret = g_file_query_exists (thumbnail, NULL);
  g_clear_object (&thumbnail);

  return ret;
}

static GFile *
create_thumbnail (GFile       *source,
                  const gchar *size)
{
  GFile *thumbnail;
  GFile *thumbnail_dir;
  GError *error = NULL;
  gchar *thumbnail_path;

  /* TODO: This is just a stub implementation to create a fake thumbnail file
   * We should implement a real thumbnail generator, but we don't care here.
   */

  if (!size || g_strcmp0 (size, THUMBNAIL_FAIL_SIZE) == 0)
    thumbnail = get_failed_thumbnail_file (source);
  else
    thumbnail = get_expected_thumbnail_file (source, size);

  thumbnail_dir = g_file_get_parent (thumbnail);

  if (!g_file_query_exists (thumbnail_dir, NULL))
    {
      g_file_make_directory_with_parents (thumbnail_dir, NULL, &error);
      g_assert_no_error (error);
      g_clear_error (&error);
    }

  g_file_copy (source, thumbnail, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);
  g_assert_no_error (error);

  g_assert_true (g_file_query_exists (thumbnail, NULL));
  thumbnail_path = g_file_get_path (thumbnail);
  g_test_message ("Created test thumbnail at %s", thumbnail_path);

  g_clear_object (&thumbnail_dir);
  g_clear_error (&error);
  g_free (thumbnail_path);

  return thumbnail;
}

static GFile *
create_thumbnail_from_test_file (const gchar  *source_name,
                                 const gchar  *size,
                                 GFile       **out_source)
{
  GFile *thumbnail;
  GFile *source = get_thumbnail_src_file (source_name);

  thumbnail = create_thumbnail (source, size);

  if (!size || g_strcmp0 (size, THUMBNAIL_FAIL_SIZE) == 0)
    {
      g_assert_true (check_failed_thumbnail_exists (source));
    }
  else
    {
      g_assert_false (check_failed_thumbnail_exists (source));
      g_assert_true (check_thumbnail_exists (source, size));
    }

  if (out_source)
    *out_source = g_steal_pointer (&source);

  g_clear_object (&source);

  return thumbnail;
}

static gboolean
get_size_attributes (const char   *size,
                     const gchar **path,
                     const gchar **is_valid,
                     const gchar **failed)
{
  if (g_str_equal (size, "normal"))
    {
      *path = G_FILE_ATTRIBUTE_THUMBNAIL_PATH_NORMAL;
      *is_valid = G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_NORMAL;
      *failed = G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_NORMAL;
      return TRUE;
    }
  else if (g_str_equal (size, "large"))
    {
      *path = G_FILE_ATTRIBUTE_THUMBNAIL_PATH_LARGE;
      *is_valid = G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_LARGE;
      *failed = G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_LARGE;
      return TRUE;
    }
  else if (g_str_equal (size, "x-large"))
    {
      *path = G_FILE_ATTRIBUTE_THUMBNAIL_PATH_XLARGE;
      *is_valid = G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XLARGE;
      *failed = G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_XLARGE;
      return TRUE;
    }
  else if (g_str_equal (size, "xx-large"))
    {
      *path = G_FILE_ATTRIBUTE_THUMBNAIL_PATH_XXLARGE;
      *is_valid = G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XXLARGE;
      *failed = G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_XXLARGE;
      return TRUE;
    }

  *path = NULL;
  *is_valid = NULL;
  *failed = NULL;

  return FALSE;
}

static void
test_valid_thumbnail_size (gconstpointer data)
{
  GFile *source;
  GFile *thumbnail;
  GFile *f;
  GError *error = NULL;
  GFileInfo *info;
  const gchar *size = data;
  const gchar *path_attr, *failed_attr, *is_valid_attr;

  thumbnail = create_thumbnail_from_test_file ("valid.png", size, &source);
  info = g_file_query_info (source, THUMBNAILS_ATTRIBS, G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  g_assert_no_error (error);

  g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
  g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
  g_assert_cmpstr (
    g_file_peek_path (f),
    ==,
    g_file_peek_path (thumbnail)
  );
  g_clear_object (&f);

  /* TODO: We can't really test this without having a proper thumbnail created
  g_assert_true (
     g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  */

  g_assert_true (get_size_attributes (size, &path_attr, &is_valid_attr, &failed_attr));

  g_assert_true (g_file_info_has_attribute (info, path_attr));
  g_assert_true (g_file_info_has_attribute (info, is_valid_attr));
  g_assert_false (g_file_info_has_attribute (info, failed_attr));

  f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, path_attr));
  g_assert_cmpstr (
    g_file_info_get_attribute_byte_string (info, path_attr),
    ==,
    g_file_peek_path (thumbnail)
  );
  g_clear_object (&f);

  /* TODO: We can't really test this without having a proper thumbnail created
  g_assert_true (g_file_info_get_attribute_boolean (info, is_valid_attr));
  */

  g_clear_object (&source);
  g_clear_object (&thumbnail);
  g_clear_error (&error);
  g_clear_object (&info);
  g_clear_object (&f);
}

static void
test_unknown_thumbnail_size (gconstpointer data)
{
  GFile *source;
  GFile *thumbnail;
  GError *error = NULL;
  GFileInfo *info;
  const gchar *size = data;

  thumbnail = create_thumbnail_from_test_file ("valid.png", size, &source);
  info = g_file_query_info (source, THUMBNAILS_ATTRIBS, G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  g_assert_no_error (error);

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH_NORMAL));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_NORMAL));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_NORMAL));

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH_LARGE));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_LARGE));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_LARGE));

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH_XLARGE));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XLARGE));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED_XLARGE));

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH_XXLARGE));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XXLARGE));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID_XXLARGE));

  g_clear_object (&source);
  g_clear_object (&thumbnail);
  g_clear_error (&error);
  g_clear_object (&info);
}

static void
test_failed_thumbnail (void)
{
  GFile *source;
  GFile *thumbnail;
  GError *error = NULL;
  GFileInfo *info;

  thumbnail = create_thumbnail_from_test_file ("valid.png", NULL, &source);
  info = g_file_query_info (source, THUMBNAILS_ATTRIBS, G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  g_assert_no_error (error);

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
  g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  g_assert_false (
     g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_true (
     g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  g_clear_object (&source);
  g_clear_object (&thumbnail);
  g_clear_error (&error);
  g_clear_object (&info);
}

static void
test_thumbnails_size_priority (void)
{
  GPtrArray *sized_thumbnails;
  GError *error = NULL;
  GFileInfo *info;
  GFile *source;
  GFile *failed_thumbnail;
  gsize i;

  failed_thumbnail = create_thumbnail_from_test_file ("valid.png", NULL, &source);
  sized_thumbnails = g_ptr_array_new_with_free_func (g_object_unref);

  /* Checking that each thumbnail with higher priority override the previous */
  for (i = 0; i < G_N_ELEMENTS (SIZES_NAMES); i++)
    {
      GFile *thumbnail = create_thumbnail (source, SIZES_NAMES[i]);
      const gchar *path_attr, *failed_attr, *is_valid_attr;
      GFile *f;

      g_ptr_array_add (sized_thumbnails, thumbnail);

      info = g_file_query_info (source, THUMBNAILS_ATTRIBS,
                               G_FILE_QUERY_INFO_NONE, NULL, &error);
      g_assert_no_error (error);

      g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
      g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
      g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

      f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
      g_assert_cmpstr (
        g_file_peek_path (f),
        ==,
        g_file_peek_path (thumbnail)
      );
      g_clear_object (&f);

      g_assert_true (get_size_attributes (SIZES_NAMES[i],
                     &path_attr, &is_valid_attr, &failed_attr));

      g_assert_true (g_file_info_has_attribute (info, path_attr));
      g_assert_true (g_file_info_has_attribute (info, is_valid_attr));
      g_assert_false (g_file_info_has_attribute (info, failed_attr));

      f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, path_attr));
      g_assert_cmpstr (
        g_file_peek_path (f),
        ==,
        g_file_peek_path (thumbnail)
      );

      g_clear_object (&info);
      g_clear_object (&f);
    }

  g_assert_cmpuint (sized_thumbnails->len, ==, G_N_ELEMENTS (SIZES_NAMES));

  /* Ensuring we can access to all the thumbnails by explicit size request */
  for (i = 0; i < G_N_ELEMENTS (SIZES_NAMES); i++)
    {
      GFile *thumbnail = g_ptr_array_index (sized_thumbnails, i);
      const gchar *path_attr, *failed_attr, *is_valid_attr;
      GFile *f;

      info = g_file_query_info (source, THUMBNAILS_ATTRIBS,
                                G_FILE_QUERY_INFO_NONE, NULL, &error);
      g_assert_no_error (error);

      g_assert_true (get_size_attributes (SIZES_NAMES[i],
                     &path_attr, &is_valid_attr, &failed_attr));

      g_assert_true (g_file_info_has_attribute (info, path_attr));
      g_assert_true (g_file_info_has_attribute (info, is_valid_attr));
      g_assert_false (g_file_info_has_attribute (info, failed_attr));

      f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, path_attr));
      g_assert_cmpstr (
        g_file_peek_path (f),
        ==,
        g_file_peek_path (thumbnail)
      );
      g_clear_object (&f);

      g_clear_object (&info);
    }

  /* Now removing them in the inverse order, to check this again */
  for (i = G_N_ELEMENTS (SIZES_NAMES); i > 1; i--)
    {
      GFile *thumbnail = g_ptr_array_index (sized_thumbnails, i - 1);
      GFile *less_priority_thumbnail = g_ptr_array_index (sized_thumbnails, i - 2);
      const gchar *path_attr, *failed_attr, *is_valid_attr;
      GFile *f;

      g_file_delete (thumbnail, NULL, &error);
      g_assert_no_error (error);

      info = g_file_query_info (source, THUMBNAILS_ATTRIBS,
                                G_FILE_QUERY_INFO_NONE, NULL, &error);
      g_assert_no_error (error);

      g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
      g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
      g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

      f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
      g_assert_cmpstr (
        g_file_peek_path (f),
        ==,
        g_file_peek_path (less_priority_thumbnail)
      );
      g_clear_object (&f);

      g_assert_true (get_size_attributes (SIZES_NAMES[i-2],
                     &path_attr, &is_valid_attr, &failed_attr));

      g_assert_true (g_file_info_has_attribute (info, path_attr));
      g_assert_true (g_file_info_has_attribute (info, is_valid_attr));
      g_assert_false (g_file_info_has_attribute (info, failed_attr));

      f = g_file_new_for_path (g_file_info_get_attribute_byte_string (info, path_attr));
      g_assert_cmpstr (
        g_file_peek_path (f),
        ==,
        g_file_peek_path (less_priority_thumbnail)
      );

      g_clear_object (&info);
      g_clear_object (&f);
    }

  /* And now let's remove the last valid one, so that failed should have priority */
  g_file_delete (G_FILE (g_ptr_array_index (sized_thumbnails, 0)), NULL, &error);
  g_assert_no_error (error);

  info = g_file_query_info (source, THUMBNAILS_ATTRIBS, G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  g_assert_no_error (error);

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
  g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_true (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  g_assert_false (
     g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_true (
     g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  g_clear_object (&info);

  /* And check if we get the failed state for all explicit requests */
  for (i = 0; i < G_N_ELEMENTS (SIZES_NAMES); i++)
    {
      const gchar *path_attr, *failed_attr, *is_valid_attr;

      info = g_file_query_info (source, THUMBNAILS_ATTRIBS,
                                G_FILE_QUERY_INFO_NONE, NULL, &error);
      g_assert_no_error (error);

      g_assert_true (get_size_attributes (SIZES_NAMES[i],
                     &path_attr, &is_valid_attr, &failed_attr));

      g_assert_false (g_file_info_has_attribute (info, path_attr));
      g_assert_true (g_file_info_has_attribute (info, is_valid_attr));
      g_assert_true (g_file_info_has_attribute (info, failed_attr));

      g_assert_false (g_file_info_get_attribute_boolean (info, is_valid_attr));
      g_assert_true (g_file_info_get_attribute_boolean (info, failed_attr));

      g_clear_object (&info);
    }

  /* Removing the failed thumbnail too, so no thumbnail should be available */
  g_file_delete (failed_thumbnail, NULL, &error);
  g_assert_no_error (error);

  info = g_file_query_info (source, THUMBNAILS_ATTRIBS, G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  g_assert_no_error (error);

  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID));
  g_assert_false (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED));

  g_clear_object (&info);

  for (i = 0; i < G_N_ELEMENTS (SIZES_NAMES); i++)
    {
      const gchar *path_attr, *failed_attr, *is_valid_attr;

      info = g_file_query_info (source, THUMBNAILS_ATTRIBS,
                                G_FILE_QUERY_INFO_NONE, NULL, &error);
      g_assert_no_error (error);

      g_assert_true (get_size_attributes (SIZES_NAMES[i],
                     &path_attr, &is_valid_attr, &failed_attr));

      g_assert_false (g_file_info_has_attribute (info, path_attr));
      g_assert_false (g_file_info_has_attribute (info, is_valid_attr));
      g_assert_false (g_file_info_has_attribute (info, failed_attr));

      g_clear_object (&info);
    }

  g_clear_object (&source);
  g_clear_pointer (&sized_thumbnails, g_ptr_array_unref);
  g_clear_object (&failed_thumbnail);
  g_clear_error (&error);
  g_clear_object (&info);
}


int
main (int   argc,
      char *argv[])
{
  gsize i;

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  for (i = 0; i < G_N_ELEMENTS (SIZES_NAMES); i++)
    {
      gchar *test_path;

      test_path = g_strconcat ("/file-thumbnail/valid/", SIZES_NAMES[i], NULL);
      g_test_add_data_func (test_path, SIZES_NAMES[i], test_valid_thumbnail_size);
      g_free (test_path);
    }

  g_test_add_data_func ("/file-thumbnail/unknown/super-large", "super-large", test_unknown_thumbnail_size);
  g_test_add_func ("/file-thumbnail/fail", test_failed_thumbnail);
  g_test_add_func ("/file-thumbnail/size-priority", test_thumbnails_size_priority);

  return g_test_run ();
}
