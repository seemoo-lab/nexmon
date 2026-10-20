/* xgettext PHP backend.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

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
#include "x-php.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SB_NO_APPENDF
#include <error.h>
#include "attribute.h"
#include "message.h"
#include "sf-istream.h"
#include "rc-str-list.h"
#include "xgettext.h"
#include "xg-pos.h"
#include "xg-mixed-string.h"
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


/* The PHP syntax is defined in phpdoc/manual/langref.html.
   See also php-8.1.0/Zend/zend_language_scanner.l
   and      php-8.1.0/Zend/zend_language_parser.y.
   Note that variable and function names can contain bytes in the range
   0x80..0xff; see
     https://www.php.net/manual/en/language.variables.basics.php
   String syntaxes (single-quoted, double-quoted, heredoc, nowdoc):
     https://www.php.net/manual/en/language.types.string.php  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_php_extract_all ()
{
  extract_all = true;
}


void
x_php_keyword (const char *name)
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

      /* The characters between name and end should form a valid C identifier.
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
      x_php_keyword ("_");
      x_php_keyword ("gettext");
      x_php_keyword ("dgettext:2");
      x_php_keyword ("dcgettext:2");
      /* The following were added in PHP 4.2.0.  */
      x_php_keyword ("ngettext:1,2");
      x_php_keyword ("dngettext:2,3");
      x_php_keyword ("dcngettext:2,3");
      default_keywords = false;
    }
}

void
init_flag_table_php ()
{
  xgettext_record_flag ("_:1:pass-php-format");
  xgettext_record_flag ("gettext:1:pass-php-format");
  xgettext_record_flag ("dgettext:2:pass-php-format");
  xgettext_record_flag ("dcgettext:2:pass-php-format");
  xgettext_record_flag ("ngettext:1:pass-php-format");
  xgettext_record_flag ("ngettext:2:pass-php-format");
  xgettext_record_flag ("dngettext:2:pass-php-format");
  xgettext_record_flag ("dngettext:3:pass-php-format");
  xgettext_record_flag ("dcngettext:2:pass-php-format");
  xgettext_record_flag ("dcngettext:3:pass-php-format");
  xgettext_record_flag ("sprintf:1:php-format");
  xgettext_record_flag ("printf:1:php-format");
}


/* =================== Variables used by the extractor.  =================== */

/* Type definitions needed for the variables.  */

enum token_type_ty
{
  token_type_eof,
  token_type_lparen,            /* ( */
  token_type_rparen,            /* ) */
  token_type_comma,             /* , */
  token_type_lbracket,          /* [ */
  token_type_rbracket,          /* ] */
  token_type_dot,               /* . */
  token_type_operator1,         /* * / % ++ -- */
  token_type_operator2,         /* + - ! ~ @ */
  token_type_lbrace,            /* { */
  token_type_rbrace,            /* } */
  token_type_string_literal,    /* "abc" */
  token_type_symbol,            /* symbol, number */
  token_type_other              /* misc. operator */
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

/* These variables are combined in a struct, so that we can invoke the
   extractor in a reentrant way.  */

struct php_extractor
{
  /* Accumulator for the output.  */
  message_list_ty *mlp;

  /* The input.  */
  sf_istream_t input;

  int line_number;

  unsigned char phase1_pushback[2];
  int phase1_pushback_length;

#if 0
  unsigned char phase2_pushback[1];
  int phase2_pushback_length;
#endif

  /* For accumulating comments.  */
  char *buffer;
  size_t bufmax;
  size_t buflen;

  /* These are for tracking whether comments count as immediately before
     keyword.  */
  int last_comment_line;
  int last_non_comment_line;

  unsigned char phase3_pushback[1];
  int phase3_pushback_length;

  token_ty phase4_pushback[3];
  int phase4_pushback_length;

  token_type_ty phase5_last;

  /* Maximum supported nesting depth.  */
  #define MAX_NESTING_DEPTH 1000

  /* Current nesting depths.  */
  int paren_nesting_depth;
  int bracket_nesting_depth;
  int brace_nesting_depth;
};

static inline void
php_extractor_init_rest (struct php_extractor *xp)
{
  xp->phase1_pushback_length = 0;
#if 0
  xp->phase2_pushback_length = 0;
#endif

  xp->buffer = NULL;
  xp->bufmax = 0;
  xp->buflen = 0;

  xp->last_comment_line = -1;
  xp->last_non_comment_line = -1;

  xp->phase3_pushback_length = 0;
  xp->phase4_pushback_length = 0;

  xp->phase5_last = token_type_eof;

  xp->paren_nesting_depth = 0;
  xp->bracket_nesting_depth = 0;
  xp->brace_nesting_depth = 0;
}

/* Forward declarations.  */
static bool extract_balanced (struct php_extractor *xp,
                              token_type_ty delim,
                              flag_region_ty *outer_region,
                              flag_context_list_iterator_ty context_iter,
                              struct arglist_parser *argparser);


/* ======================== Reading of characters.  ======================== */

/* 1. line_number handling.  */

static int
phase1_getc (struct php_extractor *xp)
{
  int c;

  if (xp->phase1_pushback_length)
    c = xp->phase1_pushback[--(xp->phase1_pushback_length)];
  else
    {
      c = sf_getc (&xp->input);

      if (c == EOF)
        {
          if (sf_ferror (&xp->input))
            error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
                   real_file_name);
          return EOF;
        }
    }

