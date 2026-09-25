/* Abstract set data type.
   Copyright (C) 2006-2007, 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2018.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifndef _GL_SET_H
#define _GL_SET_H

/* This file uses _GL_INLINE_HEADER_BEGIN, _GL_INLINE,
   _GL_ATTRIBUTE_NODISCARD.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <stddef.h>

_GL_INLINE_HEADER_BEGIN
#ifndef GL_SET_INLINE
# define GL_SET_INLINE _GL_INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* gl_set is an abstract set data type.  It can contain any number of objects
   ('void *' or 'const void *' pointers); the order does not matter.
   Duplicates (in the sense of the comparator) are forbidden.

   There are several implementations of this set datatype, optimized for
   different operations or for memory.  You can start using the simplest set
   implementation, GL_ARRAY_SET, and switch to a different implementation
   later, when you realize which operations are performed the most frequently.
   The API of the different implementations is exactly the same; when switching
   to a different implementation, you only have to change the gl_set_create
   call.

   The implementations are:
     GL_ARRAY_SET         a growable array
     GL_LINKEDHASH_SET    a hash table with a linked list
     GL_HASH_SET          a hash table

   The memory consumption is asymptotically the same: O(1) for every object
   in the set.  When looking more closely at the average memory consumed
   for an object, GL_ARRAY_SET is the most compact representation, then comes
   GL_HASH_SET, and GL_LINKEDHASH_SET needs the most memory.

   The guaranteed average performance of the operations is, for a set of
   n elements:

   Operation                  ARRAY   LINKEDHASH
                                      HASH

   gl_set_size                 O(1)   O(1)
   gl_set_add                  O(n)   O(1)
   gl_set_remove               O(n)   O(1)
   gl_set_search               O(n)   O(1)
   gl_set_iterator             O(1)   O(1)
   gl_set_iterator_next        O(1)   O(1)
 */

/* --------------------------- gl_set_t Data Type --------------------------- */

/* Type of function used to compare two elements.
   NULL denotes pointer comparison.  */
typedef bool (*gl_setelement_equals_fn) (const void *elt1, const void *elt2);

/* Type of function used to compute a hash code.
   NULL denotes a function that depends only on the pointer itself.  */
typedef size_t (*gl_setelement_hashcode_fn) (const void *elt);

#ifndef _GL_SETELEMENT_DISPOSE_FN_DEFINED
/* Type of function used to dispose an element once it's removed from a set.
   NULL denotes a no-op.  */
typedef void (*gl_setelement_dispose_fn) (const void *elt);
# define _GL_SETELEMENT_DISPOSE_FN_DEFINED 1
#endif

struct gl_set_impl;
/* Type representing an entire set.  */
typedef struct gl_set_impl * gl_set_t;

struct gl_set_implementation;
/* Type representing a set datatype implementation.  */
typedef const struct gl_set_implementation * gl_set_implementation_t;

#if 0 /* Unless otherwise specified, these are defined inline below.  */

/* Creates an empty set.
   IMPLEMENTATION is one of GL_ARRAY_SET, GL_LINKEDHASH_SET, GL_HASH_SET.
   EQUALS_FN is an element comparison function or NULL.
   HASHCODE_FN is an element hash code function or NULL.
   DISPOSE_FN is an element disposal function or NULL.  */
/* declared in gl_xset.h */
extern gl_set_t gl_set_create_empty (gl_set_implementation_t implementation,
                                     gl_setelement_equals_fn equals_fn,
                                     gl_setelement_hashcode_fn hashcode_fn,
                                     gl_setelement_dispose_fn dispose_fn)
  /*_GL_ATTRIBUTE_DEALLOC (gl_set_free, 1)*/
  _GL_ATTRIBUTE_RETURNS_NONNULL;
/* Likewise.  Returns NULL upon out-of-memory.  */
extern gl_set_t gl_set_nx_create_empty (gl_set_implementation_t implementation,
                                        gl_setelement_equals_fn equals_fn,
                                        gl_setelement_hashcode_fn hashcode_fn,
                                        gl_setelement_dispose_fn dispose_fn)
  /*_GL_ATTRIBUTE_DEALLOC (gl_set_free, 1)*/;

/* Returns the current number of elements in a set.  */
extern size_t gl_set_size (gl_set_t set);

/* Searches whether an element is already in the set.
   Returns true if found, or false if not present in the set.  */
extern bool gl_set_search (gl_set_t set, const void *elt);

/* Adds an element to a set.
   Returns true if it was not already in the set and added, false otherwise.  */
/* declared in gl_xset.h */
extern bool gl_set_add (gl_set_t set, const void *elt);

/* Likewise.  Returns -1 upon out-of-memory.  */
_GL_ATTRIBUTE_NODISCARD
extern int gl_set_nx_add (gl_set_t set, const void *elt);

/* Removes an element from a set.
   Returns true if it was found and removed.  */
extern bool gl_set_remove (gl_set_t set, const void *elt);

