/* GNU gettext - internationalization aids
   Copyright (C) 1995-1996, 1998, 2000-2001, 2003, 2005-2006, 2012, 2015-2016
   Free Software Foundation, Inc.

   This file was written by Peter Miller <pmiller@agso.gov.au>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "po-gram.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str-list.h"
#include "po-lex.h"
#include "po-charset.h"
#include "error.h"
#include "xalloc.h"
#include "gettext.h"
#include "read-catalog-abstract.h"

#define _(str) gettext (str)

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in the same program.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link time,
   then those parser generators need to be fixed instead of adding those
   names to this list. */

#define yymaxdepth po_gram_maxdepth
#define yyparse po_gram_parse
#define yylex   po_gram_lex
#define yyerror po_gram_error
#define yylval  po_gram_lval
#define yychar  po_gram_char
#define yydebug po_gram_debug
#define yypact  po_gram_pact
#define yyr1    po_gram_r1
#define yyr2    po_gram_r2
#define yydef   po_gram_def
#define yychk   po_gram_chk
#define yypgo   po_gram_pgo
#define yyact   po_gram_act
#define yyexca  po_gram_exca
#define yyerrflag po_gram_errflag
#define yynerrs po_gram_nerrs
#define yyps    po_gram_ps
#define yypv    po_gram_pv
#define yys     po_gram_s
#define yy_yys  po_gram_yys
#define yystate po_gram_state
#define yytmp   po_gram_tmp
#define yyv     po_gram_v
#define yy_yyv  po_gram_yyv
#define yyval   po_gram_val
#define yylloc  po_gram_lloc
#define yyreds  po_gram_reds          /* With YYDEBUG defined */
#define yytoks  po_gram_toks          /* With YYDEBUG defined */
#define yylhs   po_gram_yylhs
#define yylen   po_gram_yylen
#define yydefred po_gram_yydefred
#define yydgoto po_gram_yydgoto
#define yysindex po_gram_yysindex
#define yyrindex po_gram_yyrindex
#define yygindex po_gram_yygindex
#define yytable  po_gram_yytable
#define yycheck  po_gram_yycheck

static long plural_counter;

#define check_obsolete(value1,value2) \
  if ((value1).obsolete != (value2).obsolete) \
    po_gram_error_at_line (&(value2).pos, _("inconsistent use of #~"));

static inline void
do_callback_message (char *msgctxt,
                     char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
                     char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
                     char *prev_msgctxt,
                     char *prev_msgid, char *prev_msgid_plural,
                     bool obsolete)
{
  /* Test for header entry.  Ignore fuzziness of the header entry.  */
  if (msgctxt == NULL && msgid[0] == '\0' && !obsolete)
    po_lex_charset_set (msgstr, gram_pos.file_name);

  po_callback_message (msgctxt,
                       msgid, msgid_pos, msgid_plural,
                       msgstr, msgstr_len, msgstr_pos,
                       prev_msgctxt, prev_msgid, prev_msgid_plural,
                       false, obsolete);
}

#define free_message_intro(value) \
  if ((value).prev_ctxt != NULL)        \
    free ((value).prev_ctxt);           \
  if ((value).prev_id != NULL)          \
    free ((value).prev_id);             \
  if ((value).prev_id_plural != NULL)   \
    free ((value).prev_id_plural);      \
  if ((value).ctxt != NULL)             \
    free ((value).ctxt);

%}

%token COMMENT
%token DOMAIN
%token JUNK
%token PREV_MSGCTXT
%token PREV_MSGID
%token PREV_MSGID_PLURAL
%token PREV_STRING
%token MSGCTXT
%token MSGID
%token MSGID_PLURAL
%token MSGSTR
%token NAME
%token '[' ']'
%token NUMBER
%token STRING

