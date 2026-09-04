/* xgettext Perl backend.
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

/* Written by Guido Flohr and Bruno Haible.  */

#include <config.h>

/* Specification.  */
#include "x-perl.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_NO_APPENDF
#include <error.h>
#include "attribute.h"
#include "message.h"
#include "sf-istream.h"
#include "rc-str-list.h"
#include "string-desc.h"
#include "xstring-desc.h"
#include "xgettext.h"
#include "xg-pos.h"
#include "xg-encoding.h"
#include "xg-mixed-string.h"
#include "xg-arglist-context.h"
#include "xg-arglist-callshape.h"
#include "xg-arglist-parser.h"
#include "xg-message.h"
#include "if-error.h"
#include "xalloc.h"
#include "string-buffer.h"
#include "c-ctype.h"
#include "po-charset.h"
#include "unistr.h"
#include "uniname.h"
#include "gettext.h"

#define _(s) gettext(s)

/* The Perl syntax is defined in perlsyn.pod.  Try the command
   "man perlsyn" or "perldoc perlsyn".
   Also, the syntax after the 'sub' keyword is specified in perlsub.pod.
   Try the command "man perlsub" or "perldoc perlsub".
   Perl 5.10 has new operators '//' and '//=', see
   <https://perldoc.perl.org/perldelta.html#Defined-or-operator>.

   The actual Perl lexer and parser are in
     perl-5.40.0/toke.c
     perl-5.40.0/perly.y
   but, for your sanity, you better don't look at it :)
 */

#define DEBUG_PERL 0
#define DEBUG_NESTING_DEPTH 0


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_perl_extract_all ()
{
  extract_all = true;
}


void
x_perl_keyword (const char *name)
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
      x_perl_keyword ("gettext");
      x_perl_keyword ("%gettext");
      x_perl_keyword ("$gettext");
      x_perl_keyword ("dgettext:2");
      x_perl_keyword ("dcgettext:2");
      x_perl_keyword ("ngettext:1,2");
      x_perl_keyword ("dngettext:2,3");
      x_perl_keyword ("dcngettext:2,3");
      x_perl_keyword ("gettext_noop");
      x_perl_keyword ("pgettext:1c,2");
      x_perl_keyword ("dpgettext:2c,3");
      x_perl_keyword ("dcpgettext:2c,3");
      x_perl_keyword ("npgettext:1c,2,3");
      x_perl_keyword ("dnpgettext:2c,3,4");
      x_perl_keyword ("dcnpgettext:2c,3,4");

#if 0
      x_perl_keyword ("__");
      x_perl_keyword ("$__");
      x_perl_keyword ("%__");
      x_perl_keyword ("__x");
      x_perl_keyword ("__n:1,2");
      x_perl_keyword ("__nx:1,2");
      x_perl_keyword ("__xn:1,2");
      x_perl_keyword ("N__");
#endif
      default_keywords = false;
    }
}

void
init_flag_table_perl ()
{
  /* Gettext binding for Perl.  */
  xgettext_record_flag ("gettext:1:pass-perl-format");
  xgettext_record_flag ("gettext:1:pass-perl-brace-format");
  xgettext_record_flag ("%gettext:1:pass-perl-format");
  xgettext_record_flag ("%gettext:1:pass-perl-brace-format");
  xgettext_record_flag ("$gettext:1:pass-perl-format");
  xgettext_record_flag ("$gettext:1:pass-perl-brace-format");
  xgettext_record_flag ("dgettext:2:pass-perl-format");
  xgettext_record_flag ("dgettext:2:pass-perl-brace-format");
  xgettext_record_flag ("dcgettext:2:pass-perl-format");
  xgettext_record_flag ("dcgettext:2:pass-perl-brace-format");
  xgettext_record_flag ("ngettext:1:pass-perl-format");
  xgettext_record_flag ("ngettext:2:pass-perl-format");
  xgettext_record_flag ("ngettext:1:pass-perl-brace-format");
  xgettext_record_flag ("ngettext:2:pass-perl-brace-format");
  xgettext_record_flag ("dngettext:2:pass-perl-format");
  xgettext_record_flag ("dngettext:3:pass-perl-format");
  xgettext_record_flag ("dngettext:2:pass-perl-brace-format");
  xgettext_record_flag ("dngettext:3:pass-perl-brace-format");
  xgettext_record_flag ("dcngettext:2:pass-perl-format");
  xgettext_record_flag ("dcngettext:3:pass-perl-format");
  xgettext_record_flag ("dcngettext:2:pass-perl-brace-format");
  xgettext_record_flag ("dcngettext:3:pass-perl-brace-format");
  xgettext_record_flag ("gettext_noop:1:pass-perl-format");
  xgettext_record_flag ("gettext_noop:1:pass-perl-brace-format");
  xgettext_record_flag ("pgettext:2:pass-perl-format");
  xgettext_record_flag ("pgettext:2:pass-perl-brace-format");
  xgettext_record_flag ("dpgettext:3:pass-perl-format");
  xgettext_record_flag ("dpgettext:3:pass-perl-brace-format");
  xgettext_record_flag ("dcpgettext:3:pass-perl-format");
  xgettext_record_flag ("dcpgettext:3:pass-perl-brace-format");
  xgettext_record_flag ("npgettext:2:pass-perl-format");
  xgettext_record_flag ("npgettext:3:pass-perl-format");
  xgettext_record_flag ("npgettext:2:pass-perl-brace-format");
  xgettext_record_flag ("npgettext:3:pass-perl-brace-format");
  xgettext_record_flag ("dnpgettext:3:pass-perl-format");
  xgettext_record_flag ("dnpgettext:4:pass-perl-format");
  xgettext_record_flag ("dnpgettext:3:pass-perl-brace-format");
  xgettext_record_flag ("dnpgettext:4:pass-perl-brace-format");
  xgettext_record_flag ("dcnpgettext:3:pass-perl-format");
  xgettext_record_flag ("dcnpgettext:4:pass-perl-format");
  xgettext_record_flag ("dcnpgettext:3:pass-perl-brace-format");
  xgettext_record_flag ("dcnpgettext:4:pass-perl-brace-format");

  /* Perl builtins.  */
  xgettext_record_flag ("printf:1:perl-format"); /* argument 1 or 2 ?? */
  xgettext_record_flag ("sprintf:1:perl-format");
#if 0
  /* Shortcuts from libintl-perl.  */
  xgettext_record_flag ("__:1:pass-perl-format");
  xgettext_record_flag ("__:1:pass-perl-brace-format");
  xgettext_record_flag ("%__:1:pass-perl-format");
  xgettext_record_flag ("%__:1:pass-perl-brace-format");
  xgettext_record_flag ("$__:1:pass-perl-format");
  xgettext_record_flag ("$__:1:pass-perl-brace-format");
  xgettext_record_flag ("__x:1:perl-brace-format");
  xgettext_record_flag ("__n:1:pass-perl-format");
  xgettext_record_flag ("__n:2:pass-perl-format");
  xgettext_record_flag ("__n:1:pass-perl-brace-format");
  xgettext_record_flag ("__n:2:pass-perl-brace-format");
  xgettext_record_flag ("__nx:1:perl-brace-format");
  xgettext_record_flag ("__nx:2:perl-brace-format");
  xgettext_record_flag ("__xn:1:perl-brace-format");
  xgettext_record_flag ("__xn:2:perl-brace-format");
  xgettext_record_flag ("N__:1:pass-perl-format");
  xgettext_record_flag ("N__:1:pass-perl-brace-format");
#endif
}


/* =================== Variables used by the extractor.  =================== */

/* Type definitions needed for the variables.  */

enum token_type_ty
{
  token_type_eof,
  token_type_lparen,            /* ( */
  token_type_rparen,            /* ) */
  token_type_comma,             /* , */
  token_type_fat_comma,         /* => */
  token_type_dereference,       /* -> */
  token_type_semicolon,         /* ; */
  token_type_lbrace,            /* { */
  token_type_rbrace,            /* } */
  token_type_lbracket,          /* [ */
  token_type_rbracket,          /* ] */
  token_type_string,            /* quote-like */
  token_type_string_interpol,   /* quote-like with embedded expressions */
  token_type_number,            /* starting with a digit or dot */
  token_type_named_op,          /* if, unless, while, ... */
  token_type_variable,          /* $... */
  token_type_object,            /* A dereferenced variable, maybe a blessed
                                   object.  */
  token_type_symbol,            /* symbol, number */
  token_type_regex_op,          /* s, tr, y, m.  */
  token_type_dot,               /* . */
  token_type_other,             /* regexp, misc. operator */
  /* The following are not really token types, but variants used by
     the parser.  */
  token_type_keyword_symbol,    /* keyword symbol */
  token_type_r_any              /* rparen rbrace rbracket */
};
typedef enum token_type_ty token_type_ty;

typedef struct token_ty token_ty;

typedef struct token_stack_ty token_stack_ty;
struct token_stack_ty
{
  token_ty **items;
  size_t nitems;
  size_t nitems_max;
};


/* These variables are combined in a struct, so that we can invoke the
   extractor in a reentrant way.  */

struct perl_extractor
{
  /* Accumulator for the output.  */
  message_list_ty *mlp;

  /* The input.  */
  sf_istream_t input;

  int line_number;

  /* The current line buffer.  */
  char *linebuf;
  /* The size of the input buffer.  */
  size_t linebuf_size;

  /* The size of the current line.  */
  int linesize;

  /* The position in the current line.  */
  int linepos;

  /* Number of lines eaten for here documents.  */
  int eaten_here;

  /* Paranoia: EOF marker for __END__ or __DATA__.  */
  bool end_of_file;

  /* These are for tracking whether comments count as immediately before
     keyword.  */
  int last_comment_line;
  int last_non_comment_line;

  /* Maximum supported nesting depth.  */
  #define MAX_NESTING_DEPTH 1000

  /* Current nesting depth.  */
  int nesting_depth;

  /* Last token type seen in the stream.  Important for the interpretation
     of slash and question mark.  */
  token_type_ty last_token_type;

  /* A token stack used as a lookahead buffer.  */
  struct token_stack_ty token_stack;
};

static inline void
perl_extractor_init_rest (struct perl_extractor *xp)
{
  xp->line_number = 0;
  xp->linebuf = NULL;
  xp->linebuf_size = 0;
  xp->linesize = 0;
  xp->linepos = 0;
  xp->eaten_here = 0;
  xp->end_of_file = false;
  xp->last_comment_line = -1;
  xp->last_non_comment_line = -1;
  xp->nesting_depth = 0;
  /* Safe assumption.  */
  xp->last_token_type = token_type_semicolon;
  xp->token_stack.items = NULL;
  xp->token_stack.nitems = 0;
  xp->token_stack.nitems_max = 0;
}


/* ======================== Reading of characters.  ======================== */


/* 1. line_number handling.  */

/* Returns the next character from the input stream or EOF.  */
static int
phase1_getc (struct perl_extractor *xp)
{
  xp->line_number += xp->eaten_here;
  xp->eaten_here = 0;

  if (xp->end_of_file)
    return EOF;

  if (xp->input.fp != NULL)
    {
      if (xp->linepos >= xp->linesize)
        {
          xp->linesize =
            getline (&xp->linebuf, &xp->linebuf_size, xp->input.fp);

          if (xp->linesize < 0)
            {
              if (ferror (xp->input.fp))
                error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
                       real_file_name);
              xp->end_of_file = true;
              return EOF;
            }

          xp->linepos = 0;
          ++(xp->line_number);

          /* Undosify.  This is important for catching the end of <<EOF and
             <<'EOF'.  We could rely on stdio doing this for us but
             it is not uncommon to to come across Perl scripts with CRLF
             newline conventions on systems that do not follow this
             convention.  */
          if (xp->linesize >= 2 && xp->linebuf[xp->linesize - 1] == '\n'
              && xp->linebuf[xp->linesize - 2] == '\r')
            {
              xp->linebuf[xp->linesize - 2] = '\n';
              xp->linebuf[xp->linesize - 1] = '\0';
              --(xp->linesize);
            }
        }
    }
  else
    {
      if (xp->linebuf == NULL)
        {
          xp->linebuf = (char *) xp->input.input;
          xp->linesize = xp->input.input_end - xp->input.input;
          xp->linepos = 0;
        }
      if (xp->linepos >= xp->linesize)
        {
          xp->end_of_file = true;
          return EOF;
        }
    }

  return xp->linebuf[xp->linepos++];
}

/* Supports only one pushback character.  */
static void
phase1_ungetc (struct perl_extractor *xp, int c)
{
  if (c != EOF)
    {
      if (xp->linepos == 0)
        /* Attempt to ungetc across line boundary.  Shouldn't happen.
           No two phase1_ungetc calls are permitted in a row.  */
        abort ();

      --(xp->linepos);
    }
}

