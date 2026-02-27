/* xgettext JavaScript backend.
   Copyright (C) 2002-2003, 2005-2009, 2013, 2015-2016 Free Software
   Foundation, Inc.

   This file was written by Andreas Stricker <andy@knitter.ch>, 2010
   It's based on x-python from Bruno Haible.

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "x-javascript.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "xgettext.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "basename.h"
#include "xerror.h"
#include "xvasprintf.h"
#include "xalloc.h"
#include "c-strstr.h"
#include "c-ctype.h"
#include "po-charset.h"
#include "unistr.h"
#include "gettext.h"

#define _(s) gettext(s)

#define max(a,b) ((a) > (b) ? (a) : (b))

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

/* The JavaScript aka ECMA-Script syntax is defined in ECMA-262
   specification:
   http://www.ecma-international.org/publications/standards/Ecma-262.htm */

/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_javascript_extract_all ()
{
  extract_all = true;
}


void
x_javascript_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      const char *end;
      struct callshape shape;
      const char *colon;

      if (keywords.table == NULL)
        hash_init (&keywords, 100);

      split_keywordspec (name, &end, &shape);

      /* The characters between name and end should form a valid C identifier.
         A colon means an invalid parse in split_keywordspec().  */
      colon = strchr (name, ':');
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
      x_javascript_keyword ("gettext");
      x_javascript_keyword ("dgettext:2");
      x_javascript_keyword ("dcgettext:2");
      x_javascript_keyword ("ngettext:1,2");
      x_javascript_keyword ("dngettext:2,3");
      x_javascript_keyword ("pgettext:1c,2");
      x_javascript_keyword ("dpgettext:2c,3");
      x_javascript_keyword ("_");
      default_keywords = false;
    }
}

void
init_flag_table_javascript ()
{
  xgettext_record_flag ("gettext:1:pass-javascript-format");
  xgettext_record_flag ("dgettext:2:pass-javascript-format");
  xgettext_record_flag ("dcgettext:2:pass-javascript-format");
  xgettext_record_flag ("ngettext:1:pass-javascript-format");
  xgettext_record_flag ("ngettext:2:pass-javascript-format");
  xgettext_record_flag ("dngettext:2:pass-javascript-format");
  xgettext_record_flag ("dngettext:3:pass-javascript-format");
  xgettext_record_flag ("pgettext:2:pass-javascript-format");
  xgettext_record_flag ("dpgettext:3:pass-javascript-format");
  xgettext_record_flag ("_:1:pass-javascript-format");
}


/* ======================== Reading of characters.  ======================== */

/* Real filename, used in error messages about the input file.  */
static const char *real_file_name;

/* Logical filename and line number, used to label the extracted messages.  */
static char *logical_file_name;
static int line_number;

/* The input file stream.  */
static FILE *fp;


/* 1. line_number handling.  */

/* Maximum used, roughly a safer MB_LEN_MAX.  */
#define MAX_PHASE1_PUSHBACK 16
static unsigned char phase1_pushback[MAX_PHASE1_PUSHBACK];
static int phase1_pushback_length;

/* Read the next single byte from the input file.  */
static int
phase1_getc ()
{
  int c;

  if (phase1_pushback_length)
    c = phase1_pushback[--phase1_pushback_length];
  else
    {
      c = getc (fp);

      if (c == EOF)
        {
          if (ferror (fp))
            error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
                   real_file_name);
          return EOF;
        }
    }

  if (c == '\n')
    ++line_number;

  return c;
}

/* Supports MAX_PHASE1_PUSHBACK characters of pushback.  */
static void
phase1_ungetc (int c)
{
  if (c != EOF)
    {
      if (c == '\n')
        --line_number;

      if (phase1_pushback_length == SIZEOF (phase1_pushback))
        abort ();
      phase1_pushback[phase1_pushback_length++] = c;
    }
}


/* Phase 2: Conversion to Unicode.
   For now, we expect JavaScript files to be encoded as UTF-8.  */

/* End-of-file indicator for functions returning an UCS-4 character.  */
#define UEOF -1

static lexical_context_ty lexical_context;

/* Maximum used, length of "<![CDATA[" tag minus one.  */
static int phase2_pushback[8];
static int phase2_pushback_length;

