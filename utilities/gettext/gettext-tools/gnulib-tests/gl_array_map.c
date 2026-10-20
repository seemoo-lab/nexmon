/* Map data type implemented by an array.
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
#include "gl_array_map.h"

#include <stdint.h>
#include <stdlib.h>

/* Checked size_t computations.  */
#include "xsize.h"

/* --------------------------- gl_map_t Data Type --------------------------- */

struct pair
{
  const void *key;
  const void *value;
};

/* Concrete gl_map_impl type, valid for this file only.  */
struct gl_map_impl
{
  struct gl_map_impl_base base;
  /* An array of ALLOCATED pairs, of which the first COUNT are used.
     0 <= COUNT <= ALLOCATED.  */
  struct pair *pairs;
  size_t count;
  size_t allocated;
};

static gl_map_t
gl_array_nx_create_empty (gl_map_implementation_t implementation,
                          gl_mapkey_equals_fn equals_fn,
                          gl_mapkey_hashcode_fn hashcode_fn,
                          gl_mapkey_dispose_fn kdispose_fn,
                          gl_mapvalue_dispose_fn vdispose_fn)
{
  struct gl_map_impl *map =
    (struct gl_map_impl *) malloc (sizeof (struct gl_map_impl));

  if (map == NULL)
    return NULL;

  map->base.vtable = implementation;
  map->base.equals_fn = equals_fn;
  map->base.kdispose_fn = kdispose_fn;
  map->base.vdispose_fn = vdispose_fn;
  map->pairs = NULL;
  map->count = 0;
  map->allocated = 0;

  return map;
}

static size_t
gl_array_size (gl_map_t map)
{
  return map->count;
}

static size_t
gl_array_indexof (gl_map_t map, const void *key)
{
  size_t count = map->count;

  if (count > 0)
    {
      gl_mapkey_equals_fn equals = map->base.equals_fn;
      if (equals != NULL)
        {
          for (size_t i = 0; i < count; i++)
            if (equals (map->pairs[i].key, key))
              return i;
        }
      else
        {
          for (size_t i = 0; i < count; i++)
            if (map->pairs[i].key == key)
              return i;
        }
    }
  return (size_t)(-1);
}

static bool
gl_array_search (gl_map_t map, const void *key, const void **valuep)
{
  size_t index = gl_array_indexof (map, key);
  if (index != (size_t)(-1))
    {
      *valuep = map->pairs[index].value;
      return true;
    }
  else
    return false;
}

/* Ensure that map->allocated > map->count.
   Return 0 upon success, -1 upon out-of-memory.  */
static int
grow (gl_map_t map)
{
  size_t new_allocated = xtimes (map->allocated, 2);
  new_allocated = xsum (new_allocated, 1);
  size_t memory_size = xtimes (new_allocated, sizeof (struct pair));
  if (size_overflow_p (memory_size))
    /* Overflow, would lead to out of memory.  */
    return -1;
  struct pair *memory = (struct pair *) realloc (map->pairs, memory_size);
  if (memory == NULL)
    /* Out of memory.  */
    return -1;
  map->pairs = memory;
  map->allocated = new_allocated;
  return 0;
}

static int
gl_array_nx_getput (gl_map_t map, const void *key, const void *value,
                    const void **oldvaluep)
{
  size_t index = gl_array_indexof (map, key);
  if (index != (size_t)(-1))
    {
      *oldvaluep = map->pairs[index].value;
      map->pairs[index].value = value;
      return 0;
    }
  else
    {
      size_t count = map->count;
      if (count == map->allocated)
        if (grow (map) < 0)
          return -1;
      struct pair *pairs = map->pairs;
      pairs[count].key = key;
      pairs[count].value = value;
      map->count = count + 1;
      return 1;
    }
}

/* Remove the pair at the given position,
   0 <= position < gl_map_size (map).  */
static void
gl_array_remove_at (gl_map_t map, size_t position)
{
  size_t count = map->count;
  struct pair *pairs = map->pairs;
  if (map->base.kdispose_fn != NULL)
    map->base.kdispose_fn (pairs[position].key);
  for (size_t i = position + 1; i < count; i++)
    pairs[i - 1] = pairs[i];
  map->count = count - 1;
}

static bool
gl_array_getremove (gl_map_t map, const void *key, const void **oldvaluep)
{
  size_t index = gl_array_indexof (map, key);
  if (index != (size_t)(-1))
    {
      *oldvaluep = map->pairs[index].value;
      gl_array_remove_at (map, index);
      return true;
    }
  else
    return false;
}

static void
gl_array_free (gl_map_t map)
{
  if (map->pairs != NULL)
    {
      if (map->base.kdispose_fn != NULL || map->base.vdispose_fn != NULL)
        {
          size_t count = map->count;

          if (count > 0)
            {
              gl_mapkey_dispose_fn kdispose = map->base.kdispose_fn;
              gl_mapvalue_dispose_fn vdispose = map->base.vdispose_fn;
              struct pair *pairs = map->pairs;

              do
                {
                  if (vdispose)
                    vdispose (pairs->value);
                  if (kdispose)
                    kdispose (pairs->key);
                  pairs++;
                }
              while (--count > 0);
            }
        }
      free (map->pairs);
    }
  free (map);
}

/* ---------------------- gl_map_iterator_t Data Type ---------------------- */

static gl_map_iterator_t
gl_array_iterator (gl_map_t map)
{
  gl_map_iterator_t result;

  result.vtable = map->base.vtable;
  result.map = map;
  result.count = map->count;
  result.p = map->pairs + 0;
  result.q = map->pairs + map->count;
#if defined GCC_LINT || defined lint
  result.i = 0;
  result.j = 0;
#endif

  return result;
}

static bool
gl_array_iterator_next (gl_map_iterator_t *iterator,
                        const void **keyp, const void **valuep)
{
  gl_map_t map = iterator->map;
  if (iterator->count != map->count)
    {
      if (iterator->count != map->count + 1)
        /* Concurrent modifications were done on the map.  */
        abort ();
      /* The last returned pair was removed.  */
      iterator->count--;
      iterator->p = (struct pair *) iterator->p - 1;
      iterator->q = (struct pair *) iterator->q - 1;
    }
  if (iterator->p < iterator->q)
    {
      struct pair *p = (struct pair *) iterator->p;
      *keyp = p->key;
      *valuep = p->value;
      iterator->p = p + 1;
      return true;
    }
  else
    return false;
}

static void
gl_array_iterator_free (gl_map_iterator_t *iterator)
{
}


const struct gl_map_implementation gl_array_map_implementation =
  {
    gl_array_nx_create_empty,
    gl_array_size,
    gl_array_search,
    gl_array_nx_getput,
    gl_array_getremove,
    gl_array_free,
    gl_array_iterator,
    gl_array_iterator_next,
    gl_array_iterator_free
  };
