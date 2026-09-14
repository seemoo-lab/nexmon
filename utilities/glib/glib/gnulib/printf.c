/* GLIB - Library of useful routines for C programming
 * Copyright (C) 2003 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 2003.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <config.h>

#include <errno.h>
#include <limits.h> /* for INT_MAX */
#include <stdint.h> /* for uintptr_t, SIZE_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "printf.h"

#include "g-gnulib.h"
#include "vasnprintf.h"

int _g_gnulib_printf (char const *format, ...)
{
  va_list args;
  int retval;

  va_start (args, format);
  retval = _g_gnulib_vprintf (format, args);
  va_end (args);

  return retval;
}

int _g_gnulib_fprintf (FILE *file, char const *format, ...)
{
  va_list args;
  int retval;

  va_start (args, format);
  retval = _g_gnulib_vfprintf (file, format, args);
  va_end (args);
  
  return retval;
}

int _g_gnulib_sprintf (char *string, char const *format, ...)
{
  va_list args;
  int retval;

  va_start (args, format);
  retval = _g_gnulib_vsprintf (string, format, args);
  va_end (args);
  
  return retval;
}

int _g_gnulib_snprintf (char *string, size_t n, char const *format, ...)
{
  va_list args;
  int retval;

  va_start (args, format);
  retval = _g_gnulib_vsnprintf (string, n, format, args);
  va_end (args);
  
  return retval;
}

int _g_gnulib_vprintf (char const *format, va_list args)         
{
  return _g_gnulib_vfprintf (stdout, format, args);
}

int _g_gnulib_vfprintf (FILE *file, char const *format, va_list args)
{
  char *result;
  size_t length, rlength;

  result = vasnprintf (NULL, &length, format, args);
  if (result == NULL) 
    return -1;

  rlength = fwrite (result, 1, length, file);
  free (result);
  
  return rlength;
}

int _g_gnulib_vsprintf (char *str, char const *format, va_list args)
{
  char *output;
  size_t len;
  size_t lenbuf;

  /* Set lenbuf = min (SIZE_MAX, - (uintptr_t) str - 1).  */
  lenbuf = SIZE_MAX;
  if (lenbuf >= ~ (uintptr_t) str)
    lenbuf = ~ (uintptr_t) str;

  output = vasnprintf (str, &lenbuf, format, args);
  len = lenbuf;

  if (!output)
    return -1;

  if (output != str)
    {
      /* len is near SIZE_MAX.  */
      free (output);
      errno = ENOMEM;
      return -1;
    }

  if (len > INT_MAX)
    {
      errno = EOVERFLOW;
      return -1;
    }

  return len;
}

int _g_gnulib_vsnprintf (char *str, size_t size, char const *format, va_list args)
{
  char *output;
  size_t len;
  size_t lenbuf = size;

  output = vasnprintf (str, &lenbuf, format, args);
  len = lenbuf;

  if (!output)
    return -1;

  if (output != str)
    {
      if (size)
        {
          size_t pruned_len = (len < size ? len : size - 1);
          memcpy (str, output, pruned_len);
          str[pruned_len] = '\0';
        }

      free (output);
    }

  if (len > INT_MAX)
    {
      errno = EOVERFLOW;
      return -1;
    }

  return len;
}

int _g_gnulib_vasprintf (char **result, char const *format, va_list args)
{
  size_t length;

  *result = vasnprintf (NULL, &length, format, args);
  if (*result == NULL) 
    return -1;
  
  return length;  
}
