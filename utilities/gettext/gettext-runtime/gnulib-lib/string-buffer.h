/* A buffer that accumulates a string by piecewise concatenation.
   Copyright (C) 2021-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2021.  */

/* Before including this file, you may define:
     SB_NO_APPENDF           Provides for more efficient code, assuming that
                             sb_appendvf and sb_appendf will not be called and
                             that 'struct string_buffer' instances are not used
                             across different compilation units.
 */

#ifndef _STRING_BUFFER_H
#define _STRING_BUFFER_H

/* This file uses _GL_ATTRIBUTE_MALLOC, _GL_ATTRIBUTE_RETURNS_NONNULL,
   _GL_ATTRIBUTE_CAPABILITY_TYPE, _GL_ATTRIBUTE_ACQUIRE_CAPABILITY,
   _GL_ATTRIBUTE_RELEASE_CAPABILITY.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <stdarg.h>
#include <stdlib.h>

#include "attribute.h"
#include "string-desc.h"

typedef char * _GL_ATTRIBUTE_CAPABILITY_TYPE ("memory resource")
        sb_heap_allocated_pointer_t;

/* A string buffer type.  */
struct string_buffer
{
  /* data[0 .. length-1] are used.  */
  sb_heap_allocated_pointer_t data;
  size_t length;     /* used bytes, <= allocated */
  size_t allocated;  /* allocated bytes */
  bool oom;          /* true if there was an out-of-memory error */
  bool error;        /* true if there was an error other than out-of-memory */
  char space[1024];  /* stack allocated space */
};

