/* A string or file based input stream.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

/* Specification.  */
#include "sf-istream.h"

#include <stdlib.h>
#include <string.h>

void
sf_istream_init_from_file (sf_istream_t *stream, FILE *fp)
{
  stream->fp = fp;
  stream->input = NULL;
  stream->input_end = NULL;
}

void
sf_istream_init_from_string (sf_istream_t *stream,
                             const char *input)
{
  stream->fp = NULL;
  stream->input = input;
  stream->input_end = input + strlen (input);
}

void
sf_istream_init_from_string_desc (sf_istream_t *stream,
                                  string_desc_t input)
{
  stream->fp = NULL;
  stream->input = sd_data (input);
  stream->input_end = stream->input + sd_length (input);
}

int
sf_getc (sf_istream_t *stream)
{
  int c;

  if (stream->fp != NULL)
    c = getc (stream->fp);
  else
    {
      if (stream->input == stream->input_end)
        return EOF;
      c = (unsigned char) *(stream->input++);
    }

  return c;
}

int
sf_ferror (sf_istream_t *stream)
{
  return (stream->fp != NULL && ferror (stream->fp));
}

void
sf_ungetc (sf_istream_t *stream, int c)
{
  if (c != EOF)
    {
      if (stream->fp != NULL)
        ungetc (c, stream->fp);
      else
        {
          stream->input--;
          if (!(c == (unsigned char) *stream->input))
            /* C was incorrect.  */
            abort ();
        }
    }
}

void
sf_free (sf_istream_t *stream)
{
}