%union
{
  struct { char *string; lex_pos_ty pos; bool obsolete; } string;
  struct { string_list_ty stringlist; lex_pos_ty pos; bool obsolete; } stringlist;
  struct { long number; lex_pos_ty pos; bool obsolete; } number;
  struct { lex_pos_ty pos; bool obsolete; } pos;
  struct { char *ctxt; char *id; char *id_plural; lex_pos_ty pos; bool obsolete; } prev;
  struct { char *prev_ctxt; char *prev_id; char *prev_id_plural; char *ctxt; lex_pos_ty pos; bool obsolete; } message_intro;
  struct { struct msgstr_def rhs; lex_pos_ty pos; bool obsolete; } rhs;
}

%type <string> STRING PREV_STRING COMMENT NAME
               msg_intro prev_msg_intro msgid_pluralform prev_msgid_pluralform
%type <stringlist> string_list prev_string_list
%type <number> NUMBER
%type <pos> DOMAIN
            PREV_MSGCTXT PREV_MSGID PREV_MSGID_PLURAL
            MSGCTXT MSGID MSGID_PLURAL MSGSTR '[' ']'
%type <prev> prev
%type <message_intro> message_intro
%type <rhs> pluralform pluralform_list

%right MSGSTR

%%

po_file
        : /* empty */
        | po_file comment
        | po_file domain
        | po_file message
        | po_file error
        ;


comment
        : COMMENT
                {
                  po_callback_comment_dispatcher ($1.string);
                }
        ;


domain
        : DOMAIN STRING
                {
                   po_callback_domain ($2.string);
                }
        ;


message
        : message_intro string_list MSGSTR string_list
                {
                  char *string2 = string_list_concat_destroy (&$2.stringlist);
                  char *string4 = string_list_concat_destroy (&$4.stringlist);

                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  check_obsolete ($1, $4);
                  if (!$1.obsolete || pass_obsolete_entries)
                    do_callback_message ($1.ctxt, string2, &$1.pos, NULL,
                                         string4, strlen (string4) + 1, &$3.pos,
                                         $1.prev_ctxt,
                                         $1.prev_id, $1.prev_id_plural,
                                         $1.obsolete);
                  else
                    {
                      free_message_intro ($1);
                      free (string2);
                      free (string4);
                    }
                }
        | message_intro string_list msgid_pluralform pluralform_list
                {
                  char *string2 = string_list_concat_destroy (&$2.stringlist);

                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  check_obsolete ($1, $4);
                  if (!$1.obsolete || pass_obsolete_entries)
                    {
                      do_callback_message ($1.ctxt, string2, &$1.pos, $3.string,
                                           $4.rhs.msgstr, $4.rhs.msgstr_len, &$4.pos,
                                           $1.prev_ctxt,
                                           $1.prev_id, $1.prev_id_plural,
                                           $1.obsolete);
                      free ($3.string);
                    }
                  else
                    {
                      free_message_intro ($1);
                      free (string2);
                      free ($3.string);
                      free ($4.rhs.msgstr);
                    }
                }
        | message_intro string_list msgid_pluralform
                {
                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  po_gram_error_at_line (&$1.pos, _("missing 'msgstr[]' section"));
                  free_message_intro ($1);
                  string_list_destroy (&$2.stringlist);
                  free ($3.string);
                }
        | message_intro string_list pluralform_list
                {
                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  po_gram_error_at_line (&$1.pos, _("missing 'msgid_plural' section"));
                  free_message_intro ($1);
                  string_list_destroy (&$2.stringlist);
                  free ($3.rhs.msgstr);
                }
        | message_intro string_list
                {
                  check_obsolete ($1, $2);
                  po_gram_error_at_line (&$1.pos, _("missing 'msgstr' section"));
                  free_message_intro ($1);
                  string_list_destroy (&$2.stringlist);
                }
        ;