/* Read a here document and return its contents.
   The delimiter is an UTF-8 encoded string; the resulting string is UTF-8
   encoded as well.  */

static char *
get_here_document (struct perl_extractor *xp, const char *delimiter)
{
  /* Accumulator for the entire here document, including a NUL byte
     at the end.  */
  struct string_buffer buffer;
  sb_init (&buffer);

  /* Current line being appended.  */
  static char *my_linebuf = NULL;
  static size_t my_linebuf_size = 0;

  for (;;)
    {
      int read_bytes = getline (&my_linebuf, &my_linebuf_size, xp->input.fp);
      if (read_bytes < 0)
        {
          if (ferror (xp->input.fp))
            {
              error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
                     real_file_name);
            }
          else
            {
              if_error (IF_SEVERITY_WARNING,
                        real_file_name, xp->line_number, (size_t)(-1), false,
                        _("can't find string terminator \"%s\" anywhere before EOF"),
                        delimiter);
              break;
            }
        }

      ++(xp->eaten_here);

      /* Convert to UTF-8.  */
      char *my_line_utf8 =
        from_current_source_encoding (my_linebuf, lc_string, logical_file_name,
                                      xp->line_number + xp->eaten_here);
      if (my_line_utf8 != my_linebuf)
        {
          if (strlen (my_line_utf8) >= my_linebuf_size)
            {
              my_linebuf_size = strlen (my_line_utf8) + 1;
              my_linebuf = xrealloc (my_linebuf, my_linebuf_size);
            }
          strcpy (my_linebuf, my_line_utf8);
          read_bytes = strlen (my_line_utf8);
          free (my_line_utf8);
        }

      /* Undosify.  This is important for catching the end of <<EOF and
         <<'EOF'.  We could rely on stdio doing this for us but you
         it is not uncommon to to come across Perl scripts with CRLF
         newline conventions on systems that do not follow this
         convention.  */
      if (read_bytes >= 2 && my_linebuf[read_bytes - 1] == '\n'
          && my_linebuf[read_bytes - 2] == '\r')
        {
          my_linebuf[read_bytes - 2] = '\n';
          my_linebuf[read_bytes - 1] = '\0';
          --read_bytes;
        }

      /* Temporarily remove the trailing newline from my_linebuf.  */
      bool chomp = false;
      if (read_bytes >= 1 && my_linebuf[read_bytes - 1] == '\n')
        {
          chomp = true;
          my_linebuf[read_bytes - 1] = '\0';
        }

      /* See whether this line terminates the here document.  */
      if (strcmp (my_linebuf, delimiter) == 0)
        break;

      /* Add back the trailing newline to my_linebuf.  */
      if (chomp)
        my_linebuf[read_bytes - 1] = '\n';

      /* Append this line to the accumulator.  */
      sb_xappend_desc (&buffer,
                       sd_new_addr (read_bytes, (const char *) my_linebuf));
    }

  /* Done accumulating the here document.  */
  return sb_xdupfree_c (&buffer);
}

/* Skips pod sections.  */
static void
skip_pod (struct perl_extractor *xp)
{
  xp->line_number += xp->eaten_here;
  xp->eaten_here = 0;
  xp->linepos = 0;

  for (;;)
    {
      xp->linesize = getline (&xp->linebuf, &xp->linebuf_size, xp->input.fp);

      if (xp->linesize < 0)
        {
          if (ferror (xp->input.fp))
            error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
                   real_file_name);
          return;
        }

      ++(xp->line_number);

      if (str_startswith (xp->linebuf, "=cut"))
        {
          /* Force reading of a new line on next call to phase1_getc().  */
          xp->linepos = xp->linesize;
          return;
        }
    }
}


/* 2. Replace each comment that is not inside a string literal or regular
   expression with a newline character.  We need to remember the comment
   for later, because it may be attached to a keyword string.  */

static int
phase2_getc (struct perl_extractor *xp)
{
  int c;

  c = phase1_getc (xp);
  if (c == '#')
    {
      struct string_buffer buffer;
      sb_init (&buffer);
      int lineno = xp->line_number;
      /* Skip leading whitespace.  */
      for (;;)
        {
          c = phase1_getc (xp);
          if (c == EOF)
            break;
          if (c != ' ' && c != '\t' && c != '\r' && c != '\f')
            {
              phase1_ungetc (xp, c);
              break;
            }
        }
      /* Accumulate the comment.  */
      for (;;)
        {
          c = phase1_getc (xp);
          if (c == '\n' || c == EOF)
            break;
          sb_xappend1 (&buffer, c);
        }
      /* Convert it to UTF-8.  */
      const char *contents = sb_xcontents_c (&buffer);
      char *utf8_contents =
        from_current_source_encoding (contents, lc_comment,
                                      logical_file_name, lineno);
      /* Save it until we encounter the corresponding string.  */
      savable_comment_add (utf8_contents);
      if (utf8_contents != contents)
        free (utf8_contents);
      sb_free (&buffer);
      xp->last_comment_line = lineno;
    }
  return c;
}

/* Supports only one pushback character.  */
static void
phase2_ungetc (struct perl_extractor *xp, int c)
{
  if (c != EOF)
    phase1_ungetc (xp, c);
}

/* Whitespace recognition.  */

#define case_whitespace \
  case ' ': case '\t': case '\r': case '\n': case '\f'

static inline bool
is_whitespace (int c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f');
}


/* ========================== Reading of tokens.  ========================== */


/* 'enum token_type_ty' is defined above.  */

/* Subtypes for strings, important for interpolation.  */
enum string_type_ty
{
  string_type_verbatim,     /* "<<'EOF'", "m'...'", "s'...''...'",
                               "tr/.../.../", "y/.../.../".  */
  string_type_q,            /* "'..'", "q/.../".  */
  string_type_qq,           /* '"..."', "`...`", "qq/.../", "qx/.../",
                               "<file*glob>".  */
  string_type_qr            /* Not supported.  */
};

/* Subtypes for symbols, important for dollar interpretation.  */
enum symbol_type_ty
{
  symbol_type_none,         /* Nothing special.  */
  symbol_type_sub,          /* 'sub'.  */
  symbol_type_function      /* Function name after 'sub'.  */
};

struct token_ty
{
  token_type_ty type;
  token_type_ty last_type;
  int sub_type;                 /* for token_type_string, token_type_symbol */
  char *string;                 /* for:                 in encoding:
                                   token_type_named_op  ASCII
                                   token_type_string    UTF-8
                                   token_type_symbol    ASCII
                                   token_type_variable  global_source_encoding
                                   token_type_object    global_source_encoding
                                 */
  refcounted_string_list_ty *comment; /* for token_type_string */
  int line_number;
};

#if DEBUG_PERL
static const char *
token2string (const token_ty *token)
{
  switch (token->type)
    {
    case token_type_eof:
      return "token_type_eof";
    case token_type_lparen:
      return "token_type_lparen";
    case token_type_rparen:
      return "token_type_rparen";
    case token_type_comma:
      return "token_type_comma";
    case token_type_fat_comma:
      return "token_type_fat_comma";
    case token_type_dereference:
      return "token_type_dereference";
    case token_type_semicolon:
      return "token_type_semicolon";
    case token_type_lbrace:
      return "token_type_lbrace";
    case token_type_rbrace:
      return "token_type_rbrace";
    case token_type_lbracket:
      return "token_type_lbracket";
    case token_type_rbracket:
      return "token_type_rbracket";
    case token_type_string:
      return "token_type_string";
    case token_type_string_interpol:
      return "token_type_string_interpol";
    case token_type_number:
      return "token type number";
    case token_type_named_op:
      return "token_type_named_op";
    case token_type_variable:
      return "token_type_variable";
    case token_type_object:
      return "token_type_object";
    case token_type_symbol:
      return "token_type_symbol";
    case token_type_regex_op:
      return "token_type_regex_op";
    case token_type_dot:
      return "token_type_dot";
    case token_type_other:
      return "token_type_other";
    default:
      return "unknown";
    }
}
#endif

/* Free the memory pointed to by a 'struct token_ty'.  */
static inline void
free_token (token_ty *tp)
{
  switch (tp->type)
    {
    case token_type_named_op:
    case token_type_string:
    case token_type_symbol:
    case token_type_variable:
    case token_type_object:
      free (tp->string);
      break;
    default:
      break;
    }
  if (tp->type == token_type_string)
    drop_reference (tp->comment);
  free (tp);
}

/* Pass 1 of extracting quotes: Find the end of the string, regardless
   of the semantics of the construct.  Return the complete string,
   including the starting and the trailing delimiter, with backslashes
   removed where appropriate.  */
static rw_string_desc_t
extract_quotelike_pass1 (struct perl_extractor *xp, int delim)
{
  struct string_buffer buffer;
  sb_init (&buffer);
  sb_xappend1 (&buffer, delim);

  /* Find the closing delimiter.  */
  bool nested = true;
  int counter_delim;
  switch (delim)
    {
    case '(':
      counter_delim = ')';
      break;
    case '{':
      counter_delim = '}';
      break;
    case '[':
      counter_delim = ']';
      break;
    case '<':
      counter_delim = '>';
      break;
    default: /* "..." or '...' or |...| etc. */
      nested = false;
      counter_delim = delim;
      break;
    }

  for (;;)
    {
      int c = phase1_getc (xp);

      if (c == counter_delim || c == EOF)
        {
          sb_xappend1 (&buffer, counter_delim); /* will be stripped off later */
          #if DEBUG_PERL
          fprintf (stderr, "PASS1: %.*s\n", (int) buffer.length, buffer.data);
          #endif
          return sb_xdupfree (&buffer);
        }

      if (nested && c == delim)
        {
          rw_string_desc_t inner = extract_quotelike_pass1 (xp, delim);
          sb_xappend_desc (&buffer, sd_readonly (inner));
          sd_free (inner);
        }
      else if (c == '\\')
        {
          c = phase1_getc (xp);
          if (c == '\\')
            {
              sb_xappend1 (&buffer, '\\');
              sb_xappend1 (&buffer, '\\');
            }
          else if (c == delim || c == counter_delim)
            {
              /* This is pass2 in Perl.  */
              sb_xappend1 (&buffer, c);
            }
          else
            {
              sb_xappend1 (&buffer, '\\');
              phase1_ungetc (xp, c);
            }
        }
      else
        {
          sb_xappend1 (&buffer, c);
        }
    }
}

/* Like extract_quotelike_pass1, but return the complete string in UTF-8
   encoding.  */
static rw_string_desc_t
extract_quotelike_pass1_utf8 (struct perl_extractor *xp, int delim)
{
  rw_string_desc_t string = extract_quotelike_pass1 (xp, delim);
  rw_string_desc_t utf8_string =
    string_desc_from_current_source_encoding (sd_readonly (string), lc_string,
                                              logical_file_name,
                                              xp->line_number);
  if (sd_data (utf8_string) != sd_data (string))
    sd_free (string);
  return utf8_string;
}


/* ========= Reading of tokens and commands.  Extracting strings.  ========= */


/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;


/* Forward declaration of local functions.  */
static void interpolate_keywords (struct perl_extractor *xp,
                                  string_desc_t string, int lineno);
static token_ty *x_perl_lex (struct perl_extractor *xp);
static void x_perl_unlex (struct perl_extractor *xp, token_ty *tp);
static bool extract_balanced (struct perl_extractor *xp,
                              token_type_ty delim, bool eat_delim,
                              bool semicolon_delim, bool eat_semicolon_delim,
                              bool comma_delim,
                              flag_region_ty *outer_region,
                              flag_context_list_iterator_ty context_iter,
                              int arg, struct arglist_parser *argparser);
static void extract_perl_input (struct perl_extractor *xp);


/* Extract an unsigned hexadecimal number from STRING, considering at
   most LEN bytes and place the result in *RESULT.  Returns a pointer
   to the first character past the hexadecimal number.  */
static const char *
extract_hex (const char *string, size_t len, unsigned int *result)
{
  size_t i;

  *result = 0;

  for (i = 0; i < len; i++)
    {
      char c = string[i];

      int number;
      if (c >= 'A' && c <= 'F')
        number = c - 'A' + 10;
      else if (c >= 'a' && c <= 'f')
        number = c - 'a' + 10;
      else if (c >= '0' && c <= '9')
        number = c - '0';
      else
        break;

      *result <<= 4;
      *result |= number;
    }

  return string + i;
}

/* Extract an unsigned octal number from STRING, considering at
   most LEN bytes and place the result in *RESULT.  Returns a pointer
   to the first character past the octal number.  */
static const char *
extract_oct (const char *string, size_t len, unsigned int *result)
{
  size_t i;

  *result = 0;

  for (i = 0; i < len; i++)
    {
      char c = string[i];

      int number;
      if (c >= '0' && c <= '7')
        number = c - '0';
      else
        break;

      *result <<= 3;
      *result |= number;
    }

  return string + i;
}

