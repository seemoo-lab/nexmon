/* Copyright (C) 2011-2026 Free Software Foundation, Inc.
   This file is part of gnulib.

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

/* Specification */
#include <wchar.h>

#include <errno.h>
#include <stdlib.h>

#if defined _WIN32 && !defined __CYGWIN__

wchar_t *
wgetcwd (wchar_t *buf, size_t size)
{
  /* Uses _wgetcwd.
     Documentation:
     <https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getcwd-wgetcwd>
     Note that for a directory consisting of LEN wide characters, the SIZE
     argument to _wgetcwd needs to be >= LEN + 3, not only >= LEN + 1, with
     some versions of the Microsoft runtime libraries.  */

  /* Handle single size operations.  */
  if (buf)
    {
      /* Check SIZE argument.  */
      if (!size)
        {
          errno = EINVAL;
          return NULL;
        }
      /* Invoke _wgetcwd as-is.  In this case, the caller does not expect
         an ENOMEM error; therefore don't use temporary memory.  */
      return _wgetcwd (buf, size);
    }

  if (size)
    {
      /* Allocate room for two more wide characters, so that directory names
         of length <= SIZE - 1 can be returned.  */
      buf = malloc ((size + 2) * sizeof (wchar_t));
      if (!buf)
        {
          errno = ENOMEM;
          return NULL;
        }
      wchar_t *result = _wgetcwd (buf, size + 2);
      if (!result)
        {
          free (buf);
          return NULL;
        }
      if (wcslen (result) >= size)
        {
          free (buf);
          errno = ERANGE;
          return NULL;
        }
      /* Shrink result before returning it.  */
      wchar_t *shrinked_result = realloc (result, size * sizeof (wchar_t));
      if (shrinked_result != NULL)
        result = shrinked_result;
      return result;
    }

  /* Flexible sizing requested.  Avoid over-allocation for the common
     case of a name that fits within a 4k page, minus some space for
     local variables, to be sure we don't skip over a guard page.  */
  {
    wchar_t tmp[4032 / sizeof (wchar_t)];
    size = sizeof tmp / sizeof (wchar_t);
    wchar_t *ptr = _wgetcwd (tmp, size);
    if (ptr)
      {
        wchar_t *result = _wcsdup (ptr);
        if (!result)
          errno = ENOMEM;
        return result;
      }
    if (errno != ERANGE)
      return NULL;
  }

  /* My what a large directory name we have.  */
  wchar_t *result;
  do
    {
      size <<= 1;
      wchar_t *ptr = realloc (buf, size * sizeof (wchar_t));
      if (ptr == NULL)
        {
          free (buf);
          errno = ENOMEM;
          return NULL;
        }
      buf = ptr;
      result = _wgetcwd (buf, size);
    }
  while (!result && errno == ERANGE);

  if (!result)
    free (buf);
  else
    {
      /* Here result == buf.  */
      /* Shrink result before returning it.  */
      size_t actual_size = wcslen (result) + 1;
      if (actual_size < size)
        {
          wchar_t *shrinked_result =
            realloc (result, actual_size * sizeof (wchar_t));
          if (shrinked_result != NULL)
            result = shrinked_result;
        }
    }
  return result;
}

#endif
