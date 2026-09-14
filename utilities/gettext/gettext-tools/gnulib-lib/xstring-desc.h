/* String descriptors, with out-of-memory checking.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2023.  */

#ifndef _XSTRING_DESC_H
#define _XSTRING_DESC_H 1

/* This file uses _GL_INLINE_HEADER_BEGIN, _GL_INLINE,
   _GL_ATTRIBUTE_DEALLOC_FREE, _GL_ATTRIBUTE_NONNULL_IF_NONZERO,
   _GL_ATTRIBUTE_RETURNS_NONNULL.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <string.h>
#include "string-desc.h"
#include "xalloc.h"


_GL_INLINE_HEADER_BEGIN
#ifndef GL_XSTRING_DESC_INLINE
# define GL_XSTRING_DESC_INLINE _GL_INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* ==== Memory-allocating operations on string descriptors ==== */

/* Return a string of length N, with uninitialized contents.  */
#if 0 /* Defined inline below.  */
extern rw_string_desc_t xsd_new (idx_t n);
#endif

/* Return a string of length N, filled with C.  */
#if 0 /* Defined inline below.  */
extern rw_string_desc_t xsd_new_filled (idx_t n, char c);
#endif

/* Return a copy of string S.  */
#if 0 /* Defined inline below.  */
extern rw_string_desc_t xsd_copy (string_desc_t s);
#endif

/* Return the concatenation of N strings.  N must be > 0.  */
extern rw_string_desc_t xsd_concat (idx_t n, /* [rw_]string_desc_t string1, */ ...);

/* Construct and return a copy of string S, as a NUL-terminated C string.  */
#if 0 /* Defined inline below.  */
extern char * xsd_c ([rw_]string_desc_t s) _GL_ATTRIBUTE_DEALLOC_FREE;
#endif


/* ==== Inline function definitions ==== */

GL_XSTRING_DESC_INLINE rw_string_desc_t
xsd_new (idx_t n)
{
  rw_string_desc_t result;
  if (sd_new (&result, n) < 0)
    xalloc_die ();
  return result;
}

GL_XSTRING_DESC_INLINE rw_string_desc_t
xsd_new_filled (idx_t n, char c)
{
  rw_string_desc_t result;
  if (sd_new_filled (&result, n, c) < 0)
    xalloc_die ();
  return result;
}

GL_XSTRING_DESC_INLINE rw_string_desc_t
xsd_copy (string_desc_t s)
{
  rw_string_desc_t result;
  if (sd_copy (&result, s) < 0)
    xalloc_die ();
  return result;
}

GL_XSTRING_DESC_INLINE
_GL_ATTRIBUTE_DEALLOC_FREE _GL_ATTRIBUTE_RETURNS_NONNULL
char *
_xsd_c (idx_t s_nbytes, const char *s_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
GL_XSTRING_DESC_INLINE
_GL_ATTRIBUTE_DEALLOC_FREE _GL_ATTRIBUTE_RETURNS_NONNULL
char *
_xsd_c (idx_t s_nbytes, const char *s_data)
{
  char *result = _sd_c (s_nbytes, s_data);
  if (result == NULL)
    xalloc_die ();
  return result;
}
#if HAVE_RW_STRING_DESC
# define xsd_c(s) \
   ({typeof (s) _xs_ = (s); \
     _xsd_c (_xs_._nbytes, _xs_._data); \
   })
#else
GL_STRING_DESC_INLINE
_GL_ATTRIBUTE_DEALLOC_FREE _GL_ATTRIBUTE_RETURNS_NONNULL
char *
xsd_c (string_desc_t s)
{
  return _xsd_c (s._nbytes, s._data);
}
#endif


#ifdef __cplusplus
}
#endif

_GL_INLINE_HEADER_END


#endif /* _XSTRING_DESC_H */
