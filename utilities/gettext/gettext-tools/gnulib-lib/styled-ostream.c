/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

#line 1 "styled-ostream.oo.c"
/* Abstract output stream for CSS styled text.
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
#include "styled-ostream.h"


#line 28 "styled-ostream.c"
#if !IS_CPLUSPLUS
#define styled_ostream_representation any_ostream_representation
#endif
#include "styled_ostream.priv.h"

const typeinfo_t styled_ostream_typeinfo = { "styled_ostream" };

static const typeinfo_t * const styled_ostream_superclasses[] =
  { styled_ostream_SUPERCLASSES };

#define super ostream_vtable

#line 28 "styled-ostream.oo.c"

#line 43 "styled-ostream.c"
void styled_ostream__write_mem (styled_ostream_t first_arg, const void *data, size_t len);
void
styled_ostream__write_mem (styled_ostream_t first_arg, const void *data, size_t len)
{
  super.write_mem (first_arg,data,len);
}

void styled_ostream__flush (styled_ostream_t first_arg);
void
styled_ostream__flush (styled_ostream_t first_arg)
{
  super.flush (first_arg);
}

void styled_ostream__free (styled_ostream_t first_arg);
void
styled_ostream__free (styled_ostream_t first_arg)
{
  super.free (first_arg);
}

void styled_ostream__begin_use_class (styled_ostream_t first_arg, const char *classname);
void
styled_ostream__begin_use_class (styled_ostream_t first_arg, const char *classname)
{
  /* Abstract (unimplemented) method called.  */
  abort ();
  #ifndef __GNUC__
  styled_ostream__begin_use_class (first_arg,classname);
  #endif
}

void styled_ostream__end_use_class (styled_ostream_t first_arg, const char *classname);
void
styled_ostream__end_use_class (styled_ostream_t first_arg, const char *classname)
{
  /* Abstract (unimplemented) method called.  */
  abort ();
  #ifndef __GNUC__
  styled_ostream__end_use_class (first_arg,classname);
  #endif
}


const struct styled_ostream_implementation styled_ostream_vtable =
{
  styled_ostream_superclasses,
  sizeof (styled_ostream_superclasses) / sizeof (styled_ostream_superclasses[0]),
  sizeof (struct styled_ostream_representation),
  styled_ostream__write_mem,
  styled_ostream__flush,
  styled_ostream__free,
  styled_ostream__begin_use_class,
  styled_ostream__end_use_class,
};

#if !HAVE_INLINE

/* Define the functions that invoke the methods.  */

void
styled_ostream_write_mem (styled_ostream_t first_arg, const void *data, size_t len)
{
  const struct styled_ostream_implementation *vtable =
    ((struct styled_ostream_representation_header *) (struct styled_ostream_representation *) first_arg)->vtable;
  vtable->write_mem (first_arg,data,len);
}

void
styled_ostream_flush (styled_ostream_t first_arg)
{
  const struct styled_ostream_implementation *vtable =
    ((struct styled_ostream_representation_header *) (struct styled_ostream_representation *) first_arg)->vtable;
  vtable->flush (first_arg);
}

void
styled_ostream_free (styled_ostream_t first_arg)
{
  const struct styled_ostream_implementation *vtable =
    ((struct styled_ostream_representation_header *) (struct styled_ostream_representation *) first_arg)->vtable;
  vtable->free (first_arg);
}

void
styled_ostream_begin_use_class (styled_ostream_t first_arg, const char *classname)
{
  const struct styled_ostream_implementation *vtable =
    ((struct styled_ostream_representation_header *) (struct styled_ostream_representation *) first_arg)->vtable;
  vtable->begin_use_class (first_arg,classname);
}

void
styled_ostream_end_use_class (styled_ostream_t first_arg, const char *classname)
{
  const struct styled_ostream_implementation *vtable =
    ((struct styled_ostream_representation_header *) (struct styled_ostream_representation *) first_arg)->vtable;
  vtable->end_use_class (first_arg,classname);
}

#endif
