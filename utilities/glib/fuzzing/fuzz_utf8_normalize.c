/*
 * Copyright 2023 Todd Carson
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
  char *bounded, *terminated, *buf;

  fuzz_set_logging_func ();

  buf = g_malloc (size + 1);
  memcpy (buf, data, size);
  buf[size] = '\0';

  terminated = g_utf8_normalize (buf, -1, G_NORMALIZE_ALL);
  g_free (buf);

  bounded = g_utf8_normalize ((const char *) data, size, G_NORMALIZE_ALL);

  if (terminated && bounded)
    {
      g_assert (strcmp (terminated, bounded) == 0);
      g_free (terminated);
      g_free (bounded);
    }
  else
    g_assert (!(terminated || bounded));

  return 0;
}
