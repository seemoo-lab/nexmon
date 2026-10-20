/* Language-dependent format strings.
   Copyright (C) 2003-2026 Free Software Foundation, Inc.

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

#ifndef _XGETTEXT_FORMATSTRING_H
#define _XGETTEXT_FORMATSTRING_H

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum number of format string parsers needed for any particular
   language.  */
#define NXFORMATS 4

/* Instead of indices, use these macros, for easier cross-referencing.  */
/* Primary format string type.  */
#define XFORMAT_PRIMARY    0
/* Secondary format string type.  */
#define XFORMAT_SECONDARY  1
/* Tertiary format string type.  */
#define XFORMAT_TERTIARY   2
/* Fourth-ranked format string type.  */
#define XFORMAT_FOURTH     3

/* Language dependent format string parser.
   NULL if the language has no notion of format strings.  */
extern struct formatstring_parser *current_formatstring_parser[NXFORMATS];


#ifdef __cplusplus
}
#endif

#endif /* _XGETTEXT_FORMATSTRING_H */
