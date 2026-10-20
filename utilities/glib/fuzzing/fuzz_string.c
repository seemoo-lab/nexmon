/*
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  char **args = NULL;
  size_t n_args;
  const char *init, *find, *replace;
  GString *string = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (none of the functions support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const gchar *) data, size);

  /* Split the data into three arguments. */
  args = g_strsplit ((char *) nul_terminated_data, "|", 3);
  n_args = g_strv_length (args);
  init = (n_args > 0) ? args[0] : "";
  find = (n_args > 1) ? args[1] : "";
  replace = (n_args > 2) ? args[2] : "";

  /* Limit the input size. With a short @find, and a long @init and @replace
   * it’s quite possible to hit OOM. We’re not interested in testing that — it’s
   * up to the caller of g_string_replace() to handle that. 1KB on each of the
   * inputs should be plenty to find any string parsing or pointer arithmetic
   * bugs in g_string_replace(). */
  if (strlen (init) > 1000 ||
      strlen (find) > 1000 ||
      strlen (replace) > 1000)
    {
      g_strfreev (args);
      g_free (nul_terminated_data);
      return 0;
    }

  /* Test g_string_replace() and see if it crashes. */
  string = g_string_new (init);
  g_string_replace (string, find, replace, 0);
  g_string_free (string, TRUE);

  g_strfreev (args);
  g_free (nul_terminated_data);

  return 0;
}
