/* Ordered set data type implemented by an array.
   Copyright (C) 2006-2007, 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "gl_array_oset.h"

#include <stdlib.h>

/* Checked size_t computations.  */
#include "xsize.h"

/* -------------------------- gl_oset_t Data Type -------------------------- */

/* Concrete gl_oset_impl type, valid for this file only.  */
struct gl_oset_impl
{
  struct gl_oset_impl_base base;
  /* An array of ALLOCATED elements, of which the first COUNT are used.
     0 <= COUNT <= ALLOCATED.  */
  const void **elements;
  size_t count;
  size_t allocated;
};

static gl_oset_t
gl_array_nx_create_empty (gl_oset_implementation_t implementation,
                          gl_setelement_compar_fn compar_fn,
                          gl_setelement_dispose_fn dispose_fn)
{
  struct gl_oset_impl *set =
    (struct gl_oset_impl *) malloc (sizeof (struct gl_oset_impl));

  if (set == NULL)
    return NULL;

  set->base.vtable = implementation;
  set->base.compar_fn = compar_fn;
  set->base.dispose_fn = dispose_fn;
  set->elements = NULL;
  set->count = 0;
  set->allocated = 0;

  return set;
}

static size_t _GL_ATTRIBUTE_PURE
gl_array_size (gl_oset_t set)
{
  return set->count;
}

static size_t _GL_ATTRIBUTE_PURE
gl_array_indexof (gl_oset_t set, const void *elt)
{
  size_t count = set->count;

  if (count > 0)
    {
      gl_setelement_compar_fn compar = set->base.compar_fn;
      size_t low = 0;
      size_t high = count;

      /* At each loop iteration, low < high; for indices < low the values
         are smaller than ELT; for indices >= high the values are greater
         than ELT.  So, if the element occurs in the list, it is at
         low <= position < high.  */
      do
        {
          size_t mid = low + (high - low) / 2; /* low <= mid < high */
          int cmp = (compar != NULL
                     ? compar (set->elements[mid], elt)
                     : (set->elements[mid] > elt ? 1 :
                        set->elements[mid] < elt ? -1 : 0));

          if (cmp < 0)
            low = mid + 1;
          else if (cmp > 0)
            high = mid;
          else /* cmp == 0 */
            /* We have an element equal to ELT at index MID.  */
            return mid;
        }
      while (low < high);
    }
  return (size_t)(-1);
}

static bool _GL_ATTRIBUTE_PURE
gl_array_search (gl_oset_t set, const void *elt)
{
  return gl_array_indexof (set, elt) != (size_t)(-1);
}

/* Searches the least element in the ordered set that compares greater or equal
   to the given THRESHOLD.  The representation of the THRESHOLD is defined
   by the THRESHOLD_FN.
   Returns the position at which it was found, or gl_list_size (SET) if not
   found.  */
static size_t _GL_ATTRIBUTE_PURE
gl_array_indexof_atleast (gl_oset_t set,
                          gl_setelement_threshold_fn threshold_fn,
                          const void *threshold)
{
  size_t count = set->count;

  if (count > 0)
    {
      size_t low = 0;
      size_t high = count;

      /* At each loop iteration, low < high; for indices < low the values are
         smaller than THRESHOLD; for indices >= high the values are nonexistent.
         So, if an element >= THRESHOLD occurs in the list, it is at
         low <= position < high.  */
      do
        {
          size_t mid = low + (high - low) / 2; /* low <= mid < high */

          if (! threshold_fn (set->elements[mid], threshold))
            low = mid + 1;
          else
            {
              /* We have an element >= THRESHOLD at index MID.  But we need the
                 minimal such index.  */
              high = mid;
              /* At each loop iteration, low <= high and
                   compar (set->elements[high], threshold) >= 0,
                 and we know that the first occurrence of the element is at
                 low <= position <= high.  */
              while (low < high)
                {
                  size_t mid2 = low + (high - low) / 2; /* low <= mid2 < high */

                  if (! threshold_fn (set->elements[mid2], threshold))
                    low = mid2 + 1;
                  else
                    high = mid2;
                }
              return low;
            }
        }
      while (low < high);
    }
  return count;
}

static bool _GL_ATTRIBUTE_PURE
gl_array_search_atleast (gl_oset_t set,
                         gl_setelement_threshold_fn threshold_fn,
                         const void *threshold,
                         const void **eltp)
{
  size_t index = gl_array_indexof_atleast (set, threshold_fn, threshold);

  if (index == set->count)
    return false;
  else
    {
      *eltp = set->elements[index];
      return true;
    }
}

/* Ensure that set->allocated > set->count.
   Return 0 upon success, -1 upon out-of-memory.  */
static int
grow (gl_oset_t set)
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

/* Add the given element ELT at the given position,
   0 <= position <= gl_oset_size (set).
   Return 1 upon success, -1 upon out-of-memory.  */
static int
gl_array_nx_add_at (gl_oset_t set, size_t position, const void *elt)
{
  size_t count = set->count;
  if (count == set->allocated)
    if (grow (set) < 0)
      return -1;
  const void **elements = set->elements;
  for (size_t i = count; i > position; i--)
    elements[i] = elements[i - 1];
  elements[position] = elt;
  set->count = count + 1;
  return 1;
}

/* Remove the element at the given position,
   0 <= position < gl_oset_size (set).  */
static void
gl_array_remove_at (gl_oset_t set, size_t position)
{
  size_t count = set->count;
  const void **elements = set->elements;
  if (set->base.dispose_fn != NULL)
    set->base.dispose_fn (elements[position]);
  for (size_t i = position + 1; i < count; i++)
    elements[i - 1] = elements[i];
  set->count = count - 1;
}

