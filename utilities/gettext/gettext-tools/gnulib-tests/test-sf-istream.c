/* Test of string or file based input stream.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

/* Specification.  */
#include "sf-istream.h"

#include <unistd.h>

#include "macros.h"

#define CONTENTS_LEN 7
#define CONTENTS "Hello\377\n"

static void
test_open_stream (sf_istream_t *stream)
{
  int c;

  c = sf_getc (stream);
  ASSERT (c == 'H');
  c = sf_getc (stream);
  ASSERT (c == 'e');
  c = sf_getc (stream);
  ASSERT (c == 'l');
  c = sf_getc (stream);
  ASSERT (c == 'l');
  c = sf_getc (stream);
  ASSERT (c == 'o');
  sf_ungetc (stream, c);
  c = sf_getc (stream);
  ASSERT (c == 'o');
  c = sf_getc (stream);
  ASSERT (c == 0xff);
  sf_ungetc (stream, c);
  c = sf_getc (stream);
  ASSERT (c == 0xff);
  c = sf_getc (stream);
  ASSERT (c == '\n');
  c = sf_getc (stream);
  ASSERT (c == EOF);
  c = sf_getc (stream);
  ASSERT (c == EOF);
  sf_ungetc (stream, c);
  c = sf_getc (stream);
  ASSERT (c == EOF);
  ASSERT (!sf_ferror (stream));
}

int
main ()
{
  char const contents[CONTENTS_LEN] _GL_ATTRIBUTE_NONSTRING = CONTENTS;

  /* Test reading from a file.  */
  {
    const char *filename = "test-sf-istream.tmp";
    unlink (filename);
    {
      FILE *fp = fopen (filename, "wb");
      ASSERT (fwrite (contents, 1, CONTENTS_LEN, fp) == CONTENTS_LEN);
      ASSERT (fclose (fp) == 0);
    }
    {
      FILE *fp = fopen (filename, "rb");
      sf_istream_t stream;
      sf_istream_init_from_file (&stream, fp);
      test_open_stream (&stream);
      sf_free (&stream);
    }
    unlink (filename);
  }

  /* Test reading from a string in memory.  */
  {
    sf_istream_t stream;
    sf_istream_init_from_string_desc (&stream,
                                      sd_new_addr (CONTENTS_LEN, contents));
    test_open_stream (&stream);
    sf_free (&stream);
  }

  /* Test reading from a NUL-terminated string in memory.  */
  {
    sf_istream_t stream;
    sf_istream_init_from_string (&stream, CONTENTS);
    test_open_stream (&stream);
    sf_free (&stream);
  }

  return 0;
}
