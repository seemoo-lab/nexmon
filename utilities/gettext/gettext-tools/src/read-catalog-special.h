/* Parsing of special comments (#, and #= comments) in textual message catalogs.
   Copyright (C) 1995-2026 Free Software Foundation, Inc.

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

/* Written by Peter Miller, Ulrich Drepper, and Bruno Haible.  */

#ifndef _READ_CATALOG_SPECIAL_H
#define _READ_CATALOG_SPECIAL_H

#include "message.h"

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Parse a special comment and put the result in *fuzzyp, formatp, *rangep,
   *wrapp, and, if scp != NULL, scp[0..NSYNTAXCHECKS-1].  */
extern void parse_comment_special (const char *s, bool *fuzzyp,
                                   enum is_format formatp[NFORMATS],
                                   struct argument_range *rangep,
                                   enum is_wrap *wrapp,
                                   enum is_syntax_check *scp);


#ifdef __cplusplus
}
#endif


#endif /* _READ_CATALOG_SPECIAL_H */
