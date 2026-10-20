/* obstack.c - subroutines used implicitly by object stack macros
   Copyright (C) 1988-2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */


#ifdef _LIBC
# include <obstack.h>
# include <shlib-compat.h>
#else
# include <libc-config.h>
# include "obstack.h"
#endif

/* NOTE BEFORE MODIFYING THIS FILE IN GNU LIBC: _OBSTACK_INTERFACE_VERSION in
   gnu-versions.h must be incremented whenever callers compiled using an old
   obstack.h can no longer properly call the functions in this file.  */

/* If GCC, or if an oddball (testing?) host that #defines __alignof__,
   use the already-supplied __alignof__.  Otherwise, this must be Gnulib
   (as glibc assumes GCC); defer to Gnulib's alignof.  */
#if !defined __GNUC__ && !defined __alignof__
# define __alignof__(type) alignof (type)
#endif

#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef _OBSTACK_NO_ERROR_HANDLER

/* The functions allocating more room by calling 'obstack_chunk_alloc'
   jump to the handler pointed to by 'obstack_alloc_failed_handler'.
   This can be set to a user defined function which should either
   abort gracefully or use longjump - but shouldn't return.  This
   variable by default points to the internal function
   'print_and_abort'.  */
static __attribute_noreturn__ void print_and_abort (void);
__attribute_noreturn__ void (*obstack_alloc_failed_handler) (void)
  = print_and_abort;

/* Exit value used when 'print_and_abort' is used.  */
# ifdef _LIBC
int obstack_exit_failure = EXIT_FAILURE;
# else
#  include "exitfail.h"
#  define obstack_exit_failure exit_failure
# endif

#endif /* !_OBSTACK_NO_ERROR_HANDLER */

#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_3_4)
/* A looong time ago (before 1994, anyway; we're not sure) this global variable
   was used by non-GNU-C macros to avoid multiple evaluation.  The GNU C
   library still exports it because somebody might use it.  */
struct obstack *_obstack_compat = NULL;
compat_symbol (libc, _obstack_compat, _obstack, GLIBC_2_0);
#endif

/* Set *R to the least multiple of MASK + 1 that is not less than SIZE.
   MASK + 1 must be a power of 2.  Return true (setting *R = 0)
   if the result overflows, false otherwise.  */
static bool
align_chunk_size_up (_OBSTACK_CHUNK_SIZE_T *r, size_t mask,
                     _OBSTACK_CHUNK_SIZE_T size)
{
  return ckd_add (r, mask & -size, size);
}

/* Call functions with either the traditional malloc/free calling
   interface, or the mmalloc/mfree interface (that adds an extra first
   argument), based on the value of use_extra_arg.  */

static void *
call_chunkfun (struct obstack *h, _OBSTACK_CHUNK_SIZE_T size)
{
  if (h->use_extra_arg)
    return h->chunkfun.extra (h->extra_arg, size);
  else
    return h->chunkfun.plain (size);
}

static void
call_freefun (struct obstack *h, void *old_chunk)
{
  if (h->use_extra_arg)
    h->freefun.extra (h->extra_arg, old_chunk);
  else
    h->freefun.plain (old_chunk);
}


/* Initialize an obstack H for use, with given CHUNK_SIZE (0 means default).
   Objects start on multiples of ALIGNMENT (0 means use default).

   Return nonzero if successful, calls obstack_alloc_failed_handler if
   allocation fails.  */