/* Extract the various quotelike constructs except for <<EOF.  See the
   section "Gory details of parsing quoted constructs" in perlop.pod.
   Return the resulting token in *tp; tp->type == token_type_string.  */
static void
extract_quotelike (struct perl_extractor *xp, token_ty *tp, int delim)
{
  rw_string_desc_t string = extract_quotelike_pass1_utf8 (xp, delim);
  size_t len = sd_length (string);

  tp->type = token_type_string;
  /* Take the string without the delimiters at the start and at the end.  */
  if (!(len >= 2))
    abort ();
  tp->string = xsd_c (sd_substring (string, 1, len - 1));
  sd_free (string);
  tp->comment = add_reference (savable_comment);
}

/* Extract the quotelike constructs with double delimiters, like
   s/[SEARCH]/[REPLACE]/.  This function does not eat up trailing
   modifiers (left to the caller).
   Return the resulting token in *tp; tp->type == token_type_regex_op.  */
static void
extract_triple_quotelike (struct perl_extractor *xp, token_ty *tp, int delim,
                          bool interpolate)
{
  tp->type = token_type_regex_op;

  {
    rw_string_desc_t string = extract_quotelike_pass1_utf8 (xp, delim);
    if (interpolate)
      interpolate_keywords (xp, sd_readonly (string), xp->line_number);
    sd_free (string);
  }

  if (delim == '(' || delim == '<' || delim == '{' || delim == '[')
    {
      /* The delimiter for the second string can be different, e.g.
         s{SEARCH}{REPLACE} or s{SEARCH}/REPLACE/.  See "man perlrequick".  */
      delim = phase1_getc (xp);
      while (is_whitespace (delim))
        {
          /* The hash-sign is not a valid delimiter after whitespace, ergo
             use phase2_getc() and not phase1_getc() now.  */
          delim = phase2_getc (xp);
        }
    }

  {
    rw_string_desc_t string = extract_quotelike_pass1_utf8 (xp, delim);
    if (interpolate)
      interpolate_keywords (xp, sd_readonly (string), xp->line_number);
    sd_free (string);
  }
}

/* Perform pass 3 of quotelike extraction (interpolation).
   *tp is a token of type token_type_string.
   This function may either replace tp->string, or change *tp's type to
   token_type_string_interpol.
   This function does not access tp->comment.  */
/* FIXME: Currently may writes null-bytes into the string.  */
static void
extract_quotelike_pass3 (struct perl_extractor *xp, token_ty *tp)
{
  #if DEBUG_PERL
  switch (tp->sub_type)
    {
    case string_type_verbatim:
      fprintf (stderr, "Interpolating string_type_verbatim:\n");
      break;
    case string_type_q:
      fprintf (stderr, "Interpolating string_type_q:\n");
      break;
    case string_type_qq:
      fprintf (stderr, "Interpolating string_type_qq:\n");
      break;
    case string_type_qr:
      fprintf (stderr, "Interpolating string_type_qr:\n");
      break;
    }
  fprintf (stderr, "%s\n", tp->string);
  if (tp->sub_type == string_type_verbatim)
    fprintf (stderr, "---> %s\n", tp->string);
  #endif

  if (tp->sub_type == string_type_verbatim)
    return;

  struct string_buffer buffer;
  sb_init (&buffer);

  /* Loop over tp->string, accumulating the expansion in buffer.  */
  const char *crs = tp->string;
  bool uppercase = false;
  bool lowercase = false;
  bool quotemeta = false;
  while (*crs)
    {
      if (tp->sub_type == string_type_q)
        {
          switch (*crs)
            {
            case '\\':
              if (crs[1] == '\\')
                {
                  crs += 2;
                  sb_xappend1 (&buffer, '\\');
                  break;
                }
              FALLTHROUGH;
            default:
              sb_xappend1 (&buffer, *crs++);
              break;
            }
          continue;
        }

      /* We only get here for double-quoted strings or regular expressions.
         Unescape escape sequences.  */
      if (*crs == '\\')
        {
          switch (crs[1])
            {
            case 't':
              crs += 2;
              sb_xappend1 (&buffer, '\t');
              continue;
            case 'n':
              crs += 2;
              sb_xappend1 (&buffer, '\n');
              continue;
            case 'r':
              crs += 2;
              sb_xappend1 (&buffer, '\r');
              continue;
            case 'f':
              crs += 2;
              sb_xappend1 (&buffer, '\f');
              continue;
            case 'b':
              crs += 2;
              sb_xappend1 (&buffer, '\b');
              continue;
            case 'a':
              crs += 2;
              sb_xappend1 (&buffer, '\a');
              continue;
            case 'e':
              crs += 2;
              sb_xappend1 (&buffer, 0x1b);
              continue;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
              {
                unsigned int oct_number;
                crs = extract_oct (crs + 1, 3, &oct_number);

                /* FIXME: If one of the variables UPPERCASE or LOWERCASE is
                   true, the character should be converted to its uppercase
                   resp. lowercase equivalent.  I don't know if the necessary
                   facilities are already included in gettext.  For US-Ascii
                   the conversion can be already be done, however.  */
                if (uppercase && oct_number >= 'a' && oct_number <= 'z')
                  {
                    oct_number = oct_number - 'a' + 'A';
                  }
                else if (lowercase && oct_number >= 'A' && oct_number <= 'Z')
                  {
                    oct_number = oct_number - 'A' + 'a';
                  }

                /* Yes, octal escape sequences in the range 0x100..0x1ff are
                   valid.  */
                char tmpbuf[2];
                int length =
                  u8_uctomb ((unsigned char *) tmpbuf, oct_number, 2);
                if (length > 0)
                  sb_xappend_desc (&buffer,
                                   sd_new_addr (length, (const char *) tmpbuf));
              }
              continue;
            case 'x':
              {
                crs += 2;

                unsigned int hex_number = 0;
                if (*crs == '{')
                  {
                    const char *end = strchr (crs, '}');
                    if (end == NULL)
                      {
                        if_error (IF_SEVERITY_WARNING,
                                  real_file_name, xp->line_number, (size_t)(-1), false,
                                  _("missing right brace on \\x{HEXNUMBER}"));
                        ++crs;
                        continue;
                      }
                    else
                      {
                        ++crs;
                        (void) extract_hex (crs, end - crs, &hex_number);
                        crs = end + 1;
                      }
                  }
                else
                  {
                    crs = extract_hex (crs, 2, &hex_number);
                  }

                /* FIXME: If one of the variables UPPERCASE or LOWERCASE is
                   true, the character should be converted to its uppercase
                   resp. lowercase equivalent.  I don't know if the necessary
                   facilities are already included in gettext.  For US-Ascii
                   the conversion can be already be done, however.  */
                if (uppercase && hex_number >= 'a' && hex_number <= 'z')
                  {
                    hex_number = hex_number - 'a' + 'A';
                  }
                else if (lowercase && hex_number >= 'A' && hex_number <= 'Z')
                  {
                    hex_number = hex_number - 'A' + 'a';
                  }

                char tmpbuf[6];
                int length =
                  u8_uctomb ((unsigned char *) tmpbuf, hex_number, 6);
                if (length > 0)
                  sb_xappend_desc (&buffer,
                                   sd_new_addr (length, (const char *) tmpbuf));
              }
              continue;
            case 'c':
              /* Perl's notion of control characters.  */
              crs += 2;
              if (*crs)
                {
                  int the_char = (unsigned char) *crs;
                  if (the_char >= 'a' && the_char <= 'z')
                    the_char = the_char - 'a' + 'A';
                  sb_xappend1 (&buffer, the_char ^ 0x40);
                }
              continue;
            case 'N':
              crs += 2;
              if (*crs == '{')
                {
                  const char *end = strchr (crs + 1, '}');
                  if (end != NULL)
                    {
                      char *name = XNMALLOC (end - (crs + 1) + 1, char);
                      memcpy (name, crs + 1, end - (crs + 1));
                      name[end - (crs + 1)] = '\0';

                      unsigned int unicode = unicode_name_character (name);
                      if (unicode != UNINAME_INVALID)
                        {
                          /* FIXME: Convert to upper/lowercase if the
                             corresponding flag is set to true.  */
                          char tmpbuf[6];
                          int length =
                            u8_uctomb ((unsigned char *) tmpbuf, unicode, 6);
                          if (length > 0)
                            sb_xappend_desc (&buffer,
                                             sd_new_addr (length, (const char *) tmpbuf));
                        }

                      free (name);

                      crs = end + 1;
                    }
                }
              continue;
            }
        }

      /* No escape sequence, go on.  */
      bool backslashed;
      if (*crs == '\\')
        {
          ++crs;
          switch (*crs)
            {
            case 'E':
              uppercase = false;
              lowercase = false;
              quotemeta = false;
              ++crs;
              continue;
            case 'L':
              uppercase = false;
              lowercase = true;
              ++crs;
              continue;
            case 'U':
              uppercase = true;
              lowercase = false;
              ++crs;
              continue;
            case 'Q':
              quotemeta = true;
              ++crs;
              continue;
            case 'l':
              ++crs;
              if (*crs >= 'A' && *crs <= 'Z')
                {
                  sb_xappend1 (&buffer, *crs - 'A' + 'a');
                }
              else if ((unsigned char) *crs >= 0x80)
                {
                  if_error (IF_SEVERITY_WARNING,
                            real_file_name, xp->line_number, (size_t)(-1), false,
                            _("unsupported interpolation (\"\\l\") of 8bit character \"%c\""),
                            *crs);
                }
              else
                {
                  sb_xappend1 (&buffer, *crs);
                }
              ++crs;
              continue;
            case 'u':
              ++crs;
              if (*crs >= 'a' && *crs <= 'z')
                {
                  sb_xappend1 (&buffer, *crs - 'a' + 'A');
                }
              else if ((unsigned char) *crs >= 0x80)
                {
                  if_error (IF_SEVERITY_WARNING,
                            real_file_name, xp->line_number, (size_t)(-1), false,
                            _("unsupported interpolation (\"\\u\") of 8bit character \"%c\""),
                            *crs);
                }
              else
                {
                  sb_xappend1 (&buffer, *crs);
                }
              ++crs;
              continue;
            case '\\':
              sb_xappend1 (&buffer, *crs);
              ++crs;
              continue;
            default:
              backslashed = true;
              break;
            }
        }
      else
        backslashed = false;

      if (quotemeta
          && !((*crs >= 'A' && *crs <= 'Z') || (*crs >= 'A' && *crs <= 'z')
               || (*crs >= '0' && *crs <= '9') || *crs == '_'))
        {
          sb_xappend1 (&buffer, '\\');
          backslashed = true;
        }

      if (!backslashed && !extract_all && (*crs == '$' || *crs == '@'))
        {
          if_error (IF_SEVERITY_WARNING,
                    real_file_name, xp->line_number, (size_t)(-1), false,
                    _("unsupported variable interpolation at \"%c\""), *crs);
          tp->type = token_type_string_interpol;
          ++crs;
        }
      else if (lowercase)
        {
          if (*crs >= 'A' && *crs <= 'Z')
            sb_xappend1 (&buffer, *crs - 'A' + 'a');
          else if ((unsigned char) *crs >= 0x80)
            {
              if_error (IF_SEVERITY_WARNING,
                        real_file_name, xp->line_number, (size_t)(-1), false,
                        _("unsupported interpolation (\"\\L\") of 8bit character \"%c\""),
                        *crs);
              sb_xappend1 (&buffer, *crs);
            }
          else
            sb_xappend1 (&buffer, *crs);
          ++crs;
        }
      else if (uppercase)
        {
          if (*crs >= 'a' && *crs <= 'z')
            sb_xappend1 (&buffer, *crs - 'a' + 'A');
          else if ((unsigned char) *crs >= 0x80)
            {
              if_error (IF_SEVERITY_WARNING,
                        real_file_name, xp->line_number, (size_t)(-1), false,
                        _("unsupported interpolation (\"\\U\") of 8bit character \"%c\""),
                        *crs);
              sb_xappend1 (&buffer, *crs);
            }
          else
            sb_xappend1 (&buffer, *crs);
          ++crs;
        }
      else
        {
          sb_xappend1 (&buffer, *crs++);
        }
    }

  #if DEBUG_PERL
  fprintf (stderr, "---> %s\n", buffer);
  #endif

  /* Replace tp->string.  */
  free (tp->string);
  if (tp->type == token_type_string)
    tp->string = sb_xdupfree_c (&buffer);
  else
    sb_free (&buffer);
}

/* Parse a variable.  This is done in several steps:
     1) Consume all leading occurencies of '$', '@', '%', and '*'.
     2) Determine the name of the variable from the following input.
     3) Parse possible following hash keys or array indexes.
 */
