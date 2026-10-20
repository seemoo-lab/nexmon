/*
 * Copyright 2020 Endless OS Foundation, LLC
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

static void
test_with_flags (const gchar *data,
                 GUriFlags    flags)
{
  GUri *uri = NULL;
  gchar *uri_string = NULL;

  uri = g_uri_parse (data, flags, NULL);

  if (uri == NULL)
    return;

  uri_string = g_uri_to_string (uri);
  g_uri_unref (uri);

  if (uri_string == NULL)
    return;

  g_free (uri_string);
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (g_uri_parse() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const gchar *) data, size);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_NONE);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_PARSE_RELAXED);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_NON_DNS);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_HAS_AUTH_PARAMS);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_HAS_PASSWORD);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_ENCODED_QUERY);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_ENCODED_PATH);
  test_with_flags ((const gchar *) nul_terminated_data, G_URI_FLAGS_SCHEME_NORMALIZE);
  g_free (nul_terminated_data);

  return 0;
}