message_intro
        : msg_intro
                {
                  $$.prev_ctxt = NULL;
                  $$.prev_id = NULL;
                  $$.prev_id_plural = NULL;
                  $$.ctxt = $1.string;
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        | prev msg_intro
                {
                  check_obsolete ($1, $2);
                  $$.prev_ctxt = $1.ctxt;
                  $$.prev_id = $1.id;
                  $$.prev_id_plural = $1.id_plural;
                  $$.ctxt = $2.string;
                  $$.pos = $2.pos;
                  $$.obsolete = $2.obsolete;
                }
        ;


prev
        : prev_msg_intro prev_string_list
                {
                  check_obsolete ($1, $2);
                  $$.ctxt = $1.string;
                  $$.id = string_list_concat_destroy (&$2.stringlist);
                  $$.id_plural = NULL;
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        | prev_msg_intro prev_string_list prev_msgid_pluralform
                {
                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  $$.ctxt = $1.string;
                  $$.id = string_list_concat_destroy (&$2.stringlist);
                  $$.id_plural = $3.string;
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;


msg_intro
        : MSGID
                {
                  $$.string = NULL;
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        | MSGCTXT string_list MSGID
                {
                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  $$.string = string_list_concat_destroy (&$2.stringlist);
                  $$.pos = $3.pos;
                  $$.obsolete = $3.obsolete;
                }
        ;

prev_msg_intro
        : PREV_MSGID
                {
                  $$.string = NULL;
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        | PREV_MSGCTXT prev_string_list PREV_MSGID
                {
                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  $$.string = string_list_concat_destroy (&$2.stringlist);
                  $$.pos = $3.pos;
                  $$.obsolete = $3.obsolete;
                }
        ;


msgid_pluralform
        : MSGID_PLURAL string_list
                {
                  check_obsolete ($1, $2);
                  plural_counter = 0;
                  $$.string = string_list_concat_destroy (&$2.stringlist);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;

prev_msgid_pluralform
        : PREV_MSGID_PLURAL prev_string_list
                {
                  check_obsolete ($1, $2);
                  $$.string = string_list_concat_destroy (&$2.stringlist);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;


pluralform_list
        : pluralform
                {
                  $$ = $1;
                }
        | pluralform_list pluralform
                {
                  check_obsolete ($1, $2);
                  $$.rhs.msgstr = XNMALLOC ($1.rhs.msgstr_len + $2.rhs.msgstr_len, char);
                  memcpy ($$.rhs.msgstr, $1.rhs.msgstr, $1.rhs.msgstr_len);
                  memcpy ($$.rhs.msgstr + $1.rhs.msgstr_len, $2.rhs.msgstr, $2.rhs.msgstr_len);
                  $$.rhs.msgstr_len = $1.rhs.msgstr_len + $2.rhs.msgstr_len;
                  free ($1.rhs.msgstr);
                  free ($2.rhs.msgstr);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;

pluralform
        : MSGSTR '[' NUMBER ']' string_list
                {
                  check_obsolete ($1, $2);
                  check_obsolete ($1, $3);
                  check_obsolete ($1, $4);
                  check_obsolete ($1, $5);
                  if ($3.number != plural_counter)
                    {
                      if (plural_counter == 0)
                        po_gram_error_at_line (&$1.pos, _("first plural form has nonzero index"));
                      else
                        po_gram_error_at_line (&$1.pos, _("plural form has wrong index"));
                    }
                  plural_counter++;
                  $$.rhs.msgstr = string_list_concat_destroy (&$5.stringlist);
                  $$.rhs.msgstr_len = strlen ($$.rhs.msgstr) + 1;
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;


string_list
        : STRING
                {
                  string_list_init (&$$.stringlist);
                  string_list_append (&$$.stringlist, $1.string);
                  free ($1.string);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        | string_list STRING
                {
                  check_obsolete ($1, $2);
                  $$.stringlist = $1.stringlist;
                  string_list_append (&$$.stringlist, $2.string);
                  free ($2.string);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;

prev_string_list
        : PREV_STRING
                {
                  string_list_init (&$$.stringlist);
                  string_list_append (&$$.stringlist, $1.string);
                  free ($1.string);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        | prev_string_list PREV_STRING
                {
                  check_obsolete ($1, $2);
                  $$.stringlist = $1.stringlist;
                  string_list_append (&$$.stringlist, $2.string);
                  free ($2.string);
                  $$.pos = $1.pos;
                  $$.obsolete = $1.obsolete;
                }
        ;