static int
_obstack_begin_worker (struct obstack *h,
                       _OBSTACK_INDEX_T chunk_size, _OBSTACK_INDEX_T alignment)
{
  struct _obstack_chunk *chunk; /* points to new chunk */

  if (alignment == 0)
    alignment = __alignof__ (max_align_t);

  /* The minimum size to request from the allocator, such that the
     result is guaranteed to have enough room to start with the struct
     _obstack_chunk sans contents, followed by minimal padding, up to
     but possibly not including the start of an aligned object.
     This value is zero if no size is large enough.  */
  _OBSTACK_CHUNK_SIZE_T aligned_prefix_size;
  bool v = align_chunk_size_up (&aligned_prefix_size, alignment - 1,
                                offsetof (struct _obstack_chunk, contents));

  _OBSTACK_CHUNK_SIZE_T size = chunk_size;
  if (size < aligned_prefix_size)
    {
      size = aligned_prefix_size;

      /* For speed in the typical case, allocate at least a "good" size.  */
      int good_size = 4000;
      if (size < good_size)
        size = good_size;
    }

  h->chunk_size = size;
  h->alignment_mask = alignment - 1;

  chunk = h->chunk = v ? NULL : call_chunkfun (h, size);
  if (!chunk)
    (*obstack_alloc_failed_handler) ();
  h->next_free = h->object_base = __PTR_ALIGN ((char *) chunk, chunk->contents,
                                               alignment - 1);
  h->chunk_limit = chunk->limit =
    __PTR_ALIGN ((char *) chunk, (char *) chunk + size - (alignment - 1),
                 alignment - 1);
  chunk->prev = NULL;
  /* The initial chunk now contains no empty object.  */
  h->maybe_empty_object = 0;
  h->alloc_failed = 0;
  return 1;
}

int
_obstack_begin (struct obstack *h,
                _OBSTACK_INDEX_T size, _OBSTACK_INDEX_T alignment,
                void *(*chunkfun) (_OBSTACK_CHUNK_SIZE_T),
                void (*freefun) (void *))
{
  h->chunkfun.plain = chunkfun;
  h->freefun.plain = freefun;
  h->use_extra_arg = 0;
  return _obstack_begin_worker (h, size, alignment);
}

int
_obstack_begin_1 (struct obstack *h,
                  _OBSTACK_INDEX_T size, _OBSTACK_INDEX_T alignment,
                  void *(*chunkfun) (void *, _OBSTACK_CHUNK_SIZE_T),
                  void (*freefun) (void *, void *),
                  void *arg)
{
  h->chunkfun.extra = chunkfun;
  h->freefun.extra = freefun;
  h->extra_arg = arg;
  h->use_extra_arg = 1;
  return _obstack_begin_worker (h, size, alignment);
}

/* Allocate a new current chunk for the obstack *H
   on the assumption that LENGTH bytes need to be added
   to the current object, or a new object of length LENGTH allocated.
   Fail if LENGTH <= 0, as this means obstack_grow0's length overflowed.
   Copies any partial object from the end of the old chunk
   to the beginning of the new one.  */

void
_obstack_newchunk (struct obstack *h, _OBSTACK_INDEX_T length)
{
  struct _obstack_chunk *old_chunk = h->chunk;
  size_t obj_size = h->next_free - h->object_base;

  /* Compute size for new chunk.  */
  _OBSTACK_CHUNK_SIZE_T s, new_size;
  bool v = length < 0;
  v |= ckd_add (&s, obj_size, length);
  v |= align_chunk_size_up (&s, h->alignment_mask, s);
  v |= ckd_add (&s, s,
                (offsetof (struct _obstack_chunk, contents)
                 + h->alignment_mask));
  if (ckd_add (&new_size, s, (obj_size >> 3) + 100))
    new_size = s;
  if (new_size < h->chunk_size)
    new_size = h->chunk_size;

  /* Allocate and initialize the new chunk.  */
  struct _obstack_chunk *new_chunk =
    v ? NULL : call_chunkfun (h, new_size);
  if (!new_chunk)
    (*obstack_alloc_failed_handler)();
  h->chunk = new_chunk;
  new_chunk->prev = old_chunk;
  new_chunk->limit = h->chunk_limit =
    __PTR_ALIGN ((char *) new_chunk,
                 (char *) new_chunk + new_size - h->alignment_mask,
                 h->alignment_mask);

  /* Compute an aligned object_base in the new chunk */
  char *object_base =
    __PTR_ALIGN ((char *) new_chunk, new_chunk->contents, h->alignment_mask);

  /* Move the existing object to the new chunk.  */
  memcpy (object_base, h->object_base, obj_size);

  /* If the object just copied was the only data in OLD_CHUNK,
     free that chunk and remove it from the chain.
     But not if that chunk might contain an empty object.  */
  if (!h->maybe_empty_object
      && (h->object_base
          == __PTR_ALIGN ((char *) old_chunk, old_chunk->contents,
                          h->alignment_mask)))
    {
      new_chunk->prev = old_chunk->prev;
      call_freefun (h, old_chunk);
    }

  h->object_base = object_base;
  h->next_free = h->object_base + obj_size;
  /* The new chunk certainly contains no empty object yet.  */
  h->maybe_empty_object = 0;
}
libc_hidden_def (_obstack_newchunk)

