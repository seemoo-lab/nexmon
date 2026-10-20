/* String descriptors.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2023.  */

#define GL_STRING_DESC_INLINE _GL_EXTERN_INLINE
#include <config.h>

/* Specification and inline definitions.  */
#include "string-desc.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "c-ctype.h"
#include "ialloc.h"
#include "full-write.h"


/* ==== Side-effect-free operations on string descriptors ==== */

/* Return true if A and B are equal.  */
bool
_sd_equals (idx_t a_nbytes, const char *a_data,
            idx_t b_nbytes, const char *b_data)
{
  return (a_nbytes == b_nbytes
          && (a_nbytes == 0 || memeq (a_data, b_data, a_nbytes)));
}

bool
_sd_startswith (idx_t s_nbytes, const char *s_data,
                idx_t prefix_nbytes, const char *prefix_data)
{
  return (s_nbytes >= prefix_nbytes
          && (prefix_nbytes == 0
              || memeq (s_data, prefix_data, prefix_nbytes)));
}

bool
_sd_endswith (idx_t s_nbytes, const char *s_data,
                idx_t suffix_nbytes, const char *suffix_data)
{
  return (s_nbytes >= suffix_nbytes
          && (suffix_nbytes == 0
              || memeq (s_data + (s_nbytes - suffix_nbytes), suffix_data,
                        suffix_nbytes)));
}

int
_sd_cmp (idx_t a_nbytes, const char *a_data,
         idx_t b_nbytes, const char *b_data)
{
  if (a_nbytes > b_nbytes)
    {
      if (b_nbytes == 0)
        return 1;
      return (memcmp (a_data, b_data, b_nbytes) < 0 ? -1 : 1);
    }
  else if (a_nbytes < b_nbytes)
    {
      if (a_nbytes == 0)
        return -1;
      return (memcmp (a_data, b_data, a_nbytes) > 0 ? 1 : -1);
    }
  else /* a_nbytes == b_nbytes */
    {
      if (a_nbytes == 0)
        return 0;
      return memcmp (a_data, b_data, a_nbytes);
    }
}

int
_sd_c_casecmp (idx_t a_nbytes, const char *a_data,
               idx_t b_nbytes, const char *b_data)
{
  /* Don't use memcasecmp here, since it uses the current locale, not the
     "C" locale.  */
  idx_t n = (a_nbytes < b_nbytes ? a_nbytes : b_nbytes);
  for (idx_t i = 0; i < n; i++)
    {
      int ac = c_tolower ((unsigned char) a_data[i]);
      int bc = c_tolower ((unsigned char) b_data[i]);
      if (ac != bc)
        return (UCHAR_MAX <= INT_MAX ? ac - bc : _GL_CMP (ac, bc));
    }
  /* Found n = min (a_nbytes, b_nbytes) equal characters.  */
  return _GL_CMP (a_nbytes, b_nbytes);
}

ptrdiff_t
_sd_index (idx_t s_nbytes, const char *s_data, char c)
{
  if (s_nbytes > 0)
    {
      char const *found = memchr (s_data, (unsigned char) c, s_nbytes);
      if (found != NULL)
        return found - s_data;
    }
  return -1;
}

ptrdiff_t
_sd_last_index (idx_t s_nbytes, const char *s_data, char c)
{
  if (s_nbytes > 0)
    {
      void *found = memrchr (s_data, (unsigned char) c, s_nbytes);
      if (found != NULL)
        return (char *) found - s_data;
    }
  return -1;
}

string_desc_t
sd_new_empty (void)
{
  string_desc_t result;

  result._nbytes = 0;
  result._data = NULL;

  return result;
}

#if __GNUC__ >= 5
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif

string_desc_t
_sd_new_addr (idx_t n, const char *addr)
{
  string_desc_t result;

  result._nbytes = n;
  /* No, it is not a good idea to canonicalize (0, non-NULL) to (0, NULL).
     Some functions that return a string_desc_t use a return value of (0, NULL)
     to denote failure.  */
#if 0
  if (n == 0)
    result._data = NULL;
  else
#endif
    result._data = (char *) addr;

  return result;
}
rw_string_desc_t
_rwsd_new_addr (idx_t n, /*!*/const/*!*/ char *addr)
{
  rw_string_desc_t result;

  result._nbytes = n;
  /* No, it is not a good idea to canonicalize (0, non-NULL) to (0, NULL).
     Some functions that return a rw_string_desc_t use a return value of
     (0, NULL) to denote failure.  */
#if 0
  if (n == 0)
    result._data = NULL;
  else
#endif
    result._data = (char *) addr;

  return result;
}

string_desc_t
sd_from_c (const char *s)
{
  string_desc_t result;

  result._nbytes = strlen (s);
  result._data = (char *) s;

  return result;
}

