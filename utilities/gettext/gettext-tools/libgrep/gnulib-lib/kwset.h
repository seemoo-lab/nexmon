/* kwset.h - header declaring the keyword set library.
   Copyright (C) 1989, 1998, 2005, 2007, 2009-2026 Free Software Foundation,
   Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written August 1989 by Mike Haertel.  */

#ifndef _KWSET_H
#define _KWSET_H

/* This file uses _GL_ATTRIBUTE_PURE, _GL_ATTRIBUTE_RETURNS_NONNULL.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <stddef.h>

#include "arg-nonnull.h"
#include "idx.h"

#ifdef __cplusplus
extern "C" {
#endif

struct kwsmatch
{
  idx_t index;  /* Index number of matching keyword.  */
  idx_t offset; /* Offset of match.  */
  idx_t size;   /* Length of match.  */
};

struct kwset;
typedef struct kwset *kwset_t;

/* Return a newly allocated keyword set.  A nonnull TRANS specifies a
   table (indexed by 'unsigned char' values: 0..UCHAR_MAX+1) of character
   translations to be applied to all pattern and search text.  */
extern kwset_t kwsalloc (char const *trans)
  _GL_ATTRIBUTE_RETURNS_NONNULL;

/* Add the given string to the contents of the keyword set.  */
extern void kwsincr (kwset_t kwset, char const *text, idx_t len)
  _GL_ARG_NONNULL ((1, 2));

/* Return the number of keywords in SET.  */
extern idx_t kwswords (kwset_t kwset)
  _GL_ARG_NONNULL ((1)) _GL_ATTRIBUTE_PURE;

/* Prepare a built keyword set for use.  */
extern void kwsprep (kwset_t kwset)
  _GL_ARG_NONNULL ((1));

/* Find the first instance of a KWSET member in TEXT, which has SIZE bytes.
   Return the offset (into TEXT) of the first byte of the matching substring,
   or -1 if no match is found.
   Upon a match:
     - Store details in *MATCH: index of matched keyword, start offset
       (same as the return value), and length.
     - If LONGEST, find the longest match that starts at this offset;
       otherwise any match that starts at this offset will do.
   NOTE! LONGEST does *not* mean to search for the longest KWSET member
   across the entire string.  */
extern ptrdiff_t kwsexec (kwset_t kwset, char const *text, idx_t size,
                          struct kwsmatch *match, bool longest)
  _GL_ARG_NONNULL ((1, 2, 4));

/* Free the components of the given keyword set.  */
extern void kwsfree (kwset_t kwset)
  _GL_ARG_NONNULL ((1));

#ifdef __cplusplus
}
#endif

#endif /* _KWSET_H */
