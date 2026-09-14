/*
 * Copyright 2020 Endless OS Foundation, LLC
 * Copyright 2020 Red Hat, Inc.
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
  GError *error = NULL;
  GHashTable *parsed_params = NULL;

  fuzz_set_logging_func ();

  if (size > G_MAXSSIZE)
    return 0;

  parsed_params = g_uri_parse_params ((const gchar *) data, (gssize) size,
                                      "&", G_URI_PARAMS_NONE, &error);
  if (parsed_params == NULL)
    {
      g_assert (error);
      g_clear_error (&error);
      return 0;
    }


  g_assert_no_error (error);
  g_hash_table_unref (parsed_params);

  return 0;
}
