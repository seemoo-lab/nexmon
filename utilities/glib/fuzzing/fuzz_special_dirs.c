/*
 * Copyright 2025 Philip Withnall
 * Copyright 2025 Tobias Stoeckmann
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "fuzz.h"
#include "glib.h"
#include "gutilsprivate.c"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  gchar *special_dirs[G_USER_N_DIRECTORIES] = { 0 };
  unsigned char *nul_terminated_data = NULL;

  fuzz_set_logging_func ();

  nul_terminated_data = (unsigned char *) g_strndup ((const char *) data, size);

  load_user_special_dirs_from_string ((const gchar *) nul_terminated_data, "/dev/null", special_dirs);

  /* Test directories and make sure that, if they exist, they are absolute. */
  for (GUserDirectory dir_type = G_USER_DIRECTORY_DESKTOP; dir_type < G_USER_N_DIRECTORIES; dir_type++)
    {
      char *dir = special_dirs[dir_type];
      g_assert_true (dir == NULL || g_path_is_absolute (dir));
      g_free (dir);
    }

  g_free (nul_terminated_data);

  return 0;
}