/* Read the next Unicode UCS-4 character from the input file.  */
static int
phase2_getc ()
{
  if (phase2_pushback_length)
    return phase2_pushback[--phase2_pushback_length];

  if (xgettext_current_source_encoding == po_charset_ascii)
    {
      int c = phase1_getc ();
      if (c == EOF)
        return UEOF;
      if (!c_isascii (c))
        {
          multiline_error (xstrdup (""),
                           xasprintf ("%s\n%s\n",
                                      non_ascii_error_message (lexical_context,
                                                               real_file_name,
                                                               line_number),
                                      _("\
Please specify the source encoding through --from-code\n")));
          exit (EXIT_FAILURE);
        }
      return c;
    }
  else if (xgettext_current_source_encoding != po_charset_utf8)
    {
#if HAVE_ICONV
      /* Use iconv on an increasing number of bytes.  Read only as many bytes
         through phase1_getc as needed.  This is needed to give reasonable
         interactive behaviour when fp is connected to an interactive tty.  */
      unsigned char buf[MAX_PHASE1_PUSHBACK];
      size_t bufcount;
      int c = phase1_getc ();
      if (c == EOF)
        return UEOF;
      buf[0] = (unsigned char) c;
      bufcount = 1;

      for (;;)
        {
          unsigned char scratchbuf[6];
          const char *inptr = (const char *) &buf[0];
          size_t insize = bufcount;
          char *outptr = (char *) &scratchbuf[0];
          size_t outsize = sizeof (scratchbuf);

          size_t res = iconv (xgettext_current_source_iconv,
                              (ICONV_CONST char **) &inptr, &insize,
                              &outptr, &outsize);
          /* We expect that a character has been produced if and only if
             some input bytes have been consumed.  */
          if ((insize < bufcount) != (outsize < sizeof (scratchbuf)))
            abort ();
          if (outsize == sizeof (scratchbuf))
            {
              /* No character has been produced.  Must be an error.  */
              if (res != (size_t)(-1))
                abort ();

              if (errno == EILSEQ)
                {
                  /* An invalid multibyte sequence was encountered.  */
                  multiline_error (xstrdup (""),
                                   xasprintf (_("\
%s:%d: Invalid multibyte sequence.\n\
Please specify the correct source encoding through --from-code\n"),
                                   real_file_name, line_number));
                  exit (EXIT_FAILURE);
                }
              else if (errno == EINVAL)
                {
                  /* An incomplete multibyte character.  */
                  int c;

                  if (bufcount == MAX_PHASE1_PUSHBACK)
                    {
                      /* An overlong incomplete multibyte sequence was
                         encountered.  */
                      multiline_error (xstrdup (""),
                                       xasprintf (_("\
%s:%d: Long incomplete multibyte sequence.\n\
Please specify the correct source encoding through --from-code\n"),
                                       real_file_name, line_number));
                      exit (EXIT_FAILURE);
                    }

                  /* Read one more byte and retry iconv.  */
                  c = phase1_getc ();
                  if (c == EOF)
                    {
                      multiline_error (xstrdup (""),
                                       xasprintf (_("\
%s:%d: Incomplete multibyte sequence at end of file.\n\
Please specify the correct source encoding through --from-code\n"),
                                       real_file_name, line_number));
                      exit (EXIT_FAILURE);
                    }
                  if (c == '\n')
                    {
                      multiline_error (xstrdup (""),
                                       xasprintf (_("\
%s:%d: Incomplete multibyte sequence at end of line.\n\
Please specify the correct source encoding through --from-code\n"),
                                       real_file_name, line_number - 1));
                      exit (EXIT_FAILURE);
                    }
                  buf[bufcount++] = (unsigned char) c;
                }
              else
                error (EXIT_FAILURE, errno, _("%s:%d: iconv failure"),
                       real_file_name, line_number);
            }
          else
            {
              size_t outbytes = sizeof (scratchbuf) - outsize;
              size_t bytes = bufcount - insize;
              ucs4_t uc;

              /* We expect that one character has been produced.  */
              if (bytes == 0)
                abort ();
              if (outbytes == 0)
                abort ();
              /* Push back the unused bytes.  */
              while (insize > 0)
                phase1_ungetc (buf[--insize]);
              /* Convert the character from UTF-8 to UCS-4.  */
              if (u8_mbtoucr (&uc, scratchbuf, outbytes) < (int) outbytes)
                {
                  /* scratchbuf contains an out-of-range Unicode character
                     (> 0x10ffff).  */
                  multiline_error (xstrdup (""),
                                   xasprintf (_("\
%s:%d: Invalid multibyte sequence.\n\
Please specify the source encoding through --from-code\n"),
                                   real_file_name, line_number));
                  exit (EXIT_FAILURE);
                }
              return uc;
            }
        }
#else
      /* If we don't have iconv(), the only supported values for
         xgettext_global_source_encoding and thus also for
         xgettext_current_source_encoding are ASCII and UTF-8.  */
      abort ();
#endif
    }
  else
    {
      /* Read an UTF-8 encoded character.  */
      unsigned char buf[6];
      unsigned int count;
      int c;
      ucs4_t uc;

      c = phase1_getc ();
      if (c == EOF)
        return UEOF;
      buf[0] = c;
      count = 1;

      if (buf[0] >= 0xc0)
        {
          c = phase1_getc ();
          if (c == EOF)
            return UEOF;
          buf[1] = c;
          count = 2;
        }

      if (buf[0] >= 0xe0
          && ((buf[1] ^ 0x80) < 0x40))
        {
          c = phase1_getc ();
          if (c == EOF)
            return UEOF;
          buf[2] = c;
          count = 3;
        }

      if (buf[0] >= 0xf0
          && ((buf[1] ^ 0x80) < 0x40)
          && ((buf[2] ^ 0x80) < 0x40))
        {
          c = phase1_getc ();
          if (c == EOF)
            return UEOF;
          buf[3] = c;
          count = 4;
        }

      if (buf[0] >= 0xf8
          && ((buf[1] ^ 0x80) < 0x40)
          && ((buf[2] ^ 0x80) < 0x40)
          && ((buf[3] ^ 0x80) < 0x40))
        {
          c = phase1_getc ();
          if (c == EOF)
            return UEOF;
          buf[4] = c;
          count = 5;
        }

      if (buf[0] >= 0xfc
          && ((buf[1] ^ 0x80) < 0x40)
          && ((buf[2] ^ 0x80) < 0x40)
          && ((buf[3] ^ 0x80) < 0x40)
          && ((buf[4] ^ 0x80) < 0x40))
        {
          c = phase1_getc ();
          if (c == EOF)
            return UEOF;
          buf[5] = c;
          count = 6;
        }

      u8_mbtouc (&uc, buf, count);
      return uc;
    }
}

