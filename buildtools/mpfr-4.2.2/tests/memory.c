/* Memory allocation used during tests.

Copyright 2001-2003, 2006-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>. */

/* Note: this file originally came from GMP's tests/memory.c
   (some features have been added). */

#define MPFR_NEED_INTMAX_H
#include "mpfr-test.h"

/* Each block allocated is a separate malloc, for the benefit of a redzoning
   malloc debugger during development or when bug hunting.

   Sizes passed when reallocating or freeing are checked (the default
   routines don't care about these).

   Memory leaks are checked by requiring that all blocks have been freed
   when tests_memory_end() is called.  Test programs must be sure to have
   "clear"s for all temporary variables used.  */

/* Note about error messages
   -------------------------
   Error messages in MPFR are usually written to stdout. However, those
   coming from the memory allocator need to be written to stderr in order
   to be visible when the standard output is redirected, e.g. in the tests
   of I/O functions (like tprintf). For consistency, all error messages in
   this file should be written to stderr. */

struct header {
  void           *ptr;
  size_t         size;
  struct header  *next;
};

/* The memory limit can be changed with the MPFR_TESTS_MEMORY_LIMIT
   environment variable. This is normally not necessary (a failure
   would mean a bug), thus not recommended, for "make check". But
   some test programs can take arguments for particular tests, which
   may need more memory. This variable is exported, so that such
   programs may also change the memory limit. */
size_t tests_memory_limit = DEFAULT_MEMORY_LIMIT;

static struct header  *tests_memory_list;
static size_t tests_total_size = 0;
MPFR_LOCK_DECL(mpfr_lock_memory)

static void *
mpfr_default_allocate (size_t size)
{
  void *ret;
  ret = malloc (size);
  if (MPFR_UNLIKELY (ret == NULL))
    {
      fprintf (stderr, "[MPFR] mpfr_default_allocate(): "
               "can't allocate memory (size=%" MPFR_INTMAX_FSPEC "u)\n",
               (mpfr_uintmax_t) size);
      fflush (NULL);
      abort ();
    }
  return ret;
}

static void *
mpfr_default_reallocate (void *oldptr, size_t old_size, size_t new_size)
{
  void *ret;
  ret = realloc (oldptr, new_size);
  if (MPFR_UNLIKELY(ret == NULL))
    {
      fprintf (stderr, "[MPFR] mpfr_default_reallocate(): "
               "can't reallocate memory (old_size=%" MPFR_INTMAX_FSPEC
               "u new_size=%" MPFR_INTMAX_FSPEC "u)\n",
               (mpfr_uintmax_t) old_size, (mpfr_uintmax_t) new_size);
      fflush (NULL);
      abort ();
    }
  return ret;
}

static void
mpfr_default_free (void *blk_ptr, size_t blk_size)
{
  free (blk_ptr);
}

/* Return a pointer to a pointer to the found block (so it can be updated
   when unlinking). */
/* FIXME: This is a O(n) search, while it could be done in nearly
   constant time with a better data structure! */
static struct header **
tests_memory_find (void *ptr)
{
  struct header  **hp;

  for (hp = &tests_memory_list; *hp != NULL; hp = &((*hp)->next))
    if ((*hp)->ptr == ptr)
      return hp;

  return NULL;
}

/*
static int
tests_memory_valid (void *ptr)
{
  return (tests_memory_find (ptr) != NULL);
}
*/

static void
tests_addsize (size_t size)
{
  tests_total_size += size;
  if (tests_total_size > tests_memory_limit)
    {
      /* The total size taken by MPFR on the heap is more than 4 MB:
         either a bug or a huge inefficiency. */
      fprintf (stderr, "[MPFR] tests_addsize(): "
               "too much memory (%" MPFR_INTMAX_FSPEC "u bytes)\n",
              (mpfr_uintmax_t) tests_total_size);
      fflush (NULL);
      abort ();
    }
}

void *
tests_allocate (size_t size)
{
  struct header  *h;

  MPFR_LOCK_WRITE(mpfr_lock_memory);

  if (size == 0)
    {
      fprintf (stderr, "[MPFR] tests_allocate(): "
               "attempt to allocate 0 bytes\n");
      fflush (NULL);
      abort ();
    }

  tests_addsize (size);

  h = (struct header *) mpfr_default_allocate (sizeof (*h));
  h->next = tests_memory_list;
  tests_memory_list = h;

  h->size = size;
  h->ptr = mpfr_default_allocate (size);

  MPFR_UNLOCK_WRITE(mpfr_lock_memory);

  return h->ptr;
}

