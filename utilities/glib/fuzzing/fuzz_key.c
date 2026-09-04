/*
 * Copyright 2018 pdknsk
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
test_parse (const gchar   *data,
            size_t         size,
            GKeyFileFlags  flags)
{
  GKeyFile *key = NULL;
  char *comment = NULL;
  char **list = NULL;

  key = g_key_file_new ();
  g_key_file_load_from_data (key, (const gchar*) data, size, G_KEY_FILE_NONE,
                             NULL);

  /* Also try some additional parsing and see if it crashes */
  comment = g_key_file_get_comment (key, "group", "key", NULL);
  g_free (comment);

  list = g_key_file_get_locale_string_list (key, "group", "key", "de", NULL, NULL);
  g_strfreev (list);

  g_key_file_free (key);
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  fuzz_set_logging_func ();

  test_parse ((const gchar *) data, size, G_KEY_FILE_NONE);
  test_parse ((const gchar *) data, size, G_KEY_FILE_KEEP_COMMENTS);
  test_parse ((const gchar *) data, size, G_KEY_FILE_KEEP_TRANSLATIONS);

  return 0;
}
