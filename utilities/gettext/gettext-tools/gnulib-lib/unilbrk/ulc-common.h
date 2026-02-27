/* Line breaking auxiliary functions.
   Copyright (C) 2001-2003, 2006-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Get size_t.  */
#include <stddef.h>

#include "c-ctype.h"

#define is_utf8_encoding unilbrk_is_utf8_encoding
extern int is_utf8_encoding (const char *encoding);

#if C_CTYPE_ASCII

# define is_all_ascii unilbrk_is_all_ascii
extern int is_all_ascii (const char *s, size_t n);

#endif /* C_CTYPE_ASCII */
