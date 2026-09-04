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
  GInputStream *base_stream = NULL;
  GDataInputStream *input_stream = NULL;
  char *line = NULL;
  size_t line_length = 0;

  fuzz_set_logging_func ();

  base_stream = g_memory_input_stream_new_from_data (data, size, NULL);
  input_stream = g_data_input_stream_new (base_stream);

  line = g_data_input_stream_read_line_utf8 (input_stream, &line_length, NULL, NULL);
  g_assert (line != NULL || line_length == 0);
  g_assert (line_length <= size);
  g_free (line);

  g_clear_object (&input_stream);
  g_clear_object (&base_stream);

  return 0;
}
