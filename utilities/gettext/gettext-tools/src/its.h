/* Internationalization Tag Set (ITS) handling
   Copyright (C) 2015-2026 Free Software Foundation, Inc.

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

/* Written by Daiki Ueno and Bruno Haible.  */

#ifndef _ITS_H_
#define _ITS_H_

#include <stdio.h>

#include "message.h"
#include "pos.h"

#ifdef __cplusplus
extern "C" {
#endif


/* =================== Common API for xgettext and msgfmt =================== */

typedef struct its_rule_list_ty its_rule_list_ty;

/* Creates a fresh its_rule_list_ty holding global ITS rules.  */
extern its_rule_list_ty *its_rule_list_alloc (void);

/* Releases memory allocated for RULES.  */
extern void its_rule_list_free (its_rule_list_ty *rules);

/* Loads global ITS rules from STRING.  */
extern bool its_rule_list_add_from_string (struct its_rule_list_ty *rules,
                                           const char *rule);

/* Loads global ITS rules from FILENAME.  */
extern bool its_rule_list_add_from_file (its_rule_list_ty *rules,
                                         const char *filename);


/* ========================= API only for xgettext ========================= */

enum its_whitespace_type_ty
{
  ITS_WHITESPACE_PRESERVE,
  ITS_WHITESPACE_NORMALIZE,
  ITS_WHITESPACE_NORMALIZE_PARAGRAPH,
  ITS_WHITESPACE_TRIM
};

typedef message_ty *
        (*its_extract_callback_ty) (message_list_ty *mlp,
                                    const char *msgctxt,
                                    const char *msgid,
                                    lex_pos_ty *pos,
                                    const char *extracted_comment,
                                    const char *marker,
                                    enum its_whitespace_type_ty whitespace);

/* Extracts messages from FP, according to the loaded ITS rules.  */
extern void its_rule_list_extract (its_rule_list_ty *rules,
                                   FILE *fp, const char *real_filename,
                                   const char *logical_filename,
                                   msgdomain_list_ty *mdlp,
                                   its_extract_callback_ty callback);


/* ========================== API only for msgfmt ========================== */

typedef struct its_merge_context_ty its_merge_context_ty;

extern its_merge_context_ty *
       its_merge_context_alloc (its_rule_list_ty *rules, const char *filename);
extern void its_merge_context_free (its_merge_context_ty *context);
extern void its_merge_context_merge (its_merge_context_ty *context,
                                     const char *language,
                                     message_list_ty *mlp,
                                     bool replace_text);

extern void its_merge_context_write (its_merge_context_ty *context,
                                     FILE *fp);


#ifdef __cplusplus
}
#endif

#endif	/* _ITS_H_ */