static void
extract_variable (struct perl_extractor *xp, token_ty *tp, int first)
{
  struct string_buffer buffer;
  sb_init (&buffer);

  tp->type = token_type_variable;

  #if DEBUG_PERL
  fprintf (stderr, "%s:%d: extracting variable type '%c'\n",
           real_file_name, xp->line_number, first);
  #endif

  size_t varbody_length = 0;
  bool maybe_hash_deref = false;
  bool maybe_hash_value = false;

  /*
   * 1) Consume dollars and so on (not euros ...).  Unconditionally
   *    accepting the hash sign (#) will maybe lead to inaccurate
   *    results.  FIXME!
   */
  {
    int c = first;

    while (c == '$' || c == '*' || c == '#' || c == '@' || c == '%')
      {
        sb_xappend1 (&buffer, c);
        c = phase1_getc (xp);
      }

    if (c == EOF)
      {
        sb_free (&buffer);
        tp->type = token_type_eof;
        return;
      }

    /* Hash references are treated in a special way, when looking for
       our keywords.  */
    string_desc_t contents = sb_contents (&buffer);
    if (sd_char_at (contents, 0) == '$')
      {
        if (sd_length (contents) == 1)
          maybe_hash_value = true;
        else if (sd_length (contents) == 2
                 && sd_char_at (contents, 1) == '$')
          {
            if (!(c == '{'
                  || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                  || (c >= '0' && c <= '9')
                  || c == '_' || c == ':' || c == '\'' || c >= 0x80))
              {
                /* Special variable $$ for pid.  */
                tp->string = sb_xdupfree_c (&buffer);
                #if DEBUG_PERL
                fprintf (stderr, "%s:%d: is PID ($$)\n",
                         real_file_name, xp->line_number);
                #endif

                phase1_ungetc (xp, c);
                return;
              }

            maybe_hash_deref = true;
            /* Truncate to length 1.  */
            buffer.length = 1;
          }
      }

    /*
     * 2) Get the name of the variable.  The first character is practically
     *    arbitrary.  Punctuation and numbers automagically put a variable
     *    in the global namespace but that subtle difference is not interesting
     *    for us.
     */
    if (c == '{')
      {
        /* Yuck, we cannot accept ${gettext} as a keyword...  Except for
         * debugging purposes it is also harmless, that we suppress the
         * real name of the variable.
         */
        #if DEBUG_PERL
        fprintf (stderr, "%s:%d: braced {variable_name}\n",
                 real_file_name, xp->line_number);
        #endif

        if (extract_balanced (xp,
                              token_type_rbrace, true,
                              false, false, false,
                              null_context_region (), null_context_list_iterator,
                              1, arglist_parser_alloc (xp->mlp, NULL)))
          {
            sb_free (&buffer);
            tp->type = token_type_eof;
            return;
          }
        sb_xappend1 (&buffer, c);
        ++varbody_length;
        sb_xappend1 (&buffer, '}');
      }
    else
      {
        while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
               || (c >= '0' && c <= '9')
               || c == '_' || c == ':' || c == '\'' || c >= 0x80)
          {
            ++varbody_length;
            sb_xappend1 (&buffer, c);
            c = phase1_getc (xp);
          }
        phase1_ungetc (xp, c);
      }
  }

  /* Probably some strange Perl variable like $`.  */
  if (varbody_length == 0)
    {
      int c = phase1_getc (xp);
      if (c == EOF || is_whitespace (c))
        phase1_ungetc (xp, c);  /* Loser.  */
      else
        sb_xappend1 (&buffer, c);
    }

  tp->string = sb_xdupfree_c (&buffer);

  #if DEBUG_PERL
  fprintf (stderr, "%s:%d: complete variable name: %s\n",
           real_file_name, xp->line_number, tp->string);
  #endif

  /*
   * 3) If the following looks strange to you, this is valid Perl syntax:
   *
   *      $var = $$hashref    # We can place a
   *                          # comment here and then ...
   *             {key_into_hashref};
   *
   *    POD sections are not allowed but we leave complaints about
   *    that to the compiler/interpreter.
   */
  /* We only extract strings from the first hash key (if present).  */

  if (maybe_hash_deref || maybe_hash_value)
    {
      int c;

      do
        c = phase2_getc (xp);
      while (is_whitespace (c));

      bool is_dereference = false;
      if (c == '-')
        {
          int c2 = phase1_getc (xp);

          if (c2 == '>')
            {
              is_dereference = true;

              do
                c = phase2_getc (xp);
              while (is_whitespace (c));
            }
          else if (c2 != '\n')
            {
              /* Discarding the newline is harmless here.  The only
                 special character recognized after a minus is greater-than
                 for dereference.  However, the sequence "-\n>" that we
                 treat incorrectly here, is a syntax error.  */
              phase1_ungetc (xp, c2);
            }
        }

      if (maybe_hash_value && is_dereference)
        {
          tp->type = token_type_object;
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: first keys preceded by \"->\"\n",
                   real_file_name, xp->line_number);
          #endif
        }
      else if (maybe_hash_value)
        {
          /* Fake it into a hash.  */
          tp->string[0] = '%';
        }

      /* Do NOT change that into else if (see above).  */
      if ((maybe_hash_value || maybe_hash_deref) && c == '{')
        {
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: first keys preceded by '{'\n",
                   real_file_name, xp->line_number);
          #endif

          void *keyword_value;
          if (hash_find_entry (&keywords, tp->string, strlen (tp->string),
                               &keyword_value) == 0)
            {
              /* TODO: Shouldn't we use the shapes of the keyword, instead
                 of hardwiring argnum1 = 1 ?
              const struct callshapes *shapes =
                (const struct callshapes *) keyword_value;
              */
              struct callshapes shapes;
              shapes.keyword = tp->string; /* XXX storage duration? */
              shapes.keyword_len = strlen (tp->string);
              shapes.nshapes = 1;
              shapes.shapes[0].argnum1 = 1;
              shapes.shapes[0].argnum2 = 0;
              shapes.shapes[0].argnumc = 0;
              shapes.shapes[0].argnum1_glib_context = false;
              shapes.shapes[0].argnum2_glib_context = false;
              shapes.shapes[0].argtotal = 0;
              string_list_init (&shapes.shapes[0].xcomments);

              {
                /* Extract a possible string from the key.  Before proceeding
                   we check whether the open curly is followed by a symbol and
                   then by a right curly.  */
                flag_context_list_iterator_ty context_iter =
                  flag_context_list_iterator (
                    flag_context_list_table_lookup (
                      flag_context_list_table,
                      tp->string, strlen (tp->string)));
                token_ty *t1 = x_perl_lex (xp);

                #if DEBUG_PERL
                fprintf (stderr, "%s:%d: extracting string key\n",
                         real_file_name, xp->line_number);
                #endif

                if (t1->type == token_type_symbol
                    || t1->type == token_type_named_op)
                  {
                    token_ty *t2 = x_perl_lex (xp);
                    if (t2->type == token_type_rbrace)
                      {
                        flag_region_ty *region =
                          inheriting_region (null_context_region (),
                                             flag_context_list_iterator_advance (
                                               &context_iter));

                        lex_pos_ty pos;
                        pos.line_number = xp->line_number;
                        pos.file_name = logical_file_name;

                        remember_a_message (xp->mlp, NULL, xstrdup (t1->string),
                                            true, false, region, &pos,
                                            NULL, savable_comment, true);
                        free_token (t2);
                        free_token (t1);
                      }
                    else
                      {
                        x_perl_unlex (xp, t2);
                      }
                  }
                else
                  {
                    x_perl_unlex (xp, t1);
                    if (extract_balanced (xp,
                                          token_type_rbrace, true,
                                          false, false, false,
                                          null_context_region (), context_iter,
                                          1, arglist_parser_alloc (xp->mlp, &shapes)))
                      return;
                  }
              }
            }
          else
            {
              phase2_ungetc (xp, c);
            }
        }
      else
        {
          phase2_ungetc (xp, c);
        }
    }

  /* Now consume "->", "[...]", and "{...}".  */
  for (;;)
    {
      int c = phase2_getc (xp);

      switch (c)
        {
        case '{':
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: extracting balanced '{' after varname\n",
                   real_file_name, xp->line_number);
          #endif
          extract_balanced (xp,
                            token_type_rbrace, true,
                            false, false, false,
                            null_context_region (), null_context_list_iterator,
                            1, arglist_parser_alloc (xp->mlp, NULL));
          break;

        case '[':
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: extracting balanced '[' after varname\n",
                   real_file_name, xp->line_number);
          #endif
          extract_balanced (xp,
                            token_type_rbracket, true,
                            false, false, false,
                            null_context_region (), null_context_list_iterator,
                            1, arglist_parser_alloc (xp->mlp, NULL));
          break;

        case '-':
          {
            int c2 = phase1_getc (xp);
            if (c2 == '>')
              {
                #if DEBUG_PERL
                fprintf (stderr, "%s:%d: another \"->\" after varname\n",
                         real_file_name, xp->line_number);
                #endif
                break;
              }
            else if (c2 != '\n')
              {
                /* Discarding the newline is harmless here.  The only
                   special character recognized after a minus is greater-than
                   for dereference.  However, the sequence "-\n>" that we
                   treat incorrectly here, is a syntax error.  */
                phase1_ungetc (xp, c2);
              }
          }
          FALLTHROUGH;

        default:
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: variable finished\n",
                   real_file_name, xp->line_number);
          #endif
          phase2_ungetc (xp, c);
          return;
        }
    }
}

/* Actually a simplified version of extract_variable().  It searches for
   variables inside a double-quoted string that may interpolate to
   some keyword hash (reference).  The string is UTF-8 encoded.  */
