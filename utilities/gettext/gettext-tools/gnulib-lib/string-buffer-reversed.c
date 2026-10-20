/* A buffer that accumulates a string by piecewise concatenation, from the end
   to the start.
   Copyright (C) 2021-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

#include <config.h>

/* Specification.  */
#include "string-buffer-reversed.h"

/* Undocumented.  */
extern int sbr_ensure_more_bytes (struct string_buffer_reversed *buffer,
                                  size_t increment);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The warnings about memory resource 'buffer->data' in this file are not
   relevant.  Silence them.  */
#if __clang_major__ >= 3
# pragma clang diagnostic ignored "-Wthread-safety"
#endif

void
sbr_init (struct string_buffer_reversed *buffer)
{
  buffer->data = buffer->space;
  /* Pre-allocate a trailing NUL.  This makes it easy to implement
     sbr_contents_c().  */
  buffer->data[sizeof (buffer->space) - 1] = '\0';
  buffer->length = 1;
  buffer->allocated = sizeof (buffer->space);
  buffer->oom = false;
  buffer->error = false;
}

/* Ensures that INCREMENT bytes are available beyond the current used length
   of BUFFER.
   Returns 0, or -1 in case of out-of-memory error.  */
int
sbr_ensure_more_bytes (struct string_buffer_reversed *buffer, size_t increment)
{
  size_t incremented_length = increment + buffer->length;
  if (incremented_length < increment)
    /* Overflow.  */
    return -1;

  if (buffer->allocated < incremented_length)
    {
      size_t new_allocated = 2 * buffer->allocated;
      if (new_allocated < buffer->allocated)
        /* Overflow.  */
        return -1;
      if (new_allocated < incremented_length)
        new_allocated = incremented_length;

      char *new_data;
      if (buffer->data == buffer->space)
        {
          new_data = (char *) malloc (new_allocated);
          if (new_data == NULL)
            /* Out-of-memory.  */
            return -1;
          memcpy (new_data + new_allocated - buffer->length,
                  buffer->data + buffer->allocated - buffer->length,
                  buffer->length);
        }
      else
        {
          new_data = (char *) realloc (buffer->data, new_allocated);
          if (new_data == NULL)
            /* Out-of-memory.  */
            return -1;
          memmove (new_data + new_allocated - buffer->length,
                   new_data + buffer->allocated - buffer->length,
                   buffer->length);
        }
      buffer->data = new_data;
      buffer->allocated = new_allocated;
    }
  return 0;
}

int
sbr_prepend1 (struct string_buffer_reversed *buffer, char c)
{
  if (sbr_ensure_more_bytes (buffer, 1) < 0)
    {
      buffer->oom = true;
      return -1;
    }
  buffer->data[buffer->allocated - buffer->length - 1] = c;
  buffer->length++;
  return 0;
}

int
sbr_prepend_desc (struct string_buffer_reversed *buffer, string_desc_t s)
{
  size_t len = sd_length (s);
  if (sbr_ensure_more_bytes (buffer, len) < 0)
    {
      buffer->oom = true;
      return -1;
    }
  memcpy (buffer->data + buffer->allocated - buffer->length - len, sd_data (s), len);
  buffer->length += len;
  return 0;
}

int
sbr_prepend_c (struct string_buffer_reversed *buffer, const char *str)
{
  size_t len = strlen (str);
  if (sbr_ensure_more_bytes (buffer, len) < 0)
    {
      buffer->oom = true;
      return -1;
    }
  memcpy (buffer->data + buffer->allocated - buffer->length - len, str, len);
  buffer->length += len;
  return 0;
}

void
sbr_free (struct string_buffer_reversed *buffer)
{
  if (buffer->data != buffer->space)
    free (buffer->data);
}

string_desc_t
sbr_contents (struct string_buffer_reversed *buffer)
{
  return sd_new_addr (buffer->length - 1,
                      (const char *) buffer->data + buffer->allocated - buffer->length);
}

const char *
sbr_contents_c (struct string_buffer_reversed *buffer)
{
  return buffer->data + buffer->allocated - buffer->length;
}

rw_string_desc_t
sbr_dupfree (struct string_buffer_reversed *buffer)
{
  if (buffer->oom || buffer->error)
    goto fail;

  size_t length = buffer->length;
  if (buffer->data == buffer->space)
    {
      char *copy = (char *) malloc (length > 1 ? length - 1 : 1);
      if (copy == NULL)
        goto fail;
      memcpy (copy, buffer->data + buffer->allocated - length, length - 1);
      return sd_new_addr (length - 1, copy);
    }
  else
    {
      /* Shrink the string before returning it.  */
      char *contents = buffer->data;
      memmove (contents, contents + buffer->allocated - length, length - 1);
      contents = realloc (contents, length > 1 ? length - 1 : 1);
      if (contents == NULL)
        goto fail;
      return sd_new_addr (length - 1, contents);
    }

 fail:
  sbr_free (buffer);
  return sd_readwrite (sd_new_empty ());
}

char *
sbr_dupfree_c (struct string_buffer_reversed *buffer)
{
  if (buffer->oom || buffer->error)
    goto fail;

  size_t length = buffer->length;
  if (buffer->data == buffer->space)
    {
      char *copy = (char *) malloc (length);
      if (copy == NULL)
        goto fail;
      memcpy (copy, buffer->data + buffer->allocated - length, length);
      return copy;
    }
  else
    {
      /* Shrink the string before returning it.  */
      char *contents = buffer->data;
      if (length < buffer->allocated)
        {
          memmove (contents, contents + buffer->allocated - length, length);
          contents = realloc (contents, length);
          if (contents == NULL)
            goto fail;
        }
      return contents;
    }

 fail:
  sbr_free (buffer);
  return NULL;
}
