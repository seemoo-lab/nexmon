/*
 * Copyright 2024 GNOME Foundation
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
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>

#include "girepository.h"
#include "test-common.h"

/* Dummy GTypes which can be introspected by the dumper. */

/* Dummy object type with no properties or signals. */
struct _TestObject
{
  GObject parent_instance;
};

#define TEST_TYPE_OBJECT test_object_get_type ()
G_MODULE_EXPORT G_DECLARE_FINAL_TYPE (TestObject, test_object, TEST, OBJECT, GObject)
G_DEFINE_FINAL_TYPE (TestObject, test_object, G_TYPE_OBJECT)

static void
test_object_class_init (TestObjectClass *klass)
{
}

static void
test_object_init (TestObject *self)
{
}

/* Dummy interface type with no properties or signals. */
struct _TestInterfaceInterface
{
  GTypeInterface g_iface;
};

#define TEST_TYPE_INTERFACE test_interface_get_type ()
G_MODULE_EXPORT G_DECLARE_INTERFACE (TestInterface, test_interface, TEST, INTERFACE, GObject)
G_DEFINE_INTERFACE (TestInterface, test_interface, G_TYPE_OBJECT)

static void
test_interface_default_init (TestInterfaceInterface *iface)
{
}

/* Test functions */
static void
assert_dump (const char *input,
             const char *expected_output)
{
  int fd = -1;
  char *in_file_path = NULL;
  char *out_file_path = NULL;
  char *output = NULL;
  gboolean retval;
  GError *local_error = NULL;

  fd = g_file_open_tmp ("dump_XXXXXX", &in_file_path, NULL);
  g_assert_cmpint (fd, >=, 0);
  g_assert_true (g_close (fd, NULL));

  out_file_path = g_strconcat (in_file_path, ".out", NULL);

  g_file_set_contents (in_file_path, input, -1, &local_error);
  g_assert_no_error (local_error);

  retval = gi_repository_dump (in_file_path, out_file_path, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (retval);

  g_file_get_contents (out_file_path, &output, NULL, &local_error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (output, ==, expected_output);

  g_unlink (out_file_path);
  g_unlink (in_file_path);
  g_free (output);
  g_free (out_file_path);
  g_free (in_file_path);
}

static void
assert_dump_error (const char *input,
                   GQuark      expected_error_domain,
                   int         expected_error_code)
{
  int fd = -1;
  char *in_file_path = NULL;
  char *out_file_path = NULL;
  gboolean retval;
  GError *local_error = NULL;

  fd = g_file_open_tmp ("dump_XXXXXX", &in_file_path, NULL);
  g_assert_cmpint (fd, >=, 0);
  g_assert_true (g_close (fd, NULL));

  out_file_path = g_strconcat (in_file_path, ".out", NULL);

  g_file_set_contents (in_file_path, input, -1, &local_error);
  g_assert_no_error (local_error);

  retval = gi_repository_dump (in_file_path, out_file_path, &local_error);
  g_assert_error (local_error, expected_error_domain, expected_error_code);
  g_assert_false (retval);
  g_clear_error (&local_error);

  g_unlink (out_file_path);
  g_unlink (in_file_path);
  g_free (out_file_path);
  g_free (in_file_path);
}

static void
test_empty_file (void)
{
  assert_dump ("",
               "<?xml version=\"1.0\"?>\n"
               "<dump>\n"
               "</dump>\n");
}

static void
test_missing_get_type (void)
{
  assert_dump_error ("get-type:does_not_exist_get_type",
                     G_FILE_ERROR, G_FILE_ERROR_FAILED);
}

static void
test_missing_quark (void)
{
  assert_dump_error ("error-quark:does_not_exist_error",
                     G_FILE_ERROR, G_FILE_ERROR_FAILED);
}

static void
test_basic (void)
{
  assert_dump ("get-type:test_object_get_type\n"
               "get-type:test_interface_get_type\n",
               "<?xml version=\"1.0\"?>\n"
               "<dump>\n"
               "  <class name=\"TestObject\" get-type=\"test_object_get_type\" parents=\"GObject\" final=\"1\">\n"
               "  </class>\n"
               "  <interface name=\"TestInterface\" get-type=\"test_interface_get_type\">\n"
               "  </interface>\n"
               "</dump>\n");
}

static void
test_empty_lines (void)
{
  assert_dump ("get-type:test_object_get_type\n"
               "\n"
               "get-type:test_interface_get_type\n"
               "\n\n",
               "<?xml version=\"1.0\"?>\n"
               "<dump>\n"
               "  <class name=\"TestObject\" get-type=\"test_object_get_type\" parents=\"GObject\" final=\"1\">\n"
               "  </class>\n"
               "  <interface name=\"TestInterface\" get-type=\"test_interface_get_type\">\n"
               "  </interface>\n"
               "</dump>\n");
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  g_test_add_func ("/dump/empty-file", test_empty_file);
  g_test_add_func ("/dump/missing-get-type", test_missing_get_type);
  g_test_add_func ("/dump/missing-quark", test_missing_quark);
  g_test_add_func ("/dump/basic", test_basic);
  g_test_add_func ("/dump/empty-lines", test_empty_lines);

  return g_test_run ();
}
