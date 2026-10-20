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
  const unsigned char *separator, *first_nul;
  const unsigned char *stop_chars;
  unsigned char *owned_stop_chars = NULL;
  gssize stop_chars_len;
  const unsigned char *stream_data;
  size_t stream_data_len;

  fuzz_set_logging_func ();

  /* Split the data into two arguments: the first for the array of stop
   * characters, and the second for the data in the stream. Both may contain
   * embedded nuls, so we have to be careful about lengths here. It’s also
   * possible for the fuzzer to not include any nuls and/or the pipe separator
   * (which has been chosen arbitrarily), so we need to handle that too.
   *
   * The fuzzer *will* manage to exploit all code paths here, as it uses
   * coverage guided fuzzing. */
  separator = memchr (data, '|',  size);
  first_nul = memchr (data, '\0', size);

  if (separator == NULL)
    {
      stop_chars = (const unsigned char *) "";
      stop_chars_len = 0;
      stream_data = data;
      stream_data_len = size;
    }
  else
    {
      stop_chars = data;
      stop_chars_len = (first_nul != NULL && first_nul < separator) ? separator - data : -1;
      stream_data = separator + 1;
      stream_data_len = size - (separator + 1 - data);
    }

  /* If stop_chars_len < 0, we have to guarantee that it’s nul-terminated. */
  if (stop_chars_len < 0)
    stop_chars = owned_stop_chars = (unsigned char *) g_strndup ((const char *) stop_chars, separator - data);

  /* Build the stream and test read_upto(). */
  base_stream = g_memory_input_stream_new_from_data (stream_data, stream_data_len, NULL);
  input_stream = g_data_input_stream_new (base_stream);

  line = g_data_input_stream_read_upto (input_stream, (const char *) stop_chars, stop_chars_len, &line_length, NULL, NULL);
  g_assert (line != NULL || line_length == 0);
  g_assert (line_length <= size);
  g_free (line);

  g_free (owned_stop_chars);
  g_clear_object (&input_stream);
  g_clear_object (&base_stream);

  return 0;
}
