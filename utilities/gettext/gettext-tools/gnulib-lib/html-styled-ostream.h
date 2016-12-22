/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

#line 1 "html-styled-ostream.oo.h"
/* Output stream for CSS styled text, producing HTML output.
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

#ifndef _HTML_STYLED_OSTREAM_H
#define _HTML_STYLED_OSTREAM_H

#include "styled-ostream.h"


#line 28 "html-styled-ostream.h"
struct html_styled_ostream_representation;
/* html_styled_ostream_t is defined as a pointer to struct html_styled_ostream_representation.
   In C++ mode, we use a smart pointer class.
   In C mode, we have no other choice than a typedef to the root class type.  */
#if IS_CPLUSPLUS
struct html_styled_ostream_t
{
private:
  struct html_styled_ostream_representation *_pointer;
public:
  html_styled_ostream_t () : _pointer (NULL) {}
  html_styled_ostream_t (struct html_styled_ostream_representation *pointer) : _pointer (pointer) {}
  struct html_styled_ostream_representation * operator -> () { return _pointer; }
  operator struct html_styled_ostream_representation * () { return _pointer; }
  operator struct any_ostream_representation * () { return (struct any_ostream_representation *) _pointer; }
  operator struct styled_ostream_representation * () { return (struct styled_ostream_representation *) _pointer; }
  operator void * () { return _pointer; }
  bool operator == (const void *p) { return _pointer == p; }
  bool operator != (const void *p) { return _pointer != p; }
  operator ostream_t () { return (ostream_t) (struct any_ostream_representation *) _pointer; }
  explicit html_styled_ostream_t (ostream_t x) : _pointer ((struct html_styled_ostream_representation *) (void *) x) {}
  operator styled_ostream_t () { return (styled_ostream_t) (struct styled_ostream_representation *) _pointer; }
  explicit html_styled_ostream_t (styled_ostream_t x) : _pointer ((struct html_styled_ostream_representation *) (void *) x) {}
};
#else
typedef styled_ostream_t html_styled_ostream_t;
#endif

/* Functions that invoke the methods.  */
extern        void html_styled_ostream_write_mem (html_styled_ostream_t first_arg, const void *data, size_t len);
extern         void html_styled_ostream_flush (html_styled_ostream_t first_arg);
extern         void html_styled_ostream_free (html_styled_ostream_t first_arg);
extern          void html_styled_ostream_begin_use_class (html_styled_ostream_t first_arg, const char *classname);
extern          void html_styled_ostream_end_use_class (html_styled_ostream_t first_arg, const char *classname);

/* Type representing an implementation of html_styled_ostream_t.  */
struct html_styled_ostream_implementation
{
  const typeinfo_t * const *superclasses;
  size_t superclasses_length;
  size_t instance_size;
#define THIS_ARG html_styled_ostream_t first_arg
#include "html_styled_ostream.vt.h"
#undef THIS_ARG
};

/* Public portion of the object pointed to by a html_styled_ostream_t.  */
struct html_styled_ostream_representation_header
{
  const struct html_styled_ostream_implementation *vtable;
};

#if HAVE_INLINE

/* Define the functions that invoke the methods as inline accesses to
   the html_styled_ostream_implementation.
   Use #define to avoid a warning because of extern vs. static.  */

# define html_styled_ostream_write_mem html_styled_ostream_write_mem_inline
static inline void
html_styled_ostream_write_mem (html_styled_ostream_t first_arg, const void *data, size_t len)
{
  const struct html_styled_ostream_implementation *vtable =
    ((struct html_styled_ostream_representation_header *) (struct html_styled_ostream_representation *) first_arg)->vtable;
  vtable->write_mem (first_arg,data,len);
}

# define html_styled_ostream_flush html_styled_ostream_flush_inline
static inline void
html_styled_ostream_flush (html_styled_ostream_t first_arg)
{
  const struct html_styled_ostream_implementation *vtable =
    ((struct html_styled_ostream_representation_header *) (struct html_styled_ostream_representation *) first_arg)->vtable;
  vtable->flush (first_arg);
}

# define html_styled_ostream_free html_styled_ostream_free_inline
static inline void
html_styled_ostream_free (html_styled_ostream_t first_arg)
{
  const struct html_styled_ostream_implementation *vtable =
    ((struct html_styled_ostream_representation_header *) (struct html_styled_ostream_representation *) first_arg)->vtable;
  vtable->free (first_arg);
}

# define html_styled_ostream_begin_use_class html_styled_ostream_begin_use_class_inline
static inline void
html_styled_ostream_begin_use_class (html_styled_ostream_t first_arg, const char *classname)
{
  const struct html_styled_ostream_implementation *vtable =
    ((struct html_styled_ostream_representation_header *) (struct html_styled_ostream_representation *) first_arg)->vtable;
  vtable->begin_use_class (first_arg,classname);
}

# define html_styled_ostream_end_use_class html_styled_ostream_end_use_class_inline
static inline void
html_styled_ostream_end_use_class (html_styled_ostream_t first_arg, const char *classname)
{
  const struct html_styled_ostream_implementation *vtable =
    ((struct html_styled_ostream_representation_header *) (struct html_styled_ostream_representation *) first_arg)->vtable;
  vtable->end_use_class (first_arg,classname);
}

#endif

extern const typeinfo_t html_styled_ostream_typeinfo;
#define html_styled_ostream_SUPERCLASSES &html_styled_ostream_typeinfo, styled_ostream_SUPERCLASSES
#define html_styled_ostream_SUPERCLASSES_LENGTH (1 + styled_ostream_SUPERCLASSES_LENGTH)

extern const struct html_styled_ostream_implementation html_styled_ostream_vtable;

#line 28 "html-styled-ostream.oo.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Create an output stream that takes input in the UTF-8 encoding and
   writes it in HTML form on DESTINATION, styled with the file CSS_FILENAME.
   Note that the resulting stream must be closed before DESTINATION can be
   closed.  */
extern html_styled_ostream_t
       html_styled_ostream_create (ostream_t destination,
                                   const char *css_filename);


#ifdef __cplusplus
}
#endif

#endif /* _HTML_STYLED_OSTREAM_H */
