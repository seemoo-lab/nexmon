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

#include <config.h>

/* Specification.  */
#include "x-modula2.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_NO_APPENDF
#include <error.h>
#include "attribute.h"
#include "message.h"
#include "xgettext.h"
#include "xg-pos.h"
#include "xg-arglist-context.h"
#include "xg-arglist-callshape.h"
#include "xg-arglist-parser.h"
#include "xg-message.h"
#include "if-error.h"
#include "xalloc.h"
#include "string-buffer.h"
#include "gettext.h"

#define _(s) gettext(s)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* The Modula-2 syntax is defined in the book
   "The Programming Language Modula-2" by Niklaus Wirth
   <https://freepages.modula2.org/report4/modula-2.html>.
   The syntax understood by GNU Modula-2 is listed in
   <https://gcc.gnu.org/onlinedocs/gm2/EBNF.html>.  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_modula2_extract_all ()
{
  extract_all = true;
}


void
x_modula2_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (keywords.table == NULL)
        hash_init (&keywords, 100);

      const char *end;
      struct callshape shape;
      split_keywordspec (name, &end, &shape);

      /* The characters between name and end should form a valid Modula-2
         identifier.
         A colon means an invalid parse in split_keywordspec().  */
      const char *colon = strchr (name, ':');
      if (colon == NULL || colon >= end)
        insert_keyword_callshape (&keywords, name, end - name, &shape);
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      x_modula2_keyword ("Gettext");
      x_modula2_keyword ("DGettext:2");
      x_modula2_keyword ("DCGettext:2");
      x_modula2_keyword ("NGettext:1,2");
      x_modula2_keyword ("DNGettext:2,3");
      x_modula2_keyword ("DCNGettext:2,3");
      default_keywords = false;
    }
}

void
init_flag_table_modula2 ()
{
  xgettext_record_flag ("Gettext:1:pass-modula2-format");
  xgettext_record_flag ("DGettext:2:pass-modula2-format");
  xgettext_record_flag ("DCGettext:2:pass-modula2-format");
  xgettext_record_flag ("NGettext:1:pass-modula2-format");
  xgettext_record_flag ("NGettext:2:pass-modula2-format");
  xgettext_record_flag ("DNGettext:2:pass-modula2-format");
  xgettext_record_flag ("DNGettext:3:pass-modula2-format");
  xgettext_record_flag ("DCNGettext:2:pass-modula2-format");
  xgettext_record_flag ("DCNGettext:3:pass-modula2-format");
  /* FormatStrings.def */
  xgettext_record_flag ("Sprintf0:1:modula2-format");
  xgettext_record_flag ("Sprintf1:1:modula2-format");
  xgettext_record_flag ("Sprintf2:1:modula2-format");
  xgettext_record_flag ("Sprintf3:1:modula2-format");
  xgettext_record_flag ("Sprintf4:1:modula2-format");
}


/* ======================== Reading of characters.  ======================== */

/* The input file stream.  */
static FILE *fp;


/* 1. line_number handling.  */

static int
phase1_getc ()
{
  int c = getc (fp);

  if (c == EOF)
    {
      if (ferror (fp))
        error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
               real_file_name);
      return EOF;
    }

  if (c == '\n')
    line_number++;

  return c;
}

/* Supports only one pushback character.  */
static void
phase1_ungetc (int c)
{
  if (c != EOF)
    {
      if (c == '\n')
        --line_number;

      ungetc (c, fp);
    }
}


/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line;
static int last_non_comment_line;


/* 2. Replace each comment that is not inside a character constant or
   string literal with a space character.  We need to remember the
   comment for later, because it may be attached to a keyword string.
   Modula-2 comments are specified in
   <https://freepages.modula2.org/report4/modula-2.html#SEC3>:
     "Comments may be inserted between any two symbols in a program.
      They are arbitrary character sequences opened by the bracket (* and
      closed by *). Comments may be nested, and they do not affect the
      meaning of a program."  */

static unsigned char phase2_pushback[1];
static int phase2_pushback_length;

