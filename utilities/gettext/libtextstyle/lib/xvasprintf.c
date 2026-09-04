/* vasprintf and asprintf with out-of-memory checking.
   Copyright (C) 1999, 2002-2004, 2006-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "xvasprintf.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

/* Checked size_t computations.  */
#include "xsize.h"

static char *
xstrcat (size_t argcount, va_list args)
{
  /* Determine the total size.  */
  size_t totalsize = 0;
  {
    va_list ap;
    va_copy (ap, args);
    for (size_t i = argcount; i > 0; i--)
      {
        const char *next = va_arg (ap, const char *);
        totalsize = xsum (totalsize, strlen (next));
      }
    va_end (ap);
  }

  /* Test for overflow in the summing pass above or in (totalsize + 1)
     below.  */
  if (totalsize == SIZE_MAX)
    xalloc_die ();

  /* Allocate and fill the result string.  */
  char *result = XNMALLOC (totalsize + 1, char);
  {
    char *p = result;
    for (size_t i = argcount; i > 0; i--)
      {
        const char *next = va_arg (args, const char *);
        size_t len = strlen (next);
        memcpy (p, next, len);
        p += len;
      }
    *p = '\0';
  }

  return result;
}

char *
xvasprintf (const char *format, va_list args)
{

  /* Recognize the special case format = "%s...%s".  It is a frequently used
     idiom for string concatenation and needs to be fast.  We don't want to
     have a separate function xstrcat() for this purpose.  */
  {
    size_t argcount = 0;

    for (const char *f = format;;)
      {
        if (*f == '\0')
          /* Recognized the special case of string concatenation.  */
          return xstrcat (argcount, args);
        if (*f != '%')
          break;
        f++;
        if (*f != 's')
          break;
        f++;
        argcount++;
      }
  }

  char *result;
  if (vaszprintf (&result, format, args) < 0)
    {
      if (errno == ENOMEM)
        xalloc_die ();
      else
        {
          /* The programmer ought to have ensured that none of the other errors
             can occur.  */
          int err = errno;
          char errbuf[20];
          const char *errname;
#if HAVE_WORKING_STRERRORNAME_NP
          errname = strerrorname_np (err);
          if (errname == NULL)
#else
          if (err == EINVAL)
            errname = "EINVAL";
          else if (err == EILSEQ)
            errname = "EILSEQ";
          else if (err == EOVERFLOW)
            errname = "EOVERFLOW";
          else
#endif
            {
              sprintf (errbuf, "%d", err);
              errname = errbuf;
            }
          fprintf (stderr, "vasprintf failed! format=\"%s\", errno=%s\n",
                   format, errname);
          fflush (stderr);
          abort ();
        }
    }

  return result;
}
