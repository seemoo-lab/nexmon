/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * licence, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>

#ifndef G_OS_UNIX
#error This is a Unix-specific test
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gunixmounts.h>
#include <fcntl.h>

#ifdef HAVE_COCOA
static gchar *
create_home_tmp_file (const gchar *tmpl)
{
  GFile *file = NULL;
  GFileIOStream *stream = NULL;
  GError *error = NULL;
  gchar *path;

  file = g_file_new_tmp (tmpl, &stream, &error);
  g_assert_no_error (error);
  path = g_strdup (g_file_peek_path (file));

  g_assert_true (g_io_stream_close (G_IO_STREAM (stream), NULL, &error));
  g_assert_no_error (error);

  g_object_unref (stream);
  g_object_unref (file);

  return path;
}

static void
remove_trashed_home_file (const gchar *basename)
{
  g_autofree gchar *trashed_path = NULL;

  trashed_path = g_build_filename (g_get_home_dir (), ".Trash", basename, NULL);
  g_remove (trashed_path);
}

static void
test_trash_macos_native (void)
{
  g_autofree gchar *filepath = NULL;
  g_autofree gchar *basename = NULL;
  g_autofree gchar *legacy_trash_path = NULL;
  g_autoptr (GFile) file = NULL;
  GError *error = NULL;

  filepath = create_home_tmp_file ("test-trash-macos-XXXXXX");
  basename = g_path_get_basename (filepath);
  legacy_trash_path = g_build_filename (g_get_user_data_dir (), "Trash", "files", basename, NULL);
  file = g_file_new_for_path (filepath);

  g_assert_true (g_file_trash (file, NULL, &error));
  g_assert_no_error (error);
  g_assert_false (g_file_test (filepath, G_FILE_TEST_EXISTS));
  g_assert_false (g_file_test (legacy_trash_path, G_FILE_TEST_EXISTS));

  remove_trashed_home_file (basename);
}
#endif

/* Test that g_file_trash() returns G_IO_ERROR_NOT_SUPPORTED for files on system mounts. */
static void
test_trash_not_supported (void)
{
#ifdef HAVE_COCOA
  g_test_skip ("This test covers the freedesktop trash implementation, not the macOS native backend");
  return;
#else
  GFile *file;
  GFileIOStream *stream;
  GUnixMountEntry *mount;
  GFileInfo *info;
  GError *error = NULL;
  gboolean ret;
  gchar *parent_dirname;
  GStatBuf parent_stat, home_stat;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/251");

  /* The test assumes that tmp file is located on system internal mount. */
  file = g_file_new_tmp ("test-trashXXXXXX", &stream, &error);
  parent_dirname = g_path_get_dirname (g_file_peek_path (file));
  g_assert_no_error (error);
  g_assert_cmpint (g_stat (parent_dirname, &parent_stat), ==, 0);
  g_test_message ("File: %s (parent st_dev: %" G_GUINT64_FORMAT ")",
                  g_file_peek_path (file), (guint64) parent_stat.st_dev);

  g_assert_cmpint (g_stat (g_get_home_dir (), &home_stat), ==, 0);
  g_test_message ("Home: %s (st_dev: %" G_GUINT64_FORMAT ")",
                  g_get_home_dir (), (guint64) home_stat.st_dev);

  if (parent_stat.st_dev == home_stat.st_dev)
    {
      g_test_skip ("The file has to be on another filesystem than the home trash to run this test");

      g_free (parent_dirname);
      g_object_unref (stream);
      g_object_unref (file);

      return;
    }

  mount = g_unix_mount_entry_for (g_file_peek_path (file), NULL);
  g_assert_true (mount == NULL || g_unix_mount_entry_is_system_internal (mount));
  g_test_message ("Mount: %s", (mount != NULL) ? g_unix_mount_entry_get_mount_path (mount) : "(null)");
  g_clear_pointer (&mount, g_unix_mount_entry_free);

  /* g_file_trash() shouldn't be supported on system internal mounts,
   * because those are not monitored by gvfsd-trash.
   */
  ret = g_file_trash (file, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_test_message ("Error: %s", error->message);
  g_assert_false (ret);
  g_clear_error (&error);

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            NULL,
                            &error);
  g_assert_no_error (error);

  g_assert_false (g_file_info_get_attribute_boolean (info,
                                                     G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH));

  g_io_stream_close (G_IO_STREAM (stream), NULL, &error);
  g_assert_no_error (error);

  g_free (parent_dirname);
  g_object_unref (info);
  g_object_unref (stream);
  g_object_unref (file);
#endif
}

