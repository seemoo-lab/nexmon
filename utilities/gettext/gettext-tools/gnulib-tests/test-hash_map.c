/* Test of map data type implementation.
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

#include "gl_hash_map.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "gl_array_map.h"
#include "xalloc.h"
#include "macros.h"

static const char *objects[30] =
  {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "<", ">", "[", "]"
  };

#define SIZE_BITS (sizeof (size_t) * CHAR_BIT)

static bool
string_equals (const void *x1, const void *x2)
{
  const char *s1 = x1;
  const char *s2 = x2;
  return strcmp (s1, s2) == 0;
}

/* A hash function for NUL-terminated char* strings using
   the method described by Bruno Haible.
   See https://www.haible.de/bruno/hashfunc.html.  */
static size_t
string_hash (const void *x)
{
  const char *s = x;
  size_t h = 0;

  for (; *s; s++)
    h = *s + ((h << 9) | (h >> (SIZE_BITS - 9)));

  return h;
}

#define RANDOM(n) (rand () % (n))
#define RANDOM_OBJECT() objects[RANDOM (SIZEOF (objects))]

struct pair
{
  const void *key;
  const void *value;
};

static int
cmp_pairs_in_array (const void *pairptr1, const void *pairptr2)
{
  const void *key1 = ((struct pair const *)pairptr1)->key;
  const void *key2 = ((struct pair const *)pairptr2)->key;
  return strcmp ((const char *) key1, (const char *) key2);
}

static void
check_equals (gl_map_t map1, gl_map_t map2)
{
  size_t n = gl_map_size (map1);
  struct pair *pairs_of_map1 = XNMALLOC (n, struct pair);
  struct pair *pairs_of_map2 = XNMALLOC (n, struct pair);

  gl_map_iterator_t iter1, iter2;
  const void *key1;
  const void *value1;
  const void *key2;
  const void *value2;

  iter1 = gl_map_iterator (map1);
  iter2 = gl_map_iterator (map2);
  for (size_t i = 0; i < n; i++)
    {
      ASSERT (gl_map_iterator_next (&iter1, &key1, &value1));
      ASSERT (gl_map_iterator_next (&iter2, &key2, &value2));
      pairs_of_map1[i].key = key1;
      pairs_of_map1[i].value = value1;
      pairs_of_map2[i].key = key2;
      pairs_of_map2[i].value = value2;
    }
  ASSERT (!gl_map_iterator_next (&iter1, &key1, &value1));
  ASSERT (!gl_map_iterator_next (&iter2, &key2, &value2));
  gl_map_iterator_free (&iter1);
  gl_map_iterator_free (&iter2);

  if (n > 0)
    {
      qsort (pairs_of_map1, n, sizeof (struct pair), cmp_pairs_in_array);
      qsort (pairs_of_map2, n, sizeof (struct pair), cmp_pairs_in_array);
    }
  for (size_t i = 0; i < n; i++)
    {
      ASSERT (pairs_of_map1[i].key == pairs_of_map2[i].key);
      ASSERT (pairs_of_map1[i].value == pairs_of_map2[i].value);
    }
  free (pairs_of_map2);
  free (pairs_of_map1);
}

static void
check_all (gl_map_t map1, gl_map_t map2)
{
  check_equals (map1, map2);
}

int
main (int argc, char *argv[])
{
  gl_map_t map1, map2;

  /* Allow the user to provide a non-default random seed on the command line.  */
  if (argc > 1)
    srand (atoi (argv[1]));

  {
    size_t initial_size = RANDOM (20);

    /* Create map1.  */
    map1 = gl_map_nx_create_empty (GL_ARRAY_MAP,
                                   string_equals, string_hash, NULL, NULL);
    ASSERT (map1 != NULL);

    /* Create map2.  */
    map2 = gl_map_nx_create_empty (GL_HASH_MAP,
                                   string_equals, string_hash, NULL, NULL);
    ASSERT (map2 != NULL);

    check_all (map1, map2);

    /* Initialize them.  */
    for (size_t i = 0; i < initial_size; i++)
      {
        const char *key = RANDOM_OBJECT ();
        const char *value = RANDOM_OBJECT ();
        ASSERT (gl_map_nx_put (map1, key, value) == gl_map_nx_put (map2, key, value));
        check_all (map1, map2);
      }

    for (unsigned int repeat = 0; repeat < 100000; repeat++)
      {
        unsigned int operation = RANDOM (3);
        switch (operation)
          {
          case 0:
            {
              const char *key = RANDOM_OBJECT ();
              ASSERT (gl_map_get (map1, key) == gl_map_get (map2, key));
            }
            break;
          case 1:
            {
              const char *key = RANDOM_OBJECT ();
              const char *value = RANDOM_OBJECT ();
              ASSERT (gl_map_nx_put (map1, key, value) == gl_map_nx_put (map2, key, value));
            }
            break;
          case 2:
            {
              const char *key = RANDOM_OBJECT ();
              ASSERT (gl_map_remove (map1, key) == gl_map_remove (map2, key));
            }
            break;
          }
        check_all (map1, map2);
      }

    gl_map_free (map1);
    gl_map_free (map2);
  }

  return test_exit_status;
}