/* Frees an entire set.
   (But this call does not free the elements of the set.  It only invokes
   the DISPOSE_FN on each of the elements of the set.)  */
extern void gl_set_free (gl_set_t set);

#endif /* End of inline and gl_xset.h-defined functions.  */

/* ---------------------- gl_set_iterator_t Data Type ---------------------- */

/* Functions for iterating through a set.
   Note: Iterating through a set of type GL_HASH_SET returns the elements in an
   unpredictable order.  If you need a predictable order, use GL_LINKEDHASH_SET
   instead of GL_HASH_SET.  */

/* Type of an iterator that traverses a set.
   This is a fixed-size struct, so that creation of an iterator doesn't need
   memory allocation on the heap.  */
typedef struct
{
  /* For fast dispatch of gl_set_iterator_next.  */
  const struct gl_set_implementation *vtable;
  /* For detecting whether the last returned element was removed.  */
  gl_set_t set;
  size_t count;
  /* Other, implementation-private fields.  */
  void *p; void *q;
  size_t i; size_t j;
} gl_set_iterator_t;

#if 0 /* These are defined inline below.  */

/* Creates an iterator traversing a set.
   The set's contents must not be modified while the iterator is in use,
   except for removing the last returned element.  */
extern gl_set_iterator_t gl_set_iterator (gl_set_t set);

/* If there is a next element, stores the next element in *ELTP, advances the
   iterator and returns true.  Otherwise, returns false.  */
extern bool gl_set_iterator_next (gl_set_iterator_t *iterator,
                                  const void **eltp);

/* Frees an iterator.  */
extern void gl_set_iterator_free (gl_set_iterator_t *iterator);

#endif /* End of inline functions.  */

/* ------------------------ Implementation Details ------------------------ */

struct gl_set_implementation
{
  /* gl_set_t functions.  */
  gl_set_t (*nx_create_empty) (gl_set_implementation_t implementation,
                               gl_setelement_equals_fn equals_fn,
                               gl_setelement_hashcode_fn hashcode_fn,
                               gl_setelement_dispose_fn dispose_fn);
  size_t (*size) (gl_set_t set);
  bool (*search) (gl_set_t set, const void *elt);
  int (*nx_add) (gl_set_t set, const void *elt);
  bool (*remove_elt) (gl_set_t set, const void *elt);
  void (*set_free) (gl_set_t set);
  /* gl_set_iterator_t functions.  */
  gl_set_iterator_t (*iterator) (gl_set_t set);
  bool (*iterator_next) (gl_set_iterator_t *iterator, const void **eltp);
  void (*iterator_free) (gl_set_iterator_t *iterator);
};

struct gl_set_impl_base
{
  const struct gl_set_implementation *vtable;
  gl_setelement_equals_fn equals_fn;
  gl_setelement_dispose_fn dispose_fn;
};

/* Define all functions of this file as accesses to the
   struct gl_set_implementation.  */

GL_SET_INLINE
/*_GL_ATTRIBUTE_DEALLOC (gl_set_free, 1)*/
gl_set_t
gl_set_nx_create_empty (gl_set_implementation_t implementation,
                        gl_setelement_equals_fn equals_fn,
                        gl_setelement_hashcode_fn hashcode_fn,
                        gl_setelement_dispose_fn dispose_fn)
{
  return implementation->nx_create_empty (implementation, equals_fn,
                                          hashcode_fn, dispose_fn);
}

GL_SET_INLINE size_t
gl_set_size (gl_set_t set)
{
  return ((const struct gl_set_impl_base *) set)->vtable->size (set);
}

GL_SET_INLINE bool
gl_set_search (gl_set_t set, const void *elt)
{
  return ((const struct gl_set_impl_base *) set)->vtable->search (set, elt);
}

_GL_ATTRIBUTE_NODISCARD GL_SET_INLINE int
gl_set_nx_add (gl_set_t set, const void *elt)
{
  return ((const struct gl_set_impl_base *) set)->vtable->nx_add (set, elt);
}

GL_SET_INLINE bool
gl_set_remove (gl_set_t set, const void *elt)
{
  return ((const struct gl_set_impl_base *) set)->vtable->remove_elt (set, elt);
}

GL_SET_INLINE void
gl_set_free (gl_set_t set)
{
  ((const struct gl_set_impl_base *) set)->vtable->set_free (set);
}

GL_SET_INLINE gl_set_iterator_t
gl_set_iterator (gl_set_t set)
{
  return ((const struct gl_set_impl_base *) set)->vtable->iterator (set);
}

GL_SET_INLINE bool
gl_set_iterator_next (gl_set_iterator_t *iterator, const void **eltp)
{
  return iterator->vtable->iterator_next (iterator, eltp);
}

GL_SET_INLINE void
gl_set_iterator_free (gl_set_iterator_t *iterator)
{
  iterator->vtable->iterator_free (iterator);
}

#ifdef __cplusplus
}
#endif

_GL_INLINE_HEADER_END

#endif /* _GL_SET_H */
