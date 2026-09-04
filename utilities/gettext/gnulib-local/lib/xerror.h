/* Multiline error-reporting functions.
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

#ifndef _XERROR_H
#define _XERROR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Emit a multiline warning to stderr, consisting of MESSAGE, with the
   first line prefixed with PREFIX and the remaining lines prefixed with
   the same amount of spaces.  Free the PREFIX and MESSAGE when done.
   Return the width of PREFIX, for later uses of multiline_append.  */
extern size_t multiline_warning (char *prefix, char *message);

/* Emit a multiline error to stderr, consisting of MESSAGE, with the
   first line prefixed with PREFIX and the remaining lines prefixed with
   the same amount of spaces.  Free the PREFIX and MESSAGE when done.
   Return the width of PREFIX, for later uses of multiline_append.  */
extern size_t multiline_error (char *prefix, char *message);

/* Following a call to multiline_warning or multiline_error, append another
   MESSAGE, with each line prefixed with PREFIX_WIDTH spaces.
   Free the MESSAGE when done.  */
extern void multiline_append (size_t prefix_width, char *message);


#ifdef __cplusplus
}
#endif


#endif /* _XERROR_H */
