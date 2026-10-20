/*
 * Copyright 2023 GNOME Foundation, Inc.
 * Copyright 2024 Philip Chimento <philip.chimento@gmail.com>
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

#include "girepository.h"
#include "glib.h"
#include "test-common.h"

void
repository_init (int *argc,
                 char **argv[])
{
  /* Isolate from the system typelibs and GIRs. */
  g_setenv ("GI_TYPELIB_PATH", "/dev/null", TRUE);
  g_setenv ("GI_GIR_PATH", "/dev/null", TRUE);

  g_test_init (argc, argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
}

void
repository_setup (RepositoryFixture *fx,
                  const void *data)
{
  GITypelib *typelib = NULL;
  GError *local_error = NULL;
  TypelibLoadSpec *load_spec = (TypelibLoadSpec *) data;

  fx->repository = gi_repository_new ();
  g_assert_nonnull (fx->repository);

  fx->gobject_typelib_dir = g_test_build_filename (G_TEST_BUILT, "..", "introspection", NULL);
  g_test_message ("Using GI_TYPELIB_DIR = %s", fx->gobject_typelib_dir);
  gi_repository_prepend_search_path (fx->repository, fx->gobject_typelib_dir);

  if (load_spec)
    {
      typelib = gi_repository_require (fx->repository, load_spec->name, load_spec->version, 0, &local_error);
      g_assert_no_error (local_error);
      g_assert_nonnull (typelib);
    }
}

void
repository_teardown (RepositoryFixture *fx,
                     const void *unused)
{
  g_clear_pointer (&fx->gobject_typelib_dir, g_free);
  g_clear_object (&fx->repository);
}
