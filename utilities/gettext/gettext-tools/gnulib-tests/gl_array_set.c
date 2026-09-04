/* Set data type implemented by an array.
   Copyright (C) 2006-2026 Free Software Foundation, Inc.
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

#include <config.h>

/* Specification.  */
#include "gl_array_set.h"

#include <stdint.h>
#include <stdlib.h>

/* Checked size_t computations.  */
#include "xsize.h"

/* --------------------------- gl_set_t Data Type --------------------------- */

/* Concrete gl_set_impl type, valid for this file only.  */
struct gl_set_impl
{
  struct gl_set_impl_base base;
  /* An array of ALLOCATED elements, of which the first COUNT are used.
     0 <= COUNT <= ALLOCATED.  */
  const void **elements;
  size_t count;
  size_t allocated;
};

static gl_set_t
gl_array_nx_create_empty (gl_set_implementation_t implementation,
                          gl_setelement_equals_fn equals_fn,
                          gl_setelement_hashcode_fn hashcode_fn,
                          gl_setelement_dispose_fn dispose_fn)
{
  struct gl_set_impl *set =
    (struct gl_set_impl *) malloc (sizeof (struct gl_set_impl));

  if (set == NULL)
    return NULL;

  set->base.vtable = implementation;
  set->base.equals_fn = equals_fn;
  set->base.dispose_fn = dispose_fn;
  set->elements = NULL;
  set->count = 0;
  set->allocated = 0;

  return set;
}

static size_t
gl_array_size (gl_set_t set)
{
  return set->count;
}

static bool
gl_array_search (gl_set_t set, const void *elt)
{
  size_t count = set->count;

  if (count > 0)
    {
      gl_setelement_equals_fn equals = set->base.equals_fn;
      const void **elements = set->elements;
      if (equals != NULL)
        {
          for (size_t i = 0; i < count; i++)
            if (equals (elements[i], elt))
              return true;
        }
      else
        {
          for (size_t i = 0; i < count; i++)
            if (elements[i] == elt)
              return true;
        }
    }
  return false;
}

/* Ensure that set->allocated > set->count.
   Return 0 upon success, -1 upon out-of-memory.  */
static int
grow (gl_set_t set)
{
  size_t new_allocated = xtimes (set->allocated, 2);
  new_allocated = xsum (new_allocated, 1);
  size_t memory_size = xtimes (new_allocated, sizeof (const void *));
  if (size_overflow_p (memory_size))
    /* Overflow, would lead to out of memory.  */
    return -1;
  const void **memory = (const void **) realloc (set->elements, memory_size);
  if (memory == NULL)
    /* Out of memory.  */
    return -1;
  set->elements = memory;
  set->allocated = new_allocated;
  return 0;
}

static int
gl_array_nx_add (gl_set_t set, const void *elt)
{
  if (gl_array_search (set, elt))
    return 0;
  else
    {
      size_t count = set->count;

      if (count == set->allocated)
        if (grow (set) < 0)
          return -1;
      set->elements[count] = elt;
      set->count = count + 1;
      return 1;
    }
}

/* Remove the element at the given position,
   0 <= position < gl_set_size (set).  */
static void
gl_array_remove_at (gl_set_t set, size_t position)
{
  size_t count = set->count;
  const void **elements = set->elements;

  if (set->base.dispose_fn != NULL)
    set->base.dispose_fn (elements[position]);
  for (size_t i = position + 1; i < count; i++)
    elements[i - 1] = elements[i];
  set->count = count - 1;
}

static bool
gl_array_remove (gl_set_t set, const void *elt)
{
  size_t count = set->count;

  if (count > 0)
    {
      gl_setelement_equals_fn equals = set->base.equals_fn;
      const void **elements = set->elements;

      if (equals != NULL)
        {
          for (size_t i = 0; i < count; i++)
            if (equals (elements[i], elt))
              {
                gl_array_remove_at (set, i);
                return true;
              }
        }
      else
        {
          for (size_t i = 0; i < count; i++)
            if (elements[i] == elt)
              {
                gl_array_remove_at (set, i);
                return true;
              }
        }
    }
  return false;
}

static void
gl_array_free (gl_set_t set)
{
  if (set->elements != NULL)
    {
      if (set->base.dispose_fn != NULL)
        {
          size_t count = set->count;

          if (count > 0)
            {
              gl_setelement_dispose_fn dispose = set->base.dispose_fn;
              const void **elements = set->elements;

              do
                dispose (*elements++);
              while (--count > 0);
            }
        }
      free (set->elements);
    }
  free (set);
}

/* ---------------------- gl_set_iterator_t Data Type ---------------------- */

static gl_set_iterator_t
gl_array_iterator (gl_set_t set)
{
  gl_set_iterator_t result;

  result.vtable = set->base.vtable;
  result.set = set;
  result.count = set->count;
  result.p = set->elements + 0;
  result.q = set->elements + set->count;
#if defined GCC_LINT || defined lint
  result.i = 0;
  result.j = 0;
#endif

  return result;
}

static bool
gl_array_iterator_next (gl_set_iterator_t *iterator, const void **eltp)
{
  gl_set_t set = iterator->set;
  if (iterator->count != set->count)
    {
      if (iterator->count != set->count + 1)
        /* Concurrent modifications were done on the set.  */
        abort ();
      /* The last returned element was removed.  */
      iterator->count--;
      iterator->p = (const void **) iterator->p - 1;
      iterator->q = (const void **) iterator->q - 1;
    }
  if (iterator->p < iterator->q)
    {
      const void **p = (const void **) iterator->p;
      *eltp = *p;
      iterator->p = p + 1;
      return true;
    }
  else
    return false;
}

static void
gl_array_iterator_free (gl_set_iterator_t *iterator)
{
}


const struct gl_set_implementation gl_array_set_implementation =
  {
    gl_array_nx_create_empty,
    gl_array_size,
    gl_array_search,
    gl_array_nx_add,
    gl_array_remove,
    gl_array_free,
    gl_array_iterator,
    gl_array_iterator_next,
    gl_array_iterator_free
  };