/* Supports max (9, UNINAME_MAX + 3) pushback characters.  */
static void
phase2_ungetc (int c)
{
  if (c != UEOF)
    {
      if (phase2_pushback_length == SIZEOF (phase2_pushback))
        abort ();
      phase2_pushback[phase2_pushback_length++] = c;
    }
}


/* ========================= Accumulating strings.  ======================== */

/* A string buffer type that allows appending Unicode characters.
   Returns the entire string in UTF-8 encoding.  */

struct unicode_string_buffer
{
  /* The part of the string that has already been converted to UTF-8.  */
  char *utf8_buffer;
  size_t utf8_buflen;
  size_t utf8_allocated;
};

/* Initialize a 'struct unicode_string_buffer' to empty.  */
static inline void
init_unicode_string_buffer (struct unicode_string_buffer *bp)
{
  bp->utf8_buffer = NULL;
  bp->utf8_buflen = 0;
  bp->utf8_allocated = 0;
}

/* Auxiliary function: Ensure count more bytes are available in bp->utf8.  */
static inline void
unicode_string_buffer_append_unicode_grow (struct unicode_string_buffer *bp,
                                           size_t count)
{
  if (bp->utf8_buflen + count > bp->utf8_allocated)
    {
      size_t new_allocated = 2 * bp->utf8_allocated + 10;
      if (new_allocated < bp->utf8_buflen + count)
        new_allocated = bp->utf8_buflen + count;
      bp->utf8_allocated = new_allocated;
      bp->utf8_buffer = xrealloc (bp->utf8_buffer, new_allocated);
    }
}

/* Auxiliary function: Append a Unicode character to bp->utf8.
   uc must be < 0x110000.  */
static inline void
unicode_string_buffer_append_unicode (struct unicode_string_buffer *bp,
                                      unsigned int uc)
{
  unsigned char utf8buf[6];
  int count = u8_uctomb (utf8buf, uc, 6);

  if (count < 0)
    /* The caller should have ensured that uc is not out-of-range.  */
    abort ();

  unicode_string_buffer_append_unicode_grow (bp, count);
  memcpy (bp->utf8_buffer + bp->utf8_buflen, utf8buf, count);
  bp->utf8_buflen += count;
}

/* Return the string buffer's contents.  */
static char *
unicode_string_buffer_result (struct unicode_string_buffer *bp)
{
  /* NUL-terminate it.  */
  unicode_string_buffer_append_unicode_grow (bp, 1);
  bp->utf8_buffer[bp->utf8_buflen] = '\0';
  /* Return it.  */
  return bp->utf8_buffer;
}

/* Free the memory pointed to by a 'struct unicode_string_buffer'.  */
static inline void
free_unicode_string_buffer (struct unicode_string_buffer *bp)
{
  free (bp->utf8_buffer);
}


/* ======================== Accumulating comments.  ======================== */


/* Accumulating a single comment line.  */

static struct unicode_string_buffer comment_buffer;

static inline void
comment_start ()
{
  lexical_context = lc_comment;
  comment_buffer.utf8_buflen = 0;
}

static inline bool
comment_at_start ()
{
  return (comment_buffer.utf8_buflen == 0);
}

static inline void
comment_add (int c)
{
  unicode_string_buffer_append_unicode (&comment_buffer, c);
}

static inline const char *
comment_line_end (size_t chars_to_remove)
{
  char *buffer = unicode_string_buffer_result (&comment_buffer);
  size_t buflen = strlen (buffer) - chars_to_remove;

  while (buflen >= 1
         && (buffer[buflen - 1] == ' ' || buffer[buflen - 1] == '\t'))
    --buflen;
  buffer[buflen] = '\0';
  savable_comment_add (buffer);
  lexical_context = lc_outside;
  return buffer;
}


/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line;
static int last_non_comment_line;


/* ======================== Recognizing comments.  ======================== */


/* Canonicalized encoding name for the current input file.  */
static const char *xgettext_current_file_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_current_file_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
static iconv_t xgettext_current_file_source_iconv;
#endif

/* Tracking whether the current line is a continuation line or contains a
   non-blank character.  */