static void
interpolate_keywords (struct perl_extractor *xp, string_desc_t string,
                      int lineno)
{
  if (++(xp->nesting_depth) > MAX_NESTING_DEPTH)
    if_error (IF_SEVERITY_FATAL_ERROR,
              logical_file_name, xp->line_number, (size_t)(-1), false,
              _("too deeply nested expressions"));

  struct string_buffer buffer;
  sb_init (&buffer);

  bool maybe_hash_deref = false;

  /* States are:
   *
   * initial:           initial
   * one_dollar:        dollar sign seen in state INITIAL
   * two_dollars:       another dollar-sign has been seen in state ONE_DOLLAR
   * identifier:        a valid identifier character has been seen in state
   *                    ONE_DOLLAR or TWO_DOLLARS
   *
   * seen_lbracket:     a left bracket has been seen in state IDENTIFIER
   * lbracket_dquote:   a double-quote has been seen in state SEEN_LBRACKET
   * lbracket_squote:   a single-quote has been seen in state SEEN_LBRACKET
   *
   * minus:             a minus-sign has been seen in state IDENTIFIER
   * wait_lbrace:       a greater-than has been seen in state MINUS
   * seen_lbrace:       a left brace has been seen in state IDENTIFIER or in
   *                    state WAIT_LBRACE
   * lbrace_dquote:     a double-quote has been seen in state SEEN_LBRACE
   * lbrace_squote:     a single-quote has been seen in state SEEN_LBRACE
   * lbrace_barekey:    a bareword character has been seen in state SEEN_LBRACE
   * wait_rbrace:       closing quote has been seen in state LBRACE_DQUOTE or
   *                    LBRACE_SQUOTE
   *
   * In the states initial...wait_rbracket the context is null_context_region ();
   * in the states minus...wait_rbrace the context is the one suitable for the
   * first argument of the last seen identifier.
   */
  enum parser_state
    {
      initial,
      one_dollar,
      two_dollars,
      identifier,
      seen_lbracket,
      lbracket_dquote,
      lbracket_squote,
      minus,
      wait_lbrace,
      seen_lbrace,
      lbrace_dquote,
      lbrace_squote,
      lbrace_barekey,
      wait_rbrace
    }
    state = initial;

  flag_region_ty *region = null_context_region ();

  size_t length = sd_length (string);
  size_t index = 0;

  token_ty token;
  token.type = token_type_string;
  token.sub_type = string_type_qq;
  token.line_number = xp->line_number;
  /* No need for  token.comment = add_reference (savable_comment);  here.
     We can let token.comment uninitialized here, and use savable_comment
     directly, because this function only parses the given string and does
     not call phase2_getc.  */
  lex_pos_ty pos;
  pos.file_name = logical_file_name;
  pos.line_number = lineno;

  while (index < length)
    {
      char c;

      c = sd_char_at (string, index++);
      if (state == initial)
        buffer.length = 0;

      if (c == '\n')
        lineno++;

      switch (state)
        {
        case initial:
          switch (c)
            {
            case '\\':
              if (index == length)
                {
                  xp->nesting_depth--;
                  sb_free (&buffer);
                  return;
                }
              c = sd_char_at (string, index++);
              break;
            case '$':
              sb_xappend1 (&buffer, '$');
              maybe_hash_deref = false;
              state = one_dollar;
              break;
            default:
              break;
            }
          break;
        case one_dollar:
          switch (c)
            {
            case '$':
              /*
               * This is enough to make us believe later that we dereference
               * a hash reference.
               */
              maybe_hash_deref = true;
              state = two_dollars;
              break;
            default:
              if (!c_isascii ((unsigned char) c)
                  || c == '_' || c == ':' || c == '\''
                  || (c >= 'A' && c <= 'Z')
                  || (c >= 'a' && c <= 'z')
                  || (c >= '0' && c <= '9'))
                {
                  sb_xappend1 (&buffer, c);
                  state = identifier;
                }
              else
                state = initial;
              break;
            }
          break;
        case two_dollars:
          if (!c_isascii ((unsigned char) c)
              || c == '_' || c == ':' || c == '\''
              || (c >= 'A' && c <= 'Z')
              || (c >= 'a' && c <= 'z')
              || (c >= '0' && c <= '9'))
            {
              sb_xappend1 (&buffer, c);
              state = identifier;
            }
          else
            state = initial;
          break;
        case identifier:
          switch (c)
            {
            case '-':
              {
                string_desc_t contents = sb_contents (&buffer);
                void *keyword_value;
                if (hash_find_entry (&keywords,
                                     sd_data (contents),
                                     sd_length (contents),
                                     &keyword_value)
                    == 0)
                  {
                    flag_context_list_iterator_ty context_iter =
                      flag_context_list_iterator (
                        flag_context_list_table_lookup (
                          flag_context_list_table,
                          sd_data (contents),
                          sd_length (contents)));
                    region =
                      inheriting_region (null_context_region (),
                                         flag_context_list_iterator_advance (
                                           &context_iter));
                    state = minus;
                  }
                else
                  state = initial;
              }
              break;
            case '[':
              buffer.length = 0;
              state = seen_lbracket;
              break;
            case '{':
              {
                string_desc_t contents = sb_contents (&buffer);
                if (!maybe_hash_deref)
                  ((char *) sd_data (contents))[0] = '%';
                void *keyword_value;
                if (hash_find_entry (&keywords,
                                     sd_data (contents),
                                     sd_length (contents),
                                     &keyword_value)
                    == 0)
                  {
                    flag_context_list_iterator_ty context_iter =
                      flag_context_list_iterator (
                        flag_context_list_table_lookup (
                          flag_context_list_table,
                          sd_data (contents),
                          sd_length (contents)));
                    region =
                      inheriting_region (null_context_region (),
                                         flag_context_list_iterator_advance (
                                           &context_iter));
                    state = seen_lbrace;
                  }
                else
                  state = initial;
              }
              break;
            default:
              if (!c_isascii ((unsigned char) c)
                  || c == '_' || c == ':' || c == '\''
                  || (c >= 'A' && c <= 'Z')
                  || (c >= 'a' && c <= 'z')
                  || (c >= '0' && c <= '9'))
                {
                  sb_xappend1 (&buffer, c);
                }
              else
                state = initial;
              break;
            }
          break;
        case seen_lbracket:
          switch (c)
            {
            case '\'':
              sb_xappend1 (&buffer, c);
              state = lbracket_squote;
              break;
            case '"':
              sb_xappend1 (&buffer, c);
              state = lbracket_dquote;
              break;
            case ']':
              /* Recursively extract messages from the bracketed expression.  */
              {
                string_desc_t substring = sb_contents (&buffer);

                struct perl_extractor *rxp = XMALLOC (struct perl_extractor);
                rxp->mlp = xp->mlp;
                sf_istream_init_from_string_desc (&rxp->input, substring);
                rxp->line_number = xp->line_number;
                perl_extractor_init_rest (rxp);

                extract_perl_input (rxp);

                free (rxp);
              }
              break;
            default:
              sb_xappend1 (&buffer, c);
              break;
            }
          break;
        case lbracket_dquote:
          switch (c)
            {
            case '"':
              sb_xappend1 (&buffer, c);
              state = seen_lbracket;
              break;
            case '\\':
              if (index == length)
                {
                  region = null_context_region ();
                  state = initial;
                }
              else
                {
                  c = sd_char_at (string, index++);
                  if (c == '\"')
                    {
                      sb_xappend1 (&buffer, c);
                    }
                  else
                    {
                      sb_xappend1 (&buffer, '\\');
                      sb_xappend1 (&buffer, c);
                    }
                }
              break;
            default:
              sb_xappend1 (&buffer, c);
              break;
            }
          break;
        case lbracket_squote:
          switch (c)
            {
            case '\'':
              sb_xappend1 (&buffer, c);
              state = seen_lbracket;
              break;
            case '\\':
              if (index == length)
                {
                  region = null_context_region ();
                  state = initial;
                }
              else
                {
                  c = sd_char_at (string, index++);
                  if (c == '\'')
                    {
                      sb_xappend1 (&buffer, c);
                    }
                  else
                    {
                      sb_xappend1 (&buffer, '\\');
                      sb_xappend1 (&buffer, c);
                    }
                }
              break;
            default:
              sb_xappend1 (&buffer, c);
              break;
            }
          break;
        case minus:
          switch (c)
            {
            case '>':
              state = wait_lbrace;
              break;
            default:
              region = null_context_region ();
              state = initial;
              break;
            }
          break;
        case wait_lbrace:
          switch (c)
            {
            case '{':
              state = seen_lbrace;
              break;
            default:
              region = null_context_region ();
              state = initial;
              break;
            }
          break;
        case seen_lbrace:
          switch (c)
            {
            case_whitespace:
              break;
            case '\'':
              pos.line_number = lineno;
              buffer.length = 0;
              state = lbrace_squote;
              break;
            case '"':
              pos.line_number = lineno;
              buffer.length = 0;
              state = lbrace_dquote;
              break;
            default:
              if (!c_isascii ((unsigned char) c)
                  || c == '_' || (c >= '0' && c <= '9')
                  || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                {
                  pos.line_number = lineno;
                  buffer.length = 0;
                  sb_xappend1 (&buffer, c);
                  state = lbrace_barekey;
                }
              else
                {
                  region = null_context_region ();
                  state = initial;
                }
              break;
            }
          break;
        case lbrace_dquote:
          switch (c)
            {
            case '"':
              /* The resulting string has to be interpolated twice.  */
              token.string = sb_xdupfree_c (&buffer);
              sb_init (&buffer);
              extract_quotelike_pass3 (xp, &token);
              if (token.type == token_type_string)
                {
                  sb_xappend_c (&buffer, token.string);
                  free (token.string);
                }
              state = wait_rbrace;
              break;
            case '\\':
              if (index == length)
                {
                  region = null_context_region ();
                  state = initial;
                }
              else
                {
                  c = sd_char_at (string, index++);
                  if (c == '\"')
                    {
                      sb_xappend1 (&buffer, c);
                    }
                  else
                    {
                      sb_xappend1 (&buffer, '\\');
                      sb_xappend1 (&buffer, c);
                    }
                }
              break;
            default:
              sb_xappend1 (&buffer, c);
              break;
            }
          break;
        case lbrace_squote:
          switch (c)
            {
            case '\'':
              state = wait_rbrace;
              break;
            case '\\':
              if (index == length)
                {
                  region = null_context_region ();
                  state = initial;
                }
              else
                {
                  c = sd_char_at (string, index++);
                  if (c == '\'')
                    {
                      sb_xappend1 (&buffer, c);
                    }
                  else
                    {
                      sb_xappend1 (&buffer, '\\');
                      sb_xappend1 (&buffer, c);
                    }
                }
              break;
            default:
              sb_xappend1 (&buffer, c);
              break;
            }
          break;
        case lbrace_barekey:
          if (!c_isascii ((unsigned char) c)
              || c == '_' || (c >= '0' && c <= '9')
              || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            {
              sb_xappend1 (&buffer, c);
              break;
            }
          else if (is_whitespace (c))
            {
              state = wait_rbrace;
              break;
            }
          else if (c != '}')
            {
              region = null_context_region ();
              state = initial;
              break;
            }
          /* Must be right brace.  */
          FALLTHROUGH;
        case wait_rbrace:
          switch (c)
            {
            case_whitespace:
              break;
            case '}':
              if (token.type == token_type_string)
                {
                  token.string = sb_xdupfree_c (&buffer);
                  extract_quotelike_pass3 (xp, &token);
                  if (token.type == token_type_string)
                    {
                      remember_a_message (xp->mlp, NULL, token.string, true,
                                          false, region, &pos, NULL,
                                          savable_comment, true);
                    }
                  sb_init (&buffer);
                }
              FALLTHROUGH;
            default:
              region = null_context_region ();
              state = initial;
              break;
            }
          break;
        }
    }

  xp->nesting_depth--;
  sb_free (&buffer);
  return;
}

/* There is an ambiguity about '/' and '?': They can start an operator
   (division operator '/' or '/=' or the conditional operator '?'), or they can
   start a regular expression.  The distinction is important because inside
   regular expressions, '#' loses its special meaning.  This function helps
   making the decision (a heuristic).  See the documentation for details.  */
static bool
prefer_regexp_over_division (token_type_ty type)
{
  bool retval = true;

  switch (type)
    {
      case token_type_eof:
        retval = true;
        break;
      case token_type_lparen:
        retval = true;
        break;
      case token_type_rparen:
        retval = false;
        break;
      case token_type_comma:
        retval = true;
        break;
      case token_type_fat_comma:
        retval = true;
        break;
      case token_type_dereference:
        retval = true;
        break;
      case token_type_semicolon:
        retval = true;
        break;
      case token_type_lbrace:
        retval = true;
        break;
      case token_type_rbrace:
        retval = false;
        break;
      case token_type_lbracket:
        retval = true;
        break;
      case token_type_rbracket:
        retval = false;
        break;
      case token_type_string:
      case token_type_string_interpol:
        retval = false;
        break;
      case token_type_number:
        retval = false;
        break;
      case token_type_named_op:
        retval = true;
        break;
      case token_type_variable:
        retval = false;
        break;
      case token_type_object:
        retval = false;
        break;
      case token_type_symbol:
      case token_type_keyword_symbol:
        retval = true;
        break;
      case token_type_regex_op:
        retval = false;
        break;
      case token_type_dot:
        retval = true;
        break;
      case token_type_other:
        retval = true;
        break;
      case token_type_r_any:
        retval = false;
        break;
  }

  #if DEBUG_PERL
  token_ty ty;
  ty.type = type;
  fprintf (stderr, "Prefer regexp over division after %s: %s\n",
           token2string (&ty), retval ? "true" : "false");
  #endif

  return retval;
}

/* Combine characters into tokens.  Discard whitespace.  */

static void
x_perl_prelex (struct perl_extractor *xp, token_ty *tp)
{
  for (;;)
    {
      int c;

      c = phase2_getc (xp);
      tp->line_number = xp->line_number;
      tp->last_type = xp->last_token_type;

      switch (c)
        {
        case EOF:
          tp->type = token_type_eof;
          return;

        case '\n':
          if (xp->last_non_comment_line > xp->last_comment_line)
            savable_comment_reset ();
          FALLTHROUGH;
        case '\t':
        case ' ':
          /* Ignore whitespace.  */
          continue;

        case '%':
        case '@':
        case '*':
        case '$':
          if (!extract_all)
            {
              extract_variable (xp, tp, c);
              return;
            }
          break;
        }

      xp->last_non_comment_line = tp->line_number;

      switch (c)
        {
        case '.':
          {
            int c2 = phase1_getc (xp);
            phase1_ungetc (xp, c2);
            if (c2 == '.')
              {
                tp->type = token_type_other;
                return;
              }
            else if (!(c2 >= '0' && c2 <= '9'))
              {
                tp->type = token_type_dot;
                return;
              }
          }
          FALLTHROUGH;
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
          /* Symbol, or part of a number.  */
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
                    continue;

                  default:
                    phase1_ungetc (xp, c);
                    break;
                  }
                break;
              }
            const char *contents = sb_xcontents_c (&buffer);

            if (strcmp (contents, "__END__") == 0
                || strcmp (contents, "__DATA__") == 0)
              {
                sb_free (&buffer);
                xp->end_of_file = true;
                tp->type = token_type_eof;
                return;
              }
            else if (strcmp (contents, "and") == 0
                     || strcmp (contents, "cmp") == 0
                     || strcmp (contents, "eq") == 0
                     || strcmp (contents, "if") == 0
                     || strcmp (contents, "ge") == 0
                     || strcmp (contents, "gt") == 0
                     || strcmp (contents, "le") == 0
                     || strcmp (contents, "lt") == 0
                     || strcmp (contents, "ne") == 0
                     || strcmp (contents, "not") == 0
                     || strcmp (contents, "or") == 0
                     || strcmp (contents, "unless") == 0
                     || strcmp (contents, "while") == 0
                     || strcmp (contents, "xor") == 0)
              {
                tp->type = token_type_named_op;
                tp->string = sb_xdupfree_c (&buffer);
                return;
              }
            else if (strcmp (contents, "s") == 0
                   || strcmp (contents, "y") == 0
                   || strcmp (contents, "tr") == 0)
              {
                int delim = phase1_getc (xp);

                while (is_whitespace (delim))
                  delim = phase2_getc (xp);

                if (delim == EOF)
                  {
                    sb_free (&buffer);
                    tp->type = token_type_eof;
                    return;
                  }
                if ((delim >= '0' && delim <= '9')
                    || (delim >= 'A' && delim <= 'Z')
                    || (delim >= 'a' && delim <= 'z'))
                  {
                    /* False positive.  */
                    phase2_ungetc (xp, delim);
                    tp->type = token_type_symbol;
                    tp->sub_type = symbol_type_none;
                    tp->string = sb_xdupfree_c (&buffer);
                    return;
                  }
                extract_triple_quotelike (xp, tp, delim,
                                          contents[0] == 's' && delim != '\'');
                sb_free (&buffer);

                /* Eat the following modifiers.  */
                do
                  c = phase1_getc (xp);
                while (c >= 'a' && c <= 'z');
                phase1_ungetc (xp, c);
                return;
              }
            else if (strcmp (contents, "m") == 0)
              {
                int delim = phase1_getc (xp);

                while (is_whitespace (delim))
                  delim = phase2_getc (xp);

                if (delim == EOF)
                  {
                    sb_free (&buffer);
                    tp->type = token_type_eof;
                    return;
                  }
                if ((delim >= '0' && delim <= '9')
                    || (delim >= 'A' && delim <= 'Z')
                    || (delim >= 'a' && delim <= 'z'))
                  {
                    /* False positive.  */
                    phase2_ungetc (xp, delim);
                    tp->type = token_type_symbol;
                    tp->sub_type = symbol_type_none;
                    tp->string = sb_xdupfree_c (&buffer);
                    return;
                  }
                extract_quotelike (xp, tp, delim);
                sb_free (&buffer);
                if (delim != '\'')
                  interpolate_keywords (xp, sd_from_c (tp->string),
                                        xp->line_number);
                free (tp->string);
                drop_reference (tp->comment);
                tp->type = token_type_regex_op;

                /* Eat the following modifiers.  */
                do
                  c = phase1_getc (xp);
                while (c >= 'a' && c <= 'z');
                phase1_ungetc (xp, c);
                return;
              }
            else if (strcmp (contents, "qq") == 0
                     || strcmp (contents, "q") == 0
                     || strcmp (contents, "qx") == 0
                     || strcmp (contents, "qw") == 0
                     || strcmp (contents, "qr") == 0)
              {
                /* The qw (...) construct is not really a string but we
                   can treat in the same manner and then pretend it is
                   a symbol.  Rationale: Saying "qw (foo bar)" is the
                   same as "my @list = ('foo', 'bar'); @list;".  */

                int delim = phase1_getc (xp);

                while (is_whitespace (delim))
                  delim = phase2_getc (xp);

                if (delim == EOF)
                  {
                    sb_free (&buffer);
                    tp->type = token_type_eof;
                    return;
                  }

                if ((delim >= '0' && delim <= '9')
                    || (delim >= 'A' && delim <= 'Z')
                    || (delim >= 'a' && delim <= 'z'))
                  {
                    /* False positive.  */
                    phase2_ungetc (xp, delim);
                    tp->type = token_type_symbol;
                    tp->sub_type = symbol_type_none;
                    tp->string = sb_xdupfree_c (&buffer);
                    return;
                  }

                extract_quotelike (xp, tp, delim);

                switch (contents[1])
                  {
                  case 'q':
                  case 'x':
                    tp->type = token_type_string;
                    tp->sub_type = string_type_qq;
                    interpolate_keywords (xp, sd_from_c (tp->string),
                                          xp->line_number);
                    break;
                  case 'r':
                    drop_reference (tp->comment);
                    tp->type = token_type_regex_op;
                    break;
                  case 'w':
                    drop_reference (tp->comment);
                    tp->type = token_type_symbol;
                    tp->sub_type = symbol_type_none;
                    break;
                  case '\0':
                    tp->type = token_type_string;
                    tp->sub_type = string_type_q;
                    break;
                  default:
                    abort ();
                  }
                sb_free (&buffer);
                return;
              }
            else if ((contents[0] >= '0' && contents[0] <= '9')
                     || contents[0] == '.')
              {
                sb_free (&buffer);
                tp->type = token_type_number;
                return;
              }
            tp->type = token_type_symbol;
            tp->sub_type = (strcmp (contents, "sub") == 0
                            ? symbol_type_sub
                            : symbol_type_none);
            tp->string = sb_xdupfree_c (&buffer);
          }
          return;

        case '"':
          extract_quotelike (xp, tp, c);
          tp->sub_type = string_type_qq;
          interpolate_keywords (xp, sd_from_c (tp->string),
                                xp->line_number);
          return;

        case '`':
          extract_quotelike (xp, tp, c);
          tp->sub_type = string_type_qq;
          interpolate_keywords (xp, sd_from_c (tp->string),
                                xp->line_number);
          return;

        case '\'':
          extract_quotelike (xp, tp, c);
          tp->sub_type = string_type_q;
          return;

        case '(':
          tp->type = token_type_lparen;
          return;

        case ')':
          tp->type = token_type_rparen;
          return;

        case '{':
          tp->type = token_type_lbrace;
          return;

        case '}':
          tp->type = token_type_rbrace;
          return;

        case '[':
          tp->type = token_type_lbracket;
          return;

        case ']':
          tp->type = token_type_rbracket;
          return;

        case ';':
          tp->type = token_type_semicolon;
          return;

        case ',':
          tp->type = token_type_comma;
          return;

        case '=':
          /* Check for fat comma.  */
          c = phase1_getc (xp);
          if (c == '>')
            {
              tp->type = token_type_fat_comma;
              return;
            }
          else if (xp->linepos == 2
                   && (xp->last_token_type == token_type_semicolon
                       || xp->last_token_type == token_type_rbrace)
                   && ((c >= 'A' && c <='Z')
                       || (c >= 'a' && c <= 'z')))
            {
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: start pod section\n",
                       real_file_name, xp->line_number);
              #endif
              skip_pod (xp);
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: end pod section\n",
                       real_file_name, xp->line_number);
              #endif
              continue;
            }
          phase1_ungetc (xp, c);
          tp->type = token_type_other;
          return;

        case '<':
          /* Check for <<EOF and friends.  */
          c = phase1_getc (xp);
          if (c == '<')
            {
              c = phase1_getc (xp);
              if (c == '\'')
                {
                  extract_quotelike (xp, tp, c);
                  char *string = get_here_document (xp, tp->string);
                  free (tp->string);
                  tp->string = string;
                  tp->type = token_type_string;
                  tp->sub_type = string_type_verbatim;
                  tp->line_number = xp->line_number + 1;
                  return;
                }
              else if (c == '"')
                {
                  extract_quotelike (xp, tp, c);
                  char *string = get_here_document (xp, tp->string);
                  free (tp->string);
                  tp->string = string;
                  tp->type = token_type_string;
                  tp->sub_type = string_type_qq;
                  tp->line_number = xp->line_number + 1;
                  interpolate_keywords (xp, sd_from_c (tp->string),
                                        tp->line_number);
                  return;
                }
              else if ((c >= 'A' && c <= 'Z')
                       || (c >= 'a' && c <= 'z')
                       || c == '_')
                {
                  struct string_buffer buffer;
                  sb_init (&buffer);
                  while ((c >= 'A' && c <= 'Z')
                         || (c >= 'a' && c <= 'z')
                         || (c >= '0' && c <= '9')
                         || c == '_' || c >= 0x80)
                    {
                      sb_xappend1 (&buffer, c);
                      c = phase1_getc (xp);
                    }
                  if (c == EOF)
                    {
                      sb_free (&buffer);
                      tp->type = token_type_eof;
                      return;
                    }
                  else
                    {
                      phase1_ungetc (xp, c);
                      char *string =
                        get_here_document (xp, sb_xdupfree_c (&buffer));
                      tp->string = string;
                      tp->type = token_type_string;
                      tp->sub_type = string_type_qq;
                      tp->comment = add_reference (savable_comment);
                      tp->line_number = xp->line_number + 1;
                      interpolate_keywords (xp, sd_from_c (tp->string),
                                            tp->line_number);
                      return;
                    }
                }
              else
                {
                  tp->type = token_type_other;
                  return;
                }
            }
          else
            {
              phase1_ungetc (xp, c);
              tp->type = token_type_other;
            }
          return;  /* End of case '>'.  */

        case '-':
          /* Check for dereferencing operator.  */
          c = phase1_getc (xp);
          if (c == '>')
            {
              tp->type = token_type_dereference;
              return;
            }
          else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            {
              /* One of the -X (filetest) functions.  We play safe
                 and accept all alphabetical characters here.  */
              tp->type = token_type_other;
              return;
            }
          phase1_ungetc (xp, c);
          tp->type = token_type_other;
          return;

        case '/':
        case '?':
          if (prefer_regexp_over_division (tp->last_type))
            {
              extract_quotelike (xp, tp, c);
              interpolate_keywords (xp, sd_from_c (tp->string),
                                    xp->line_number);
              free (tp->string);
              drop_reference (tp->comment);
              tp->type = token_type_regex_op;
              /* Eat the following modifiers.  */
              do
                c = phase1_getc (xp);
              while (c >= 'a' && c <= 'z');
              phase1_ungetc (xp, c);
              return;
            }
          /* Recognize operator '//'.  */
          if (c == '/')
            {
              c = phase1_getc (xp);
              if (c != '/')
                phase1_ungetc (xp, c);
            }
          FALLTHROUGH;

        default:
          /* We could carefully recognize each of the 2 and 3 character
             operators, but it is not necessary, except for the '//' operator,
             as we only need to recognize gettext invocations.  Don't
             bother.  */
          tp->type = token_type_other;
          return;
        }
    }
}