  if (xp->input.fp != NULL && c == '\n')
    xp->line_number++;

  return c;
}

/* Supports 2 characters of pushback.  */
static void
phase1_ungetc (struct php_extractor *xp, int c)
{
  if (c != EOF)
    {
      if (c == '\n')
        --(xp->line_number);

      if (xp->phase1_pushback_length == SIZEOF (xp->phase1_pushback))
        abort ();
      xp->phase1_pushback[xp->phase1_pushback_length++] = c;
    }
}


/* 2. Ignore HTML sections.  They are equivalent to PHP echo commands and
   therefore don't contain translatable strings.  */

static void
skip_html (struct php_extractor *xp)
{
  for (;;)
    {
      int c = phase1_getc (xp);

      if (c == EOF)
        return;

      if (c == '<')
        {
          int c2 = phase1_getc (xp);

          if (c2 == EOF)
            break;

          if (c2 == '?')
            {
              /* <?php is the normal way to enter PHP mode. <? and <?= are
                 recognized by PHP depending on a configuration setting.  */
              int c3 = phase1_getc (xp);

              if (c3 != '=')
                phase1_ungetc (xp, c3);

              return;
            }

          if (c2 == '<')
            {
              phase1_ungetc (xp, c2);
              continue;
            }

          /* < script language = php >
             < script language = "php" >
             < script language = 'php' >
             are always recognized.  */
          while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
            c2 = phase1_getc (xp);
          if (c2 != 's' && c2 != 'S')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'c' && c2 != 'C')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'r' && c2 != 'R')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'i' && c2 != 'I')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'p' && c2 != 'P')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 't' && c2 != 'T')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (!(c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r'))
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          do
            c2 = phase1_getc (xp);
          while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
          if (c2 != 'l' && c2 != 'L')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'a' && c2 != 'A')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'n' && c2 != 'N')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'g' && c2 != 'G')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'u' && c2 != 'U')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'a' && c2 != 'A')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'g' && c2 != 'G')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          if (c2 != 'e' && c2 != 'E')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
            c2 = phase1_getc (xp);
          if (c2 != '=')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          c2 = phase1_getc (xp);
          while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
            c2 = phase1_getc (xp);
          if (c2 == '"')
            {
              c2 = phase1_getc (xp);
              if (c2 != 'p')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != 'h')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != 'p')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != '"')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
            }
          else if (c2 == '\'')
            {
              c2 = phase1_getc (xp);
              if (c2 != 'p')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != 'h')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != 'p')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != '\'')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
            }
          else
            {
              if (c2 != 'p')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != 'h')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
              c2 = phase1_getc (xp);
              if (c2 != 'p')
                {
                  phase1_ungetc (xp, c2);
                  continue;
                }
            }
          c2 = phase1_getc (xp);
          while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
            c2 = phase1_getc (xp);
          if (c2 != '>')
            {
              phase1_ungetc (xp, c2);
              continue;
            }
          return;
        }
    }
}

#if 0

