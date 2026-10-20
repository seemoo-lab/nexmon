/* Counting the multibyte characters in a string.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2007.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <string.h>

#include <stdlib.h>

#if GNULIB_MCEL_PREFER
# include "mcel.h"
#else
# include "mbiterf.h"
#endif

/* Return the number of multibyte characters in the character string starting
   at STRING and ending at STRING + LEN.  */
size_t
mbsnlen (const char *string, size_t len)
{
  if (MB_CUR_MAX > 1)
    {
      size_t count = 0;

      const char *string_end = string + len;

#if GNULIB_MCEL_PREFER
      for (; string < string_end; string += mcel_scan (string, string_end).len)
        count++;
#else
      mbif_state_t state;
      const char *iter;
      for (mbif_init (state), iter = string; mbif_avail (state, iter, string_end); )
        {
          mbchar_t cur = mbif_next (state, iter, string_end);
          count++;
          iter += mb_len (cur);
        }
#endif

      return count;
    }
  else
    return len;
}
