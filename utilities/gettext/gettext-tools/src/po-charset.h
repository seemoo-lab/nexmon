/* Charset handling while reading PO files.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

#ifndef _PO_CHARSET_H
#define _PO_CHARSET_H

#include <stdbool.h>
#include <stddef.h>

#if HAVE_ICONV
#include <iconv.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Canonicalize an encoding name.
   The results of this function are statically allocated and can be
   compared using ==.
   Return NULL if CHARSET is not a valid encoding name.  */
extern const char *po_charset_canonicalize (const char *charset);

/* The canonicalized encoding name for ASCII.  */
extern LIBGETTEXTSRC_DLL_VARIABLE const char *po_charset_ascii;

/* The canonicalized encoding name for UTF-8.  */
extern LIBGETTEXTSRC_DLL_VARIABLE const char *po_charset_utf8;

/* Test for ASCII compatibility.  */
extern bool po_charset_ascii_compatible (const char *canon_charset);

/* Test for a weird encoding, i.e. an encoding which has double-byte
   characters ending in 0x5C.  */
extern bool po_is_charset_weird (const char *canon_charset);

/* Test for a weird CJK encoding, i.e. a weird encoding with CJK structure.
   An encoding has CJK structure if every valid character stream is composed
   of single bytes in the range 0x{00..7F} and of byte pairs in the range
   0x{80..FF}{30..FF}.  */
extern bool po_is_charset_weird_cjk (const char *canon_charset);

/* Returns a character iterator for a given encoding.
   Given a pointer into a string, it returns the number occupied by the next
   single character.  If the piece of string is not valid or if the *s == '\0',
   it returns 1.  */
typedef size_t (*character_iterator_t) (const char *s);
extern character_iterator_t po_charset_character_iterator (const char *canon_charset);


#ifdef __cplusplus
}
#endif


#endif /* _PO_CHARSET_H */
