/* Output stream referring to an stdio FILE.
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

#include <config.h>

/* Specification.  */
#include "file-ostream.h"

#include <stdlib.h>

#include "xalloc.h"

struct file_ostream : struct ostream
{
fields:
  FILE *fp;
};

/* Implementation of ostream_t methods.  */

static void
file_ostream::write_mem (file_ostream_t stream, const void *data, size_t len)
{
  if (len > 0)
    fwrite (data, 1, len, stream->fp);
}

static void
file_ostream::flush (file_ostream_t stream)
{
  /* This ostream has no internal buffer.  No need to fflush (stream->fp),
     since it's external to this ostream.  */
}

static void
file_ostream::free (file_ostream_t stream)
{
  free (stream);
}

/* Constructor.  */

file_ostream_t
file_ostream_create (FILE *fp)
{
  file_ostream_t stream = XMALLOC (struct file_ostream_representation);

  stream->base.vtable = &file_ostream_vtable;
  stream->fp = fp;

  return stream;
}
