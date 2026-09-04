/* GLib testing framework examples and tests
 *
 * Copyright 2025 GNOME Foundation, Inc.
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

#include <gio/gio.h>

#define TYPE_TEST_FILE_ENUMERATOR (test_file_enumerator_get_type ())
#define TEST_FILE_ENUMERATOR(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TEST_FILE_ENUMERATOR, TestFileEnumerator))
#define IS_TEST_FILE_ENUMERATOR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TEST_FILE_ENUMERATOR))

typedef struct
{
  GFileEnumerator parent_instance;
  unsigned int n_times_closed; // Number of times the enumerator has been closed
} TestFileEnumerator;

typedef struct
{
  GFileEnumeratorClass parent_class;
} TestFileEnumeratorClass;

GType test_file_enumerator_get_type (void);

G_DEFINE_TYPE (TestFileEnumerator, test_file_enumerator, G_TYPE_FILE_ENUMERATOR)

static gboolean
test_file_enumerator_close (GFileEnumerator *enumerator,
                            GCancellable *cancellable,
                            GError **error)
{
  TestFileEnumerator *test = TEST_FILE_ENUMERATOR (enumerator);
  ++test->n_times_closed;
  return TRUE;
}

static void
test_file_enumerator_init (TestFileEnumerator *enumerator)
{
  enumerator->n_times_closed = 0;
}

static void
test_file_enumerator_class_init (TestFileEnumeratorClass *klass)
{
  GFileEnumeratorClass *enumerator_class;

  enumerator_class = G_FILE_ENUMERATOR_CLASS (klass);
  enumerator_class->close_fn = test_file_enumerator_close;
}

static void
test_close_on_dispose (void)
{
  GFile *dir;
  TestFileEnumerator *enumerator;

  dir = g_file_new_for_path (g_get_tmp_dir ());

  enumerator = g_object_new (TYPE_TEST_FILE_ENUMERATOR,
                             "container", dir,
                             NULL);

  // Check enumerator is not closed initially
  g_assert_cmpuint (enumerator->n_times_closed, ==, 0);

  g_object_run_dispose (G_OBJECT (enumerator));

  // Check enumerator is closed after 1st dispose
  g_assert_cmpuint (enumerator->n_times_closed, ==, 1);

  g_object_run_dispose (G_OBJECT (enumerator));

  // Check enumerator is not closed twice after 2nd dispose
  g_assert_cmpuint (enumerator->n_times_closed, ==, 1);

  g_object_unref (enumerator);
  g_object_unref (dir);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/file-enumerator/close-on-dispose", test_close_on_dispose);
  return g_test_run ();
}