#if DEBUG_PERL
/* Dumps all resources allocated by stack STACK.  */
static int
token_stack_dump (token_stack_ty *stack)
{
  fprintf (stderr, "BEGIN STACK DUMP\n");
  for (size_t i = 0; i < stack->nitems; i++)
    {
      token_ty *token = stack->items[i];
      fprintf (stderr, "  [%s]\n", token2string (token));
      switch (token->type)
        {
        case token_type_named_op:
        case token_type_string:
        case token_type_symbol:
        case token_type_variable:
          fprintf (stderr, "    string: %s\n", token->string);
          break;
        case token_type_object:
          fprintf (stderr, "    string: %s->\n", token->string);
        default:
          break;
        }
    }
  fprintf (stderr, "END STACK DUMP\n");
  return 0;
}
#endif

/* Pushes the token TOKEN onto the stack STACK.  */
static inline void
token_stack_push (token_stack_ty *stack, token_ty *token)
{
  if (stack->nitems >= stack->nitems_max)
    {
      stack->nitems_max = 2 * stack->nitems_max + 4;
      size_t nbytes = stack->nitems_max * sizeof (token_ty *);
      stack->items = xrealloc (stack->items, nbytes);
    }
  stack->items[stack->nitems++] = token;
}

/* Pops the most recently pushed token from the stack STACK and returns it.
   Returns NULL if the stack is empty.  */
static inline token_ty *
token_stack_pop (token_stack_ty *stack)
{
  if (stack->nitems > 0)
    return stack->items[--(stack->nitems)];
  else
    return NULL;
}

/* Return the top of the stack without removing it from the stack, or
   NULL if the stack is empty.  */
static inline token_ty *
token_stack_peek (const token_stack_ty *stack)
{
  if (stack->nitems > 0)
    return stack->items[stack->nitems - 1];
  else
    return NULL;
}

/* Frees all resources allocated by stack STACK.  */
static inline void
token_stack_free (token_stack_ty *stack)
{
  for (size_t i = 0; i < stack->nitems; i++)
    free_token (stack->items[i]);
  free (stack->items);
}