/* Test that symlinks are properly expanded when looking for topdir (e.g. for trash folder). */
static void
test_trash_symlinks (void)
{
#ifdef HAVE_COCOA
  g_test_skip ("This test covers topdir lookup for the freedesktop trash implementation, which is not supported on macOS");
  return;
#else
  GFile *symlink;
  GUnixMountEntry *target_mount, *tmp_mount, *symlink_mount, *target_over_symlink_mount;
  gchar *target, *tmp, *target_over_symlink;
  GError *error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1522");

  target = g_build_filename (g_get_home_dir (), ".local", NULL);

  if (!g_file_test (target, G_FILE_TEST_IS_DIR))
    {
      g_test_skip_printf ("Directory '%s' does not exist", target);
      g_free (target);
      return;
    }

  target_mount = g_unix_mount_entry_for (target, NULL);

  if (target_mount == NULL)
    {
      g_test_skip_printf ("Unable to determine mount point for %s", target);
      g_free (target);
      return;
    }

  g_assert_nonnull (target_mount);
  g_test_message ("Target: %s (mount: %s)", target, g_unix_mount_entry_get_mount_path (target_mount));

  tmp = g_dir_make_tmp ("test-trashXXXXXX", &error);
  g_assert_no_error (error);
  g_assert_nonnull (tmp);
  tmp_mount = g_unix_mount_entry_for (tmp, NULL);

  if (tmp_mount == NULL)
    {
      g_test_skip_printf ("Unable to determine mount point for %s", tmp);
      g_unix_mount_entry_free (target_mount);
      g_free (target);
      g_free (tmp);
      return;
    }

  g_assert_nonnull (tmp_mount);
  g_test_message ("Tmp: %s (mount: %s)", tmp, g_unix_mount_entry_get_mount_path (tmp_mount));

  if (g_unix_mount_entry_compare (target_mount, tmp_mount) == 0)
    {
      g_test_skip ("The tmp has to be on another mount than the home to run this test");

      g_unix_mount_entry_free (tmp_mount);
      g_free (tmp);
      g_unix_mount_entry_free (target_mount);
      g_free (target);

      return;
    }

  symlink = g_file_new_build_filename (tmp, "symlink", NULL);
  g_file_make_symbolic_link (symlink, g_get_home_dir (), NULL, &error);
  g_assert_no_error (error);

  symlink_mount = g_unix_mount_entry_for (g_file_peek_path (symlink), NULL);
  g_assert_nonnull (symlink_mount);
  g_test_message ("Symlink: %s (mount: %s)", g_file_peek_path (symlink), g_unix_mount_entry_get_mount_path (symlink_mount));

  g_assert_cmpint (g_unix_mount_entry_compare (symlink_mount, tmp_mount), ==, 0);

  target_over_symlink = g_build_filename (g_file_peek_path (symlink),
                                          ".local",
                                          NULL);
  target_over_symlink_mount = g_unix_mount_entry_for (target_over_symlink, NULL);
  g_assert_nonnull (symlink_mount);
  g_test_message ("Target over symlink: %s (mount: %s)", target_over_symlink, g_unix_mount_entry_get_mount_path (target_over_symlink_mount));

  g_assert_cmpint (g_unix_mount_entry_compare (target_over_symlink_mount, target_mount), ==, 0);

  g_unix_mount_entry_free (target_over_symlink_mount);
  g_unix_mount_entry_free (symlink_mount);
  g_free (target_over_symlink);
  g_object_unref (symlink);
  g_unix_mount_entry_free (tmp_mount);
  g_free (tmp);
  g_unix_mount_entry_free (target_mount);
  g_free (target);
#endif
}

/* Test that long filename are handled correctly */
static void
test_trash_long_filename (void)
{
  const gchar *long_filename = "test_trash_long_filename_aaaaaaaaaaaaaaaaaaaaaaaaa" \
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
    "aaaaa"; /* 255 bytes */
  gchar *filepath;
  int fd;
  GFile *file;
  GError *error = NULL;

  /* The test assumes that test file is located on ext fs. */
  filepath = g_build_filename (g_get_home_dir (), long_filename, NULL);
  fd = g_open (filepath, O_CREAT | O_RDONLY, 0666);
  if (fd == -1)
    {
      g_test_skip ("Failed to create test file");
      g_free (filepath);
      return;
    }
  (void) g_close (fd, NULL);
  file = g_file_new_for_path (filepath);
  g_file_trash (file, NULL, &error);
  g_unlink (filepath);
  g_assert_no_error (error);

  /* Delete trashed version of test file */
#ifdef HAVE_COCOA
  {
    g_autofree gchar *basename = g_path_get_basename (filepath);
    remove_trashed_home_file (basename);
  }
#else
  {
    GFileEnumerator *enumerator;
    GFile *trash;

    trash = g_file_new_for_uri ("trash:///");
    enumerator = g_file_enumerate_children (trash,
                                            G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                            G_FILE_ATTRIBUTE_TRASH_ORIG_PATH,
                                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                            NULL, NULL);

    if (enumerator)
      {
        GFileInfo *info;

        while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL)
          {
            const char *origpath = g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);

            if (strcmp (filepath, origpath) == 0)
              {
                GFile *item = g_file_get_child (trash, g_file_info_get_name (info));
                g_file_delete (item, NULL, NULL);
                g_object_unref (item);
                g_object_unref (info);
                break;
              }

            g_object_unref (info);
          }

        g_file_enumerator_close (enumerator, NULL, NULL);
        g_object_unref (enumerator);
      }
    g_object_unref (trash);
  }
#endif

  g_free (filepath);
  g_object_unref (file);
  g_clear_error (&error);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#ifdef HAVE_COCOA
  g_test_add_func ("/trash/macos/native", test_trash_macos_native);
#endif
  g_test_add_func ("/trash/not-supported", test_trash_not_supported);
  g_test_add_func ("/trash/symlinks", test_trash_symlinks);
  g_test_add_func ("/trash/long-filename", test_trash_long_filename);

  return g_test_run ();
}