static int
phase2_getc (struct php_extractor *xp)
{
  if (xp->phase2_pushback_length)
    return xp->phase2_pushback[--(xp->phase2_pushback_length)];

  int c;

  c = phase1_getc (xp);
  switch (c)
    {
    case '?':
      {
        int c2 = phase1_getc (xp);
        if (c2 == '>')
          {
            /* ?> and %> terminate PHP mode and switch back to HTML mode.  */
            skip_html ();
            return ' ';
          }
        phase1_ungetc (xp, c2);
      }
      break;

    case '<':
      {
        int c2 = phase1_getc (xp);

        /* < / script > terminates PHP mode and switches back to HTML mode.  */
        while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
          c2 = phase1_getc (xp);
        if (c2 == '/')
          {
            do
              c2 = phase1_getc (xp);
            while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
            if (c2 == 's' || c2 == 'S')
              {
                c2 = phase1_getc (xp);
                if (c2 == 'c' || c2 == 'C')
                  {
                    c2 = phase1_getc (xp);
                    if (c2 == 'r' || c2 == 'R')
                      {
                        c2 = phase1_getc (xp);
                        if (c2 == 'i' || c2 == 'I')
                          {
                            c2 = phase1_getc (xp);
                            if (c2 == 'p' || c2 == 'P')
                              {
                                c2 = phase1_getc (xp);
                                if (c2 == 't' || c2 == 'T')
                                  {
                                    do
                                      c2 = phase1_getc (xp);
                                    while (c2 == ' ' || c2 == '\t'
                                           || c2 == '\n' || c2 == '\r');
                                    if (c2 == '>')
                                      {
                                        skip_html (xp);
                                        return ' ';
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          }
        phase1_ungetc (xp, c2);
      }
      break;
    }

  return c;
}

static void
phase2_ungetc (struct php_extractor *xp, int c)
{
  if (c != EOF)
    {
      if (xp->phase2_pushback_length == SIZEOF (xp->phase2_pushback))
        abort ();
      xp->phase2_pushback[xp->phase2_pushback_length++] = c;
    }
}

#endif


/* Accumulating comments.  */

static inline void
comment_start (struct php_extractor *xp)
{
  xp->buflen = 0;
}

static inline void
comment_add (struct php_extractor *xp, int c)
{
  if (xp->buflen >= xp->bufmax)
    {
      xp->bufmax = 2 * xp->bufmax + 10;
      xp->buffer = xrealloc (xp->buffer, xp->bufmax);
    }
  xp->buffer[xp->buflen++] = c;
}

static inline void
comment_line_end (struct php_extractor *xp, size_t chars_to_remove)
{
  xp->buflen -= chars_to_remove;
  while (xp->buflen >= 1
         && (xp->buffer[xp->buflen - 1] == ' '
             || xp->buffer[xp->buflen - 1] == '\t'))
    --(xp->buflen);
  if (chars_to_remove == 0 && xp->buflen >= xp->bufmax)
    {
      xp->bufmax = 2 * xp->bufmax + 10;
      xp->buffer = xrealloc (xp->buffer, xp->bufmax);
    }
  xp->buffer[xp->buflen] = '\0';
  savable_comment_add (xp->buffer);
}


/* 3. Replace each comment that is not inside a string literal with a
   space character.  We need to remember the comment for later, because
   it may be attached to a keyword string.  */

static int
phase3_getc (struct php_extractor *xp)
{
  int c;

  if (xp->phase3_pushback_length)
    return xp->phase3_pushback[--(xp->phase3_pushback_length)];

  c = phase1_getc (xp);

  if (c == '#')
    {
      /* sh comment.  */
      comment_start (xp);
      int lineno = xp->line_number;
      bool last_was_qmark = false;
      for (;;)
        {
          c = phase1_getc (xp);
          if (c == '\n' || c == EOF)
            {
              comment_line_end (xp, 0);
              break;
            }
          if (last_was_qmark && c == '>')
            {
              comment_line_end (xp, 1);
              skip_html (xp);
              break;
            }
          /* We skip all leading white space, but not EOLs.  */
          if (!(xp->buflen == 0 && (c == ' ' || c == '\t')))
            comment_add (xp, c);
          last_was_qmark = (c == '?');
        }
      xp->last_comment_line = lineno;
      return '\n';
    }
  else if (c == '/')
    {
      c = phase1_getc (xp);

      switch (c)
        {
        default:
          phase1_ungetc (xp, c);
          return '/';

        case '*':
          {
            /* C comment.  */
            comment_start (xp);
            int lineno = xp->line_number;
            bool last_was_star = false;
            for (;;)
              {
                c = phase1_getc (xp);
                if (c == EOF)
                  break;
                /* We skip all leading white space, but not EOLs.  */
                if (xp->buflen == 0 && (c == ' ' || c == '\t'))
                  continue;
                comment_add (xp, c);
                switch (c)
                  {
                  case '\n':
                    comment_line_end (xp, 1);
                    comment_start (xp);
                    lineno = xp->line_number;
                    last_was_star = false;
                    continue;

                  case '*':
                    last_was_star = true;
                    continue;

                  case '/':
                    if (last_was_star)
                      {
                        comment_line_end (xp, 2);
                        break;
                      }
                    FALLTHROUGH;

                  default:
                    last_was_star = false;
                    continue;
                  }
                break;
              }
            xp->last_comment_line = lineno;
            return ' ';
          }

        case '/':
          {
            /* C++ comment.  */
            comment_start (xp);
            int lineno = xp->line_number;
            bool last_was_qmark = false;
            for (;;)
              {
                c = phase1_getc (xp);
                if (c == '\n' || c == EOF)
                  {
                    comment_line_end (xp, 0);
                    break;
                  }
                if (last_was_qmark && c == '>')
                  {
                    comment_line_end (xp, 1);
                    skip_html (xp);
                    break;
                  }
                /* We skip all leading white space, but not EOLs.  */
                if (!(xp->buflen == 0 && (c == ' ' || c == '\t')))
                  comment_add (xp, c);
                last_was_qmark = (c == '?');
              }
            xp->last_comment_line = lineno;
            return '\n';
          }
        }
    }
  else
    return c;
}

MAYBE_UNUSED static void
phase3_ungetc (struct php_extractor *xp, int c)
{
  if (c != EOF)
    {
      if (xp->phase3_pushback_length == SIZEOF (xp->phase3_pushback))
        abort ();
      xp->phase3_pushback[xp->phase3_pushback_length++] = c;
    }
}


/* ========================== Reading of tokens.  ========================== */


/* 'struct token_ty' is defined above.  */

/* Free the memory pointed to by a 'struct token_ty'.  */
static inline void
free_token (token_ty *tp)
{
  if (tp->type == token_type_string_literal || tp->type == token_type_symbol)
    free (tp->string);
  if (tp->type == token_type_string_literal)
    drop_reference (tp->comment);
}


/* In heredoc and nowdoc, assume a tab width of 8.  */
#define TAB_WIDTH 8


/* 4. Combine characters into tokens.  Discard whitespace.  */

/* Do the processing of a double-quoted string or heredoc string.
   Return the processed string, or NULL if it contains variables or embedded
   expressions.  */
static char *
process_dquote_or_heredoc (struct php_extractor *xp, bool heredoc)
{
  bool is_constant = true;

 string_continued:
  {
    struct string_buffer buffer;
    sb_init (&buffer);
    for (;;)
      {
        int c;

        c = phase1_getc (xp);
        if (c == EOF || (!heredoc && c == '"'))
          break;
        if (heredoc && c == '\n')
          xp->line_number++;
        if (c == '$')
          {
            c = phase1_getc (xp);
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                || c == '_' || c >= 0x7f)
              {
                /* String with variables.  */
                is_constant = false;
                continue;
              }
            if (c == '{')
              {
                /* String with embedded expressions.  */
                sb_free (&buffer);
                goto string_with_embedded_expressions;
              }
            phase1_ungetc (xp, c);
            c = '$';
          }
        if (c == '{')
          {
            c = phase1_getc (xp);
            if (c == '$')
              {
                /* String with embedded expressions.  */
                sb_free (&buffer);
                goto string_with_embedded_expressions;
              }
            phase1_ungetc (xp, c);
            c = '{';
          }
        if (c == '\\')
          {
            c = phase1_getc (xp);
            switch (c)
              {
              case '\\':
              case '$':
                break;

              case '0': case '1': case '2': case '3':
              case '4': case '5': case '6': case '7':
                {
                  int n = 0;
                  for (int j = 0; j < 3; ++j)
                    {
                      n = n * 8 + c - '0';
                      c = phase1_getc (xp);
                      switch (c)
                        {
                        default:
                          break;

                        case '0': case '1': case '2': case '3':
                        case '4': case '5': case '6': case '7':
                          continue;
                        }
                      break;
                    }
                  phase1_ungetc (xp, c);
                  c = n;
                }
                break;

              case 'x':
                {
                  int j;
                  int n = 0;
                  for (j = 0; j < 2; ++j)
                    {
                      c = phase1_getc (xp);
                      switch (c)
                        {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                          n = n * 16 + c - '0';
                          break;
                        case 'A': case 'B': case 'C': case 'D': case 'E':
                        case 'F':
                          n = n * 16 + 10 + c - 'A';
                          break;
                        case 'a': case 'b': case 'c': case 'd': case 'e':
                        case 'f':
                          n = n * 16 + 10 + c - 'a';
                          break;
                        default:
                          phase1_ungetc (xp, c);
                          c = 0;
                          break;
                        }
                      if (c == 0)
                        break;
                    }
                  if (j == 0)
                    {
                      phase1_ungetc (xp, 'x');
                      c = '\\';
                    }
                  else
                    c = n;
                }
                break;

              case 'n':
                c = '\n';
                break;
              case 't':
                c = '\t';
                break;
              case 'r':
                c = '\r';
                break;

              case '"':
                if (!heredoc)
                  break;
                FALLTHROUGH;
              default:
                phase1_ungetc (xp, c);
                c = '\\';
                break;
              }
          }
        sb_xappend1 (&buffer, c);
      }
    if (is_constant)
      return sb_xdupfree_c (&buffer);
    else
      {
        sb_free (&buffer);
        return NULL;
      }
  }

 string_with_embedded_expressions:
  is_constant = false;
  if (++(xp->brace_nesting_depth) > MAX_NESTING_DEPTH)
    if_error (IF_SEVERITY_FATAL_ERROR,
              logical_file_name, xp->line_number, (size_t)(-1), false,
              _("too many open braces"));
  if (extract_balanced (xp, token_type_rbrace,
                        null_context_region (),
                        null_context_list_iterator,
                        arglist_parser_alloc (xp->mlp, NULL)))
    return NULL;
  xp->brace_nesting_depth--;
  goto string_continued;
}

static void
phase4_get (struct php_extractor *xp, token_ty *tp)
{
  if (xp->phase4_pushback_length)
    {
      *tp = xp->phase4_pushback[--(xp->phase4_pushback_length)];
      return;
    }
  tp->string = NULL;

  for (;;)
    {
      int c;

      tp->line_number = xp->line_number;
      c = phase3_getc (xp);
      switch (c)
        {
        case EOF:
          tp->type = token_type_eof;
          return;

        case '\n':
          if (xp->last_non_comment_line > xp->last_comment_line)
            savable_comment_reset ();
          FALLTHROUGH;
        case ' ':
        case '\t':
        case '\r':
          /* Ignore whitespace.  */
          continue;
        }

      xp->last_non_comment_line = tp->line_number;

      switch (c)
        {
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '_':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case 128: case 129: case 130: case 131: case 132: case 133: case 134:
        case 135: case 136: case 137: case 138: case 139: case 140: case 141:
        case 142: case 143: case 144: case 145: case 146: case 147: case 148:
        case 149: case 150: case 151: case 152: case 153: case 154: case 155:
        case 156: case 157: case 158: case 159: case 160: case 161: case 162:
        case 163: case 164: case 165: case 166: case 167: case 168: case 169:
        case 170: case 171: case 172: case 173: case 174: case 175: case 176:
        case 177: case 178: case 179: case 180: case 181: case 182: case 183:
        case 184: case 185: case 186: case 187: case 188: case 189: case 190:
        case 191: case 192: case 193: case 194: case 195: case 196: case 197:
        case 198: case 199: case 200: case 201: case 202: case 203: case 204:
        case 205: case 206: case 207: case 208: case 209: case 210: case 211:
        case 212: case 213: case 214: case 215: case 216: case 217: case 218:
        case 219: case 220: case 221: case 222: case 223: case 224: case 225:
        case 226: case 227: case 228: case 229: case 230: case 231: case 232:
        case 233: case 234: case 235: case 236: case 237: case 238: case 239:
        case 240: case 241: case 242: case 243: case 244: case 245: case 246:
        case 247: case 248: case 249: case 250: case 251: case 252: case 253:
        case 254: case 255:
          {
            struct string_buffer buffer;
            sb_init (&buffer);
            for (;;)
              {
                sb_xappend1 (&buffer, c);
                c = phase1_getc (xp);
                switch (c)
                  {
                  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                  case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
                  case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
                  case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
                  case 'Y': case 'Z':
                  case '_':
                  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                  case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
                  case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
                  case 's': case 't': case 'u': case 'v': case 'w': case 'x':
                  case 'y': case 'z':
                  case '0': case '1': case '2': case '3': case '4':
                  case '5': case '6': case '7': case '8': case '9':
                  case 128: case 129: case 130: case 131: case 132: case 133:
                  case 134: case 135: case 136: case 137: case 138: case 139:
                  case 140: case 141: case 142: case 143: case 144: case 145:
                  case 146: case 147: case 148: case 149: case 150: case 151:
                  case 152: case 153: case 154: case 155: case 156: case 157:
                  case 158: case 159: case 160: case 161: case 162: case 163:
                  case 164: case 165: case 166: case 167: case 168: case 169:
                  case 170: case 171: case 172: case 173: case 174: case 175:
                  case 176: case 177: case 178: case 179: case 180: case 181:
                  case 182: case 183: case 184: case 185: case 186: case 187:
                  case 188: case 189: case 190: case 191: case 192: case 193:
                  case 194: case 195: case 196: case 197: case 198: case 199:
                  case 200: case 201: case 202: case 203: case 204: case 205:
                  case 206: case 207: case 208: case 209: case 210: case 211:
                  case 212: case 213: case 214: case 215: case 216: case 217:
                  case 218: case 219: case 220: case 221: case 222: case 223:
                  case 224: case 225: case 226: case 227: case 228: case 229:
                  case 230: case 231: case 232: case 233: case 234: case 235:
                  case 236: case 237: case 238: case 239: case 240: case 241:
                  case 242: case 243: case 244: case 245: case 246: case 247:
                  case 248: case 249: case 250: case 251: case 252: case 253:
                  case 254: case 255:
                    continue;

                  default:
                    phase1_ungetc (xp, c);
                    break;
                  }
                break;
              }
            tp->string = sb_xdupfree_c (&buffer);
            tp->type = token_type_symbol;
          }
          return;

        case '\'':
          /* Single-quoted string literal.  */
          {
            struct string_buffer buffer;
            sb_init (&buffer);
            for (;;)
              {
                c = phase1_getc (xp);
                if (c == EOF || c == '\'')
                  break;
                if (c == '\\')
                  {
                    c = phase1_getc (xp);
                    if (c != '\\' && c != '\'')
                      {
                        phase1_ungetc (xp, c);
                        c = '\\';
                      }
                  }
                sb_xappend1 (&buffer, c);
              }
            tp->type = token_type_string_literal;
            tp->string = sb_xdupfree_c (&buffer);
            tp->comment = add_reference (savable_comment);
          }
          return;

        case '"':
          /* Double-quoted string literal.  */
          {
            char *string = process_dquote_or_heredoc (xp, false);
            if (string != NULL)
              {
                tp->type = token_type_string_literal;
                tp->string = string;
                tp->comment = add_reference (savable_comment);
              }
            else
              tp->type = token_type_other;
          }
          return;

        case '?':
          {
            int c2 = phase1_getc (xp);
            if (c2 == '>')
              {
                /* ?> terminates PHP mode and switches back to HTML mode.  */
                skip_html (xp);
                tp->type = token_type_other;
              }
            else
              {
                phase1_ungetc (xp, c2);
                tp->type = token_type_other;
              }
            return;
          }

        case '(':
          tp->type = token_type_lparen;
          return;

        case ')':
          tp->type = token_type_rparen;
          return;

        case ',':
          tp->type = token_type_comma;
          return;

        case '[':
          tp->type = token_type_lbracket;
          return;

        case ']':
          tp->type = token_type_rbracket;
          return;

        case '.':
          tp->type = token_type_dot;
          return;

        case '*':
        case '/':
          tp->type = token_type_operator1;
          return;

        case '+':
        case '-':
          {
            int c2 = phase1_getc (xp);
            if (c2 == c)
              /* ++ or -- */
              tp->type = token_type_operator1;
            else
              /* + or - */
              {
                phase1_ungetc (xp, c2);
                tp->type = token_type_operator2;
              }
            return;
          }

        case '!':
        case '~':
        case '@':
          tp->type = token_type_operator2;
          return;

        case '{':
          tp->type = token_type_lbrace;
          return;

        case '}':
          tp->type = token_type_rbrace;
          return;

        case '<':
          {
            int c2 = phase1_getc (xp);
            if (c2 == '<')
              {
                int c3 = phase1_getc (xp);
                if (c3 == '<')
                  {
                    /* Start of heredoc or nowdoc.
                       Parse whitespace, then label, then newline.  */
                    do
                      c = phase3_getc (xp);
                    while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

                    struct string_buffer buffer;
                    sb_init (&buffer);
                    do
                      {
                        sb_xappend1 (&buffer, c);
                        c = phase3_getc (xp);
                      }
                    while (c != EOF && c != '\n' && c != '\r');
                    /* buffer now contains the label
                       (including single or double quotes).  */

                    int doc_line_number = xp->line_number;

                    bool heredoc = true;
                    string_desc_t label = sb_contents (&buffer);
                    size_t label_start = 0;
                    size_t label_end = sd_length (label);
                    if (label_end >= 2
                        && ((sd_char_at (label, label_start) == '\''
                             && sd_char_at (label, label_end - 1) == '\'')
                            || (sd_char_at (label, label_start) == '"'
                                && sd_char_at (label, label_end - 1) == '"')))
                      {
                        heredoc = (sd_char_at (label, label_start) == '"');
                        label_start++;
                        label_end--;
                      }

                    /* Now read the heredoc or nowdoc.  */
                    size_t doc_alloc = 10;
                    char *doc = xmalloc (doc_alloc);
                    size_t doc_len = 0;
                    size_t doc_start_of_line = 0;

                    /* These two variables keep track of the matching of the
                       end label.  */
                    int in_label_pos = -1; /* <= label_end - label_start */
                    int end_label_indent = 0;

                    for (;;)
                      {
                        c = phase1_getc (xp);
                        if (c == EOF)
                          break;

                        if (doc_len >= doc_alloc)
                          {
                            doc_alloc = 2 * doc_alloc + 10;
                            doc = xrealloc (doc, doc_alloc);
                          }
                        doc[doc_len++] = c;

                        if (c == '\n')
                          doc_start_of_line = doc_len;

                        /* Incrementally match the label.  */
                        if (in_label_pos == 0 && (c == ' ' || c == '\t'))
                          {
                            if (c == '\t')
                              end_label_indent |= TAB_WIDTH - 1;
                            end_label_indent++;
                          }
                        else if (in_label_pos >= 0
                                 && in_label_pos < label_end - label_start
                                 && c == sd_char_at (label, label_start + in_label_pos))
                          {
                            in_label_pos++;
                          }
                        else if (in_label_pos == label_end - label_start)
                          {
                            switch (c)
                              {
                              case 'A': case 'B': case 'C': case 'D': case 'E':
                              case 'F': case 'G': case 'H': case 'I': case 'J':
                              case 'K': case 'L': case 'M': case 'N': case 'O':
                              case 'P': case 'Q': case 'R': case 'S': case 'T':
                              case 'U': case 'V': case 'W': case 'X': case 'Y':
                              case 'Z':
                              case '_':
                              case 'a': case 'b': case 'c': case 'd': case 'e':
                              case 'f': case 'g': case 'h': case 'i': case 'j':
                              case 'k': case 'l': case 'm': case 'n': case 'o':
                              case 'p': case 'q': case 'r': case 's': case 't':
                              case 'u': case 'v': case 'w': case 'x': case 'y':
                              case 'z':
                              case '0': case '1': case '2': case '3': case '4':
                              case '5': case '6': case '7': case '8': case '9':
                              case 128: case 129: case 130: case 131: case 132:
                              case 133: case 134: case 135: case 136: case 137:
                              case 138: case 139: case 140: case 141: case 142:
                              case 143: case 144: case 145: case 146: case 147:
                              case 148: case 149: case 150: case 151: case 152:
                              case 153: case 154: case 155: case 156: case 157:
                              case 158: case 159: case 160: case 161: case 162:
                              case 163: case 164: case 165: case 166: case 167:
                              case 168: case 169: case 170: case 171: case 172:
                              case 173: case 174: case 175: case 176: case 177:
                              case 178: case 179: case 180: case 181: case 182:
                              case 183: case 184: case 185: case 186: case 187:
                              case 188: case 189: case 190: case 191: case 192:
                              case 193: case 194: case 195: case 196: case 197:
                              case 198: case 199: case 200: case 201: case 202:
                              case 203: case 204: case 205: case 206: case 207:
                              case 208: case 209: case 210: case 211: case 212:
                              case 213: case 214: case 215: case 216: case 217:
                              case 218: case 219: case 220: case 221: case 222:
                              case 223: case 224: case 225: case 226: case 227:
                              case 228: case 229: case 230: case 231: case 232:
                              case 233: case 234: case 235: case 236: case 237:
                              case 238: case 239: case 240: case 241: case 242:
                              case 243: case 244: case 245: case 246: case 247:
                              case 248: case 249: case 250: case 251: case 252:
                              case 253: case 254: case 255:
                                in_label_pos = -1;
                                break;
                              default:
                                break;
                              }
                            if (in_label_pos >= 0)
                              {
                                /* Finished recognizing the label.  */
                                phase1_ungetc (xp, c);
                                break;
                              }
                          }
                        else if (c == '\n' || c == '\r')
                          {
                            in_label_pos = 0;
                            end_label_indent = 0;
                          }
                        else
                          {
                            in_label_pos = -1;
                            end_label_indent = 0;
                          }
                      }

                    sb_free (&buffer);

                    /* The contents is the substring
                       [doc, doc + doc_start_of_line).  */
                    doc_len = doc_start_of_line;

                    /* Discard leading indentation.  */
                    if (end_label_indent > 0)
                      {
                        /* Scan through the doc string, copying *q = *p.  */
                        char *q = doc;
                        int curr_line_indent = 0;

                        for (const char *p = doc; p < doc + doc_len; p++)
                          {
                            /* Invariant: doc <= q <= p <= doc + doc_len.  */
                            char d = *p;
                            *q++ = d;
                            if (curr_line_indent < end_label_indent)
                              {
                                if (d == ' ')
                                  {
                                    curr_line_indent++;
                                    --q;
                                  }
                                else if (d == '\t')
                                  {
                                    curr_line_indent |= TAB_WIDTH - 1;
                                    curr_line_indent++;
                                    if (curr_line_indent <= end_label_indent)
                                      --q;
                                  }
                              }
                            if (d == '\n')
                              curr_line_indent = 0;
                          }
                        doc_len = q - doc;
                      }

                    /* Discard the trailing newline.  */
                    if (doc_len > 0 && doc[doc_len - 1] == '\n')
                      {
                        --doc_len;
                        if (doc_len > 0 && doc[doc_len - 1] == '\r')
                          --doc_len;
                      }

                    /* NUL-terminate it.  */
                    if (doc_len >= doc_alloc)
                      {
                        doc_alloc = doc_alloc + 1;
                        doc = xrealloc (doc, doc_alloc);
                      }
                    doc[doc_len++] = '\0';

                    /* For a here document, do the same processing as in
                       double-quoted strings (except for recognizing a
                       double-quote as end-of-string, of course).  */
                    if (heredoc)
                      {
                        struct php_extractor hxp;
                        hxp.mlp = xp->mlp;
                        sf_istream_init_from_string (&hxp.input, doc);
                        hxp.line_number = doc_line_number;
                        php_extractor_init_rest (&hxp);

                        char *processed_doc =
                          process_dquote_or_heredoc (&hxp, true);
                        free (doc);
                        doc = processed_doc;
                      }

                    if (doc != NULL)
                      {
                        tp->type = token_type_string_literal;
                        tp->string = doc;
                        tp->comment = add_reference (savable_comment);
                      }
                    else
                      tp->type = token_type_other;
                    return;
                  }
                phase1_ungetc (xp, c3);
              }

            /* < / script > terminates PHP mode and switches back to HTML
               mode.  */
            while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
              c2 = phase1_getc (xp);
            if (c2 == '/')
              {
                do
                  c2 = phase1_getc (xp);
                while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
                if (c2 == 's' || c2 == 'S')
                  {
                    c2 = phase1_getc (xp);
                    if (c2 == 'c' || c2 == 'C')
                      {
                        c2 = phase1_getc (xp);
                        if (c2 == 'r' || c2 == 'R')
                          {
                            c2 = phase1_getc (xp);
                            if (c2 == 'i' || c2 == 'I')
                              {
                                c2 = phase1_getc (xp);
                                if (c2 == 'p' || c2 == 'P')
                                  {
                                    c2 = phase1_getc (xp);
                                    if (c2 == 't' || c2 == 'T')
                                      {
                                        do
                                          c2 = phase1_getc (xp);
                                        while (c2 == ' ' || c2 == '\t'
                                               || c2 == '\n' || c2 == '\r');
                                        if (c2 == '>')
                                          {
                                            skip_html (xp);
                                          }
                                        else
                                          phase1_ungetc (xp, c2);
                                      }
                                    else
                                      phase1_ungetc (xp, c2);
                                  }
                                else
                                  phase1_ungetc (xp, c2);
                              }
                            else
                              phase1_ungetc (xp, c2);
                          }
                        else
                          phase1_ungetc (xp, c2);
                      }
                    else
                      phase1_ungetc (xp, c2);
                  }
                else
                  phase1_ungetc (xp, c2);
              }
            else
              phase1_ungetc (xp, c2);

            tp->type = token_type_other;
            return;
          }

        case '`':
          /* Execution operator.  */
        default:
          /* We could carefully recognize each of the 2 and 3 character
             operators, but it is not necessary, as we only need to recognize
             gettext invocations.  Don't bother.  */
          tp->type = token_type_other;
          return;
        }
    }
}

/* Supports 3 tokens of pushback.  */
static void
phase4_unget (struct php_extractor *xp, token_ty *tp)
{
  if (tp->type != token_type_eof)
    {
      if (xp->phase4_pushback_length == SIZEOF (xp->phase4_pushback))
        abort ();
      xp->phase4_pushback[xp->phase4_pushback_length++] = *tp;
    }
}


/* 5. Compile-time optimization of string literal concatenation.
   Combine "string1" . ... . "stringN" to the concatenated string if
     - the token before this expression is none of
       '+' '-' '.' '*' '/' '%' '!' '~' '++' '--' ')' '@'
       (because then the first string could be part of an expression with
       the same or higher precedence as '.', such as an additive,
       multiplicative, negation, preincrement, or cast expression),
     - the token after this expression is none of
       '*' '/' '%' '++' '--'
       (because then the last string could be part of an expression with
       higher precedence as '.', such as a multiplicative or postincrement
       expression).  */

static void
x_php_lex (struct php_extractor *xp, token_ty *tp)
{
  phase4_get (xp, tp);
  if (tp->type == token_type_string_literal
      && !(xp->phase5_last == token_type_dot
           || xp->phase5_last == token_type_operator1
           || xp->phase5_last == token_type_operator2
           || xp->phase5_last == token_type_rparen))
    {
      char *sum = tp->string;
      size_t sum_len = strlen (sum);

      for (;;)
        {
          token_ty token2;
          phase4_get (xp, &token2);

          if (token2.type == token_type_dot)
            {
              token_ty token3;
              phase4_get (xp, &token3);

              if (token3.type == token_type_string_literal)
                {
                  token_ty token_after;
                  phase4_get (xp, &token_after);

                  if (token_after.type != token_type_operator1)
                    {
                      char *addend = token3.string;
                      size_t addend_len = strlen (addend);

                      sum = (char *) xrealloc (sum, sum_len + addend_len + 1);
                      memcpy (sum + sum_len, addend, addend_len + 1);
                      sum_len += addend_len;

                      phase4_unget (xp, &token_after);
                      free_token (&token3);
                      free_token (&token2);
                      continue;
                    }
                  phase4_unget (xp, &token_after);
                }
              phase4_unget (xp, &token3);
            }
          phase4_unget (xp, &token2);
          break;
        }
      tp->string = sum;
    }
  xp->phase5_last = tp->type;
}


/* ========================= Extracting strings.  ========================== */


/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;


/* The file is broken into tokens.  Scan the token stream, looking for
   a keyword, followed by a left paren, followed by a string.  When we
   see this sequence, we have something to remember.  We assume we are
   looking at a valid C or C++ program, and leave the complaints about
   the grammar to the compiler.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */


/* Extract messages until the next balanced closing parenthesis or bracket.
   Extracted messages are added to XP->MLP.
   DELIM can be either token_type_rparen or token_type_rbracket or
   token_type_rbrace, or token_type_eof to accept any of them.
   Return true upon eof, false upon closing parenthesis or bracket.  */
static bool
extract_balanced (struct php_extractor *xp,
                  token_type_ty delim,
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
      x_php_lex (xp, &token);

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
          if (++(xp->paren_nesting_depth) > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, xp->line_number, (size_t)(-1), false,
                      _("too many open parentheses"));
          if (extract_balanced (xp, token_type_rparen,
                                inner_region, next_context_iter,
                                arglist_parser_alloc (xp->mlp,
                                                      state ? next_shapes : NULL)))
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return true;
            }
          xp->paren_nesting_depth--;
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_rparen:
          if (delim == token_type_rparen || delim == token_type_eof)
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return false;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

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

        case token_type_lbracket:
          if (++(xp->bracket_nesting_depth) > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, xp->line_number, (size_t)(-1), false,
                      _("too many open brackets"));
          if (extract_balanced (xp, token_type_rbracket,
                                null_context_region (),
                                null_context_list_iterator,
                                arglist_parser_alloc (xp->mlp, NULL)))
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return true;
            }
          xp->bracket_nesting_depth--;
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_rbracket:
          if (delim == token_type_rbracket || delim == token_type_eof)
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return false;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_lbrace:
          if (++(xp->brace_nesting_depth) > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, xp->line_number, (size_t)(-1), false,
                      _("too many open braces"));
          if (extract_balanced (xp, token_type_rbrace,
                                null_context_region (),
                                null_context_list_iterator,
                                arglist_parser_alloc (xp->mlp, NULL)))
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return true;
            }
          xp->brace_nesting_depth--;
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_rbrace:
          if (delim == token_type_rbrace || delim == token_type_eof)
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return false;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_string_literal:
          {
            lex_pos_ty pos;
            pos.file_name = logical_file_name;
            pos.line_number = token.line_number;

            if (extract_all)
              remember_a_message (xp->mlp, NULL, token.string, false, false,
                                  inner_region, &pos,
                                  NULL, token.comment, false);
            else
              {
                mixed_string_ty *ms =
                  mixed_string_alloc_simple (token.string, lc_string,
                                             pos.file_name, pos.line_number);
                free (token.string);
                arglist_parser_remember (argparser, arg, ms, inner_region,
                                         pos.file_name, pos.line_number,
                                         token.comment, false);
              }
            drop_reference (token.comment);
          }
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_dot:
        case token_type_operator1:
        case token_type_operator2:
        case token_type_other:
          next_context_iter = null_context_list_iterator;
          state = 0;
          break;

        case token_type_eof:
          arglist_parser_done (argparser, arg);
          unref_region (inner_region);
          return true;

        default:
          abort ();
        }
    }
}


void
extract_php (FILE *f,
             const char *real_filename, const char *logical_filename,
             flag_context_list_table_ty *flag_table,
             msgdomain_list_ty *mdlp)
{
  flag_context_list_table = flag_table;

  init_keywords ();

  struct php_extractor *xp = XMALLOC (struct php_extractor);

  xp->mlp = mdlp->item[0]->messages;
  sf_istream_init_from_file (&xp->input, f);
  real_file_name = real_filename;
  logical_file_name = xstrdup (logical_filename);
  xp->line_number = 1;
  php_extractor_init_rest (xp);

  /* Initial mode is HTML mode, not PHP mode.  */
  skip_html (xp);

  /* Eat tokens until eof is seen.  When extract_balanced returns
     due to an unbalanced closing parenthesis, just restart it.  */
  while (!extract_balanced (xp, token_type_eof,
                            null_context_region (), null_context_list_iterator,
                            arglist_parser_alloc (xp->mlp, NULL)))
    ;

  /* Close scanner.  */
  free (xp);
  real_file_name = NULL;
  logical_file_name = NULL;
}