/* Return nonzero if object OBJ has been allocated from obstack H.
   This is here for debugging.
   If you use it in a program, you are probably losing.  */

/* Suppress -Wmissing-prototypes warning.  We don't want to declare this in
   obstack.h because it is just for debugging.  */
int _obstack_allocated_p (struct obstack *h, void *obj) __attribute_pure__;

int
_obstack_allocated_p (struct obstack *h, void *obj)
{
  struct _obstack_chunk *lp = /* below addr of any objects in this chunk */
    h->chunk;
  /* We use >= rather than > since the object cannot be exactly at
     the beginning of the chunk but might be an empty object exactly
     at the end of an adjacent chunk.  */
  while (lp != NULL && ((void *) lp >= obj || (void *) (lp)->limit < obj))
    {
      struct _obstack_chunk *plp = lp->prev; /* point to previous chunk */
      lp = plp;
    }
  return lp != NULL;
}

/* Free objects in obstack H, including OBJ and everything allocate
   more recently than OBJ.  If OBJ is zero, free everything in H.  */

void
__obstack_free (struct obstack *h, void *obj)
{
  struct _obstack_chunk *lp = /* below addr of any objects in this chunk */
    h->chunk;
  /* We use >= because there cannot be an object at the beginning of a chunk.
     But there can be an empty object at that address
     at the end of another chunk.  */
  while (lp != NULL && ((void *) lp >= obj || (void *) (lp)->limit < obj))
    {
      struct _obstack_chunk *plp = lp->prev; /* point to previous chunk */
      call_freefun (h, lp);
      lp = plp;
      /* If we switch chunks, we can't tell whether the new current
         chunk contains an empty object, so assume that it may.  */
      h->maybe_empty_object = 1;
    }
  if (lp)
    {
      h->object_base = h->next_free = (char *) (obj);
      h->chunk_limit = lp->limit;
      h->chunk = lp;
    }
  else if (obj != NULL)
    /* obj is not in any of the chunks! */
    abort ();
}

/* Older versions of libc used a function _obstack_free intended to be
   called by non-GCC compilers.  */
#undef obstack_free
strong_alias (_obstack_free, obstack_free)

_OBSTACK_INDEX_T
_obstack_memory_used (struct obstack *h)
{
  _OBSTACK_INDEX_T nbytes = 0;

  for (struct _obstack_chunk *lp = h->chunk; lp != NULL; lp = lp->prev)
    {
      nbytes += lp->limit - (char *) lp;
    }
  return nbytes;
}

#ifndef _OBSTACK_NO_ERROR_HANDLER
/* Define the error handler.  */
# include <stdio.h>

# ifdef _LIBC
#  include <libintl.h>
#  ifndef _
#   define _(msgid) gettext (msgid)
#  endif
# else
#  include "gettext.h"
#  ifndef _
#   define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)
#  endif
# endif

# ifdef _LIBC
#  include <libio/iolibio.h>
# endif

static __attribute_noreturn__ void
print_and_abort (void)
{
  /* Don't change any of these strings.  Yes, it would be possible to add
     the newline to the string and use fputs or so.  But this must not
     happen because the "memory exhausted" message appears in other places
     like this and the translation should be reused instead of creating
     a very similar string which requires a separate translation.  */
# ifdef _LIBC
  (void) __fxprintf (NULL, "%s\n", _("memory exhausted"));
# else
  fprintf (stderr, "%s\n", _("memory exhausted"));
# endif
  exit (obstack_exit_failure);
}
#endif /* !_OBSTACK_NO_ERROR_HANDLER */
