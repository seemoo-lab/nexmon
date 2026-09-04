/* Support for using a string as a hash key.
   Copyright (C) 2006-2026 Free Software Foundation, Inc.

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

#ifndef _GL_HASHKEY_STRING_H
#define _GL_HASHKEY_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* String equality and hash code functions that operate on plain C strings
   ('const char *').  */
extern bool hashkey_string_equals (const void *x1, const void *x2);
extern size_t hashkey_string_hash (const void *x);

#ifdef __cplusplus
}
#endif

#endif /* _GL_HASHKEY_STRING_H */