static bool continuation_or_nonblank_line = false;


/* Phase 3: Outside strings, replace backslash-newline with nothing and a
   comment with nothing.  */

static int
phase3_getc ()
{
  int c;

  for (;;)
    {
      c = phase2_getc ();
      if (c == '\\')
        {
          c = phase2_getc ();
          if (c != '\n')
            {
              phase2_ungetc (c);
              /* This shouldn't happen usually, because "A backslash is
                 illegal elsewhere on a line outside a string literal."  */
              return '\\';
            }
          /* Eat backslash-newline.  */
          continuation_or_nonblank_line = true;
        }
      else if (c == '/')
        {
          c = phase2_getc ();
          if (c == '/')
            {
              /* C++ style comment.  */
              last_comment_line = line_number;
              comment_start ();
              for (;;)
                {
                  c = phase2_getc ();
                  if (c == UEOF || c == '\n')
                    {
                      comment_line_end (0);
                      break;
                    }
                  /* We skip all leading white space, but not EOLs.  */
                  if (!(comment_at_start () && (c == ' ' || c == '\t')))
                    comment_add (c);
                }
              continuation_or_nonblank_line = false;
              return c;
            }
          else if (c == '*')
            {
              /* C style comment.  */
              bool last_was_star = false;
              last_comment_line = line_number;
              comment_start ();
              for (;;)
                {
                  c = phase2_getc ();
                  if (c == UEOF)
                    break;
                  /* We skip all leading white space, but not EOLs.  */
                  if (!(comment_at_start () && (c == ' ' || c == '\t')))
                    comment_add (c);
                  switch (c)
                    {
                    case '\n':
                      comment_line_end (1);
                      comment_start ();
                      last_was_star = false;
                      continue;

                    case '*':
                      last_was_star = true;
                      continue;
                    case '/':
                      if (last_was_star)
                        {
                          comment_line_end (2);
                          break;
                        }
                      /* FALLTHROUGH */

                    default:
                      last_was_star = false;
                      continue;
                    }
                  break;
                }
              continuation_or_nonblank_line = false;
            }
          else
            {
              phase2_ungetc (c);
              return '/';
            }
        }
      else
        {
          if (c == '\n')
            continuation_or_nonblank_line = false;
          else if (!(c == ' ' || c == '\t' || c == '\f'))
            continuation_or_nonblank_line = true;
          return c;
        }
    }
}

/* Supports only one pushback character.  */
static void
phase3_ungetc (int c)
{
  phase2_ungetc (c);
}


/* ========================= Accumulating strings.  ======================== */

/* Return value of phase7_getuc when EOF is reached.  */
#define P7_EOF (-1)
#define P7_STRING_END (-2)

/* Convert an UTF-16 or UTF-32 code point to a return value that can be
   distinguished from a single-byte return value.  */
#define UNICODE(code) (0x100 + (code))

/* Test a return value of phase7_getuc whether it designates an UTF-16 or
   UTF-32 code point.  */
#define IS_UNICODE(p7_result) ((p7_result) >= 0x100)

/* Extract the UTF-16 or UTF-32 code of a return value that satisfies
   IS_UNICODE.  */
#define UNICODE_VALUE(p7_result) ((p7_result) - 0x100)


/* ========================== Reading of tokens.  ========================== */


enum token_type_ty
{
  token_type_eof,
  token_type_lparen,            /* ( */
  token_type_rparen,            /* ) */
  token_type_comma,             /* , */
  token_type_lbracket,          /* [ */
  token_type_rbracket,          /* ] */
  token_type_plus,              /* + */
  token_type_regexp,            /* /.../ */
  token_type_operator,          /* - * / % . < > = ~ ! | & ? : ^ */
  token_type_equal,             /* = */
  token_type_string,            /* "abc", 'abc' */
  token_type_keyword,           /* return, else */
  token_type_symbol,            /* symbol, number */
  token_type_other              /* misc. operator */
};
typedef enum token_type_ty token_type_ty;

typedef struct token_ty token_ty;
struct token_ty
{
  token_type_ty type;
  char *string;         /* for token_type_string, token_type_symbol,
                           token_type_keyword */
  refcounted_string_list_ty *comment;   /* for token_type_string */
  int line_number;
};


/* Free the memory pointed to by a 'struct token_ty'.  */
static inline void
free_token (token_ty *tp)
{
  if (tp->type == token_type_string || tp->type == token_type_symbol)
    free (tp->string);
  if (tp->type == token_type_string)
    drop_reference (tp->comment);
}


/* JavaScript provides strings with either double or single quotes:
     "abc" or 'abc'
   Both may contain special sequences after a backslash:
     \', \", \\, \b, \f, \n, \r, \t, \v
   Special characters can be entered using hexadecimal escape
   sequences or deprecated octal escape sequences:
     \xXX, \OOO
   Any unicode point can be entered using Unicode escape sequences:
     \uNNNN
   If a sequence after a backslash is not a legitimate character
   escape sequence, the character value is the sequence itself without
   a backslash.  For example, \xxx is treated as xxx.  */

