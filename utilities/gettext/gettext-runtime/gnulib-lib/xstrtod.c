/* error-checking interface to strtod-like functions

   Copyright (C) 1996, 1999-2000, 2003-2006, 2009-2026 Free Software
   Foundation, Inc.

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

/* Written by Jim Meyering.  */

#include <config.h>

#include "xstrtod.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#if LONG
# define XSTRTOD xstrtold
# define DOUBLE long double
#else
# define XSTRTOD xstrtod
# define DOUBLE double
#endif

/* An interface to a string-to-floating-point conversion function that
   encapsulates all the error checking one should usually perform.
   Like strtod/strtold, but stores the conversion in *RESULT,
   and returns true upon successful conversion.
   CONVERT specifies the conversion function, e.g., strtod itself.  */

bool
XSTRTOD (char const *str, char const **ptr, DOUBLE *result,
         DOUBLE (*convert) (char const *, char **))
{
  errno = 0;
  char *terminator;
  DOUBLE val = convert (str, &terminator);

  /* Having a non-zero terminator is an error only when PTR is NULL. */
  bool ok = true;
  if (terminator == str || (ptr == NULL && *terminator != '\0'))
    ok = false;
  else
    {
      /* Flag overflow as an error.
         Flag gradual underflow as no error.
         Flag flush-to-zero underflow as no error.
         In either case, the caller can inspect *RESULT to get more details.  */
      if (val != 0 && errno == ERANGE)
        {
          if (val >= 1 || val <= -1)
            /* Overflow.  */
            ok = false;
          else
            /* Gradual underflow.  */
            errno = 0;
        }
    }

  if (ptr != NULL)
    *ptr = terminator;

  /* Callers rely on *RESULT even when !ok.  */
  *result = val;

  return ok;
}
