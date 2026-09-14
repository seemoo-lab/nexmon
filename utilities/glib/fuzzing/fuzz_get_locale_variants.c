/*
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  - Philip Withnall <pwithnall@gnome.org>
 */

#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  char **v;

  fuzz_set_logging_func ();

  /* ignore @size (g_get_locale_variants() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const char *) data, size);

  v = g_get_locale_variants ((char *) nul_terminated_data);
  g_assert_nonnull (v);
  /* g_get_locale_variants() guarantees that the input is always in the output: */
  g_assert_true (g_strv_contains ((const char * const *) v, (char *) nul_terminated_data));
  g_strfreev (v);

  g_free (nul_terminated_data);

  return 0;
}
