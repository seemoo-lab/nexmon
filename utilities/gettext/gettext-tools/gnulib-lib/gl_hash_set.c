/* Set data type implemented by a hash table.
   Copyright (C) 2006, 2008-2026 Free Software Foundation, Inc.
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
#include "gl_hash_set.h"

#include <stdint.h> /* for uintptr_t, SIZE_MAX */
#include <stdlib.h>

#include "xsize.h"

/* --------------------------- gl_set_t Data Type --------------------------- */

#include "gl_anyhash1.h"

/* Concrete list node implementation, valid for this file only.  */
struct gl_list_node_impl
{
  struct gl_hash_entry h;           /* hash table entry fields; must be first */
  const void *value;
};
typedef struct gl_list_node_impl * gl_list_node_t;

/* Concrete gl_set_impl type, valid for this file only.  */
struct gl_set_impl
{
  struct gl_set_impl_base base;
  gl_setelement_hashcode_fn hashcode_fn;
  /* A hash table: managed as an array of collision lists.  */
  struct gl_hash_entry **table;
  size_t table_size;
  /* Number of hash table entries.  */
  size_t count;
};

#define CONTAINER_T gl_set_t
#define CONTAINER_COUNT(set) (set)->count
#include "gl_anyhash2.h"

/* --------------------------- gl_set_t Data Type --------------------------- */

static gl_set_t
gl_hash_nx_create_empty (gl_set_implementation_t implementation,
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
  set->hashcode_fn = hashcode_fn;
  set->table_size = 11;
  set->table =
    (gl_hash_entry_t *) calloc (set->table_size, sizeof (gl_hash_entry_t));
  if (set->table == NULL)
    goto fail;
  set->count = 0;

  return set;

 fail:
  free (set);
  return NULL;
}

static size_t _GL_ATTRIBUTE_PURE
gl_hash_size (gl_set_t set)
{
  return set->count;
}

static bool _GL_ATTRIBUTE_PURE
gl_hash_search (gl_set_t set, const void *elt)
{
  size_t hashcode =
    (set->hashcode_fn != NULL
     ? set->hashcode_fn (elt)
     : (size_t)(uintptr_t) elt);
  size_t bucket = hashcode % set->table_size;
  gl_setelement_equals_fn equals = set->base.equals_fn;

  /* Look for a match in the hash bucket.  */
  for (gl_list_node_t node = (gl_list_node_t) set->table[bucket];
       node != NULL;
       node = (gl_list_node_t) node->h.hash_next)
    if (node->h.hashcode == hashcode
        && (equals != NULL
            ? equals (elt, node->value)
            : elt == node->value))
      return true;
  return false;
}

static int
gl_hash_nx_add (gl_set_t set, const void *elt)
{
  size_t hashcode =
    (set->hashcode_fn != NULL
     ? set->hashcode_fn (elt)
     : (size_t)(uintptr_t) elt);
  size_t bucket = hashcode % set->table_size;
  gl_setelement_equals_fn equals = set->base.equals_fn;

  /* Look for a match in the hash bucket.  */
  for (gl_list_node_t node = (gl_list_node_t) set->table[bucket];
       node != NULL;
       node = (gl_list_node_t) node->h.hash_next)
    if (node->h.hashcode == hashcode
        && (equals != NULL
            ? equals (elt, node->value)
            : elt == node->value))
      return 0;

  /* Allocate a new node.  */
  gl_list_node_t node =
    (struct gl_list_node_impl *) malloc (sizeof (struct gl_list_node_impl));

  if (node == NULL)
    return -1;

  node->value = elt;
  node->h.hashcode = hashcode;

  /* Add node to the hash table.  */
  node->h.hash_next = set->table[bucket];
  set->table[bucket] = &node->h;

  /* Add node to the set.  */
  set->count++;

  hash_resize_after_add (set);

  return 1;
}

static bool
gl_hash_remove (gl_set_t set, const void *elt)
{
  size_t hashcode =
    (set->hashcode_fn != NULL
     ? set->hashcode_fn (elt)
     : (size_t)(uintptr_t) elt);
  size_t bucket = hashcode % set->table_size;
  gl_setelement_equals_fn equals = set->base.equals_fn;

  /* Look for the first match in the hash bucket.  */
  for (gl_list_node_t *nodep = (gl_list_node_t *) &set->table[bucket];
       *nodep != NULL;
       nodep = (gl_list_node_t *) &(*nodep)->h.hash_next)
    {
      gl_list_node_t node = *nodep;
      if (node->h.hashcode == hashcode
          && (equals != NULL
              ? equals (elt, node->value)
              : elt == node->value))
        {
          /* Remove node from the hash table.  */
          *nodep = (gl_list_node_t) node->h.hash_next;

          /* Remove node from the set.  */
          set->count--;

          if (set->base.dispose_fn != NULL)
            set->base.dispose_fn (node->value);
          free (node);
          return true;
        }
    }

  return false;
}

static void
gl_hash_free (gl_set_t set)
{
  if (set->count > 0)
    {
      gl_setelement_dispose_fn dispose = set->base.dispose_fn;
      struct gl_hash_entry **table = set->table;

      for (size_t i = set->table_size; i > 0; )
        {
          gl_hash_entry_t node = table[--i];

          while (node != NULL)
            {
              gl_hash_entry_t next = node->hash_next;

              /* Free the entry.  */
              if (dispose != NULL)
                dispose (((gl_list_node_t) node)->value);
              free (node);

              node = next;
            }
        }
    }

  free (set->table);
  free (set);
}

/* ---------------------- gl_set_iterator_t Data Type ---------------------- */

/* Here we iterate through the hash buckets.  Therefore the order in which the
   elements are returned is unpredictable.  */

static gl_set_iterator_t
gl_hash_iterator (gl_set_t set)
{
  gl_set_iterator_t result;

  result.vtable = set->base.vtable;
  result.set = set;
  result.p = NULL;         /* runs through the nodes of a bucket */
  result.i = 0;            /* index of the bucket that p points into + 1 */
  result.j = set->table_size;
#if defined GCC_LINT || defined lint
  result.q = NULL;
  result.count = 0;
#endif

  return result;
}

static bool
gl_hash_iterator_next (gl_set_iterator_t *iterator, const void **eltp)
{
  if (iterator->p != NULL)
    {
      /* We're traversing a bucket.  */
      gl_list_node_t node = (gl_list_node_t) iterator->p;
      *eltp = node->value;
      iterator->p = (gl_list_node_t) node->h.hash_next;
      return true;
    }
  else
    {
      /* Find the next bucket that is not empty.  */
      size_t j = iterator->j;
      size_t i = iterator->i;

      if (i < j)
        {
          gl_hash_entry_t *table = iterator->set->table;
          do
            {
              gl_list_node_t node = (gl_list_node_t) table[i++];
              if (node != NULL)
                {
                  *eltp = node->value;
                  iterator->p = (gl_list_node_t) node->h.hash_next;
                  iterator->i = i;
                  return true;
                }
            }
          while (i < j);
        }
      /* Here iterator->p is still NULL, and i == j.  */
      iterator->i = j;
      return false;
    }
}

static void
gl_hash_iterator_free (gl_set_iterator_t *iterator)
{
}


const struct gl_set_implementation gl_hash_set_implementation =
  {
    gl_hash_nx_create_empty,
    gl_hash_size,
    gl_hash_search,
    gl_hash_nx_add,
    gl_hash_remove,
    gl_hash_free,
    gl_hash_iterator,
    gl_hash_iterator_next,
    gl_hash_iterator_free
  };