static token_ty *
x_perl_lex (struct perl_extractor *xp)
{
  if (++(xp->nesting_depth) > MAX_NESTING_DEPTH)
    if_error (IF_SEVERITY_FATAL_ERROR,
              logical_file_name, xp->line_number, (size_t)(-1), false,
              _("too deeply nested expressions"));

  #if DEBUG_PERL
  int dummy = token_stack_dump (&xp->token_stack);
  #endif
  token_ty *tp = token_stack_pop (&xp->token_stack);

  if (!tp)
    {
      tp = XMALLOC (token_ty);
      x_perl_prelex (xp, tp);
      tp->last_type = xp->last_token_type;
      xp->last_token_type = tp->type;

      #if DEBUG_PERL
      fprintf (stderr, "%s:%d: x_perl_prelex returned %s\n",
               real_file_name, xp->line_number, token2string (tp));
      #endif

      /* The interpretation of a slash or question mark after a function call
         depends on the prototype of that function.  If the function expects
         at least one argument, a regular expression is preferred, otherwise
         an operator.  With our limited means, we can only guess here.  If
         the function is a builtin that takes no arguments, we prefer an
         operator by silently turning the last symbol into a variable instead
         of a symbol.

         Method calls without parentheses are not ambiguous.  After them, an
         operator must follow.  Due to some ideosyncrasies in this parser
         they are treated in two different manners.  If the call is
         chained ($foo->bar->baz) the token left of the symbol is a
         dereference operator.  If it is not chained ($foo->bar) the
         dereference operator is consumed with the extracted variable.  The
         latter case is handled below.  */
      if (tp->type == token_type_symbol)
        {
          if (tp->last_type == token_type_dereference)
            {
              /* Class method call or chained method call (with at least
                 two arrow operators).  */
              xp->last_token_type = token_type_variable;
            }
          else if (tp->last_type == token_type_object)
            {
              /* Instance method, not chained.  */
              xp->last_token_type = token_type_variable;
            }
          else if (strcmp (tp->string, "wantarray") == 0
                   || strcmp (tp->string, "fork") == 0
                   || strcmp (tp->string, "getlogin") == 0
                   || strcmp (tp->string, "getppid") == 0
                   || strcmp (tp->string, "getpwent") == 0
                   || strcmp (tp->string, "getgrent") == 0
                   || strcmp (tp->string, "gethostent") == 0
                   || strcmp (tp->string, "getnetent") == 0
                   || strcmp (tp->string, "getprotoent") == 0
                   || strcmp (tp->string, "getservent") == 0
                   || strcmp (tp->string, "setpwent") == 0
                   || strcmp (tp->string, "setgrent") == 0
                   || strcmp (tp->string, "endpwent") == 0
                   || strcmp (tp->string, "endgrent") == 0
                   || strcmp (tp->string, "endhostent") == 0
                   || strcmp (tp->string, "endnetent") == 0
                   || strcmp (tp->string, "endprotoent") == 0
                   || strcmp (tp->string, "endservent") == 0
                   || strcmp (tp->string, "time") == 0
                   || strcmp (tp->string, "times") == 0
                   || strcmp (tp->string, "wait") == 0
                   || strcmp (tp->string, "wantarray") == 0)
            {
              /* A Perl built-in function that does not accept arguments.  */
              xp->last_token_type = token_type_variable;
            }
        }
    }
  #if DEBUG_PERL
  else
    {
      fprintf (stderr, "%s:%d: %s recycled from stack\n",
               real_file_name, xp->line_number, token2string (tp));
    }
  #endif

  /* A symbol followed by a fat comma is really a single-quoted string.
     Function definitions or forward declarations also need a special
     handling because the dollars and at signs inside the parentheses
     must not be interpreted as the beginning of a variable ')'.  */
  if (tp->type == token_type_symbol || tp->type == token_type_named_op)
    {
      token_ty *next = token_stack_peek (&xp->token_stack);

      if (!next)
        {
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: pre-fetching next token\n",
                   real_file_name, xp->line_number);
          #endif
          next = x_perl_lex (xp);
          x_perl_unlex (xp, next);
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: unshifted next token\n",
                   real_file_name, xp->line_number);
          #endif
        }

      #if DEBUG_PERL
      fprintf (stderr, "%s:%d: next token is %s\n",
               real_file_name, xp->line_number, token2string (next));
      #endif

      if (next->type == token_type_fat_comma)
        {
          tp->type = token_type_string;
          tp->sub_type = string_type_q;
          tp->comment = add_reference (savable_comment);
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: token %s mutated to token_type_string\n",
                   real_file_name, xp->line_number, token2string (tp));
          #endif
        }
      else if (tp->type == token_type_symbol && tp->sub_type == symbol_type_sub
               && next->type == token_type_symbol)
        {
          /* Start of a function declaration or definition.  Mark this
             symbol as a function name, so that we can later eat up
             possible prototype information.  */
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: subroutine declaration/definition '%s'\n",
                   real_file_name, xp->line_number, next->string);
          #endif
          next->sub_type = symbol_type_function;
        }
      else if (tp->type == token_type_symbol
               && (tp->sub_type == symbol_type_sub
                   || tp->sub_type == symbol_type_function)
               && next->type == token_type_lparen)
        {
          /* For simplicity we simply consume everything up to the
             closing parenthesis.  Actually only a limited set of
             characters is allowed inside parentheses but we leave
             complaints to the interpreter and are prepared for
             future extensions to the Perl syntax.  */

          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: consuming prototype information\n",
                   real_file_name, xp->line_number);
          #endif

          int c;

          do
            {
              c = phase1_getc (xp);
              #if DEBUG_PERL
              fprintf (stderr, "  consuming character '%c'\n", c);
              #endif
            }
          while (c != EOF && c != ')');
          phase1_ungetc (xp, c);
        }
    }

  xp->nesting_depth--;
  return tp;
}

static void
x_perl_unlex (struct perl_extractor *xp, token_ty *tp)
{
  token_stack_push (&xp->token_stack, tp);
}


/* ========================= Extracting strings.  ========================== */

/* Assuming TP is a string token, this function accumulates all subsequent
   . string2 . string3 ... to the string.  (String concatenation.)
   If at least one of the tokens gets transformed into a token of type
   token_type_string_interpol, it returns NULL instead.  */

static char *
collect_message (struct perl_extractor *xp, token_ty *tp)
{
  extract_quotelike_pass3 (xp, tp);

  char *string;
  size_t len;
  if (tp->type == token_type_string)
    {
      string = xstrdup (tp->string);
      len = strlen (tp->string) + 1;
    }
  else
    {
      string = NULL;
      len = 0;
    }

  for (;;)
    {
      int c;

      do
        c = phase2_getc (xp);
      while (is_whitespace (c));

      if (c != '.')
        {
          phase2_ungetc (xp, c);
          return string;
        }

      do
        c = phase2_getc (xp);
      while (is_whitespace (c));

      phase2_ungetc (xp, c);

      if (c == '"' || c == '\'' || c == '`'
          || ((c == '/' || c == '?')
              && prefer_regexp_over_division (tp->last_type))
          || c == 'q')
        {
          token_ty *qstring = x_perl_lex (xp);
          if (qstring->type != token_type_string)
            {
              /* assert (qstring->type == token_type_symbol) */
              x_perl_unlex (xp, qstring);
              return string;
            }

          extract_quotelike_pass3 (xp, qstring);
          if (qstring->type == token_type_string)
            {
              if (string != NULL)
                {
                  len += strlen (qstring->string);
                  string = xrealloc (string, len);
                  strcat (string, qstring->string);
                }
              free_token (qstring);
            }
        }
    }
}

/* The file is broken into tokens.  Scan the token stream, looking for
   a keyword, followed by a left paren, followed by a string.  When we
   see this sequence, we have something to remember.  We assume we are
   looking at a valid Perl program, and leave the complaints about
   the grammar to the compiler.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.

   In Perl, parentheses around function arguments can be omitted.

   The general rules are:
     1) Functions declared with a prototype take exactly the specified number
        of arguments.
          sub one_arg ($) { ... }
          sub two_args ($$) { ... }
     2) When a function name is immediately followed by an opening parenthesis,
        the argument list ends at the corresponding closing parenthesis.

   If rule 1 and rule 2 are contradictory, i.e. when the program calls a
   function with an explicit argument list and the wrong number of arguments,
   the program is invalid:
     sub two_args ($$) { ... }
     foo two_args (x), y             - invalid due to rules 1 and 2

   Ambiguities are resolved as follows:
     3) Some built-ins, such as 'abs', 'sqrt', 'sin', 'cos', ..., and functions
        declared with a prototype of exactly one argument take exactly one
        argument:
          foo sin x, y  ==>  foo (sin (x), y)
          sub one_arg ($) { ... }
          foo one_arg x, y, z  ==>  foo (one_arg (x), y, z)
     4) Other identifiers, if not immediately followed by an opening
        parenthesis, consume the entire remaining argument list:
          foo bar x, y  ==>  foo (bar (x, y))
          sub two_args ($$) { ... }
          foo two_args x, y  ==>  foo (two_args (x, y))

   Other series of comma separated expressions without a function name at
   the beginning are comma expressions:
          sub two_args ($$) { ... }
          foo two_args x, (y, z)  ==>  foo (two_args (x, (y, z)))
   Note that the evaluation of comma expressions returns a list of values
   when in list context (e.g. inside the argument list of a function without
   prototype) but only one value when inside the argument list of a function
   with a prototype:
          sub print3 ($$$) { print @_ }
          print3 5, (6, 7), 8  ==>  578
          print 5, (6, 7), 8  ==>  5678

   Where rule 3 or 4 contradict rule 1 or 2, the program is invalid:
     sin (x, y)                      - invalid due to rules 2 and 3
     sub one_arg ($) { ... }
     one_arg (x, y)                  - invalid due to rules 2 and 3
     sub two_args ($$) { ... }
     foo two_args x, y, z            - invalid due to rules 1 and 4
 */

/* Extract messages until the next balanced closing parenthesis.
   Extracted messages are added to MLP.

   DELIM can be either token_type_rbrace, token_type_rbracket,
   token_type_rparen, or token_type_r_any.
   Additionally, if SEMICOLON_DELIM is true, parsing stops at the next
   semicolon outside parentheses.
   Similarly, if COMMA_DELIM is true, parsing stops at the next comma
   outside parentheses.

   ARG is the current argument list position, starts with 1.
   ARGPARSER is the corresponding argument list parser.

   Returns true for EOF, false otherwise.  */