static int
phase2_getc ()
{
  if (phase2_pushback_length)
    return phase2_pushback[--phase2_pushback_length];

  int c;

  c = phase1_getc ();
  if (c == '(')
    {
      c = phase1_getc ();
      if (c == '*')
        {
          /* A comment.  */
          struct string_buffer buffer;
          sb_init (&buffer);
          int lineno = line_number;
          unsigned int nesting = 0;
          bool last_was_star = false;
          bool last_was_opening_paren = false;
          for (;;)
            {
              c = phase1_getc ();
              if (c == EOF)
                {
                  sb_free (&buffer);
                  break;
                }

              if (last_was_opening_paren && c == '*')
                nesting++;
              else if (last_was_star && c == ')')
                {
                  if (nesting == 0)
                    {
                      --buffer.length;
                      while (buffer.length >= 1
                             && (buffer.data[buffer.length - 1] == ' '
                                 || buffer.data[buffer.length - 1] == '\t'))
                        --buffer.length;
                      savable_comment_add (sb_xdupfree_c (&buffer));
                      break;
                    }
                  nesting--;
                }
              last_was_star = (c == '*');
              last_was_opening_paren = (c == '(');

              /* We skip all leading white space, but not EOLs.  */
              if (sd_length (sb_contents (&buffer)) == 0
                  && (c == ' ' || c == '\t'))
                continue;
              sb_xappend1 (&buffer, c);
              if (c == '\n')
                {
                  --buffer.length;
                  while (buffer.length >= 1
                         && (buffer.data[buffer.length - 1] == ' '
                             || buffer.data[buffer.length - 1] == '\t'))
                    --buffer.length;
                  savable_comment_add (sb_xdupfree_c (&buffer));
                  sb_init (&buffer);
                  lineno = line_number;
                }
            }
          last_comment_line = lineno;
          return ' ';
        }
      else
        {
          phase1_ungetc (c);
          return '(';
        }
    }
  else
    return c;
}

/* Supports only one pushback character.  */
static void
phase2_ungetc (int c)
{
  if (c != EOF)
    {
      if (phase2_pushback_length == SIZEOF (phase2_pushback))
        abort ();
      phase2_pushback[phase2_pushback_length++] = c;
    }
}


/* ========================== Reading of tokens.  ========================== */


enum token_type_ty
{
  token_type_eof,
  token_type_lparen,            /* ( */
  token_type_rparen,            /* ) */
  token_type_comma,             /* , */
  token_type_plus,              /* + */
  token_type_operator,          /* - * / = # < <= > >= */
  token_type_string_literal,    /* "abc", 'abc' */
  token_type_symbol,            /* symbol */
  token_type_other              /* :=, number, other */
};
typedef enum token_type_ty token_type_ty;

typedef struct token_ty token_ty;
struct token_ty
{
  token_type_ty type;
  char *string;         /* for token_type_string_literal, token_type_symbol */
  refcounted_string_list_ty *comment;   /* for token_type_string_literal */
  int line_number;
};


/* Free the memory pointed to by a 'struct token_ty'.  */
static inline void
free_token (token_ty *tp)
{
  if (tp->type == token_type_string_literal || tp->type == token_type_symbol)
    free (tp->string);
  if (tp->type == token_type_string_literal)
    drop_reference (tp->comment);
}


/* Combine characters into tokens.  Discard whitespace.  */

static token_ty phase3_pushback[2];
static int phase3_pushback_length;

