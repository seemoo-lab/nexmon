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

/* Written by Peter Miller and Bruno Haible.  */

#ifndef _PO_GRAM_H
#define _PO_GRAM_H

#include <stdbool.h>
#include <stdio.h>

#if HAVE_ICONV
#include <iconv.h>
# include "unistr.h"
#endif

#include "read-catalog-abstract.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Multibyte character data type.  */
/* Note this depends on po_lex_charset and po_lex_iconv, which get set
   while the file is being parsed.  */

#define MBCHAR_BUF_SIZE 24

struct mbchar
{
  size_t bytes;         /* number of bytes of current character, > 0 */
#if HAVE_ICONV
  bool uc_valid;        /* true if uc is a valid Unicode character */
  ucs4_t uc;            /* if uc_valid: the current character */
#endif
  char buf[MBCHAR_BUF_SIZE]; /* room for the bytes */
};

/* We want to pass multibyte characters by reference automatically,
   therefore we use an array type.  */
typedef struct mbchar mbchar_t[1];


/* Number of characters that can be pushed back.
   We need 1 for mbfile_getc_normalized, plus 1 for lex_getc,
   plus 1 for lex_ungetc.  */
#define MBFILE_MAX_PUSHBACK 3

/* Data type of a multibyte character input stream.  */
struct mbfile
{
  FILE *fp;
  bool eof_seen;
  unsigned int pushback_count; /* <= MBFILE_MAX_PUSHBACK */
  unsigned int bufcount;
  char buf[MBCHAR_BUF_SIZE];
  struct mbchar pushback[MBFILE_MAX_PUSHBACK];
};

/* We want to pass multibyte streams by reference automatically,
   therefore we use an array type.  */
typedef struct mbfile mbfile_t[1];


/* Input, output, and local variables of a PO parser instance.  */
struct po_parser_state
{
  /* ----- Input variables -----  */

  /* The catalog reader that implements the callbacks.  */
  struct abstract_catalog_reader_ty *catr;

  /* Whether the PO file is in the role of a POT file.  */
  bool gram_pot_role;

  /* ----- Output variables -----  */

  /* ----- Local variables of read-po-lex.c -----  */

  /* The PO file's encoding, as specified in the header entry.  */
  const char *po_lex_charset;

#if HAVE_ICONV
  /* Converter from the PO file's encoding to UTF-8.  */
  iconv_t po_lex_iconv;
#endif
  /* If no converter is available, some information about the structure of the
     PO file's encoding.  */
  bool po_lex_weird_cjk;

  /* Current position within the PO file.  */
  lex_pos_ty gram_pos;
  int gram_pos_column;

  /* Whether invalid multibyte sequences in the input shall be signalled
     or silently tolerated.  */
  bool signal_eilseq;

  /* A buffer for po_gram_lex().  */
  char *buf;
  size_t bufmax;

  mbfile_t mbf;
  bool po_lex_obsolete;
  bool po_lex_previous;

  /* ----- Local variables of read-po-gram.y -----  */
  long plural_counter;
};

extern int po_gram_parse (struct po_parser_state *ps);

#ifdef __cplusplus
}
#endif

#endif
