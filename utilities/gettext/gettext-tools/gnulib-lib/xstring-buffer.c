/* Error-checking functions on a string buffer.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

/* Specification.  */
#include "string-buffer.h"

#include "xalloc.h"

void
sb_xappend1 (struct string_buffer *buffer, char c)
{
  if (sb_append1 (buffer, c) < 0)
    xalloc_die ();
}

void
sb_xappend_desc (struct string_buffer *buffer, string_desc_t s)
{
  if (sb_append_desc (buffer, s) < 0)
    xalloc_die ();
}

void
sb_xappend_c (struct string_buffer *buffer, const char *str)
{
  if (sb_append_c (buffer, str) < 0)
    xalloc_die ();
}

const char *
sb_xcontents_c (struct string_buffer *buffer)
{
  const char *contents = sb_contents_c (buffer);
  if (contents == NULL)
    xalloc_die ();
  return contents;
}

rw_string_desc_t
sb_xdupfree (struct string_buffer *buffer)
{
  if (buffer->error)
    {
      sb_free (buffer);
      return sd_readwrite (sd_new_empty ());
    }
  rw_string_desc_t contents = sb_dupfree (buffer);
  if (sd_data (contents) == NULL)
    xalloc_die ();
  return contents;
}

char *
sb_xdupfree_c (struct string_buffer *buffer)
{
  if (buffer->error)
    {
      sb_free (buffer);
      return NULL;
    }
  char *contents = sb_dupfree_c (buffer);
  if (contents == NULL)
    xalloc_die ();
  return contents;
}