/* Note: the double cast (mpfr_uintmax_t) (uintptr_t) below allows to avoid a
   pointer-to-int-cast warning with GCC. The AC_TYPE_UINTPTR_T Autoconf macro
   must be used to define uintptr_t if not available.
   Note that pointers may be larger than uintmax_t, even in practice[*];
   however, since this is just used in error messages, the loss of
   information may be acceptable (but we should probably use %p).
   [*] https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2889.htm
*/
void *
tests_reallocate (void *ptr, size_t old_size, size_t new_size)
{
  struct header  **hp, *h;

  MPFR_LOCK_WRITE(mpfr_lock_memory);

  if (new_size == 0)
    {
      fprintf (stderr, "[MPFR] tests_reallocate(): "
               "attempt to reallocate 0x%" MPFR_INTMAX_FSPEC "X to 0 bytes\n",
               (mpfr_uintmax_t) (uintptr_t) ptr);
      fflush (NULL);
      abort ();
    }

  hp = tests_memory_find (ptr);
  if (hp == NULL)
    {
      fprintf (stderr, "[MPFR] tests_reallocate(): "
               "attempt to reallocate bad pointer 0x%" MPFR_INTMAX_FSPEC "X\n",
              (mpfr_uintmax_t) (uintptr_t) ptr);
      fflush (NULL);
      abort ();
    }
  h = *hp;

  if (h->size != old_size)
    {
      /* Note: we should use the standard %zu to print sizes, but
         this is not supported by old C implementations. */
      fprintf (stderr, "[MPFR] tests_reallocate(): "
               "bad old size %" MPFR_INTMAX_FSPEC
               "u, should be %" MPFR_INTMAX_FSPEC "u\n",
              (mpfr_uintmax_t) old_size, (mpfr_uintmax_t) h->size);
      fflush (NULL);
      abort ();
    }

  tests_total_size -= old_size;
  tests_addsize (new_size);

  h->size = new_size;
  h->ptr = mpfr_default_reallocate (ptr, old_size, new_size);

  MPFR_UNLOCK_WRITE(mpfr_lock_memory);

  return h->ptr;
}

static struct header **
tests_free_find (void *ptr)
{
  struct header  **hp = tests_memory_find (ptr);
  if (hp == NULL)
    {
      fprintf (stderr, "[MPFR] tests_free(): "
               "attempt to free bad pointer 0x%" MPFR_INTMAX_FSPEC "X\n",
              (mpfr_uintmax_t) (uintptr_t) ptr);
      fflush (NULL);
      abort ();
    }
  return hp;
}

static void
tests_free_nosize (void *ptr)
{
  struct header  **hp = tests_free_find (ptr);
  struct header  *h = *hp;

  *hp = h->next;  /* unlink */

  mpfr_default_free (ptr, h->size);
  mpfr_default_free (h, sizeof (*h));
}

void
tests_free (void *ptr, size_t size)
{
  struct header  **hp;
  struct header  *h;

  MPFR_LOCK_WRITE(mpfr_lock_memory);

  hp = tests_free_find (ptr);
  h = *hp;

  if (h->size != size)
    {
      /* Note: we should use the standard %zu to print sizes, but
         this is not supported by old C implementations. */
      fprintf (stderr, "[MPFR] tests_free(): bad size %"
               MPFR_INTMAX_FSPEC "u, should be %" MPFR_INTMAX_FSPEC "u\n",
              (mpfr_uintmax_t) size, (mpfr_uintmax_t) h->size);
      fflush (NULL);
      abort ();
    }

  tests_total_size -= size;
  tests_free_nosize (ptr);

  MPFR_UNLOCK_WRITE(mpfr_lock_memory);
}

void
tests_memory_start (void)
{
  char *p;

  tests_memory_list = NULL;
  mp_set_memory_functions (tests_allocate, tests_reallocate, tests_free);

  p = getenv ("MPFR_TESTS_MEMORY_LIMIT");
  if (p != NULL)
    {
      tests_memory_limit = strtoul (p, NULL, 0);
      if (tests_memory_limit == 0)
        tests_memory_limit = (size_t) -1;  /* no memory limit */
    }
}

void
tests_memory_end (void)
{
  if (tests_memory_list != NULL)
    {
      struct header  *h;
      unsigned  count;

      fprintf (stderr, "[MPFR] tests_memory_end(): not all memory freed\n");

      count = 0;
      for (h = tests_memory_list; h != NULL; h = h->next)
        count++;

      fprintf (stderr, "[MPFR]    %u blocks remaining\n", count);
      fflush (NULL);
      abort ();
    }
}
