/* Unit tests for gdocumentportal
 * GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2025 Marco Trevisan
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

#include "gdbus-sessionbus.h"
#include "fake-document-portal.h"
#include "gdocumentportal.h"

/* Returns TRUE and calls g_test_skip() if g_unix_fd_query_path() is not
 * supported on this platform (e.g. Hurd). Tests that depend on the fake
 * document portal being able to resolve fd paths should call this and
 * return early if it returns TRUE. */
static gboolean
skip_if_fd_query_path_unsupported (void)
{
#ifdef __GNU__
  g_test_skip ("g_unix_fd_query_path() not supported on Hurd");
  return TRUE;
#else
  return FALSE;
#endif
}

static void
test_document_portal_add_uri (void)
{
  if (skip_if_fd_query_path_unsupported ())
    return;

  GFakeDocumentPortalThread *thread = NULL;
  GFile *file;
  GList *uris = NULL;  /* (element-type utf8) */
  GList *portal_uris = NULL;  /* (element-type utf8) */
  GFileIOStream *iostream = NULL;
  GError *error = NULL;
  const char *app_id;
  const char *document_portal_mount_point;
  char *basename;
  char *expected_uri;

  /* Run a fake-document-portal */
  app_id = "org.gnome.glib.gio";
  thread = g_fake_document_portal_thread_new (session_bus_get_address (), app_id);
  g_fake_document_portal_thread_run (thread);
  document_portal_mount_point = g_fake_document_portal_thread_get_mount_point (thread);

  file = g_file_new_tmp ("test_document_portal_add_uri_XXXXXX",
                         &iostream, NULL);
  g_assert_no_error (error);
  g_io_stream_close ((GIOStream *) iostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (iostream);

  uris = g_list_append (uris, g_file_get_uri (file));
  portal_uris = g_document_portal_add_documents (uris, app_id, &error);
  g_assert_no_error (error);

  basename = g_file_get_basename (file);
  expected_uri = g_strdup_printf ("file:%s/document-id-0/%s",
                                   document_portal_mount_point, basename);

  g_assert_cmpuint (g_list_length (portal_uris), ==, 1);
  g_assert_cmpstr (g_list_nth_data (portal_uris, 0), ==, expected_uri);

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  g_clear_object (&file);
  g_clear_pointer (&expected_uri, g_free);
  g_clear_pointer (&basename, g_free);
  g_clear_list (&uris, g_free);
  g_clear_list (&portal_uris, g_free);
}

static void
test_document_portal_add_not_existent_uri (void)
{
  GFakeDocumentPortalThread *thread = NULL;
  GList *uris = NULL;  /* (element-type const char*) */
  GList *portal_uris = NULL;  /* (element-type utf8) */
  GError *error = NULL;
  const char *app_id;
  const char *document_portal_mount_point;
  const char *uri;
  char *portal_uri;

  /* Run a fake-document-portal */
  app_id = "org.gnome.glib.gio.not-existent-uri";
  thread = g_fake_document_portal_thread_new (session_bus_get_address (), app_id);
  g_fake_document_portal_thread_run (thread);
  document_portal_mount_point = g_fake_document_portal_thread_get_mount_point (thread);

  uri = "file:/no-existent-path-really!";
  uris = g_list_append (uris, (char *) uri);
  portal_uris = g_document_portal_add_documents (uris, app_id, &error);
  g_assert_no_error (error);

  portal_uri = g_strdup_printf ("file:%s/document-id-0/no-existent-path-really!",
                                document_portal_mount_point);
  g_assert_false (g_file_test (portal_uri, G_FILE_TEST_EXISTS));

  g_assert_cmpuint (g_list_length (portal_uris), ==, 1);
  g_assert_cmpstr (g_list_nth_data (portal_uris, 0), ==, uri);

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  g_clear_pointer (&portal_uri, g_free);
  g_clear_list (&uris, NULL);
  g_clear_list (&portal_uris, g_free);
}

static void
test_document_portal_add_existent_and_not_existent_uris (void)
{
  if (skip_if_fd_query_path_unsupported ())
    return;

  GFakeDocumentPortalThread *thread = NULL;
  GFile *file;
  GFile *expected_file0;
  GFile *expected_file1;
  GList *uris = NULL;  /* (element-type utf8) */
  GList *portal_uris = NULL;  /* (element-type utf8) */
  GFileIOStream *iostream = NULL;
  GError *error = NULL;
  const char *app_id;
  const char *document_portal_mount_point;
  const char *invalid_uri;
  char *basename;

  /* Run a fake-document-portal */
  app_id = "org.gnome.glib.gio.mixed-uris";
  thread = g_fake_document_portal_thread_new (session_bus_get_address (), app_id);
  g_fake_document_portal_thread_run (thread);
  document_portal_mount_point = g_fake_document_portal_thread_get_mount_point (thread);

  file = g_file_new_tmp ("test_document_portal_add_existent_and_not_existent_uris_XXXXXX",
                         &iostream, NULL);
  g_assert_no_error (error);
  g_io_stream_close ((GIOStream *) iostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (iostream);

  invalid_uri = "file:/no-existent-path-really!";

  uris = g_list_append (uris, g_file_get_uri (file));
  uris = g_list_append (uris, g_strdup (invalid_uri));
  uris = g_list_append (uris, g_file_get_uri (file));
  uris = g_list_append (uris, g_strdup (invalid_uri));

  portal_uris = g_document_portal_add_documents (uris, app_id, &error);
  g_assert_no_error (error);

  basename = g_file_get_basename (file);
  expected_file0 = g_file_new_build_filename (document_portal_mount_point,
                                              "document-id-0", basename, NULL);
  expected_file1 = g_file_new_build_filename (document_portal_mount_point,
                                              "document-id-1", basename, NULL);

  g_assert_cmpuint (g_list_length (portal_uris), ==, 4);
  g_assert_cmpstr ((char * ) g_list_nth_data (portal_uris, 0) + strlen ("file:"), ==, g_file_peek_path (expected_file0));
  g_assert_cmpstr (g_list_nth_data (portal_uris, 1), ==, invalid_uri);
  g_assert_cmpstr ((char * ) g_list_nth_data (portal_uris, 2) + strlen ("file:"), ==, g_file_peek_path (expected_file1));
  g_assert_cmpstr (g_list_nth_data (portal_uris, 3), ==, invalid_uri);

  g_assert_true (g_file_test (g_file_peek_path (expected_file0), G_FILE_TEST_IS_REGULAR));
  g_assert_true (g_file_test (g_file_peek_path (expected_file1), G_FILE_TEST_IS_REGULAR));

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  g_clear_object (&file);
  g_clear_object (&expected_file1);
  g_clear_object (&expected_file0);
  g_clear_pointer (&basename, g_free);
  g_clear_list (&uris, g_free);
  g_clear_list (&portal_uris, g_free);
}

static void
test_document_portal_add_symlink_uri (void)
{
  if (skip_if_fd_query_path_unsupported ())
    return;

  GFakeDocumentPortalThread *thread = NULL;
  GFile *target;
  GFile *link1;
  GFile *link2;
  GFile *parent_dir;
  GList *uris = NULL;  /* (element-type utf8) */
  GList *portal_uris = NULL;  /* (element-type utf8) */
  GFileIOStream *iostream = NULL;
  GError *error = NULL;
  const char *app_id;
  const char *document_portal_mount_point;
  char *tmpdir_path;
  char *basename;
  char *expected_uri;

  /* Run a fake-document-portal */
  app_id = "org.gnome.glib.gio.symlinks";
  thread = g_fake_document_portal_thread_new (session_bus_get_address (), app_id);
  g_fake_document_portal_thread_run (thread);
  document_portal_mount_point = g_fake_document_portal_thread_get_mount_point (thread);

  target = g_file_new_tmp ("test_document_portal_add_symlink_uri_XXXXXX",
                           &iostream, NULL);
  g_assert_no_error (error);
  g_io_stream_close ((GIOStream *) iostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (iostream);

  tmpdir_path = g_dir_make_tmp ("g_file_symlink_XXXXXX", &error);
  g_assert_no_error (error);

  parent_dir = g_file_new_for_path (tmpdir_path);
  g_assert_true (g_file_query_exists (parent_dir, NULL));

  link1 = g_file_get_child (parent_dir, "symlink");
  g_assert_false (g_file_query_exists (link1, NULL));

  g_file_make_symbolic_link (link1, g_file_peek_path (target), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_file_query_exists (link1, NULL));

  link2 = g_file_get_child (parent_dir, "symlink-of-symlink");
  g_assert_false (g_file_query_exists (link2, NULL));

  basename = g_file_get_basename (link1);
  g_file_make_symbolic_link (link2, basename, NULL, &error);
  g_clear_pointer (&basename, g_free);
  g_assert_true (g_file_query_exists (link2, NULL));
  g_assert_no_error (error);

  uris = g_list_append (uris, g_file_get_uri (link1));
  uris = g_list_append (uris, g_file_get_uri (link2));

  portal_uris = g_document_portal_add_documents (uris, app_id, &error);
  g_assert_no_error (error);

  basename = g_file_get_basename (target);
  g_assert_cmpuint (g_list_length (portal_uris), ==, 2);

  expected_uri = g_strdup_printf ("file:%s/document-id-0/%s",
                                   document_portal_mount_point, basename);
  g_assert_cmpstr (g_list_nth_data (portal_uris, 0), ==, expected_uri);
  g_clear_pointer (&expected_uri, g_free);

  expected_uri = g_strdup_printf ("file:%s/document-id-1/%s",
                                   document_portal_mount_point, basename);
  g_assert_cmpstr (g_list_nth_data (portal_uris, 1), ==, expected_uri);
  g_clear_pointer (&expected_uri, g_free);

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  g_clear_object (&target);
  g_clear_object (&link1);
  g_clear_object (&link2);
  g_clear_object (&parent_dir);
  g_clear_pointer (&expected_uri, g_free);
  g_clear_pointer (&basename, g_free);
  g_clear_pointer (&tmpdir_path, g_free);
  g_clear_list (&uris, g_free);
  g_clear_list (&portal_uris, g_free);
}

static void
test_document_portal_add_uri_with_missing_doc_id_path (void)
{
  GFakeDocumentPortalThread *thread = NULL;
  GFile *file;
  GList *uris = NULL;  /* (element-type utf8) */
  GList *portal_uris = NULL;  /* (element-type utf8) */
  GFileIOStream *iostream = NULL;
  GError *error = NULL;
  const char *app_id;

  /* Run a fake-document-portal */
  app_id = G_FAKE_DOCUMENT_PORTAL_NO_CREATE_DIR_APP_ID;
  thread = g_fake_document_portal_thread_new (session_bus_get_address (), app_id);
  g_fake_document_portal_thread_run (thread);

  file = g_file_new_tmp ("test_document_portal_add_uri_with_missing_doc_id_path_XXXXXX",
                         &iostream, NULL);
  g_assert_no_error (error);
  g_io_stream_close ((GIOStream *) iostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (iostream);

  uris = g_list_append (uris, g_file_get_uri (file));
  portal_uris = g_document_portal_add_documents (uris, app_id, &error);
  g_assert_null (portal_uris);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  g_clear_object (&file);
  g_clear_list (&uris, g_free);
  g_clear_error (&error);
  g_clear_list (&portal_uris, g_free);
}

static void
test_document_portal_add_uri_with_missing_doc_file (void)
{
  GFakeDocumentPortalThread *thread = NULL;
  GFile *file;
  GList *uris = NULL;  /* (element-type utf8) */
  GList *portal_uris = NULL;  /* (element-type utf8) */
  GFileIOStream *iostream = NULL;
  GError *error = NULL;
  const char *app_id;

  /* Run a fake-document-portal */
  app_id = G_FAKE_DOCUMENT_PORTAL_NO_CREATE_FILE_APP_ID;
  thread = g_fake_document_portal_thread_new (session_bus_get_address (), app_id);
  g_fake_document_portal_thread_run (thread);

  file = g_file_new_tmp ("test_document_portal_add_uri_with_missing_doc_file_XXXXXX",
                         &iostream, NULL);
  g_assert_no_error (error);
  g_io_stream_close ((GIOStream *) iostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (iostream);

  uris = g_list_append (uris, g_file_get_uri (file));
  portal_uris = g_document_portal_add_documents (uris, app_id, &error);
  g_assert_null (portal_uris);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  g_clear_object (&file);
  g_clear_list (&uris, g_free);
  g_clear_error (&error);
  g_clear_list (&portal_uris, g_free);
}

int
main (int   argc,
      char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/document-portal/add-uri", test_document_portal_add_uri);
  g_test_add_func ("/document-portal/add-not-existent-uri", test_document_portal_add_not_existent_uri);
  g_test_add_func ("/document-portal/add-existent-and-not-existent-uri", test_document_portal_add_existent_and_not_existent_uris);
  g_test_add_func ("/document-portal/add-symlink-uri", test_document_portal_add_symlink_uri);
  g_test_add_func ("/document-portal/add-uri-with-missing-doc-id-path", test_document_portal_add_uri_with_missing_doc_id_path);
  g_test_add_func ("/document-portal/add-uri-with-missing-doc-file", test_document_portal_add_uri_with_missing_doc_file);

  return session_bus_run ();
}