static int
phase7_getuc (int quote_char)
{
  int c;

  for (;;)
    {
      /* Use phase 2, because phase 3 elides comments.  */
      c = phase2_getc ();

      if (c == UEOF)
        return P7_EOF;

      if (c == quote_char)
        return P7_STRING_END;

      if (c == '\n')
        {
          phase2_ungetc (c);
          error_with_progname = false;
          error (0, 0, _("%s:%d: warning: unterminated string"),
                 logical_file_name, line_number);
          error_with_progname = true;
          return P7_STRING_END;
        }

      if (c != '\\')
        return UNICODE (c);

      /* Dispatch according to the character following the backslash.  */
      c = phase2_getc ();
      if (c == UEOF)
        return P7_EOF;

      switch (c)
        {
        case '\n':
          continue;
        case 'b':
          return UNICODE ('\b');
        case 'f':
          return UNICODE ('\f');
        case 'n':
          return UNICODE ('\n');
        case 'r':
          return UNICODE ('\r');
        case 't':
          return UNICODE ('\t');
        case 'v':
          return UNICODE ('\v');
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7':
          {
            int n = c - '0';

            c = phase2_getc ();
            if (c != UEOF)
              {
                if (c >= '0' && c <= '7')
                  {
                    n = (n << 3) + (c - '0');
                    c = phase2_getc ();
                    if (c != UEOF)
                      {
                        if (c >= '0' && c <= '7')
                          n = (n << 3) + (c - '0');
                        else
                          phase2_ungetc (c);
                      }
                  }
                else
                  phase2_ungetc (c);
              }
            return UNICODE (n);
          }
        case 'x':
          {
            int c1 = phase2_getc ();
            int n1;

            if (c1 >= '0' && c1 <= '9')
              n1 = c1 - '0';
            else if (c1 >= 'A' && c1 <= 'F')
              n1 = c1 - 'A' + 10;
            else if (c1 >= 'a' && c1 <= 'f')
              n1 = c1 - 'a' + 10;
            else
              n1 = -1;

            if (n1 >= 0)
              {
                int c2 = phase2_getc ();
                int n2;

                if (c2 >= '0' && c2 <= '9')
                  n2 = c2 - '0';
                else if (c2 >= 'A' && c2 <= 'F')
                  n2 = c2 - 'A' + 10;
                else if (c2 >= 'a' && c2 <= 'f')
                  n2 = c2 - 'a' + 10;
                else
                  n2 = -1;

                if (n2 >= 0)
                  {
                    int n = (n1 << 4) + n2;
                    return UNICODE (n);
                  }

                phase2_ungetc (c2);
              }
            phase2_ungetc (c1);
            return UNICODE (c);
          }
        case 'u':
          {
            unsigned char buf[4];
            unsigned int n = 0;
            int i;

            for (i = 0; i < 4; i++)
              {
                int c1 = phase2_getc ();

                if (c1 >= '0' && c1 <= '9')
                  n = (n << 4) + (c1 - '0');
                else if (c1 >= 'A' && c1 <= 'F')
                  n = (n << 4) + (c1 - 'A' + 10);
                else if (c1 >= 'a' && c1 <= 'f')
                  n = (n << 4) + (c1 - 'a' + 10);
                else
                  {
                    phase2_ungetc (c1);
                    while (--i >= 0)
                      phase2_ungetc (buf[i]);
                    return UNICODE (c);
                  }

                buf[i] = c1;
              }
            return UNICODE (n);
          }
        default:
          return UNICODE (c);
        }
    }
}


/* Combine characters into tokens.  Discard whitespace except newlines at
   the end of logical lines.  */

static token_ty phase5_pushback[2];
static int phase5_pushback_length;

static token_type_ty last_token_type = token_type_other;

static void
phase5_scan_regexp ()
{
    int c;

    /* Scan for end of RegExp literal ('/').  */
    for (;;)
      {
        /* Must use phase2 as there can't be comments.  */
        c = phase2_getc ();
        if (c == '/')
          break;
        if (c == '\\')
          {
            c = phase2_getc ();
            if (c != UEOF)
              continue;
          }
        if (c == UEOF)
          {
            error_with_progname = false;
            error (0, 0,
                   _("%s:%d: warning: RegExp literal terminated too early"),
                   logical_file_name, line_number);
            error_with_progname = true;
            return;
          }
      }

    /* Scan for modifier flags (ECMA-262 5th section 15.10.4.1).  */
    c = phase2_getc ();
    if (!(c == 'g' || c == 'i' || c == 'm'))
      phase2_ungetc (c);
}

static int xml_element_depth = 0;
static bool inside_embedded_js_in_xml = false;

