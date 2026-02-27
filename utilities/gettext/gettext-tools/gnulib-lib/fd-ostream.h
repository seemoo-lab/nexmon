/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

#line 1 "fd-ostream.oo.h"
/* Output stream referring to a file descriptor.
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

#ifndef _FD_OSTREAM_H
#define _FD_OSTREAM_H

#include <stdbool.h>

#include "ostream.h"


#line 30 "fd-ostream.h"
struct fd_ostream_representation;
/* fd_ostream_t is defined as a pointer to struct fd_ostream_representation.
   In C++ mode, we use a smart pointer class.
   In C mode, we have no other choice than a typedef to the root class type.  */
#if IS_CPLUSPLUS
struct fd_ostream_t
{
private:
  struct fd_ostream_representation *_pointer;
public:
  fd_ostream_t () : _pointer (NULL) {}
  fd_ostream_t (struct fd_ostream_representation *pointer) : _pointer (pointer) {}
  struct fd_ostream_representation * operator -> () { return _pointer; }
  operator struct fd_ostream_representation * () { return _pointer; }
  operator struct any_ostream_representation * () { return (struct any_ostream_representation *) _pointer; }
  operator void * () { return _pointer; }
  bool operator == (const void *p) { return _pointer == p; }
  bool operator != (const void *p) { return _pointer != p; }
  operator ostream_t () { return (ostream_t) (struct any_ostream_representation *) _pointer; }
  explicit fd_ostream_t (ostream_t x) : _pointer ((struct fd_ostream_representation *) (void *) x) {}
};
#else
typedef ostream_t fd_ostream_t;
#endif

/* Functions that invoke the methods.  */
extern        void fd_ostream_write_mem (fd_ostream_t first_arg, const void *data, size_t len);
extern         void fd_ostream_flush (fd_ostream_t first_arg);
extern         void fd_ostream_free (fd_ostream_t first_arg);

/* Type representing an implementation of fd_ostream_t.  */
struct fd_ostream_implementation
{
  const typeinfo_t * const *superclasses;
  size_t superclasses_length;
  size_t instance_size;
#define THIS_ARG fd_ostream_t first_arg
#include "fd_ostream.vt.h"
#undef THIS_ARG
};

/* Public portion of the object pointed to by a fd_ostream_t.  */
struct fd_ostream_representation_header
{
  const struct fd_ostream_implementation *vtable;
};

#if HAVE_INLINE

/* Define the functions that invoke the methods as inline accesses to
   the fd_ostream_implementation.
   Use #define to avoid a warning because of extern vs. static.  */

# define fd_ostream_write_mem fd_ostream_write_mem_inline
static inline void
fd_ostream_write_mem (fd_ostream_t first_arg, const void *data, size_t len)
{
  const struct fd_ostream_implementation *vtable =
    ((struct fd_ostream_representation_header *) (struct fd_ostream_representation *) first_arg)->vtable;
  vtable->write_mem (first_arg,data,len);
}

# define fd_ostream_flush fd_ostream_flush_inline
static inline void
fd_ostream_flush (fd_ostream_t first_arg)
{
  const struct fd_ostream_implementation *vtable =
    ((struct fd_ostream_representation_header *) (struct fd_ostream_representation *) first_arg)->vtable;
  vtable->flush (first_arg);
}

# define fd_ostream_free fd_ostream_free_inline
static inline void
fd_ostream_free (fd_ostream_t first_arg)
{
  const struct fd_ostream_implementation *vtable =
    ((struct fd_ostream_representation_header *) (struct fd_ostream_representation *) first_arg)->vtable;
  vtable->free (first_arg);
}

#endif

extern const typeinfo_t fd_ostream_typeinfo;
#define fd_ostream_SUPERCLASSES &fd_ostream_typeinfo, ostream_SUPERCLASSES
#define fd_ostream_SUPERCLASSES_LENGTH (1 + ostream_SUPERCLASSES_LENGTH)

extern const struct fd_ostream_implementation fd_ostream_vtable;

#line 30 "fd-ostream.oo.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Create an output stream referring to the file descriptor FD.
   FILENAME is used only for error messages.
   Note that the resulting stream must be closed before FD can be closed.  */
extern fd_ostream_t fd_ostream_create (int fd, const char *filename,
                                       bool buffered);


#ifdef __cplusplus
}
#endif

#endif /* _FD_OSTREAM_H */