static void
phase3_get (token_ty *tp)
{
  if (phase3_pushback_length)
    {
      *tp = phase3_pushback[--phase3_pushback_length];
      return;
    }
  for (;;)
    {
      int c;

      tp->line_number = line_number;
      c = phase2_getc ();

      switch (c)
        {
        case EOF:
          tp->type = token_type_eof;
          return;

        case '\n':
          if (last_non_comment_line > last_comment_line)
            savable_comment_reset ();
          FALLTHROUGH;
        case '\r':
        case '\t':
        case ' ':
          /* Ignore whitespace and comments.  */
          continue;
        }

      last_non_comment_line = tp->line_number;

      switch (c)
        {
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':
        case '_': /* GNU Modula-2 treats '_' like a letter.  */
          /* Symbol.  */
          {
            struct string_buffer buffer;
            sb_init (&buffer);
            for (;;)
              {
                sb_xappend1 (&buffer, c);
                c = phase2_getc ();
                switch (c)
                  {
                  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                  case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
                  case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
                  case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
                  case 'Y': case 'Z':
                  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                  case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
                  case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
                  case 's': case 't': case 'u': case 'v': case 'w': case 'x':
                  case 'y': case 'z':
                  case '_': /* GNU Modula-2 treats '_' like a letter.  */
                  case '0': case '1': case '2': case '3': case '4':
                  case '5': case '6': case '7': case '8': case '9':
                    continue;
                  default:
                    phase2_ungetc (c);
                    break;
                  }
                break;
              }
            tp->string = sb_xdupfree_c (&buffer);
            /* We could carefully recognize each of the 2 and 3 character
               operators (IN, DIV, MOD, etc.), but it is not necessary, as we
               only need to recognize gettext invocations.  Don't bother.  */
            tp->type = token_type_symbol;
          }
          return;

        /* String syntax:
           <https://freepages.modula2.org/report4/modula-2.html#SEC3> says:
             "Strings are sequences of characters enclosed in quote marks.
              Both double quotes and single quotes (apostrophes) may be used
              as quote marks. However, the opening and closing marks must be
              the same character, and this character cannot occur within the
              string. A string must not extend over the end of a line."  */
        case '"': case '\'':
          {
            int delimiter = c;
            struct string_buffer buffer;
            sb_init (&buffer);
            for (;;)
              {
                c = phase1_getc ();
                if (c == EOF || c == '\n')
                  {
                    if_error (IF_SEVERITY_WARNING,
                              logical_file_name, line_number - (c == '\n'), (size_t)(-1),
                              false,
                              _("unterminated string literal"));
                    break;
                  }
                if (c == delimiter)
                  break;
                sb_xappend1 (&buffer, c);
              }
            tp->string = sb_xdupfree_c (&buffer);
            tp->type = token_type_string_literal;
            tp->comment = add_reference (savable_comment);
          }
          return;

        case '(':
          tp->type = token_type_lparen;
          return;

        case ')':
          tp->type = token_type_rparen;
          return;

        case ',':
          tp->type = token_type_comma;
          return;

        case '+':
          tp->type = token_type_plus;
          return;

        case '-':
        case '*':
        case '/':
        case '=':
        case '#':
          tp->type = token_type_operator;
          return;

        case '<':
        case '>':
          c = phase1_getc ();
          if (c != '=')
            phase1_ungetc (c);
          tp->type = token_type_operator;
          return;

        case ':':
          c = phase1_getc ();
          if (c != '=')
            phase1_ungetc (c);
          tp->type = token_type_other;
          return;

        default:
          tp->type = token_type_other;
          return;
        }
    }
}

/* Supports only 2 pushback tokens.  */
static void
phase3_unget (token_ty *tp)
{
  if (tp->type != token_type_eof)
    {
      if (phase3_pushback_length == SIZEOF (phase3_pushback))
        abort ();
      phase3_pushback[phase3_pushback_length++] = *tp;
    }
}


/* Compile-time optimization of string literal concatenation.
   Combine "string1" + ... + "stringN" to the concatenated string.  */

/* Concatenates two strings, and frees the first argument.  */
static char *
string_concat_free1 (char *s1, const char *s2)
{
  size_t len1 = strlen (s1);
  size_t len2 = strlen (s2);
  size_t len = len1 + len2 + 1;
  char *result = XNMALLOC (len, char);
  memcpy (result, s1, len1);
  memcpy (result + len1, s2, len2 + 1);
  free (s1);
  return result;
}

static token_ty phase4_pushback[2];
static int phase4_pushback_length;

static void
phase4_get (token_ty *tp)
{
  if (phase4_pushback_length)
    {
      *tp = phase4_pushback[--phase4_pushback_length];
      return;
    }

  phase3_get (tp);
  if (tp->type == token_type_string_literal)
    {
      char *sum = tp->string;

      for (;;)
        {
          token_ty token2;
          phase3_get (&token2);

          if (token2.type == token_type_plus)
            {
              token_ty token3;
              phase3_get (&token3);

              if (token3.type == token_type_string_literal)
                {
                  sum = string_concat_free1 (sum, token3.string);

                  free_token (&token3);
                  free_token (&token2);
                  continue;
                }
              phase3_unget (&token3);
            }
          phase3_unget (&token2);
          break;
        }
      tp->string = sum;
    }
}

/* Supports 2 tokens of pushback.  */
static void
phase4_unget (token_ty *tp)
{
  if (tp->type != token_type_eof)
    {
      if (phase4_pushback_length == SIZEOF (phase4_pushback))
        abort ();
      phase4_pushback[phase4_pushback_length++] = *tp;
    }
}


static void
x_modula2_lex (token_ty *tp)
{
  phase4_get (tp);
}

/* Supports 2 tokens of pushback.  */
MAYBE_UNUSED static void
x_modula2_unlex (token_ty *tp)
{
  phase4_unget (tp);
}


/* ========================= Extracting strings.  ========================== */


/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;


/* Maximum supported nesting depth.  */
#define MAX_NESTING_DEPTH 1000