static bool
phase5_scan_xml_markup (token_ty *tp)
{
  struct
  {
    const char *start;
    const char *end;
  } markers[] =
      {
        { "!--", "--" },
        { "![CDATA[", "]]" },
        { "?", "?" }
      };
  int i;

  for (i = 0; i < SIZEOF (markers); i++)
    {
      const char *start = markers[i].start;
      const char *end = markers[i].end;
      int j;

      /* Look for a start marker.  */
      for (j = 0; start[j] != '\0'; j++)
        {
          int c;

          assert (phase2_pushback_length + j < SIZEOF (phase2_pushback));
          c = phase2_getc ();
          if (c == UEOF)
            goto eof;
          if (c != start[j])
            {
              int k = j;

              phase2_ungetc (c);
              k--;

              for (; k >= 0; k--)
                phase2_ungetc (start[k]);
              break;
            }
        }

      if (start[j] != '\0')
        continue;

      /* Skip until the end marker.  */
      for (;;)
        {
          int c;

          for (j = 0; end[j] != '\0'; j++)
            {
              assert (phase2_pushback_length + 1 < SIZEOF (phase2_pushback));
              c = phase2_getc ();
              if (c == UEOF)
                goto eof;
              if (c != end[j])
                {
                  /* Don't push the first character back so the next
                     iteration start from the second character.  */
                  if (j > 0)
                    {
                      int k = j;

                      phase2_ungetc (c);
                      k--;

                      for (; k > 0; k--)
                        phase2_ungetc (end[k]);
                    }
                  break;
                }
            }

          if (end[j] != '\0')
            continue;

          c = phase2_getc ();
          if (c == UEOF)
            goto eof;
          if (c != '>')
            {
              error_with_progname = false;
              error (0, 0,
                     _("%s:%d: warning: %s is not allowed"),
                     logical_file_name, line_number,
                     end);
              error_with_progname = true;
              return false;
            }
          return true;
        }
    }
  return false;

 eof:
  error_with_progname = false;
  error (0, 0,
         _("%s:%d: warning: unterminated XML markup"),
         logical_file_name, line_number);
  error_with_progname = true;
  return false;
}

