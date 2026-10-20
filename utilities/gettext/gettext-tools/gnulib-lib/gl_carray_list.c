/* Sequential list data type implemented by a circular array.
   Copyright (C) 2006-2026 Free Software Foundation, Inc.
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
#include "gl_carray_list.h"

#include <stdint.h>
#include <stdlib.h>
/* Get memcpy.  */
#include <string.h>

/* Checked size_t computations.  */
#include "xsize.h"

/* -------------------------- gl_list_t Data Type -------------------------- */

/* Concrete gl_list_impl type, valid for this file only.  */
struct gl_list_impl
{
  struct gl_list_impl_base base;
  /* An array of ALLOCATED elements, of which the elements
     OFFSET, (OFFSET + 1) % ALLOCATED, ..., (OFFSET + COUNT - 1) % ALLOCATED
     are used.  0 <= COUNT <= ALLOCATED.  Either OFFSET = ALLOCATED = 0 or
     0 <= OFFSET < ALLOCATED.  */
  const void **elements;
  size_t offset;
  size_t count;
  size_t allocated;
};

/* struct gl_list_node_impl doesn't exist here.  The pointers are actually
   indices + 1.  */
#define INDEX_TO_NODE(index) (gl_list_node_t)(uintptr_t)(size_t)((index) + 1)
#define NODE_TO_INDEX(node) ((uintptr_t)(node) - 1)

static gl_list_t
gl_carray_nx_create_empty (gl_list_implementation_t implementation,
                           gl_listelement_equals_fn equals_fn,
                           gl_listelement_hashcode_fn hashcode_fn,
                           gl_listelement_dispose_fn dispose_fn,
                           bool allow_duplicates)
{
  struct gl_list_impl *list =
    (struct gl_list_impl *) malloc (sizeof (struct gl_list_impl));

  if (list == NULL)
    return NULL;

  list->base.vtable = implementation;
  list->base.equals_fn = equals_fn;
  list->base.hashcode_fn = hashcode_fn;
  list->base.dispose_fn = dispose_fn;
  list->base.allow_duplicates = allow_duplicates;
  list->elements = NULL;
  list->offset = 0;
  list->count = 0;
  list->allocated = 0;

  return list;
}

static gl_list_t
gl_carray_nx_create (gl_list_implementation_t implementation,
                  gl_listelement_equals_fn equals_fn,
                  gl_listelement_hashcode_fn hashcode_fn,
                  gl_listelement_dispose_fn dispose_fn,
                  bool allow_duplicates,
                  size_t count, const void **contents)
{
  struct gl_list_impl *list =
    (struct gl_list_impl *) malloc (sizeof (struct gl_list_impl));

  if (list == NULL)
    return NULL;

  list->base.vtable = implementation;
  list->base.equals_fn = equals_fn;
  list->base.hashcode_fn = hashcode_fn;
  list->base.dispose_fn = dispose_fn;
  list->base.allow_duplicates = allow_duplicates;
  if (count > 0)
    {
      if (size_overflow_p (xtimes (count, sizeof (const void *))))
        goto fail;
      list->elements = (const void **) malloc (count * sizeof (const void *));
      if (list->elements == NULL)
        goto fail;
      memcpy (list->elements, contents, count * sizeof (const void *));
    }
  else
    list->elements = NULL;
  list->offset = 0;
  list->count = count;
  list->allocated = count;

  return list;

 fail:
  free (list);
  return NULL;
}

static size_t _GL_ATTRIBUTE_PURE
gl_carray_size (gl_list_t list)
{
  return list->count;
}

static const void * _GL_ATTRIBUTE_PURE
gl_carray_node_value (gl_list_t list, gl_list_node_t node)
{
  uintptr_t index = NODE_TO_INDEX (node);

  if (!(index < list->count))
    /* Invalid argument.  */
    abort ();
  size_t i = list->offset + index;
  if (i >= list->allocated)
    i -= list->allocated;
  return list->elements[i];
}

