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

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  GDateTime *dt = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (the function doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const gchar *) data, size);
  dt = g_date_time_new_from_iso8601 ((const gchar *) nul_terminated_data, NULL);
  g_free (nul_terminated_data);

  if (dt != NULL)
    {
      gchar *text = g_date_time_format_iso8601 (dt);
      g_free (text);
    }

  g_clear_pointer (&dt, g_date_time_unref);

  return 0;
}