#ifdef __cplusplus
extern "C" {
#endif

/* ================== Functions in module 'string-buffer' ================== */

/* Initializes BUFFER to the empty string.  */
extern void sb_init (struct string_buffer *buffer)
  _GL_ATTRIBUTE_ACQUIRE_CAPABILITY (buffer->data);

/* Appends the character C to BUFFER.
   Returns 0, or -1 in case of out-of-memory error.  */
extern int sb_append1 (struct string_buffer *buffer, char c);

/* Appends the contents of the memory area S to BUFFER.
   Returns 0, or -1 in case of out-of-memory error.  */
extern int sb_append_desc (struct string_buffer *buffer, string_desc_t s);

/* Appends the contents of the C string STR to BUFFER.
   Returns 0, or -1 in case of out-of-memory error.  */
extern int sb_append_c (struct string_buffer *buffer, const char *str);

#ifndef SB_NO_APPENDF

/* Appends the result of the printf-compatible FORMATSTRING with the argument
   list LIST to BUFFER.
   Returns 0, or -1 with errno set in case of error.
   Error code EOVERFLOW can only occur when a width > INT_MAX is used.
   Therefore, if the format string is valid and does not use %ls/%lc
   directives nor widths, the only possible error code is ENOMEM.  */
extern int sb_appendvf (struct string_buffer *buffer,
                        const char *formatstring, va_list list)
  #if (__GNUC__ + (__GNUC_MINOR__ >= 4) > 4) && !defined __clang__
  ATTRIBUTE_FORMAT ((__gnu_printf__, 2, 0))
  #else
  ATTRIBUTE_FORMAT ((__printf__, 2, 0))
  #endif
  ;

/* Appends the result of the printf-compatible FORMATSTRING with the following
   arguments to BUFFER.
   Returns 0, or -1 with errno set in case of error.
   Error code EOVERFLOW can only occur when a width > INT_MAX is used.
   Therefore, if the format string is valid and does not use %ls/%lc
   directives nor widths, the only possible error code is ENOMEM.  */
extern int sb_appendf (struct string_buffer *buffer,
                       const char *formatstring, ...)
  #if (__GNUC__ + (__GNUC_MINOR__ >= 4) > 4) && !defined __clang__
  ATTRIBUTE_FORMAT ((__gnu_printf__, 2, 3))
  #else
  ATTRIBUTE_FORMAT ((__printf__, 2, 3))
  #endif
  ;

#endif

/* Frees the memory held by BUFFER.  */
extern void sb_free (struct string_buffer *buffer)
  _GL_ATTRIBUTE_RELEASE_CAPABILITY (buffer->data);

/* Returns a read-only view of the current contents of BUFFER.
   The result is only valid until the next operation on BUFFER.  */
extern string_desc_t sb_contents (struct string_buffer *buffer);

/* Ensures the contents of BUFFER is followed by a NUL byte (without
   incrementing the length of the contents).
   Then returns a read-only view of the current contents of BUFFER,
   that is, the current contents of BUFFER as a C string.
   Returns NULL upon out-of-memory error.
   The result is only valid until the next operation on BUFFER.  */
extern const char * sb_contents_c (struct string_buffer *buffer);

/* Returns the contents of BUFFER and frees all other memory held by BUFFER.
   Returns (0, NULL) upon failure or if there was an error earlier.
   It is the responsibility of the caller to sd_free() the result.  */
extern rw_string_desc_t sb_dupfree (struct string_buffer *buffer)
  _GL_ATTRIBUTE_RELEASE_CAPABILITY (buffer->data);

/* Returns the contents of BUFFER (with an added trailing NUL, that is,
   as a C string), and frees all other memory held by BUFFER.
   Returns NULL upon failure or if there was an error earlier.
   It is the responsibility of the caller to free() the result.  */
extern char * sb_dupfree_c (struct string_buffer *buffer)
  _GL_ATTRIBUTE_RELEASE_CAPABILITY (buffer->data);

/* ================== Functions in module 'xstring-buffer' ================== */

#if GNULIB_XSTRING_BUFFER

/* The following functions invoke xalloc_die () in case of out-of-memory
   error.  */

/* Appends the character C to BUFFER.  */
extern void sb_xappend1 (struct string_buffer *buffer, char c);

/* Appends the contents of the memory area S to BUFFER.  */
extern void sb_xappend_desc (struct string_buffer *buffer, string_desc_t s);

/* Appends the contents of the C string STR to BUFFER.  */
extern void sb_xappend_c (struct string_buffer *buffer, const char *str);

# ifndef SB_NO_APPENDF

/* Appends the result of the printf-compatible FORMATSTRING with the argument
   list LIST to BUFFER.
   Returns 0, or -1 in case of error other than out-of-memory error.
   Error code EOVERFLOW can only occur when a width > INT_MAX is used.
   Therefore, if the format string is valid and does not use %ls/%lc
   directives nor widths, no error is possible.  */
extern int sb_xappendvf (struct string_buffer *buffer,
                         const char *formatstring, va_list list)
  #if (__GNUC__ + (__GNUC_MINOR__ >= 4) > 4) && !defined __clang__
  ATTRIBUTE_FORMAT ((__gnu_printf__, 2, 0))
  #else
  ATTRIBUTE_FORMAT ((__printf__, 2, 0))
  #endif
  ;

/* Appends the result of the printf-compatible FORMATSTRING with the following
   arguments to BUFFER.
   Returns 0, or -1 in case of error other than out-of-memory error.
   Error code EOVERFLOW can only occur when a width > INT_MAX is used.
   Therefore, if the format string is valid and does not use %ls/%lc
   directives nor widths, no error is possible.  */
extern int sb_xappendf (struct string_buffer *buffer,
                        const char *formatstring, ...)
  #if (__GNUC__ + (__GNUC_MINOR__ >= 4) > 4) && !defined __clang__
  ATTRIBUTE_FORMAT ((__gnu_printf__, 2, 3))
  #else
  ATTRIBUTE_FORMAT ((__printf__, 2, 3))
  #endif
  ;

# endif

/* Ensures the contents of BUFFER is followed by a NUL byte (without
   incrementing the length of the contents).
   Then returns a read-only view of the current contents of BUFFER,
   that is, the current contents of BUFFER as a C string.
   The result is only valid until the next operation on BUFFER.  */
extern const char * sb_xcontents_c (struct string_buffer *buffer)
  _GL_ATTRIBUTE_RETURNS_NONNULL;

/* Returns the contents of BUFFER and frees all other memory held by BUFFER.
   Returns (0, NULL) if there was an error earlier.
   It is the responsibility of the caller to sd_free() the result.  */
extern rw_string_desc_t sb_xdupfree (struct string_buffer *buffer)
  _GL_ATTRIBUTE_RELEASE_CAPABILITY (buffer->data);

/* Returns the contents of BUFFER (with an added trailing NUL, that is,
   as a C string), and frees all other memory held by BUFFER.
   Returns NULL if there was an error earlier.
   It is the responsibility of the caller to free() the result.  */
extern char * sb_xdupfree_c (struct string_buffer *buffer)
  _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_DEALLOC_FREE
# ifdef SB_NO_APPENDF
  _GL_ATTRIBUTE_RETURNS_NONNULL /* because buffer->error is always false */
# endif
  _GL_ATTRIBUTE_RELEASE_CAPABILITY (buffer->data);

#endif /* GNULIB_XSTRING_BUFFER */

/* ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* _STRING_BUFFER_H */