static void
phase5_get (token_ty *tp)
{
  int c;

  if (phase5_pushback_length)
    {
      *tp = phase5_pushback[--phase5_pushback_length];
      last_token_type = tp->type;
      return;
    }

  for (;;)
    {
      tp->line_number = line_number;
      c = phase3_getc ();

      switch (c)
        {
        case UEOF:
          tp->type = last_token_type = token_type_eof;
          return;

        case '\n':
          if (last_non_comment_line > last_comment_line)
            savable_comment_reset ();
          /* FALLTHROUGH */
        case ' ':
        case '\t':
        case '\f':
          /* Ignore whitespace and comments.  */
          continue;
        }

      last_non_comment_line = tp->line_number;

      switch (c)
        {
        case '.':
          {
            int c1 = phase3_getc ();
            phase3_ungetc (c1);
            if (!(c1 >= '0' && c1 <= '9'))
              {

                tp->type = last_token_type = token_type_other;
                return;
              }
          }
          /* FALLTHROUGH */
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
            static char *buffer;
            static int bufmax;
            int bufpos;

            bufpos = 0;
            for (;;)
              {
                if (bufpos >= bufmax)
                  {
                    bufmax = 2 * bufmax + 10;
                    buffer = xrealloc (buffer, bufmax);
                  }
                buffer[bufpos++] = c;
                c = phase3_getc ();
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
                    phase3_ungetc (c);
                    break;
                  }
                break;
              }
            if (bufpos >= bufmax)
              {
                bufmax = 2 * bufmax + 10;
                buffer = xrealloc (buffer, bufmax);
              }
            buffer[bufpos] = '\0';
            tp->string = xstrdup (buffer);
            if (strcmp (buffer, "return") == 0
                || strcmp (buffer, "else") == 0)
              tp->type = last_token_type = token_type_keyword;
            else
              tp->type = last_token_type = token_type_symbol;
            return;
          }

        /* Strings.  */
          {
            struct mixed_string_buffer *bp;
            int quote_char;

            case '"': case '\'':
              quote_char = c;
              lexical_context = lc_string;
              /* Start accumulating the string.  */
              bp = mixed_string_buffer_alloc (lexical_context,
                                              logical_file_name,
                                              line_number);
              for (;;)
                {
                  int uc = phase7_getuc (quote_char);

                  /* Keep line_number in sync.  */
                  bp->line_number = line_number;

                  if (uc == P7_EOF || uc == P7_STRING_END)
                    break;

                  if (IS_UNICODE (uc))
                    {
                      assert (UNICODE_VALUE (uc) >= 0
                              && UNICODE_VALUE (uc) < 0x110000);
                      mixed_string_buffer_append_unicode (bp,
                                                          UNICODE_VALUE (uc));
                    }
                  else
                    mixed_string_buffer_append_char (bp, uc);
                }
              tp->string = mixed_string_buffer_done (bp);
              tp->comment = add_reference (savable_comment);
              lexical_context = lc_outside;
              tp->type = last_token_type = token_type_string;
              return;
          }

        case '+':
          tp->type = last_token_type = token_type_plus;
          return;

        /* Identify operators. The multiple character ones are simply ignored
         * as they are recognized here and are otherwise not relevant. */
        case '-': case '*': /* '+' and '/' are not listed here! */
        case '%':
        case '~': case '!': case '|': case '&': case '^':
        case '?': case ':':
          tp->type = last_token_type = token_type_operator;
          return;

        case '=':
          tp->type = last_token_type = token_type_equal;
          return;

        case '<':
          {
            /* We assume:
               - XMLMarkup and XMLElement are only allowed after '=' or '('
               - embedded JavaScript expressions in XML do not recurse
             */
            if (xml_element_depth > 0
                || (!inside_embedded_js_in_xml
                    && (last_token_type == token_type_equal
                        || last_token_type == token_type_lparen)))
              {
                /* Comments, PI, or CDATA.  */
                if (phase5_scan_xml_markup (tp))
                  return;
                c = phase2_getc ();

                /* Closing tag.  */
                if (c == '/')
                  lexical_context = lc_xml_close_tag;

                /* Opening element.  */
                else
                  {
                    phase2_ungetc (c);
                    lexical_context = lc_xml_open_tag;
                    xml_element_depth++;
                  }

                tp->type = last_token_type = token_type_other;
              }
            else
              tp->type = last_token_type = token_type_operator;
          }
          return;

        case '>':
          if (xml_element_depth > 0 && !inside_embedded_js_in_xml)
            {
              switch (lexical_context)
                {
                case lc_xml_open_tag:
                  lexical_context = lc_xml_content;
                  break;

                case lc_xml_close_tag:
                  if (xml_element_depth-- > 0)
                    lexical_context = lc_xml_content;
                  else
                    lexical_context = lc_outside;
                  break;

                default:
                  break;
                }
              tp->type = last_token_type = token_type_other;
            }
          else
            tp->type = last_token_type = token_type_operator;
          return;

        case '/':
          if (xml_element_depth > 0 && !inside_embedded_js_in_xml)
            {
              /* If it appears in an opening tag of an XML element, it's
                 part of '/>'.  */
              if (lexical_context == lc_xml_open_tag)
                {
                  c = phase2_getc ();
                  if (c == '>')
                    lexical_context = lc_outside;
                  else
                    phase2_ungetc (c);
                }
              tp->type = last_token_type = token_type_other;
              return;
            }

          /* Either a division operator or the start of a regular
             expression literal.  If the '/' token is spotted after a
             symbol it's a division, otherwise it's a regular
             expression.  */
          if (last_token_type == token_type_symbol
              || last_token_type == token_type_rparen
              || last_token_type == token_type_rbracket)
            tp->type = last_token_type = token_type_operator;
          else
            {
              phase5_scan_regexp (tp);
              tp->type = last_token_type = token_type_regexp;
            }
          return;

        case '{':
          if (xml_element_depth > 0 && !inside_embedded_js_in_xml)
            inside_embedded_js_in_xml = true;
          tp->type = last_token_type = token_type_other;
          return;

        case '}':
          if (xml_element_depth > 0 && inside_embedded_js_in_xml)
            inside_embedded_js_in_xml = false;
          tp->type = last_token_type = token_type_other;
          return;

        case '(':
          tp->type = last_token_type = token_type_lparen;
          return;

        case ')':
          tp->type = last_token_type = token_type_rparen;
          return;

        case ',':
          tp->type = last_token_type = token_type_comma;
          return;

        case '[':
          tp->type = last_token_type = token_type_lbracket;
          return;

        case ']':
          tp->type = last_token_type = token_type_rbracket;
          return;

        default:
          /* We could carefully recognize each of the 2 and 3 character
             operators, but it is not necessary, as we only need to recognize
             gettext invocations.  Don't bother.  */
          tp->type = last_token_type = token_type_other;
          return;
        }
    }
}

/* Supports only one pushback token.  */
static void
phase5_unget (token_ty *tp)
{
  if (tp->type != token_type_eof)
    {
      if (phase5_pushback_length == SIZEOF (phase5_pushback))
        abort ();
      phase5_pushback[phase5_pushback_length++] = *tp;
    }
}


/* String concatenation with '+'.  */

static void
x_javascript_lex (token_ty *tp)
{
  phase5_get (tp);
  if (tp->type == token_type_string)
    {
      char *sum = tp->string;
      size_t sum_len = strlen (sum);

      for (;;)
        {
          token_ty token2;

          phase5_get (&token2);
          if (token2.type == token_type_plus)
            {
              token_ty token3;

              phase5_get (&token3);
              if (token3.type == token_type_string)
                {
                  char *addend = token3.string;
                  size_t addend_len = strlen (addend);

                  sum = (char *) xrealloc (sum, sum_len + addend_len + 1);
                  memcpy (sum + sum_len, addend, addend_len + 1);
                  sum_len += addend_len;

                  free_token (&token3);
                  free_token (&token2);
                  continue;
                }
              phase5_unget (&token3);
            }
          phase5_unget (&token2);
          break;
        }
      tp->string = sum;
    }
}


/* ========================= Extracting strings.  ========================== */


/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;


/* The file is broken into tokens.  Scan the token stream, looking for
   a keyword, followed by a left paren, followed by a string.  When we
   see this sequence, we have something to remember.  We assume we are
   looking at a valid JavaScript program, and leave the complaints about
   the grammar to the compiler.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */


