/* Multiline error-reporting functions.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible.  */


#include <config.h>

/* Specification.  */
#include "xerror.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>
#include "error-progname.h"
#include "mbswidth.h"
#if IN_LIBGETTEXTPO
# define program_name getprogname ()
#else
# include "progname.h"
#endif

/* Emit a multiline warning to stderr, consisting of MESSAGE, with the
   first line prefixed with PREFIX or, if that is NULL, with PREFIX_WIDTH
   spaces, and the remaining lines prefixed with the same amount of spaces.
   Free the PREFIX and MESSAGE when done.  Return the amount of spaces.  */
static size_t
multiline_internal (char *prefix, size_t prefix_width, char *message)
{
  fflush (stdout);

  const char *cp = message;

  size_t width;
  if (prefix != NULL)
    {
      width = 0;
      if (error_with_progname)
        {
          fprintf (stderr, "%s: ", program_name);
          width += mbswidth (program_name, 0) + 2;
        }
      fputs (prefix, stderr);
      width += mbswidth (prefix, 0);
      free (prefix);
      goto after_indent;
    }
  else
    width = prefix_width;

  for (;;)
    {
      for (size_t i = width; i > 0; i--)
        putc (' ', stderr);

    after_indent: ;
      const char *np = strchr (cp, '\n');

      if (np == NULL || np[1] == '\0')
        {
          fputs (cp, stderr);
          break;
        }

      np++;
      fwrite (cp, 1, np - cp, stderr);
      cp = np;
    }

  free (message);

  return width;
}

size_t
multiline_warning (char *prefix, char *message)
{
  if (prefix == NULL)
    /* Invalid argument.  */
    abort ();
  return multiline_internal (prefix, 0, message);
}

size_t
multiline_error (char *prefix, char *message)
{
  if (prefix == NULL)
    /* Invalid argument.  */
    abort ();
  ++error_message_count;
  return multiline_internal (prefix, 0, message);
}

void
multiline_append (size_t prefix_width, char *message)
{
  multiline_internal (NULL, prefix_width, message);
}
