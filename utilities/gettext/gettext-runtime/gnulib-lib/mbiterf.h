/* Iterating through multibyte strings, faster: macros for multi-byte encodings.
   Copyright (C) 2001, 2005, 2007, 2009-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>,
   with insights from Paul Eggert.  */

/* The macros in this file implement forward iteration through a
   multi-byte string.

   With these macros, an iteration loop that looks like

      char *iter;
      for (iter = buf; iter < buf + buflen; iter++)
        {
          do_something (*iter);
        }

   becomes

      const char *buf_end = buf + buflen;
      mbif_state_t state;
      [const] char *iter;
      for (mbif_init (state), iter = buf; mbif_avail (state, iter, buf_end); )
        {
          mbchar_t cur = mbif_next (state, iter, buf_end);
          // Note: Here always mb_ptr (cur) == iter.
          do_something (iter, mb_len (cur));
          iter += mb_len (cur);
        }

   The benefit of these macros over plain use of mbrtowc or mbrtoc32 is:
   - Handling of invalid multibyte sequences is possible without
     making the code more complicated, while still preserving the
     invalid multibyte sequences.

   The benefit of these macros over those from mbiter.h is that it
   produces faster code with today's optimizing compilers (because mbif_next
   returns its result by value).

   mbif_state_t
     is a type usable for variable declarations.

   mbif_init (state)
     initializes the state.

   mbif_avail (state, iter, endptr)
     returns true if another loop round is needed.

   mbif_next (state, iter, endptr)
     returns the next multibyte character.
     It assumes that the state is initialized and that iter < endptr.

   Here are the function prototypes of the macros.

   extern void      mbif_init (mbif_state_t state);
   extern bool      mbif_avail (mbif_state_t state, const char *iter, const char *endptr);
   extern mbchar_t  mbif_next (mbif_state_t state, const char *iter, const char *endptr);
 */

#ifndef _MBITERF_H
#define _MBITERF_H 1

/* This file uses _GL_INLINE_HEADER_BEGIN, _GL_INLINE,
   _GL_ATTRIBUTE_ALWAYS_INLINE.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <uchar.h>
#include <wchar.h>

#include "mbchar.h"

_GL_INLINE_HEADER_BEGIN
#ifndef MBITERF_INLINE
# define MBITERF_INLINE _GL_INLINE _GL_ATTRIBUTE_ALWAYS_INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct mbif_state
{
  #if !GNULIB_MBRTOC32_REGULAR
  bool in_shift;        /* true if next byte may not be interpreted as ASCII */
                        /* If GNULIB_MBRTOC32_REGULAR, it is always false,
                           so optimize it away.  */
  #endif
  mbstate_t state;      /* if in_shift: current shift state */
                        /* If GNULIB_MBRTOC32_REGULAR, it is in an initial state
                           before and after every mbiterf_next invocation.
                         */
};

MBITERF_INLINE mbchar_t
mbiterf_next (struct mbif_state *ps, const char *iter, const char *endptr)
{
  #if !GNULIB_MBRTOC32_REGULAR
  if (ps->in_shift)
    goto with_shift;
  #endif
  /* Handle most ASCII characters quickly, without calling mbrtowc().  */
  if (is_basic (*iter))
    {
      /* These characters are part of the POSIX portable character set.
         For most of them, namely those in the ISO C basic character set,
         ISO C 99 guarantees that their wide character code is identical to
         their char code.  For the few other ones, this is the case as well,
         in all locale encodings that are in use.  The 32-bit wide character
         code is the same as well.  */
      return (mbchar_t) { .ptr = iter, .bytes = 1, .wc_valid = true, .wc = *iter };
    }
  else
    {
      assert (mbsinit (&ps->state));
      #if !GNULIB_MBRTOC32_REGULAR
      ps->in_shift = true;
    with_shift:;
      #endif
      size_t bytes;
      char32_t wc;
      bytes = mbrtoc32 (&wc, iter, endptr - iter, &ps->state);
      if (bytes == (size_t) -1)
        {
          /* An invalid multibyte sequence was encountered.  */
          /* Allow the next invocation to continue from a sane state.  */
          #if !GNULIB_MBRTOC32_REGULAR
          ps->in_shift = false;
          #endif
          mbszero (&ps->state);
          return (mbchar_t) { .ptr = iter, .bytes = 1, .wc_valid = false };
        }
      else if (bytes == (size_t) -2)
        {
          /* An incomplete multibyte character at the end.  */
          #if !GNULIB_MBRTOC32_REGULAR
          ps->in_shift = false;
          #endif
          /* Whether to reset ps->state or not is not important; the string end
             is reached anyway.  */
          return (mbchar_t) { .ptr = iter, .bytes = endptr - iter, .wc_valid = false };
        }
      else
        {
          if (bytes == 0)
            {
              /* A null wide character was encountered.  */
              bytes = 1;
              assert (*iter == '\0');
              assert (wc == 0);
            }
          #if !GNULIB_MBRTOC32_REGULAR
          else if (bytes == (size_t) -3)
            /* The previous multibyte sequence produced an additional 32-bit
               wide character.  */
            bytes = 0;
          #endif

          /* When in an initial state, we can go back treating ASCII
             characters more quickly.  */
          #if !GNULIB_MBRTOC32_REGULAR
          if (mbsinit (&ps->state))
            ps->in_shift = false;
          #endif
          return (mbchar_t) { .ptr = iter, .bytes = bytes, .wc_valid = true, .wc = wc };
        }
    }
}

/* Iteration macros.  */
typedef struct mbif_state mbif_state_t;
#if !GNULIB_MBRTOC32_REGULAR
#define mbif_init(st) \
  ((st).in_shift = false, mbszero (&(st).state))
#else
/* Optimized: no in_shift.  */
#define mbif_init(st) \
  (mbszero (&(st).state))
#endif
#if !GNULIB_MBRTOC32_REGULAR
#define mbif_avail(st, iter, endptr) ((st).in_shift || ((iter) < (endptr)))
#else
/* Optimized: no in_shift.  */
#define mbif_avail(st, iter, endptr) ((iter) < (endptr))
#endif
#define mbif_next(st, iter, endptr) \
  mbiterf_next (&(st), (iter), (endptr))


#ifdef __cplusplus
}
#endif

_GL_INLINE_HEADER_END

#endif /* _MBITERF_H */
