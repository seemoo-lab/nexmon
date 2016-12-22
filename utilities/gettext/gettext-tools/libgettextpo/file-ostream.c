/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

#line 1 "file-ostream.oo.c"
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

#line 31 "file-ostream.c"
#if !IS_CPLUSPLUS
#define file_ostream_representation any_ostream_representation
#endif
#include "file_ostream.priv.h"

const typeinfo_t file_ostream_typeinfo = { "file_ostream" };

static const typeinfo_t * const file_ostream_superclasses[] =
  { file_ostream_SUPERCLASSES };

#define super ostream_vtable

#line 32 "file-ostream.oo.c"

/* Implementation of ostream_t methods.  */

static void
file_ostream__write_mem (file_ostream_t stream, const void *data, size_t len)
{
  if (len > 0)
    fwrite (data, 1, len, stream->fp);
}

static void
file_ostream__flush (file_ostream_t stream)
{
  /* This ostream has no internal buffer.  No need to fflush (stream->fp),
     since it's external to this ostream.  */
}

static void
file_ostream__free (file_ostream_t stream)
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

#line 81 "file-ostream.c"

const struct file_ostream_implementation file_ostream_vtable =
{
  file_ostream_superclasses,
  sizeof (file_ostream_superclasses) / sizeof (file_ostream_superclasses[0]),
  sizeof (struct file_ostream_representation),
  file_ostream__write_mem,
  file_ostream__flush,
  file_ostream__free,
};

#if !HAVE_INLINE

/* Define the functions that invoke the methods.  */

void
file_ostream_write_mem (file_ostream_t first_arg, const void *data, size_t len)
{
  const struct file_ostream_implementation *vtable =
    ((struct file_ostream_representation_header *) (struct file_ostream_representation *) first_arg)->vtable;
  vtable->write_mem (first_arg,data,len);
}

void
file_ostream_flush (file_ostream_t first_arg)
{
  const struct file_ostream_implementation *vtable =
    ((struct file_ostream_representation_header *) (struct file_ostream_representation *) first_arg)->vtable;
  vtable->flush (first_arg);
}

void
file_ostream_free (file_ostream_t first_arg)
{
  const struct file_ostream_implementation *vtable =
    ((struct file_ostream_representation_header *) (struct file_ostream_representation *) first_arg)->vtable;
  vtable->free (first_arg);
}

#endif
