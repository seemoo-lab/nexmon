/*
 * Copyright 2024 Philip Chimento
 * Copyright 2024 GNOME Foundation, Inc.
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
 * Author: Philip Withnall <pwithnall@gnome.org>
 */

#include "girepository.h"
#include "girffi.h"
#include "glib.h"
#include "test-common.h"

static void
test_function_info_invoker (RepositoryFixture *fx,
                            const void *unused)
{
  GIFunctionInfo *function_info = NULL;
  GIFunctionInvoker invoker;
  GError *local_error = NULL;

  g_test_summary ("Test preparing a function invoker");

  function_info = GI_FUNCTION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "get_locale_variants"));
  g_assert_nonnull (function_info);

  gi_function_info_prep_invoker (function_info, &invoker, &local_error);
  g_assert_no_error (local_error);

  gi_function_invoker_clear (&invoker);
  g_clear_pointer (&function_info, gi_base_info_unref);
}

int
main (int   argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/function-info/invoker", test_function_info_invoker, &typelib_load_spec_glib);

  return g_test_run ();
}
