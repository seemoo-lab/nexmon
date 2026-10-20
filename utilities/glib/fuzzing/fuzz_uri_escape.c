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

static void
test_bytes (const guint8 *data,
            gsize         size)
{
  GError *error = NULL;
  GBytes *unescaped_bytes = NULL;
  gchar *escaped_string = NULL;

  if (size > G_MAXSSIZE)
    return;

  unescaped_bytes = g_uri_unescape_bytes ((const gchar *) data, (gssize) size, NULL, &error);
  if (unescaped_bytes == NULL)
    {
      g_assert_nonnull (error);
      g_clear_error (&error);
      return;
    }

  escaped_string = g_uri_escape_bytes (g_bytes_get_data (unescaped_bytes, NULL),
                                       g_bytes_get_size (unescaped_bytes),
                                       NULL);
  g_bytes_unref (unescaped_bytes);

  if (escaped_string == NULL)
    return;

  g_free (escaped_string);
}

static void
test_string (const guint8 *data,
             gsize         size)
{
  gchar *unescaped_string = NULL;
  gchar *escaped_string = NULL;

  unescaped_string = g_uri_unescape_segment ((const gchar *) data, (const gchar *) data + size, NULL);
  if (unescaped_string == NULL)
    return;

  escaped_string = g_uri_escape_string (unescaped_string, NULL, TRUE);
  g_free (unescaped_string);

  if (escaped_string == NULL)
    return;

  g_free (escaped_string);
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  fuzz_set_logging_func ();

  /* Bytes form */
  test_bytes (data, size);

  /* String form (doesn’t do %-decoding) */
  test_string (data, size);

  return 0;
}