static bool
extract_balanced (struct perl_extractor *xp,
                  token_type_ty delim, bool eat_delim,
                  bool semicolon_delim, bool eat_semicolon_delim,
                  bool comma_delim,
                  flag_region_ty *outer_region,
                  flag_context_list_iterator_ty context_iter,
                  int arg, struct arglist_parser *argparser)
{
  /* Whether we are at the first token.  */
  bool first = true;
  /* Whether the first token was a "sub".  */
  bool sub_seen = false;

  /* Whether to implicitly assume the next tokens are arguments even without
     a '('.  */
  bool next_is_argument = false;
  /* Parameters of the keyword just seen.  Defined only when next_is_argument
     is true.  */
  const struct callshapes *next_shapes = NULL;
  struct arglist_parser *next_argparser = NULL;

  /* Whether to not consider strings until the next comma.  */
  bool skip_until_comma = false;

  /* Context iterator that will be used if the next token is a '('.  */
  flag_context_list_iterator_ty next_context_iter =
    passthrough_context_list_iterator;
  /* Current region.  */
  flag_region_ty *inner_region =
    inheriting_region (outer_region,
                       flag_context_list_iterator_advance (&context_iter));

  #if DEBUG_PERL
  static int nesting_level = 0;

  ++nesting_level;
  #endif

  if (xp->nesting_depth > MAX_NESTING_DEPTH)
    if_error (IF_SEVERITY_FATAL_ERROR,
              logical_file_name, xp->line_number, (size_t)(-1), false,
              _("too deeply nested expressions"));

  for (;;)
    {
      /* The current token.  */
      token_ty *tp = x_perl_lex (xp);

      if (first)
        {
          sub_seen = (tp->type == token_type_symbol
                      && tp->sub_type == symbol_type_sub);
        }

      if (delim == tp->type
          || (delim == token_type_r_any
              && (tp->type == token_type_rparen
                  || tp->type == token_type_rbrace
                  || tp->type == token_type_rbracket)))
        {
          arglist_parser_done (argparser, arg);
          unref_region (inner_region);
          if (next_argparser != NULL)
            free (next_argparser);
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: extract_balanced finished (%d)\n",
                   logical_file_name, tp->line_number, --nesting_level);
          #endif
          if (eat_delim)
            free_token (tp);
          else
            /* Preserve the delimiter for the caller.  */
            x_perl_unlex (xp, tp);
          return false;
        }

      if (semicolon_delim && tp->type == token_type_semicolon)
        {
          arglist_parser_done (argparser, arg);
          unref_region (inner_region);
          if (next_argparser != NULL)
            free (next_argparser);
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: extract_balanced finished at semicolon (%d)\n",
                   logical_file_name, tp->line_number, --nesting_level);
          #endif
          if (eat_semicolon_delim)
            free_token (tp);
          else
            /* Preserve the semicolon for the caller.  */
            x_perl_unlex (xp, tp);
          return false;
        }

      if (comma_delim && tp->type == token_type_comma)
        {
          arglist_parser_done (argparser, arg);
          unref_region (inner_region);
          if (next_argparser != NULL)
            free (next_argparser);
          #if DEBUG_PERL
          fprintf (stderr, "%s:%d: extract_balanced finished at comma (%d)\n",
                   logical_file_name, tp->line_number, --nesting_level);
          #endif
          x_perl_unlex (xp, tp);
          return false;
        }

      if (next_is_argument && tp->type != token_type_lparen)
        {
          /* An argument list starts, even though there is no '('.  */
          x_perl_unlex (xp, tp);

          bool next_comma_delim;
          if (next_shapes != NULL)
            /* We know something about the function being called.  Assume
               that it consumes only one argument if no argument number or
               total > 1 is specified.  */
            {
              next_comma_delim = true;
              for (size_t i = 0; i < next_shapes->nshapes; i++)
                {
                  const struct callshape *shape = &next_shapes->shapes[i];

                  if (shape->argnum1 > 1
                      || shape->argnum2 > 1
                      || shape->argnumc > 1
                      || shape->argtotal > 1)
                    next_comma_delim = false;
                }
            }
          else
            /* We know nothing about the function being called.  It could be
               a function prototyped to take only one argument, or on the other
               hand it could be prototyped to take more than one argument or an
               arbitrary argument list or it could be unprototyped.  Due to
               the way the parser works, assuming the first case gives the
               best results.  */
            next_comma_delim = true;

          ++(xp->nesting_depth);
          #if DEBUG_NESTING_DEPTH
          fprintf (stderr, "extract_balanced %d>> @%d\n", xp->nesting_depth, xp->line_number);
          #endif
          if (extract_balanced (xp,
                                delim, false,
                                true, false, next_comma_delim,
                                inner_region, next_context_iter,
                                1, next_argparser))
            {
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              return true;
            }
          #if DEBUG_NESTING_DEPTH
          fprintf (stderr, "extract_balanced %d<< @%d\n", xp->nesting_depth, xp->line_number);
          #endif
          xp->nesting_depth--;

          next_is_argument = false;
          next_argparser = NULL;
          next_context_iter = null_context_list_iterator;
        }
      else
        {
          switch (tp->type)
            {
            case token_type_symbol:
              if (sub_seen)
                break;
              FALLTHROUGH;
            case token_type_keyword_symbol:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type symbol (%d) \"%s\"\n",
                       logical_file_name, tp->line_number, nesting_level,
                       tp->string);
              #endif

              {
                void *keyword_value;
                if (hash_find_entry (&keywords, tp->string, strlen (tp->string),
                                     &keyword_value) == 0)
                  {
                    const struct callshapes *shapes =
                      (const struct callshapes *) keyword_value;

                    next_shapes = shapes;
                    next_argparser = arglist_parser_alloc (xp->mlp, shapes);
                  }
                else
                  {
                    next_shapes = NULL;
                    next_argparser = arglist_parser_alloc (xp->mlp, NULL);
                  }
              }
              next_is_argument = true;
              next_context_iter =
                flag_context_list_iterator (
                  flag_context_list_table_lookup (
                    flag_context_list_table,
                    tp->string, strlen (tp->string)));
              break;

            case token_type_variable:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type variable (%d) \"%s\"\n",
                       logical_file_name, tp->line_number, nesting_level,
                       tp->string);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_object:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type object (%d) \"%s->\"\n",
                       logical_file_name, tp->line_number, nesting_level,
                       tp->string);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_lparen:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type left parenthesis (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              if (next_is_argument)
                {
                  /* Parse the argument list of a function call.  */
                  ++(xp->nesting_depth);
                  #if DEBUG_NESTING_DEPTH
                  fprintf (stderr, "extract_balanced %d>> @%d\n", xp->nesting_depth, xp->line_number);
                  #endif
                  if (extract_balanced (xp,
                                        token_type_rparen, true,
                                        false, false, false,
                                        inner_region, next_context_iter,
                                        1, next_argparser))
                    {
                      arglist_parser_done (argparser, arg);
                      unref_region (inner_region);
                      return true;
                    }
                  #if DEBUG_NESTING_DEPTH
                  fprintf (stderr, "extract_balanced %d<< @%d\n", xp->nesting_depth, xp->line_number);
                  #endif
                  xp->nesting_depth--;
                  next_is_argument = false;
                  next_argparser = NULL;
                }
              else
                {
                  /* Parse a parenthesized expression or comma expression.  */
                  ++(xp->nesting_depth);
                  #if DEBUG_NESTING_DEPTH
                  fprintf (stderr, "extract_balanced %d>> @%d\n", xp->nesting_depth, xp->line_number);
                  #endif
                  if (extract_balanced (xp,
                                        token_type_rparen, true,
                                        false, false, false,
                                        inner_region, next_context_iter,
                                        arg, arglist_parser_clone (argparser)))
                    {
                      arglist_parser_done (argparser, arg);
                      unref_region (inner_region);
                      if (next_argparser != NULL)
                        free (next_argparser);
                      free_token (tp);
                      return true;
                    }
                  #if DEBUG_NESTING_DEPTH
                  fprintf (stderr, "extract_balanced %d<< @%d\n", xp->nesting_depth, xp->line_number);
                  #endif
                  xp->nesting_depth--;
                  next_is_argument = false;
                  if (next_argparser != NULL)
                    free (next_argparser);
                  next_argparser = NULL;
                }
              skip_until_comma = true;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_rparen:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type right parenthesis (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              skip_until_comma = true;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_comma:
            case token_type_fat_comma:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type comma (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              if (arglist_parser_decidedp (argparser, arg))
                {
                  /* We have missed the argument.  */
                  arglist_parser_done (argparser, arg);
                  argparser = arglist_parser_alloc (xp->mlp, NULL);
                  arg = 0;
                }
              arg++;
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: arg: %d\n",
                       real_file_name, tp->line_number, arg);
              #endif
              unref_region (inner_region);
              inner_region =
                inheriting_region (outer_region,
                                   flag_context_list_iterator_advance (
                                     &context_iter));
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              skip_until_comma = false;
              next_context_iter = passthrough_context_list_iterator;
              break;

            case token_type_string:
            case token_type_string_interpol:
              #if DEBUG_PERL
              if (tp->type == token_type_string)
                fprintf (stderr, "%s:%d: type string (%d): \"%s\"\n",
                         logical_file_name, tp->line_number, nesting_level,
                         tp->string);
              else
                fprintf (stderr, "%s:%d: type string_interpol (%d)\n",
                         logical_file_name, tp->line_number, nesting_level);
              #endif

              if (extract_all)
                {
                  char *string = collect_message (xp, tp);
                  if (string != NULL)
                    {
                      lex_pos_ty pos;
                      pos.file_name = logical_file_name;
                      pos.line_number = tp->line_number;

                      remember_a_message (xp->mlp, NULL, string, true, false,
                                          inner_region, &pos, NULL, tp->comment,
                                          true);
                    }
                }
              else if (!skip_until_comma)
                {
                  /* Need to collect the complete string, with error checking,
                     only if the argument ARG is used in ARGPARSER.  */
                  bool must_collect = false;
                  {
                    size_t nalternatives = argparser->nalternatives;

                    for (size_t i = 0; i < nalternatives; i++)
                      {
                        struct partial_call *cp = &argparser->alternative[i];

                        if (arg == cp->argnumc
                            || arg == cp->argnum1 || arg == cp->argnum2)
                          must_collect = true;
                      }
                  }

                  if (must_collect)
                    {
                      char *string = collect_message (xp, tp);
                      if (string != NULL)
                        {
                          mixed_string_ty *ms =
                            mixed_string_alloc_utf8 (string, lc_string,
                                                     logical_file_name,
                                                     tp->line_number);
                          free (string);
                          arglist_parser_remember (argparser, arg, ms,
                                                   inner_region,
                                                   logical_file_name,
                                                   tp->line_number,
                                                   tp->comment, true);
                        }
                    }
                }

              if (arglist_parser_decidedp (argparser, arg))
                {
                  arglist_parser_done (argparser, arg);
                  argparser = arglist_parser_alloc (xp->mlp, NULL);
                }

              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_number:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type number (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_eof:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type EOF (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              arglist_parser_done (argparser, arg);
              unref_region (inner_region);
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              free_token (tp);
              return true;

            case token_type_lbrace:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type lbrace (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              ++(xp->nesting_depth);
              #if DEBUG_NESTING_DEPTH
              fprintf (stderr, "extract_balanced %d>> @%d\n", xp->nesting_depth, xp->line_number);
              #endif
              if (extract_balanced (xp,
                                    token_type_rbrace, true,
                                    false, false, false,
                                    null_context_region (),
                                    null_context_list_iterator,
                                    1, arglist_parser_alloc (xp->mlp, NULL)))
                {
                  arglist_parser_done (argparser, arg);
                  unref_region (inner_region);
                  if (next_argparser != NULL)
                    free (next_argparser);
                  free_token (tp);
                  return true;
                }
              #if DEBUG_NESTING_DEPTH
              fprintf (stderr, "extract_balanced %d<< @%d\n", xp->nesting_depth, xp->line_number);
              #endif
              xp->nesting_depth--;
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              if (sub_seen)
                {
                  /* Go back to the caller.  We don't want to recurse each time we
                     parsed a    sub name... { ... }    definition.  */
                  arglist_parser_done (argparser, arg);
                  unref_region (inner_region);
                  free_token (tp);
                  return false;
                }
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_rbrace:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type rbrace (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_lbracket:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type lbracket (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              ++(xp->nesting_depth);
              #if DEBUG_NESTING_DEPTH
              fprintf (stderr, "extract_balanced %d>> @%d\n", xp->nesting_depth, xp->line_number);
              #endif
              if (extract_balanced (xp,
                                    token_type_rbracket, true,
                                    false, false, false,
                                    null_context_region (),
                                    null_context_list_iterator,
                                    1, arglist_parser_alloc (xp->mlp, NULL)))
                {
                  arglist_parser_done (argparser, arg);
                  unref_region (inner_region);
                  if (next_argparser != NULL)
                    free (next_argparser);
                  free_token (tp);
                  return true;
                }
              #if DEBUG_NESTING_DEPTH
              fprintf (stderr, "extract_balanced %d<< @%d\n", xp->nesting_depth, xp->line_number);
              #endif
              xp->nesting_depth--;
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_rbracket:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type rbracket (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_semicolon:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type semicolon (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif

              /* The ultimate sign.  */
              arglist_parser_done (argparser, arg);
              argparser = arglist_parser_alloc (xp->mlp, NULL);

              /* FIXME: Instead of resetting outer_context here, it may be better
                 to recurse in the next_is_argument handling above, waiting for
                 the next semicolon or other statement terminator.  */
              outer_region = null_context_region ();
              context_iter = null_context_list_iterator;
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = passthrough_context_list_iterator;
              unref_region (inner_region);
              inner_region =
                inheriting_region (outer_region,
                                   flag_context_list_iterator_advance (
                                     &context_iter));
              break;

            case token_type_dereference:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type dereference (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_dot:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type dot (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_named_op:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type named operator (%d): %s\n",
                       logical_file_name, tp->line_number, nesting_level,
                       tp->string);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_regex_op:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type regex operator (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            case token_type_other:
              #if DEBUG_PERL
              fprintf (stderr, "%s:%d: type other (%d)\n",
                       logical_file_name, tp->line_number, nesting_level);
              #endif
              next_is_argument = false;
              if (next_argparser != NULL)
                free (next_argparser);
              next_argparser = NULL;
              next_context_iter = null_context_list_iterator;
              break;

            default:
              if_error (IF_SEVERITY_ERROR,
                        real_file_name, tp->line_number, (size_t)(-1), false,
                        "unknown token type %d", (int) tp->type);
              fflush (stderr);
              abort ();
            }

          free_token (tp);
        }

      first = false;
    }
}

static void
extract_perl_input (struct perl_extractor *xp)
{
  /* Eat tokens until eof is seen.  When extract_balanced returns due to an
     unbalanced closing paren / brace / bracket or due to a semicolon, just
     restart it.  */
  while (!extract_balanced (xp,
                            token_type_r_any, true,
                            true, true, false,
                            null_context_region (), null_context_list_iterator,
                            1, arglist_parser_alloc (xp->mlp, NULL)))
    ;
}

void
extract_perl (FILE *f, const char *real_filename, const char *logical_filename,
              flag_context_list_table_ty *flag_table,
              msgdomain_list_ty *mdlp)
{
  flag_context_list_table = flag_table;

  init_keywords ();

  struct perl_extractor *xp = XMALLOC (struct perl_extractor);

  xp->mlp = mdlp->item[0]->messages;
  sf_istream_init_from_file (&xp->input, f);
  real_file_name = real_filename;
  logical_file_name = xstrdup (logical_filename);
  perl_extractor_init_rest (xp);

  extract_perl_input (xp);

  token_stack_free (&xp->token_stack);
  free (xp);
  real_file_name = NULL;
  logical_file_name = NULL;
}