#if __GNUC__ >= 5
# pragma GCC diagnostic pop
#endif

int
_sd_write (int fd, idx_t s_nbytes, const char *s_data)
{
  if (s_nbytes > 0)
    if (full_write (fd, s_data, s_nbytes) != s_nbytes)
      /* errno is set here.  */
      return -1;
  return 0;
}

int
_sd_fwrite (FILE *fp, idx_t s_nbytes, const char *s_data)
{
  if (s_nbytes > 0)
    if (fwrite (s_data, 1, s_nbytes, fp) != s_nbytes)
      return -1;
  return 0;
}


/* ==== Memory-allocating operations on string descriptors ==== */

int
sd_new (rw_string_desc_t *resultp, idx_t n)
{
  if (!(n >= 0))
    /* Invalid argument.  */
    abort ();

  rw_string_desc_t result;

  result._nbytes = n;
  if (n == 0)
    result._data = NULL;
  else
    {
      result._data = (char *) imalloc (n);
      if (result._data == NULL)
        /* errno is set here.  */
        return -1;
    }

  *resultp = result;
  return 0;
}

int
sd_new_filled (rw_string_desc_t *resultp, idx_t n, char c)
{
  rw_string_desc_t result;

  result._nbytes = n;
  if (n == 0)
    result._data = NULL;
  else
    {
      result._data = (char *) imalloc (n);
      if (result._data == NULL)
        /* errno is set here.  */
        return -1;
      memset (result._data, (unsigned char) c, n);
    }

  *resultp = result;
  return 0;
}

int
_sd_copy (rw_string_desc_t *resultp, idx_t s_nbytes, const char *s_data)
{
  rw_string_desc_t result;

  result._nbytes = s_nbytes;
  if (s_nbytes == 0)
    result._data = NULL;
  else
    {
      result._data = (char *) imalloc (s_nbytes);
      if (result._data == NULL)
        /* errno is set here.  */
        return -1;
      memcpy (result._data, s_data, s_nbytes);
    }

  *resultp = result;
  return 0;
}

int
sd_concat (rw_string_desc_t *resultp, idx_t n, /* [rw_]string_desc_t string1, */ ...)
{
  if (n <= 0)
    /* Invalid argument.  */
    abort ();

  idx_t total = 0;
  {
    va_list strings;
    va_start (strings, n);
    string_desc_t string1 = va_arg (strings, string_desc_t);
    total += string1._nbytes;
    if (n > 1)
      for (idx_t i = n - 1; i > 0; i--)
        {
          string_desc_t arg = va_arg (strings, string_desc_t);
          total += arg._nbytes;
        }
    va_end (strings);
  }

  char *combined = (char *) imalloc (total);
  if (combined == NULL)
    /* errno is set here.  */
    return -1;
  idx_t pos = 0;
  {
    va_list strings;
    va_start (strings, n);
    string_desc_t string1 = va_arg (strings, string_desc_t);
    memcpy (combined, string1._data, string1._nbytes);
    pos += string1._nbytes;
    if (n > 1)
      for (idx_t i = n - 1; i > 0; i--)
        {
          string_desc_t arg = va_arg (strings, string_desc_t);
          if (arg._nbytes > 0)
            memcpy (combined + pos, arg._data, arg._nbytes);
          pos += arg._nbytes;
        }
    va_end (strings);
  }

  rw_string_desc_t result;
  result._nbytes = total;
  result._data = combined;

  *resultp = result;
  return 0;
}

char *
_sd_c (idx_t s_nbytes, const char *s_data)
{
  idx_t n = s_nbytes;
  char *result = (char *) imalloc (n + 1);
  if (result == NULL)
    /* errno is set here.  */
    return NULL;
  if (n > 0)
    memcpy (result, s_data, n);
  result[n] = '\0';

  return result;
}


/* ==== Operations with side effects on string descriptors ==== */

void
sd_set_char_at (rw_string_desc_t s, idx_t i, char c)
{
  if (!(i >= 0 && i < s._nbytes))
    /* Invalid argument.  */
    abort ();
  s._data[i] = c;
}

void
sd_fill (rw_string_desc_t s, idx_t start, idx_t end, char c)
{
  if (!(start >= 0 && start <= end))
    /* Invalid arguments.  */
    abort ();

  if (start < end)
    memset (s._data + start, (unsigned char) c, end - start);
}

void
_sd_overwrite (rw_string_desc_t s, idx_t start, idx_t t_nbytes, const char *t_data)
{
  if (!(start >= 0 && start + t_nbytes <= s._nbytes))
    /* Invalid arguments.  */
    abort ();

  if (t_nbytes > 0)
    memcpy (s._data + start, t_data, t_nbytes);
}

void
sd_free (rw_string_desc_t s)
{
  free (s._data);
}
