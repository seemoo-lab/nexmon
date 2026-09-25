/* Return diagnostic string based on error code.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

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

#ifndef _XSTRERROR_H
#define _XSTRERROR_H

/* This file uses _GL_ATTRIBUTE_MALLOC, _GL_ATTRIBUTE_RETURNS_NONNULL.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

/* Get 'free'.  */
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return a freshly allocated diagnostic string that contains a given string
   MESSAGE and the (internationalized) description of the error code ERRNUM.
   Upon [ENOMEM] memory allocation error, call xalloc_die.

   This function is multithread-safe.  */
extern char *xstrerror (const char *message, int errnum)
       _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_DEALLOC_FREE
       _GL_ATTRIBUTE_RETURNS_NONNULL;

#ifdef __cplusplus
}
#endif

#endif /* _XSTRERROR_H */
