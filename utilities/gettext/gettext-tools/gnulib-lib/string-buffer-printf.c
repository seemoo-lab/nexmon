/* A buffer that accumulates a string by piecewise concatenation.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2021.  */

#include <config.h>

/* Specification.  */
#include "string-buffer.h"

/* Undocumented.  */
extern int sb_ensure_more_bytes (struct string_buffer *buffer,
                                 size_t increment);

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int
sb_appendvf (struct string_buffer *buffer, const char *formatstring,
             va_list list)
{
  /* Make a bit of room, so that the probability that the first vsnzprintf()
     call succeeds is high.  */
  size_t room = buffer->allocated - buffer->length;
  if (room < 64)
    {
      if (sb_ensure_more_bytes (buffer, 64) < 0)
        {
          buffer->oom = true;
          errno = ENOMEM;
          return -1;
        }
      room = buffer->allocated - buffer->length;
    }

  va_list list_copy;
  va_copy (list_copy, list);

  /* First vsnzprintf() call.  */
  ptrdiff_t ret = vsnzprintf (buffer->data + buffer->length, room,
                              formatstring, list);
  if (ret < 0)
    {
      /* Failed.  errno is set.  */
      if (errno == ENOMEM)
        buffer->oom = true;
      else
        buffer->error = true;
      ret = -1;
    }
  else
    {
      if (ret <= room)
        {
          /* The result has fit into room bytes.  */
          buffer->length += (size_t) ret;
          ret = 0;
        }
      else
        {
          /* The result was truncated.  Make more room, for a second
             vsnzprintf() call.  */
          if (sb_ensure_more_bytes (buffer, (size_t) ret) < 0)
            {
              buffer->oom = true;
              errno = ENOMEM;
              ret = -1;
            }
          else
            {
              /* Second vsnzprintf() call.  */
              room = buffer->allocated - buffer->length;
              ret = vsnzprintf (buffer->data + buffer->length, room,
                                formatstring, list_copy);
              if (ret < 0)
                {
                  /* Failed.  errno is set.  */
                  if (errno == ENOMEM)
                    buffer->oom = true;
                  else
                    buffer->error = true;
                  ret = -1;
                }
              else
                {
                  if (ret <= room)
                    {
                      /* The result has fit into room bytes.  */
                      buffer->length += (size_t) ret;
                      ret = 0;
                    }
                  else
                    /* The return values of the vsnzprintf() calls are not
                       consistent.  */
                    abort ();
                }
            }
        }
    }

  va_end (list_copy);
  return ret;
}

int
sb_appendf (struct string_buffer *buffer, const char *formatstring, ...)
{
  /* Make a bit of room, so that the probability that the first vsnzprintf()
     call succeeds is high.  */
  size_t room = buffer->allocated - buffer->length;
  if (room < 64)
    {
      if (sb_ensure_more_bytes (buffer, 64) < 0)
        {
          buffer->oom = true;
          errno = ENOMEM;
          return -1;
        }
      room = buffer->allocated - buffer->length;
    }

  va_list args;
  va_start (args, formatstring);

  /* First vsnzprintf() call.  */
  ptrdiff_t ret = vsnzprintf (buffer->data + buffer->length, room,
                              formatstring, args);
  if (ret < 0)
    {
      /* Failed.  errno is set.  */
      if (errno == ENOMEM)
        buffer->oom = true;
      else
        buffer->error = true;
      ret = -1;
    }
  else
    {
      if (ret <= room)
        {
          /* The result has fit into room bytes.  */
          buffer->length += (size_t) ret;
          ret = 0;
        }
      else
        {
          /* The result was truncated.  Make more room, for a second
             vsnzprintf() call.  */
          if (sb_ensure_more_bytes (buffer, (size_t) ret) < 0)
            {
              buffer->oom = true;
              errno = ENOMEM;
              ret = -1;
            }
          else
            {
              /* Second vsnzprintf() call.  */
              room = buffer->allocated - buffer->length;
              va_end (args);
              va_start (args, formatstring);
              ret = vsnzprintf (buffer->data + buffer->length, room,
                                formatstring, args);
              if (ret < 0)
                {
                  /* Failed.  errno is set.  */
                  if (errno == ENOMEM)
                    buffer->oom = true;
                  else
                    buffer->error = true;
                  ret = -1;
                }
              else
                {
                  if (ret <= room)
                    {
                      /* The result has fit into room bytes.  */
                      buffer->length += (size_t) ret;
                      ret = 0;
                    }
                  else
                    /* The return values of the vsnzprintf() calls are not
                       consistent.  */
                    abort ();
                }
            }
        }
    }

  va_end (args);
  return ret;
}
