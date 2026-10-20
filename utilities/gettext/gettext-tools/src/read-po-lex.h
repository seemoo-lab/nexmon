/* GNU gettext - internationalization aids
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

#ifndef _PO_LEX_H
#define _PO_LEX_H

#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>

#include <error.h>
#include "str-list.h"
#include "pos.h"
#include "read-catalog-abstract.h"

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later and in clang.  */
# if !((__GNUC__ >= 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || defined __clang__) && !__STRICT_ANSI__)
#  define __attribute__(Spec) /* empty */
# endif
/* The __-protected variants of 'format' and 'printf' attributes are
   accepted by gcc versions 2.6.4 (effectively 2.7) and later and in clang.  */
# if !(__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || defined __clang__)
#  define __format__ format
#  define __printf__ printf
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Lexical analyzer for reading PO files.  */


struct po_parser_state;


/* Prepare lexical analysis.  */
extern void lex_start (struct po_parser_state *ps,
                       FILE *fp, const char *real_filename,
                       const char *logical_filename,
                       string_list_ty *arena);

/* Terminate lexical analysis.  */
extern void lex_end (struct po_parser_state *ps);

/* Return the next token in the PO file.  The return codes are defined
   in "read-po-gram.h".  Associated data is put in '*lval'.  */
union PO_GRAM_STYPE;
extern int po_gram_lex (union PO_GRAM_STYPE *lval, struct po_parser_state *ps);

extern void po_gram_error (struct po_parser_state *ps, const char *fmt, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
extern void po_gram_error_at_line (abstract_catalog_reader_ty *catr,
                                   const lex_pos_ty *pos, const char *fmt, ...)
       __attribute__ ((__format__ (__printf__, 3, 4)));

/* Set the PO file's encoding from the header entry.
   If is_pot_role is true, "charset=CHARSET" is expected and does not deserve
   a warning.  */
extern void po_lex_charset_set (struct po_parser_state *ps,
                                const char *header_entry,
                                const char *filename, bool is_pot_role);


/* Contains information about the definition of one translation.  */
struct msgstr_def
{
  char *msgstr;
  size_t msgstr_len;
};


#ifdef __cplusplus
}
#endif


#endif
