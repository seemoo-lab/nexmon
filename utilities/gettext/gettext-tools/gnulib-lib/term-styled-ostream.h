/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

#line 1 "term-styled-ostream.oo.h"
/* Output stream for CSS styled text, producing ANSI escape sequences.
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

#ifndef _TERM_STYLED_OSTREAM_H
#define _TERM_STYLED_OSTREAM_H

#include "styled-ostream.h"


#line 28 "term-styled-ostream.h"
struct term_styled_ostream_representation;
/* term_styled_ostream_t is defined as a pointer to struct term_styled_ostream_representation.
   In C++ mode, we use a smart pointer class.
   In C mode, we have no other choice than a typedef to the root class type.  */
#if IS_CPLUSPLUS
struct term_styled_ostream_t
{
private:
  struct term_styled_ostream_representation *_pointer;
public:
  term_styled_ostream_t () : _pointer (NULL) {}
  term_styled_ostream_t (struct term_styled_ostream_representation *pointer) : _pointer (pointer) {}
  struct term_styled_ostream_representation * operator -> () { return _pointer; }
  operator struct term_styled_ostream_representation * () { return _pointer; }
  operator struct any_ostream_representation * () { return (struct any_ostream_representation *) _pointer; }
  operator struct styled_ostream_representation * () { return (struct styled_ostream_representation *) _pointer; }
  operator void * () { return _pointer; }
  bool operator == (const void *p) { return _pointer == p; }
  bool operator != (const void *p) { return _pointer != p; }
  operator ostream_t () { return (ostream_t) (struct any_ostream_representation *) _pointer; }
  explicit term_styled_ostream_t (ostream_t x) : _pointer ((struct term_styled_ostream_representation *) (void *) x) {}
  operator styled_ostream_t () { return (styled_ostream_t) (struct styled_ostream_representation *) _pointer; }
  explicit term_styled_ostream_t (styled_ostream_t x) : _pointer ((struct term_styled_ostream_representation *) (void *) x) {}
};
#else
typedef styled_ostream_t term_styled_ostream_t;
#endif

/* Functions that invoke the methods.  */
extern        void term_styled_ostream_write_mem (term_styled_ostream_t first_arg, const void *data, size_t len);
extern         void term_styled_ostream_flush (term_styled_ostream_t first_arg);
extern         void term_styled_ostream_free (term_styled_ostream_t first_arg);
extern          void term_styled_ostream_begin_use_class (term_styled_ostream_t first_arg, const char *classname);
extern          void term_styled_ostream_end_use_class (term_styled_ostream_t first_arg, const char *classname);

/* Type representing an implementation of term_styled_ostream_t.  */
struct term_styled_ostream_implementation
{
  const typeinfo_t * const *superclasses;
  size_t superclasses_length;
  size_t instance_size;
#define THIS_ARG term_styled_ostream_t first_arg
#include "term_styled_ostream.vt.h"
#undef THIS_ARG
};

/* Public portion of the object pointed to by a term_styled_ostream_t.  */
struct term_styled_ostream_representation_header
{
  const struct term_styled_ostream_implementation *vtable;
};

#if HAVE_INLINE

/* Define the functions that invoke the methods as inline accesses to
   the term_styled_ostream_implementation.
   Use #define to avoid a warning because of extern vs. static.  */

# define term_styled_ostream_write_mem term_styled_ostream_write_mem_inline
static inline void
term_styled_ostream_write_mem (term_styled_ostream_t first_arg, const void *data, size_t len)
{
  const struct term_styled_ostream_implementation *vtable =
    ((struct term_styled_ostream_representation_header *) (struct term_styled_ostream_representation *) first_arg)->vtable;
  vtable->write_mem (first_arg,data,len);
}

# define term_styled_ostream_flush term_styled_ostream_flush_inline
static inline void
term_styled_ostream_flush (term_styled_ostream_t first_arg)
{
  const struct term_styled_ostream_implementation *vtable =
    ((struct term_styled_ostream_representation_header *) (struct term_styled_ostream_representation *) first_arg)->vtable;
  vtable->flush (first_arg);
}

# define term_styled_ostream_free term_styled_ostream_free_inline
static inline void
term_styled_ostream_free (term_styled_ostream_t first_arg)
{
  const struct term_styled_ostream_implementation *vtable =
    ((struct term_styled_ostream_representation_header *) (struct term_styled_ostream_representation *) first_arg)->vtable;
  vtable->free (first_arg);
}

# define term_styled_ostream_begin_use_class term_styled_ostream_begin_use_class_inline
static inline void
term_styled_ostream_begin_use_class (term_styled_ostream_t first_arg, const char *classname)
{
  const struct term_styled_ostream_implementation *vtable =
    ((struct term_styled_ostream_representation_header *) (struct term_styled_ostream_representation *) first_arg)->vtable;
  vtable->begin_use_class (first_arg,classname);
}

# define term_styled_ostream_end_use_class term_styled_ostream_end_use_class_inline
static inline void
term_styled_ostream_end_use_class (term_styled_ostream_t first_arg, const char *classname)
{
  const struct term_styled_ostream_implementation *vtable =
    ((struct term_styled_ostream_representation_header *) (struct term_styled_ostream_representation *) first_arg)->vtable;
  vtable->end_use_class (first_arg,classname);
}

#endif

extern const typeinfo_t term_styled_ostream_typeinfo;
#define term_styled_ostream_SUPERCLASSES &term_styled_ostream_typeinfo, styled_ostream_SUPERCLASSES
#define term_styled_ostream_SUPERCLASSES_LENGTH (1 + styled_ostream_SUPERCLASSES_LENGTH)

extern const struct term_styled_ostream_implementation term_styled_ostream_vtable;

#line 28 "term-styled-ostream.oo.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Create an output stream referring to the file descriptor FD, styled with
   the file CSS_FILENAME.
   FILENAME is used only for error messages.
   Note that the resulting stream must be closed before FD can be closed.
   Return NULL upon failure.  */
extern term_styled_ostream_t
       term_styled_ostream_create (int fd, const char *filename,
                                   const char *css_filename);


#ifdef __cplusplus
}
#endif

#endif /* _TERM_STYLED_OSTREAM_H */
