/* strlist.c - Argument Parser for option handling
 * Copyright (C) 1998, 2000, 2001, 2006 Free Software Foundation, Inc.
 * Copyright (C) 2015, 2024, 2025  g10 Code GmbH
 *
 * This file is part of Libgpg-error.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file was originally a part of GnuPG.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "gpgrt-int.h"

#define SL_PRIV_FLAG_WIPE   0x01


void
_gpgrt_strlist_free (gpgrt_strlist_t sl)
{
  gpgrt_strlist_t sl2;

  for (; sl; sl = sl2)
    {
      sl2 = sl->next;
      if ((sl->_private_flags & ~SL_PRIV_FLAG_WIPE))
        _gpgrt_log_fatal ("gpgrt_strlist_free: corrupted object %p\n", sl);

      if ((sl->_private_flags & SL_PRIV_FLAG_WIPE))
        _gpgrt_wipememory (sl, sizeof *sl + strlen (sl->d));
      xfree(sl);
    }
}


/* Core of gpgrt_strlist_append which take the length of the string.
 * Return the item added to the end of the list.  Or NULL in case of
 * an error.  */
static gpgrt_strlist_t
do_strlist_append (gpgrt_strlist_t *list, const char *string, size_t stringlen,
                   unsigned int flags)
{
  gpgrt_strlist_t r, sl;

  sl = xtrymalloc (sizeof *sl + stringlen);
  if (!sl)
    return NULL;

  sl->flags = 0;
  sl->_private_flags = (flags & GPGRT_STRLIST_WIPE)? SL_PRIV_FLAG_WIPE : 0;
  memcpy (sl->d, string, stringlen);
  sl->d[stringlen] = 0;
  sl->next = NULL;
  if (!*list)
    *list = sl;
  else
    {
      for (r = *list; r->next; r = r->next)
        ;
      r->next = sl;
    }
  return sl;
}


/* Add STRING to the LIST.  This function returns NULL and sets ERRNO
 * on memory shortage.  If STRING is NULL an empty string is stored
 * instead.  FLAGS are these bits:
 *  GPGRT_STRLIST_APPEND  - Append to the list; default is to prepend
 *  GPGRT_STRLIST_WIPE    - Set a marker to wipe the string on free.
 */
gpgrt_strlist_t
_gpgrt_strlist_add (gpgrt_strlist_t *list, const char *string,
                    unsigned int flags)
{
  gpgrt_strlist_t sl;

  if (!string)
    string = "";

  if ((flags & GPGRT_STRLIST_APPEND))
    return do_strlist_append (list, string, strlen (string), flags);

  /* Default is to prepend.  */
  sl = xtrymalloc (sizeof *sl + strlen (string));
  if (sl)
    {
      sl->flags = 0;
      sl->_private_flags = (flags & GPGRT_STRLIST_WIPE)? SL_PRIV_FLAG_WIPE : 0;
      strcpy (sl->d, string);
      sl->next = *list;
      *list = sl;
    }
  return sl;
}

/* Tokenize STRING using the delimiters from DELIM and append each
 * token to the string list LIST.  On success a pointer into LIST with
 * the first new token is returned.  Returns NULL on error and sets
 * ERRNO.  Take care, an error with ENOENT set mean that no tokens
 * were found in STRING.  Only GPGRT_STRLIST_WIPE has an effect here.  */
gpgrt_strlist_t
_gpgrt_strlist_tokenize (gpgrt_strlist_t *list, const char *string,
                         const char *delim, unsigned int flags)
{
  const char *s, *se;
  size_t n;
  gpgrt_strlist_t newlist = NULL;
  gpgrt_strlist_t tail;

  if (!string)
    string = "";

  s = string;
  do
    {
      se = strpbrk (s, delim);
      if (se)
        n = se - s;
      else
        n = strlen (s);
      if (!n)
        continue;  /* Skip empty string.  */
      tail = do_strlist_append (&newlist, s, n, flags);
      if (!tail)
        {
          _gpgrt_strlist_free (newlist);
          return NULL;
        }
      _gpgrt_trim_spaces (tail->d);
      if (!*tail->d)  /* Remove new but empty item from the list.  */
        {
          tail = _gpgrt_strlist_prev (newlist, tail);
          if (tail)
            {
              _gpgrt_strlist_free (tail->next);
              tail->next = NULL;
            }
          else if (newlist)
            {
              _gpgrt_strlist_free (newlist);
              newlist = NULL;
            }
          continue;
        }
    }
  while (se && (s = se + 1));

  if (!newlist)
    {
      /* Not items found.  Indicate this by returnning NULL with errno
       * set to ENOENT.  */
      _gpg_err_set_errno (ENOENT);
      return NULL;
    }

  /* Append NEWLIST to LIST.  */
  if (!*list)
    *list = newlist;
  else
    {
      for (tail = *list; tail->next; tail = tail->next)
        ;
      tail->next = newlist;
    }
  return newlist;
}


/* Return a copy of LIST.  On error set ERRNO and return NULL.  */
gpgrt_strlist_t
_gpgrt_strlist_copy (gpgrt_strlist_t list)
{
  gpgrt_strlist_t newlist = NULL;
  gpgrt_strlist_t sl, *last;

  last = &newlist;
  for (; list; list = list->next)
    {
      sl = xtrymalloc (sizeof *sl + strlen (list->d));
      if (!sl)
        {
          _gpgrt_strlist_free (newlist);
          return NULL;
        }
      sl->flags = list->flags;
      sl->_private_flags = list->_private_flags;
      strcpy (sl->d, list->d);
      sl->next = NULL;
      *last = sl;
      last = &sl;
    }
  return newlist;
}

/* Reverse the list *LIST in place.  */
gpgrt_strlist_t
_gpgrt_strlist_rev (gpgrt_strlist_t *list)
{
  gpgrt_strlist_t l = *list;
  gpgrt_strlist_t lrev = NULL;

  while (l)
    {
      gpgrt_strlist_t tail = l->next;
      l->next = lrev;
      lrev = l;
      l = tail;
    }

  *list = lrev;
  return lrev;
}


gpgrt_strlist_t
_gpgrt_strlist_prev (gpgrt_strlist_t head, gpgrt_strlist_t node)
{
  gpgrt_strlist_t n = NULL;

  for (; head && head != node; head = head->next )
    n = head;
  return n;
}


gpgrt_strlist_t
_gpgrt_strlist_last (gpgrt_strlist_t node)
{
  if (node)
    for (; node->next ; node = node->next)
      ;
  return node;
}


/* Remove the first item from LIST and return its content in an
 * allocated buffer.  This function returns NULl and sets ERRNO on
 * error.  */
char *
_gpgrt_strlist_pop (gpgrt_strlist_t *list)
{
  char *str = NULL;
  gpgrt_strlist_t sl = *list;

  if (sl)
    {
      str = xtrystrdup (sl->d);
      if (!str)
        return NULL;

      *list = sl->next;
      sl->next = NULL;
      xfree (sl);
    }

  return str;
}


/* Return the first element of the string list HAYSTACK whose string
   matches NEEDLE.  If no elements match, return NULL.  */
gpgrt_strlist_t
_gpgrt_strlist_find (gpgrt_strlist_t haystack, const char *needle)
{
  for (; haystack; haystack = haystack->next)
    if (!strcmp (haystack->d, needle))
      return haystack;
  return NULL;
}
