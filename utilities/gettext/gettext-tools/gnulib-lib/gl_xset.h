/* Abstract set data type, with out-of-memory checking.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

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

#ifndef _GL_XSET_H
#define _GL_XSET_H

/* This file uses _GL_INLINE_HEADER_BEGIN, _GL_INLINE,
   _GL_ATTRIBUTE_RETURNS_NONNULL.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include "gl_set.h"
#include "xalloc.h"

_GL_INLINE_HEADER_BEGIN
#ifndef GL_XSET_INLINE
# define GL_XSET_INLINE _GL_INLINE
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* These functions are thin wrappers around the corresponding functions with
   _nx_ infix from gl_set.h.  Upon out-of-memory, they invoke xalloc_die (),
   instead of returning an error indicator.  */
#if 0 /* These are defined inline below.  */
extern gl_set_t gl_set_create_empty (gl_set_implementation_t implementation,
                                     gl_setelement_equals_fn equals_fn,
                                     gl_setelement_hashcode_fn hashcode_fn,
                                     gl_setelement_dispose_fn dispose_fn)
  /*_GL_ATTRIBUTE_DEALLOC (gl_set_free, 1)*/
  _GL_ATTRIBUTE_RETURNS_NONNULL;
extern bool gl_set_add (gl_set_t set, const void *elt);
#endif

GL_XSET_INLINE
/*_GL_ATTRIBUTE_DEALLOC (gl_set_free, 1)*/
_GL_ATTRIBUTE_RETURNS_NONNULL
gl_set_t
gl_set_create_empty (gl_set_implementation_t implementation,
                     gl_setelement_equals_fn equals_fn,
                     gl_setelement_hashcode_fn hashcode_fn,
                     gl_setelement_dispose_fn dispose_fn)
{
  gl_set_t result =
    gl_set_nx_create_empty (implementation, equals_fn, hashcode_fn, dispose_fn);
  if (result == NULL)
    xalloc_die ();
  return result;
}

GL_XSET_INLINE bool
gl_set_add (gl_set_t set, const void *elt)
{
  int result = gl_set_nx_add (set, elt);
  if (result < 0)
    xalloc_die ();
  return result;
}

#ifdef __cplusplus
}
#endif

_GL_INLINE_HEADER_END

#endif /* _GL_XSET_H */
