/* Return diagnostic string based on error code.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

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
#include "xstrerror.h"

#include <string.h>

#include "xvasprintf.h"
#include "xalloc.h"
#include "gettext.h"

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)

char *
xstrerror (const char *message, int errnum)
{
  /* Get the internationalized description of errnum,
     in a stack-allocated buffer.  */
  const char *errdesc;
  char errbuf[1024];
#if !GNULIB_STRERROR_R_POSIX && STRERROR_R_CHAR_P
  errdesc = strerror_r (errnum, errbuf, sizeof errbuf);
#else
  if (strerror_r (errnum, errbuf, sizeof errbuf) == 0)
    errdesc = errbuf;
  else
    errdesc = NULL;
#endif
  if (errdesc == NULL)
    errdesc = _("Unknown system error");

  if (message != NULL)
    {
      /* Allow e.g. French translators to insert a space before the colon.  */
      char *result = xasprintf (_("%s: %s"), message, errdesc);
      if (result == NULL)
        /* Probably a buggy translation.  Use a safe fallback.  */
        result = xasprintf ("%s%s%s", message, ": ", errdesc);
      return result;
    }
  else
    return xstrdup (errdesc);
}