static int
gl_carray_node_nx_set_value (gl_list_t list, gl_list_node_t node,
                             const void *elt)
{
  uintptr_t index = NODE_TO_INDEX (node);

  if (!(index < list->count))
    /* Invalid argument.  */
    abort ();
  size_t i = list->offset + index;
  if (i >= list->allocated)
    i -= list->allocated;
  list->elements[i] = elt;
  return 0;
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_next_node (gl_list_t list, gl_list_node_t node)
{
  uintptr_t index = NODE_TO_INDEX (node);
  if (!(index < list->count))
    /* Invalid argument.  */
    abort ();
  index++;
  if (index < list->count)
    return INDEX_TO_NODE (index);
  else
    return NULL;
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_previous_node (gl_list_t list, gl_list_node_t node)
{
  uintptr_t index = NODE_TO_INDEX (node);
  if (!(index < list->count))
    /* Invalid argument.  */
    abort ();
  if (index > 0)
    return INDEX_TO_NODE (index - 1);
  else
    return NULL;
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_first_node (gl_list_t list)
{
  if (list->count > 0)
    return INDEX_TO_NODE (0);
  else
    return NULL;
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_last_node (gl_list_t list)
{
  if (list->count > 0)
    return INDEX_TO_NODE (list->count - 1);
  else
    return NULL;
}

static const void * _GL_ATTRIBUTE_PURE
gl_carray_get_at (gl_list_t list, size_t position)
{
  size_t count = list->count;

  if (!(position < count))
    /* Invalid argument.  */
    abort ();
  size_t i = list->offset + position;
  if (i >= list->allocated)
    i -= list->allocated;
  return list->elements[i];
}

static gl_list_node_t
gl_carray_nx_set_at (gl_list_t list, size_t position, const void *elt)
{
  size_t count = list->count;

  if (!(position < count))
    /* Invalid argument.  */
    abort ();
  size_t i = list->offset + position;
  if (i >= list->allocated)
    i -= list->allocated;
  list->elements[i] = elt;
  return INDEX_TO_NODE (position);
}

static size_t _GL_ATTRIBUTE_PURE
gl_carray_indexof_from_to (gl_list_t list, size_t start_index, size_t end_index,
                           const void *elt)
{
  size_t count = list->count;

  if (!(start_index <= end_index && end_index <= count))
    /* Invalid arguments.  */
    abort ();

  if (start_index < end_index)
    {
      gl_listelement_equals_fn equals = list->base.equals_fn;
      size_t allocated = list->allocated;

      size_t i_end = list->offset + end_index;
      if (i_end >= allocated)
        i_end -= allocated;

      if (equals != NULL)
        {
          size_t i;

          i = list->offset + start_index;
          if (i >= allocated) /* can only happen if start_index > 0 */
            i -= allocated;

          for (;;)
            {
              if (equals (elt, list->elements[i]))
                return (i >= list->offset ? i : i + allocated) - list->offset;
              i++;
              if (i == allocated)
                i = 0;
              if (i == i_end)
                break;
            }
        }
      else
        {
          size_t i;

          i = list->offset + start_index;
          if (i >= allocated) /* can only happen if start_index > 0 */
            i -= allocated;

          for (;;)
            {
              if (elt == list->elements[i])
                return (i >= list->offset ? i : i + allocated) - list->offset;
              i++;
              if (i == allocated)
                i = 0;
              if (i == i_end)
                break;
            }
        }
    }
  return (size_t)(-1);
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_search_from_to (gl_list_t list, size_t start_index, size_t end_index,
                          const void *elt)
{
  size_t index = gl_carray_indexof_from_to (list, start_index, end_index, elt);
  return INDEX_TO_NODE (index);
}

/* Ensure that list->allocated > list->count.
   Return 0 upon success, -1 upon out-of-memory.  */
static int
grow (gl_list_t list)
{
  size_t new_allocated = xtimes (list->allocated, 2);
  new_allocated = xsum (new_allocated, 1);
  size_t memory_size = xtimes (new_allocated, sizeof (const void *));
  if (size_overflow_p (memory_size))
    /* Overflow, would lead to out of memory.  */
    return -1;
  const void **memory;
  if (list->offset > 0 && list->count > 0)
    {
      memory = (const void **) malloc (memory_size);
      if (memory == NULL)
        /* Out of memory.  */
        return -1;
      if (list->offset + list->count > list->allocated)
        {
          memcpy (memory, &list->elements[list->offset],
                  (list->allocated - list->offset) * sizeof (const void *));
          memcpy (memory + (list->allocated - list->offset), list->elements,
                  (list->offset + list->count - list->allocated)
                  * sizeof (const void *));

        }
      else
        memcpy (memory, &list->elements[list->offset],
                list->count * sizeof (const void *));
      if (list->elements != NULL)
        free (list->elements);
    }
  else
    {
      memory = (const void **) realloc (list->elements, memory_size);
      if (memory == NULL)
        /* Out of memory.  */
        return -1;
    }
  list->elements = memory;
  list->offset = 0;
  list->allocated = new_allocated;
  return 0;
}

static gl_list_node_t
gl_carray_nx_add_first (gl_list_t list, const void *elt)
{
  size_t count = list->count;

  if (count == list->allocated)
    if (grow (list) < 0)
      return NULL;
  list->offset = (list->offset == 0 ? list->allocated : list->offset) - 1;
  list->elements[list->offset] = elt;
  list->count = count + 1;
  return INDEX_TO_NODE (0);
}

static gl_list_node_t
gl_carray_nx_add_last (gl_list_t list, const void *elt)
{
  size_t count = list->count;

  if (count == list->allocated)
    if (grow (list) < 0)
      return NULL;
  size_t i = list->offset + count;
  if (i >= list->allocated)
    i -= list->allocated;
  list->elements[i] = elt;
  list->count = count + 1;
  return INDEX_TO_NODE (count);
}

static gl_list_node_t
gl_carray_nx_add_at (gl_list_t list, size_t position, const void *elt)
{
  size_t count = list->count;

  if (!(position <= count))
    /* Invalid argument.  */
    abort ();
  if (count == list->allocated)
    if (grow (list) < 0)
      return NULL;
  const void **elements = list->elements;
  if (position <= (count / 2))
    {
      /* Shift at most count/2 elements to the left.  */
      list->offset = (list->offset == 0 ? list->allocated : list->offset) - 1;

      size_t i2 = list->offset + position;
      if (i2 >= list->allocated)
        {
          /* Here we must have list->offset > 0, hence list->allocated > 0.  */
          size_t i1 = list->allocated - 1;
          i2 -= list->allocated;
          for (size_t i = list->offset; i < i1; i++)
            elements[i] = elements[i + 1];
          elements[i1] = elements[0];
          for (size_t i = 0; i < i2; i++)
            elements[i] = elements[i + 1];
        }
      else
        {
          for (size_t i = list->offset; i < i2; i++)
            elements[i] = elements[i + 1];
        }
      elements[i2] = elt;
    }
  else
    {
      /* Shift at most (count+1)/2 elements to the right.  */
      size_t i1 = list->offset + position;
      size_t i3 = list->offset + count;
      if (i1 >= list->allocated)
        {
          i1 -= list->allocated;
          i3 -= list->allocated;
          for (size_t i = i3; i > i1; i--)
            elements[i] = elements[i - 1];
        }
      else if (i3 >= list->allocated)
        {
          /* Here we must have list->offset > 0, hence list->allocated > 0.  */
          size_t i2 = list->allocated - 1;
          i3 -= list->allocated;
          for (size_t i = i3; i > 0; i--)
            elements[i] = elements[i - 1];
          elements[0] = elements[i2];
          for (size_t i = i2; i > i1; i--)
            elements[i] = elements[i - 1];
        }
      else
        {
          for (size_t i = i3; i > i1; i--)
            elements[i] = elements[i - 1];
        }
      elements[i1] = elt;
    }
  list->count = count + 1;
  return INDEX_TO_NODE (position);
}

static gl_list_node_t
gl_carray_nx_add_before (gl_list_t list, gl_list_node_t node, const void *elt)
{
  size_t count = list->count;
  uintptr_t index = NODE_TO_INDEX (node);

  if (!(index < count))
    /* Invalid argument.  */
    abort ();
  return gl_carray_nx_add_at (list, index, elt);
}

static gl_list_node_t
gl_carray_nx_add_after (gl_list_t list, gl_list_node_t node, const void *elt)
{
  size_t count = list->count;
  uintptr_t index = NODE_TO_INDEX (node);

  if (!(index < count))
    /* Invalid argument.  */
    abort ();
  return gl_carray_nx_add_at (list, index + 1, elt);
}

static bool
gl_carray_remove_at (gl_list_t list, size_t position)
{
  size_t count = list->count;

  if (!(position < count))
    /* Invalid argument.  */
    abort ();
  /* Here we know count > 0.  */
  const void **elements = list->elements;
  if (position <= ((count - 1) / 2))
    {
      /* Shift at most (count-1)/2 elements to the right.  */
      size_t i0 = list->offset;
      size_t i2 = list->offset + position;
      if (i2 >= list->allocated)
        {
          /* Here we must have list->offset > 0, hence list->allocated > 0.  */
          size_t i1 = list->allocated - 1;
          i2 -= list->allocated;
          if (list->base.dispose_fn != NULL)
            list->base.dispose_fn (elements[i2]);
          for (size_t i = i2; i > 0; i--)
            elements[i] = elements[i - 1];
          elements[0] = elements[i1];
          for (size_t i = i1; i > i0; i--)
            elements[i] = elements[i - 1];
        }
      else
        {
          if (list->base.dispose_fn != NULL)
            list->base.dispose_fn (elements[i2]);
          for (size_t i = i2; i > i0; i--)
            elements[i] = elements[i - 1];
        }

      i0++;
      list->offset = (i0 == list->allocated ? 0 : i0);
    }
  else
    {
      /* Shift at most count/2 elements to the left.  */
      size_t i1 = list->offset + position;
      size_t i3 = list->offset + count - 1;
      if (i1 >= list->allocated)
        {
          i1 -= list->allocated;
          i3 -= list->allocated;
          if (list->base.dispose_fn != NULL)
            list->base.dispose_fn (elements[i1]);
          for (size_t i = i1; i < i3; i++)
            elements[i] = elements[i + 1];
        }
      else if (i3 >= list->allocated)
        {
          /* Here we must have list->offset > 0, hence list->allocated > 0.  */
          size_t i2 = list->allocated - 1;
          i3 -= list->allocated;
          if (list->base.dispose_fn != NULL)
            list->base.dispose_fn (elements[i1]);
          for (size_t i = i1; i < i2; i++)
            elements[i] = elements[i + 1];
          elements[i2] = elements[0];
          for (size_t i = 0; i < i3; i++)
            elements[i] = elements[i + 1];
        }
      else
        {
          if (list->base.dispose_fn != NULL)
            list->base.dispose_fn (elements[i1]);
          for (size_t i = i1; i < i3; i++)
            elements[i] = elements[i + 1];
        }
    }
  list->count = count - 1;
  return true;
}

static bool
gl_carray_remove_node (gl_list_t list, gl_list_node_t node)
{
  size_t count = list->count;
  uintptr_t index = NODE_TO_INDEX (node);

  if (!(index < count))
    /* Invalid argument.  */
    abort ();
  return gl_carray_remove_at (list, index);
}

static bool
gl_carray_remove (gl_list_t list, const void *elt)
{
  size_t position = gl_carray_indexof_from_to (list, 0, list->count, elt);
  if (position == (size_t)(-1))
    return false;
  else
    return gl_carray_remove_at (list, position);
}

static void
gl_carray_list_free (gl_list_t list)
{
  if (list->elements != NULL)
    {
      if (list->base.dispose_fn != NULL)
        {
          size_t count = list->count;

          if (count > 0)
            {
              gl_listelement_dispose_fn dispose = list->base.dispose_fn;
              const void **elements = list->elements;
              size_t i1 = list->offset;
              size_t i3 = list->offset + count - 1;

              if (i3 >= list->allocated)
                {
                  /* Here we must have list->offset > 0, hence
                     list->allocated > 0.  */
                  size_t i2 = list->allocated - 1;

                  i3 -= list->allocated;
                  for (size_t i = i1; i <= i2; i++)
                    dispose (elements[i]);
                  for (size_t i = 0; i <= i3; i++)
                    dispose (elements[i]);
                }
              else
                {
                  for (size_t i = i1; i <= i3; i++)
                    dispose (elements[i]);
                }
            }
        }
      free (list->elements);
    }
  free (list);
}

/* --------------------- gl_list_iterator_t Data Type --------------------- */

static gl_list_iterator_t _GL_ATTRIBUTE_PURE
gl_carray_iterator (gl_list_t list)
{
  gl_list_iterator_t result;

  result.vtable = list->base.vtable;
  result.list = list;
  result.count = list->count;
  result.i = 0;
  result.j = list->count;
#if defined GCC_LINT || defined lint
  result.p = 0;
  result.q = 0;
#endif

  return result;
}

static gl_list_iterator_t _GL_ATTRIBUTE_PURE
gl_carray_iterator_from_to (gl_list_t list, size_t start_index, size_t end_index)
{
  if (!(start_index <= end_index && end_index <= list->count))
    /* Invalid arguments.  */
    abort ();

  gl_list_iterator_t result;
  result.vtable = list->base.vtable;
  result.list = list;
  result.count = list->count;
  result.i = start_index;
  result.j = end_index;
#if defined GCC_LINT || defined lint
  result.p = 0;
  result.q = 0;
#endif

  return result;
}

static bool
gl_carray_iterator_next (gl_list_iterator_t *iterator,
                         const void **eltp, gl_list_node_t *nodep)
{
  gl_list_t list = iterator->list;
  if (iterator->count != list->count)
    {
      if (iterator->count != list->count + 1)
        /* Concurrent modifications were done on the list.  */
        abort ();
      /* The last returned element was removed.  */
      iterator->count--;
      iterator->i--;
      iterator->j--;
    }
  if (iterator->i < iterator->j)
    {
      size_t i = list->offset + iterator->i;
      if (i >= list->allocated)
        i -= list->allocated;
      *eltp = list->elements[i];
      if (nodep != NULL)
        *nodep = INDEX_TO_NODE (iterator->i);
      iterator->i++;
      return true;
    }
  else
    return false;
}

static void
gl_carray_iterator_free (gl_list_iterator_t *_GL_UNNAMED (iterator))
{
}

/* ---------------------- Sorted gl_list_t Data Type ---------------------- */

static size_t _GL_ATTRIBUTE_PURE
gl_carray_sortedlist_indexof_from_to (gl_list_t list,
                                      gl_listelement_compar_fn compar,
                                      size_t low, size_t high,
                                      const void *elt)
{
  if (!(low <= high && high <= list->count))
    /* Invalid arguments.  */
    abort ();
  if (low < high)
    {
      /* At each loop iteration, low < high; for indices < low the values
         are smaller than ELT; for indices >= high the values are greater
         than ELT.  So, if the element occurs in the list, it is at
         low <= position < high.  */
      do
        {
          size_t mid = low + (high - low) / 2; /* low <= mid < high */

          size_t i_mid = list->offset + mid;
          if (i_mid >= list->allocated)
            i_mid -= list->allocated;

          int cmp = compar (list->elements[i_mid], elt);

          if (cmp < 0)
            low = mid + 1;
          else if (cmp > 0)
            high = mid;
          else /* cmp == 0 */
            {
              /* We have an element equal to ELT at index MID.  But we need
                 the minimal such index.  */
              high = mid;
              /* At each loop iteration, low <= high and
                   compar (list->elements[i_high], elt) == 0,
                 and we know that the first occurrence of the element is at
                 low <= position <= high.  */
              while (low < high)
                {
                  size_t mid2 = low + (high - low) / 2; /* low <= mid2 < high */

                  size_t i_mid2 = list->offset + mid2;
                  if (i_mid2 >= list->allocated)
                    i_mid2 -= list->allocated;

                  int cmp2 = compar (list->elements[i_mid2], elt);

                  if (cmp2 < 0)
                    low = mid2 + 1;
                  else if (cmp2 > 0)
                    /* The list was not sorted.  */
                    abort ();
                  else /* cmp2 == 0 */
                    {
                      if (mid2 == low)
                        break;
                      high = mid2 - 1;
                    }
                }
              return low;
            }
        }
      while (low < high);
      /* Here low == high.  */
    }
  return (size_t)(-1);
}

static size_t _GL_ATTRIBUTE_PURE
gl_carray_sortedlist_indexof (gl_list_t list, gl_listelement_compar_fn compar,
                              const void *elt)
{
  return gl_carray_sortedlist_indexof_from_to (list, compar, 0, list->count,
                                               elt);
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_sortedlist_search_from_to (gl_list_t list,
                                     gl_listelement_compar_fn compar,
                                     size_t low, size_t high,
                                     const void *elt)
{
  size_t index =
    gl_carray_sortedlist_indexof_from_to (list, compar, low, high, elt);
  return INDEX_TO_NODE (index);
}

static gl_list_node_t _GL_ATTRIBUTE_PURE
gl_carray_sortedlist_search (gl_list_t list, gl_listelement_compar_fn compar,
                             const void *elt)
{
  size_t index =
    gl_carray_sortedlist_indexof_from_to (list, compar, 0, list->count, elt);
  return INDEX_TO_NODE (index);
}

static gl_list_node_t
gl_carray_sortedlist_nx_add (gl_list_t list, gl_listelement_compar_fn compar,
                             const void *elt)
{
  size_t count = list->count;
  size_t low = 0;
  size_t high = count;

  /* At each loop iteration, low <= high; for indices < low the values are
     smaller than ELT; for indices >= high the values are greater than ELT.  */
  while (low < high)
    {
      size_t mid = low + (high - low) / 2; /* low <= mid < high */

      size_t i_mid = list->offset + mid;
      if (i_mid >= list->allocated)
        i_mid -= list->allocated;

      int cmp = compar (list->elements[i_mid], elt);

      if (cmp < 0)
        low = mid + 1;
      else if (cmp > 0)
        high = mid;
      else /* cmp == 0 */
        {
          low = mid;
          break;
        }
    }
  return gl_carray_nx_add_at (list, low, elt);
}

static bool
gl_carray_sortedlist_remove (gl_list_t list, gl_listelement_compar_fn compar,
                             const void *elt)
{
  size_t index = gl_carray_sortedlist_indexof (list, compar, elt);
  if (index == (size_t)(-1))
    return false;
  else
    return gl_carray_remove_at (list, index);
}


const struct gl_list_implementation gl_carray_list_implementation =
  {
    gl_carray_nx_create_empty,
    gl_carray_nx_create,
    gl_carray_size,
    gl_carray_node_value,
    gl_carray_node_nx_set_value,
    gl_carray_next_node,
    gl_carray_previous_node,
    gl_carray_first_node,
    gl_carray_last_node,
    gl_carray_get_at,
    gl_carray_nx_set_at,
    gl_carray_search_from_to,
    gl_carray_indexof_from_to,
    gl_carray_nx_add_first,
    gl_carray_nx_add_last,
    gl_carray_nx_add_before,
    gl_carray_nx_add_after,
    gl_carray_nx_add_at,
    gl_carray_remove_node,
    gl_carray_remove_at,
    gl_carray_remove,
    gl_carray_list_free,
    gl_carray_iterator,
    gl_carray_iterator_from_to,
    gl_carray_iterator_next,
    gl_carray_iterator_free,
    gl_carray_sortedlist_search,
    gl_carray_sortedlist_search_from_to,
    gl_carray_sortedlist_indexof,
    gl_carray_sortedlist_indexof_from_to,
    gl_carray_sortedlist_nx_add,
    gl_carray_sortedlist_remove
  };
