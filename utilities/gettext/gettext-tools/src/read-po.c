/* Reading PO files.
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

#include <config.h>

/* Specification.  */
#include "read-po.h"

#include <limits.h>

#include "read-po-lex.h"
#include "read-po-internal.h"


/* Read a .po / .pot file from a stream, and dispatch to the various
   abstract_catalog_reader_class_ty methods.  */
static void
po_parse (abstract_catalog_reader_ty *catr, FILE *fp,
          const char *real_filename, const char *logical_filename,
          bool is_pot_role, string_list_ty *arena)
{
  struct po_parser_state ps;
  ps.catr = catr;
  ps.gram_pot_role = is_pot_role;
  lex_start (&ps, fp, real_filename, logical_filename, arena);
  po_gram_parse (&ps);
  lex_end (&ps);
}

const struct catalog_input_format input_format_po =
{
  po_parse,                             /* parse */
  false                                 /* produces_utf8 */
};


/* Lexer variables.  */

unsigned int gram_max_allowed_errors = UINT_MAX;
