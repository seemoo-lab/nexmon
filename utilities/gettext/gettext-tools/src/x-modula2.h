/* xgettext Modula-2 backend.
   Copyright (C) 2002-2026 Free Software Foundation, Inc.

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


#include <stdio.h>

#include "message.h"
#include "xg-arglist-context.h"


#ifdef __cplusplus
extern "C" {
#endif


#define EXTENSIONS_MODULA2 \
  { "mod",    "Modula-2" },                                                 \
  { "def",    "Modula-2" },                                                 \

#define SCANNERS_MODULA2 \
  { "Modula-2",         extract_modula2, NULL,                              \
                        &flag_table_modula2, &formatstring_modula2, NULL }, \

/* Scan a Modula-2 file and add its translatable strings to mdlp.  */
extern void extract_modula2 (FILE *fp, const char *real_filename,
                             const char *logical_filename,
                             flag_context_list_table_ty *flag_table,
                             msgdomain_list_ty *mdlp);

extern void x_modula2_keyword (const char *keyword);
extern void x_modula2_extract_all (void);

extern void init_flag_table_modula2 (void);


#ifdef __cplusplus
}
#endif