/* Current nesting depth.  */
static int nesting_depth;


/* The file is broken into tokens.  Scan the token stream, looking for
   a keyword, followed by a left paren, followed by a string.  When we
   see this sequence, we have something to remember.  We assume we are
   looking at a valid Modula-2 program, and leave the complaints about
   the grammar to the compiler.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */


/* Extract messages until the next balanced closing parenthesis.
   Extracted messages are added to MLP.
   Return true upon eof, false upon closing parenthesis.  */
static bool
extract_parenthesized (message_list_ty *mlp,
                       flag_region_ty *outer_region,
                       flag_context_list_iterator_ty context_iter,
                       struct arglist_parser *argparser)
{
  /* Current argument number.  */
  int arg = 1;
  /* 0 when no keyword has been seen.  1 right after a keyword is seen.  */
  int state;
  /* Parameters of the keyword just seen.  Defined only in state 1.  */
  const struct callshapes *next_shapes = NULL;
  /* Context iterator that will be used if the next token is a '('.  */
  flag_context_list_iterator_ty next_context_iter =
    passthrough_context_list_iterator;
  /* Current region.  */
  flag_region_ty *inner_region =
    inheriting_region (outer_region,
                       flag_context_list_iterator_advance (&context_iter));

  /* Start state is 0.  */
  state = 0;

  for (;;)
    {
      token_ty token;
      x_modula2_lex (&token);

      switch (token.type)
        {
        case token_type_symbol:
          {
            void *keyword_value;
            if (hash_find_entry (&keywords, token.string, strlen (token.string),
                                 &keyword_value)
                == 0)
              {
                next_shapes = (const struct callshapes *) keyword_value;
                state = 1;
              }
            else
              state = 0;
          }
          next_context_iter =
            flag_context_list_iterator (
              flag_context_list_table_lookup (
                flag_context_list_table,
                token.string, strlen (token.string)));
          free (token.string);
          break;

        case token_type_lparen:
          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, line_number, (size_t)(-1), false,
                      _("too many open parentheses"));
          if (extract_parenthesized (mlp, inner_region, next_context_iter,
                                     arglist_parser_alloc (mlp,
                                                           state ? next_shapes : NULL)))
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return true;
            }
          nesting_depth--;
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_rparen:
          arglist_parser_done (argparser, arg);
          unref_region (inner_region);
          return false;

        case token_type_comma:
          arg++;
          unref_region (inner_region);
          inner_region =
            inheriting_region (outer_region,
                               flag_context_list_iterator_advance (
                                 &context_iter));
          next_context_iter = passthrough_context_list_iterator;
          state = 0;
          break;

        case token_type_string_literal:
          {
            lex_pos_ty pos;
            pos.file_name = logical_file_name;
            pos.line_number = token.line_number;

            if (extract_all)
              remember_a_message (mlp, NULL, token.string, false, false,
                                  inner_region, &pos,
                                  NULL, token.comment, false);
            else
              {
                mixed_string_ty *ms =
                  mixed_string_alloc_simple (token.string, lc_string,
                                             pos.file_name, pos.line_number);
                free (token.string);
                arglist_parser_remember (argparser, arg, ms,
                                         inner_region,
                                         pos.file_name, pos.line_number,
                                         token.comment, false);
              }
          }
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_eof:
          arglist_parser_done (argparser, arg);
          unref_region (inner_region);
          return true;

        case token_type_plus:
        case token_type_operator:
        case token_type_other:
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        default:
          abort ();
        }
    }
}


void
extract_modula2 (FILE *f,
                 const char *real_filename, const char *logical_filename,
                 flag_context_list_table_ty *flag_table,
                 msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  fp = f;
  real_file_name = real_filename;
  logical_file_name = xstrdup (logical_filename);
  line_number = 1;

  last_comment_line = -1;
  last_non_comment_line = -1;

  phase2_pushback_length = 0;
  phase3_pushback_length = 0;
  phase4_pushback_length = 0;

  flag_context_list_table = flag_table;
  nesting_depth = 0;

  init_keywords ();

  /* Eat tokens until eof is seen.  When extract_parenthesized returns
     due to an unbalanced closing parenthesis, just restart it.  */
  while (!extract_parenthesized (mlp, null_context_region (), null_context_list_iterator,
                                 arglist_parser_alloc (mlp, NULL)))
    ;

  fp = NULL;
  real_file_name = NULL;
  logical_file_name = NULL;
  line_number = 0;
}