/* Extract messages until the next balanced closing parenthesis or bracket.
   Extracted messages are added to MLP.
   DELIM can be either token_type_rparen or token_type_rbracket, or
   token_type_eof to accept both.
   Return true upon eof, false upon closing parenthesis or bracket.  */
static bool
extract_balanced (message_list_ty *mlp,
                  token_type_ty delim,
                  flag_context_ty outer_context,
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
  /* Current context.  */
  flag_context_ty inner_context =
    inherited_context (outer_context,
                       flag_context_list_iterator_advance (&context_iter));

  /* Start state is 0.  */
  state = 0;

  for (;;)
    {
      token_ty token;

      x_javascript_lex (&token);
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
          continue;

        case token_type_lparen:
          if (extract_balanced (mlp, token_type_rparen,
                                inner_context, next_context_iter,
                                arglist_parser_alloc (mlp,
                                                      state ? next_shapes : NULL)))
            {
              xgettext_current_source_encoding = po_charset_utf8;
              arglist_parser_done (argparser, arg);
              xgettext_current_source_encoding = xgettext_current_file_source_encoding;
              return true;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          continue;

        case token_type_rparen:
          if (delim == token_type_rparen || delim == token_type_eof)
            {
              xgettext_current_source_encoding = po_charset_utf8;
              arglist_parser_done (argparser, arg);
              xgettext_current_source_encoding = xgettext_current_file_source_encoding;
              return false;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          continue;

        case token_type_comma:
          arg++;
          inner_context =
            inherited_context (outer_context,
                               flag_context_list_iterator_advance (
                                 &context_iter));
          next_context_iter = passthrough_context_list_iterator;
          state = 0;
          continue;

        case token_type_lbracket:
          if (extract_balanced (mlp, token_type_rbracket,
                                null_context, null_context_list_iterator,
                                arglist_parser_alloc (mlp, NULL)))
            {
              xgettext_current_source_encoding = po_charset_utf8;
              arglist_parser_done (argparser, arg);
              xgettext_current_source_encoding = xgettext_current_file_source_encoding;
              return true;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          continue;

        case token_type_rbracket:
          if (delim == token_type_rbracket || delim == token_type_eof)
            {
              xgettext_current_source_encoding = po_charset_utf8;
              arglist_parser_done (argparser, arg);
              xgettext_current_source_encoding = xgettext_current_file_source_encoding;
              return false;
            }
          next_context_iter = null_context_list_iterator;
          state = 0;
          continue;

        case token_type_string:
          {
            lex_pos_ty pos;
            pos.file_name = logical_file_name;
            pos.line_number = token.line_number;

            xgettext_current_source_encoding = po_charset_utf8;
            if (extract_all)
              remember_a_message (mlp, NULL, token.string, inner_context,
                                  &pos, NULL, token.comment);
            else
              arglist_parser_remember (argparser, arg, token.string,
                                       inner_context,
                                       pos.file_name, pos.line_number,
                                       token.comment);
            xgettext_current_source_encoding = xgettext_current_file_source_encoding;
          }
          drop_reference (token.comment);
          next_context_iter = null_context_list_iterator;
          state = 0;
          continue;

        case token_type_eof:
          xgettext_current_source_encoding = po_charset_utf8;
          arglist_parser_done (argparser, arg);
          xgettext_current_source_encoding = xgettext_current_file_source_encoding;
          return true;

        case token_type_keyword:
        case token_type_plus:
        case token_type_regexp:
        case token_type_operator:
        case token_type_equal:
        case token_type_other:
          next_context_iter = null_context_list_iterator;
          state = 0;
          continue;

        default:
          abort ();
        }
    }
}


void
extract_javascript (FILE *f,
                const char *real_filename, const char *logical_filename,
                flag_context_list_table_ty *flag_table,
                msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  fp = f;
  real_file_name = real_filename;
  logical_file_name = xstrdup (logical_filename);
  line_number = 1;

  lexical_context = lc_outside;

  last_comment_line = -1;
  last_non_comment_line = -1;

  xml_element_depth = 0;

  xgettext_current_file_source_encoding = xgettext_global_source_encoding;
#if HAVE_ICONV
  xgettext_current_file_source_iconv = xgettext_global_source_iconv;
#endif

  xgettext_current_source_encoding = xgettext_current_file_source_encoding;
#if HAVE_ICONV
  xgettext_current_source_iconv = xgettext_current_file_source_iconv;
#endif

  continuation_or_nonblank_line = false;

  flag_context_list_table = flag_table;

  init_keywords ();

  /* Eat tokens until eof is seen.  When extract_balanced returns
     due to an unbalanced closing parenthesis, just restart it.  */
  while (!extract_balanced (mlp, token_type_eof,
                            null_context, null_context_list_iterator,
                            arglist_parser_alloc (mlp, NULL)))
    ;

  fp = NULL;
  real_file_name = NULL;
  logical_file_name = NULL;
  line_number = 0;
}
