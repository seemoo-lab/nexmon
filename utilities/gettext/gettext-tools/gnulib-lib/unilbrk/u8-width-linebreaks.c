/* Line breaking of UTF-8 strings.
   Copyright (C) 2001-2003, 2006-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "unilbrk.h"

#include "unistr.h"
#include "uniwidth.h"

int
u8_width_linebreaks (const uint8_t *s, size_t n,
                     int width, int start_column, int at_end_columns,
                     const char *o, const char *encoding,
                     char *p)
{
  const uint8_t *s_end;
  char *last_p;
  int last_column;
  int piece_width;

  u8_possible_linebreaks (s, n, encoding, p);

  s_end = s + n;
  last_p = NULL;
  last_column = start_column;
  piece_width = 0;
  while (s < s_end)
    {
      ucs4_t uc;
      int count = u8_mbtouc_unsafe (&uc, s, s_end - s);

      /* Respect the override.  */
      if (o != NULL && *o != UC_BREAK_UNDEFINED)
        *p = *o;

      if (*p == UC_BREAK_POSSIBLE || *p == UC_BREAK_MANDATORY)
        {
          /* An atomic piece of text ends here.  */
          if (last_p != NULL && last_column + piece_width > width)
            {
              /* Insert a line break.  */
              *last_p = UC_BREAK_POSSIBLE;
              last_column = 0;
            }
        }

      if (*p == UC_BREAK_MANDATORY)
        {
          /* uc is a line break character.  */
          /* Start a new piece at column 0.  */
          last_p = NULL;
          last_column = 0;
          piece_width = 0;
        }
      else
        {
          /* uc is not a line break character.  */
          int w;

          if (*p == UC_BREAK_POSSIBLE)
            {
              /* Start a new piece.  */
              last_p = p;
              last_column += piece_width;
              piece_width = 0;
              /* No line break for the moment, may be turned into
                 UC_BREAK_POSSIBLE later, via last_p. */
            }

          *p = UC_BREAK_PROHIBITED;

          w = uc_width (uc, encoding);
          if (w >= 0) /* ignore control characters in the string */
            piece_width += w;
        }

      s += count;
      p += count;
      if (o != NULL)
        o += count;
    }

  /* The last atomic piece of text ends here.  */
  if (last_p != NULL && last_column + piece_width + at_end_columns > width)
    {
      /* Insert a line break.  */
      *last_p = UC_BREAK_POSSIBLE;
      last_column = 0;
    }

  return last_column + piece_width;
}


#ifdef TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read the contents of an input stream, and return it, terminated with a NUL
   byte. */
char *
read_file (FILE *stream)
{
#define BUFSIZE 4096
  char *buf = NULL;
  int alloc = 0;
  int size = 0;
  int count;

  while (! feof (stream))
    {
      if (size + BUFSIZE > alloc)
        {
          alloc = alloc + alloc / 2;
          if (alloc < size + BUFSIZE)
            alloc = size + BUFSIZE;
          buf = realloc (buf, alloc);
          if (buf == NULL)
            {
              fprintf (stderr, "out of memory\n");
              exit (1);
            }
        }
      count = fread (buf + size, 1, BUFSIZE, stream);
      if (count == 0)
        {
          if (ferror (stream))
            {
              perror ("fread");
              exit (1);
            }
        }
      else
        size += count;
    }
  buf = realloc (buf, size + 1);
  if (buf == NULL)
    {
      fprintf (stderr, "out of memory\n");
      exit (1);
    }
  buf[size] = '\0';
  return buf;
#undef BUFSIZE
}

int
main (int argc, char * argv[])
{
  if (argc == 2)
    {
      /* Insert line breaks for a given width.  */
      int width = atoi (argv[1]);
      char *input = read_file (stdin);
      int length = strlen (input);
      char *breaks = malloc (length);
      int i;

      u8_width_linebreaks ((uint8_t *) input, length, width, 0, 0, NULL, "UTF-8", breaks);

      for (i = 0; i < length; i++)
        {
          switch (breaks[i])
            {
            case UC_BREAK_POSSIBLE:
              putc ('\n', stdout);
              break;
            case UC_BREAK_MANDATORY:
              break;
            case UC_BREAK_PROHIBITED:
              break;
            default:
              abort ();
            }
          putc (input[i], stdout);
        }

      free (breaks);

      return 0;
    }
  else
    return 1;
}

#endif /* TEST */
