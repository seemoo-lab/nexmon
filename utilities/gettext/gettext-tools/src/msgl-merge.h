/* Merging a .po file with a .pot file.
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

#ifndef _MSGL_MERGE_H
#define _MSGL_MERGE_H 1

#include <stdbool.h>

#include "message.h"
#include "read-catalog-abstract.h"

#ifdef __cplusplus
extern "C" {
#endif


/* If true do not print unneeded messages.  */
extern bool quiet;

/* Verbosity level.  */
extern int verbosity_level;

/* Apply the .pot file to each of the domains in the PO file.  */
extern bool multi_domain_mode;

/* Produce output for msgfmt, not for a translator.
   msgfmt ignores
     - untranslated messages,
     - fuzzy messages, except the header entry,
     - obsolete messages.
   Therefore output for msgfmt does not need to include such messages.  */
extern bool for_msgfmt;

/* Determines whether to use fuzzy matching.  */
extern bool use_fuzzy_matching;

/* Determines whether to keep old msgids as previous msgids.  */
extern bool keep_previous;

/* Language (ISO-639 code) and optional territory (ISO-3166 code).  */
extern const char *catalogname;

/* List of user-specified compendiums.  */
extern message_list_list_ty *compendiums;

/* List of corresponding filenames.  */
extern string_list_ty *compendium_filenames;


extern msgdomain_list_ty *
       merge (const char *definitions_file_name,
              catalog_input_format_ty definitions_file_syntax,
              const char *references_file_name,
              catalog_input_format_ty references_file_syntax,
              msgdomain_list_ty **defp);


#ifdef __cplusplus
}
#endif

#endif /* _MSGL_MERGE_H */
