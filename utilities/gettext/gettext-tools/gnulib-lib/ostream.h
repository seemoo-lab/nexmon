/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

#line 1 "ostream.oo.h"
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

#line 32 "ostream.h"
struct any_ostream_representation;
/* ostream_t is defined as a pointer to struct any_ostream_representation.
   In C++ mode, we use a smart pointer class.
   In C mode, we have no other choice than a typedef to the root class type.  */
#if IS_CPLUSPLUS
struct ostream_t
{
private:
  struct any_ostream_representation *_pointer;
public:
  ostream_t () : _pointer (NULL) {}
  ostream_t (struct any_ostream_representation *pointer) : _pointer (pointer) {}
  struct any_ostream_representation * operator -> () { return _pointer; }
  operator struct any_ostream_representation * () { return _pointer; }
  operator void * () { return _pointer; }
  bool operator == (const void *p) { return _pointer == p; }
  bool operator != (const void *p) { return _pointer != p; }
};
#else
typedef struct any_ostream_representation * ostream_t;
#endif

/* Functions that invoke the methods.  */
extern        void ostream_write_mem (ostream_t first_arg, const void *data, size_t len);
extern         void ostream_flush (ostream_t first_arg);
extern         void ostream_free (ostream_t first_arg);

/* Type representing an implementation of ostream_t.  */
struct ostream_implementation
{
  const typeinfo_t * const *superclasses;
  size_t superclasses_length;
  size_t instance_size;
#define THIS_ARG ostream_t first_arg
#include "ostream.vt.h"
#undef THIS_ARG
};

/* Public portion of the object pointed to by a ostream_t.  */
struct ostream_representation_header
{
  const struct ostream_implementation *vtable;
};

#if HAVE_INLINE

/* Define the functions that invoke the methods as inline accesses to
   the ostream_implementation.
   Use #define to avoid a warning because of extern vs. static.  */

# define ostream_write_mem ostream_write_mem_inline
static inline void
ostream_write_mem (ostream_t first_arg, const void *data, size_t len)
{
  const struct ostream_implementation *vtable =
    ((struct ostream_representation_header *) (struct any_ostream_representation *) first_arg)->vtable;
  vtable->write_mem (first_arg,data,len);
}

# define ostream_flush ostream_flush_inline
static inline void
ostream_flush (ostream_t first_arg)
{
  const struct ostream_implementation *vtable =
    ((struct ostream_representation_header *) (struct any_ostream_representation *) first_arg)->vtable;
  vtable->flush (first_arg);
}

# define ostream_free ostream_free_inline
static inline void
ostream_free (ostream_t first_arg)
{
  const struct ostream_implementation *vtable =
    ((struct ostream_representation_header *) (struct any_ostream_representation *) first_arg)->vtable;
  vtable->free (first_arg);
}

#endif

extern const typeinfo_t ostream_typeinfo;
#define ostream_SUPERCLASSES &ostream_typeinfo, NULL
#define ostream_SUPERCLASSES_LENGTH (1 + 1)

extern const struct ostream_implementation ostream_vtable;

#line 41 "ostream.oo.h"

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