static int
gl_array_nx_add (gl_oset_t set, const void *elt)
{
  size_t count = set->count;
  size_t low = 0;

  if (count > 0)
    {
      gl_setelement_compar_fn compar = set->base.compar_fn;
      size_t high = count;

      /* At each loop iteration, low < high; for indices < low the values
         are smaller than ELT; for indices >= high the values are greater
         than ELT.  So, if the element occurs in the list, it is at
         low <= position < high.  */
      do
        {
          size_t mid = low + (high - low) / 2; /* low <= mid < high */
          int cmp = (compar != NULL
                     ? compar (set->elements[mid], elt)
                     : (set->elements[mid] > elt ? 1 :
                        set->elements[mid] < elt ? -1 : 0));

          if (cmp < 0)
            low = mid + 1;
          else if (cmp > 0)
            high = mid;
          else /* cmp == 0 */
            return false;
        }
      while (low < high);
    }
  return gl_array_nx_add_at (set, low, elt);
}

static bool
gl_array_remove (gl_oset_t set, const void *elt)
{
  size_t index = gl_array_indexof (set, elt);
  if (index != (size_t)(-1))
    {
      gl_array_remove_at (set, index);
      return true;
    }
  else
    return false;
}

static int
gl_array_update (gl_oset_t set, const void *elt,
                 void (*action) (const void * /*elt*/, void * /*action_data*/),
                 void *action_data)
{
  /* Like gl_array_remove, action (...), gl_array_nx_add, except that we don't
     actually remove ELT.  */
  /* Remember the old position.  */
  size_t old_index = gl_array_indexof (set, elt);
  /* Invoke ACTION.  */
  action (elt, action_data);
  /* Determine the new position.  */
  if (old_index != (size_t)(-1))
    {
      size_t count = set->count;

      if (count > 1)
        {
          gl_setelement_compar_fn compar = set->base.compar_fn;
          size_t low;
          size_t high;

          if (old_index > 0)
            {
              size_t mid = old_index - 1;
              int cmp = (compar != NULL
                         ? compar (set->elements[mid], elt)
                         : (set->elements[mid] > elt ? 1 :
                            set->elements[mid] < elt ? -1 : 0));
              if (cmp < 0)
                {
                  low = old_index + 1;
                  high = count;
                }
              else if (cmp > 0)
                {
                  low = 0;
                  high = mid;
                }
              else /* cmp == 0 */
                {
                  /* Two adjacent elements are the same.  */
                  gl_array_remove_at (set, old_index);
                  return -1;
                }
            }
          else
            {
              low = old_index + 1;
              high = count;
            }

          /* At each loop iteration, low <= high; for indices < low the values
             are smaller than ELT; for indices >= high the values are greater
             than ELT.  So, if the element occurs in the list, it is at
             low <= position < high.  */
          while (low < high)
            {
              size_t mid = low + (high - low) / 2; /* low <= mid < high */
              int cmp = (compar != NULL
                         ? compar (set->elements[mid], elt)
                         : (set->elements[mid] > elt ? 1 :
                            set->elements[mid] < elt ? -1 : 0));

              if (cmp < 0)
                low = mid + 1;
              else if (cmp > 0)
                high = mid;
              else /* cmp == 0 */
                {
                  /* Two elements are the same.  */
                  gl_array_remove_at (set, old_index);
                  return -1;
                }
            }

          if (low < old_index)
            {
              /* Move the element from old_index to low.  */
              size_t new_index = low;
              const void **elements = set->elements;

              for (size_t i = old_index; i > new_index; i--)
                elements[i] = elements[i - 1];
              elements[new_index] = elt;
              return true;
            }
          else
            {
              /* low > old_index.  */
              /* Move the element from old_index to low - 1.  */
              size_t new_index = low - 1;

              if (new_index > old_index)
                {
                  const void **elements = set->elements;

                  for (size_t i = old_index; i < new_index; i++)
                    elements[i] = elements[i + 1];
                  elements[new_index] = elt;
                  return true;
                }
            }
        }
    }
  return false;
}

static void
gl_array_free (gl_oset_t set)
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

/* --------------------- gl_oset_iterator_t Data Type --------------------- */

static gl_oset_iterator_t _GL_ATTRIBUTE_PURE
gl_array_iterator (gl_oset_t set)
{
  gl_oset_iterator_t result;

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

static gl_oset_iterator_t _GL_ATTRIBUTE_PURE
gl_array_iterator_atleast (gl_oset_t set,
                           gl_setelement_threshold_fn threshold_fn,
                           const void *threshold)
{
  size_t index = gl_array_indexof_atleast (set, threshold_fn, threshold);
  gl_oset_iterator_t result;

  result.vtable = set->base.vtable;
  result.set = set;
  result.count = set->count;
  result.p = set->elements + index;
  result.q = set->elements + set->count;
#if defined GCC_LINT || defined lint
  result.i = 0;
  result.j = 0;
#endif

  return result;
}

static bool
gl_array_iterator_next (gl_oset_iterator_t *iterator, const void **eltp)
{
  gl_oset_t set = iterator->set;
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
gl_array_iterator_free (gl_oset_iterator_t *_GL_UNNAMED (iterator))
{
}


const struct gl_oset_implementation gl_array_oset_implementation =
  {
    gl_array_nx_create_empty,
    gl_array_size,
    gl_array_search,
    gl_array_search_atleast,
    gl_array_nx_add,
    gl_array_remove,
    gl_array_update,
    gl_array_free,
    gl_array_iterator,
    gl_array_iterator_atleast,
    gl_array_iterator_next,
    gl_array_iterator_free
  };
