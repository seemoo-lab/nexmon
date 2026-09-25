/* Error-checking functions on a string buffer that accumulates from the end.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

#include <config.h>

/* Specification.  */
#include "string-buffer-reversed.h"

#include "xalloc.h"

void
sbr_xprepend1 (struct string_buffer_reversed *buffer, char c)
{
  if (sbr_prepend1 (buffer, c) < 0)
    xalloc_die ();
}

void
sbr_xprepend_desc (struct string_buffer_reversed *buffer, string_desc_t s)
{
  if (sbr_prepend_desc (buffer, s) < 0)
    xalloc_die ();
}

void
sbr_xprepend_c (struct string_buffer_reversed *buffer, const char *str)
{
  if (sbr_prepend_c (buffer, str) < 0)
    xalloc_die ();
}

rw_string_desc_t
sbr_xdupfree (struct string_buffer_reversed *buffer)
{
  if (buffer->error)
    {
      sbr_free (buffer);
      return sd_readwrite (sd_new_empty ());
    }
  rw_string_desc_t contents = sbr_dupfree (buffer);
  if (sd_data (contents) == NULL)
    xalloc_die ();
  return contents;
}

char *
sbr_xdupfree_c (struct string_buffer_reversed *buffer)
{
  if (buffer->error)
    {
      sbr_free (buffer);
      return NULL;
    }
  char *contents = sbr_dupfree_c (buffer);
  if (contents == NULL)
    xalloc_die ();
  return contents;
}
