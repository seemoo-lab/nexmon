/* Handle quoted segments of a string.
   Copyright (C) 2014-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Daiki Ueno.  */

#ifndef _QUOTE_H
#define _QUOTE_H

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Iterates over the quoted segments of the given string (starting at INPUT,
   with LENGTH bytes) and invokes
     CALLBACK (quote, segment_start, segment_length, DATA)
   on each.  The definition of "quoted segment" is as in po/quot.sed.
   It also invokes
     CALLBACK ('\0', segment_start, segment_length, DATA)
   for the segments in-between (outside quotes).  */
static void
scan_quoted (const char *input, size_t length,
             void (* callback) (char quote,
                                const char *segment_start,
                                size_t segment_length,
                                void *data),
             void *data)
{
  const char *p, *start, *end;
  bool seen_opening;

  /* START shall point to the beginning of a quoted segment, END
     points to the end of the entire input string.  */
  start = input;
  end = input + length;
  
  /* True if we have seen a character which could be an opening
     quotation mark.  Note that we can't determine if it is really an
     opening quotation mark until we see a closing quotation mark.  */
  seen_opening = false;

  for (p = start; p < end; p++)
    {
      switch (*p)
        {
        case '"':
          if (seen_opening)
            {
              if (*start == '"')
                {
                  if (p == start + 1)
                    /* Consider "" as "".  */
                    callback ('\0', "\"\"", 2, data);
                  else
                    /* "..." */
                    callback ('"', start + 1, p - (start + 1), data);

                  start = p + 1;
                  seen_opening = false;
                }
            }
          else
            {
              callback ('\0', start, p - start, data);
              start = p;
              seen_opening = true;
            }
          break;

        case '`':
          if (seen_opening)
            {
              if (*start == '`')
                {
                  callback ('\0', start, p - start, data);
                  start = p;
                }
            }
          else
            {
              callback ('\0', start, p - start, data);
              start = p;
              seen_opening = true;
            }
          break;

        case '\'':
          if (seen_opening)
            {
              if (/* `...' */
                  *start == '`'
                  /* '...', where
                     - The left quote is preceded by a space, and the
                       right quote is followed by a space.
                     - The left quote is preceded by a space, and the
                       right quote is at the end of line.
                     - The left quote is at the beginning of the line, and
                       the right quote is followed by a space.  */
                  || (*start == '\''
                      && (((start > input && *(start - 1) == ' ')
                           && (p + 1 == end || p[1] == '\n' || p[1] == ' '))
                          || ((start == input || *(start - 1) == '\n')
                              && p + 1 < end && p[1] == ' '))))
                {
                  callback ('\'', start + 1, p - (start + 1), data);
                  start = p + 1;
                }
              else
                {
                  callback ('\0', start, p - start, data);
                  start = p;
                }
              seen_opening = false;
            }
          else if (p == input || *(p - 1) == '\n' || *(p - 1) == ' ')
            {
              callback ('\0', start, p - start, data);
              start = p;
              seen_opening = true;
            }
          break;
        }
    }

  /* Copy the rest.  */
  if (p > start)
    callback ('\0', start, p - start, data);
}


#ifdef __cplusplus
}
#endif


#endif /* _QUOTE_H */
