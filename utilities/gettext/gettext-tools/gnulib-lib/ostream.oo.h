/* Abstract output stream data type.
   Copyright (C) 2006, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _OSTREAM_H
#define _OSTREAM_H

#include <stddef.h>
#include <string.h>

#include "moo.h"

/* An output stream is an object to which one can feed a sequence of bytes.  */

struct ostream
{
methods:

  /* Write a sequence of bytes to a stream.  */
  void write_mem (ostream_t stream, const void *data, size_t len);

  /* Bring buffered data to its destination.  */
  void flush (ostream_t stream);

  /* Close and free a stream.  */
  void free (ostream_t stream);
};

#ifdef __cplusplus
extern "C" {
#endif

/* Write a string's contents to a stream.  */
extern void ostream_write_str (ostream_t stream, const char *string);

#if HAVE_INLINE

#define ostream_write_str ostream_write_str_inline
static inline void
ostream_write_str (ostream_t stream, const char *string)
{
  ostream_write_mem (stream, string, strlen (string));
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSTREAM_H */
