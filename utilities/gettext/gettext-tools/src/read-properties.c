/* Reading Java .properties files.
   Copyright (C) 2003-2026 Free Software Foundation, Inc.

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
#include "read-properties.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_NO_APPENDF
#include <error.h>
#include "message.h"
#include "read-catalog-abstract.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "string-buffer.h"
#include "xstrerror.h"
#include "xerror-handler.h"
#include "msgl-ascii.h"
#include "read-file.h"
#include "unistr.h"
#include "gettext.h"

#define _(str) gettext (str)


/* The format of the Java .properties files is documented in the JDK
   documentation for class java.util.Properties.  In the case of .properties
   files for PropertyResourceBundle, each non-comment line contains a
   key/value pair in the form "key = value" or "key : value" or "key value",
   where the key is the msgid and the value is the msgstr.  Messages with
   plurals are not supported in this format.

   The encoding of Java .properties files is:
     - ASCII with Java \uxxxx escape sequences,
     - ISO-8859-1 if non-ASCII bytes are encountered,
     - UTF-8 if non-ASCII bytes are encountered and the entire file is
       valid UTF-8 (in Java 9 or newer), see
       https://docs.oracle.com/javase/9/intl/internationalization-enhancements-jdk-9.htm */

/* Handling of comments: We copy all comments from the .properties file to
   the PO file. This is not really needed; it's a service for translators
   who don't like PO files and prefer to maintain the .properties file.  */

/* Real filename, used in error messages about the input file.  */
static const char *real_file_name;

/* File name and line number.  */
static lex_pos_ty pos;

/* The contents of the input file.  */
static char *contents;
static size_t contents_length;

/* True if the input file is assumed to be in UTF-8 encoding.
   False if it is assumed to be in ISO-8859-1 encoding.  */
static bool assume_utf8;

/* Current position in contents.  */
static size_t position;

/* Phase 1: Read an input byte.
   Max. 1 pushback byte.  */

static int
phase1_getc ()
{
  if (position == contents_length)
    return EOF;

  return (unsigned char) contents[position++];
}

static inline void
phase1_ungetc (int c)
{
  if (c != EOF)
    position--;
}


/* Phase 2: Read an input byte, treating CR/LF like a single LF.
   Max. 2 pushback bytes.  */

static unsigned char phase2_pushback[2];
static int phase2_pushback_length;

static int
phase2_getc ()
{
  int c;

  if (phase2_pushback_length)
    c = phase2_pushback[--phase2_pushback_length];
  else
    {
      c = phase1_getc ();

      if (c == '\r')
        {
          int c2 = phase1_getc ();
          if (c2 == '\n')
            c = c2;
          else
            phase1_ungetc (c2);
        }
    }

  if (c == '\n')
    pos.line_number++;

  return c;
}

static void
phase2_ungetc (int c)
{
  if (c == '\n')
    --pos.line_number;
  if (c != EOF)
    phase2_pushback[phase2_pushback_length++] = c;
}


/* Phase 3: Read an input byte, treating CR/LF like a single LF,
   with handling of continuation lines.
   Max. 1 pushback character.  */

static int
phase3_getc ()
{
  int c = phase2_getc ();

  for (;;)
    {
      if (c != '\\')
        return c;

      c = phase2_getc ();
      if (c != '\n')
        {
          phase2_ungetc (c);
          return '\\';
        }

      /* Skip the backslash-newline and all whitespace that follows it.  */
      do
        c = phase2_getc ();
      while (c == ' ' || c == '\t' || c == '\r' || c == '\f');
    }
}

static inline void
phase3_ungetc (int c)
{
  phase2_ungetc (c);
}


/* Converts a string from ISO-8859-1 encoding to UTF-8 encoding.  */
static char *
conv_from_iso_8859_1 (char *string)
{
  if (is_ascii_string (string))
    return string;
  else
    {
      size_t length = strlen (string);
      /* Each ISO-8859-1 character needs 2 bytes at worst.  */
      unsigned char *utf8_string = XNMALLOC (2 * length + 1, unsigned char);
      {
        unsigned char *q = utf8_string;
        const char *str = string;
        const char *str_limit = str + length;
        while (str < str_limit)
          {
            unsigned int uc = (unsigned char) *str++;
            int n = u8_uctomb (q, uc, 6);
            assert (n > 0);
            q += n;
          }
        *q = '\0';
        assert (q - utf8_string <= 2 * length);
      }

      return (char *) utf8_string;
    }
}


/* Converts a string from JAVA encoding (with \uxxxx sequences) to UTF-8
   encoding.  May destructively modify the argument string.  */
static char *
conv_from_java (char *string)
{
  /* This conversion can only shrink the string, never increase its size.
     So there is no need to xmalloc the result freshly.  */
  const char *p = string;
  unsigned char *q = (unsigned char *) string;

  while (*p != '\0')
    {
      if (p[0] == '\\' && p[1] == 'u')
        {
          unsigned int n = 0;
          int i;

          for (i = 0; i < 4; i++)
            {
              int c1 = (unsigned char) p[2 + i];

              if (c1 >= '0' && c1 <= '9')
                n = (n << 4) + (c1 - '0');
              else if (c1 >= 'A' && c1 <= 'F')
                n = (n << 4) + (c1 - 'A' + 10);
              else if (c1 >= 'a' && c1 <= 'f')
                n = (n << 4) + (c1 - 'a' + 10);
              else
                goto just_one_byte;
            }

          if (i == 4)
            {
              unsigned int uc;

              if (n >= 0xd800 && n < 0xdc00)
                {
                  if (p[6] == '\\' && p[7] == 'u')
                    {
                      unsigned int m = 0;
                      int i2;

                      for (i2 = 0; i2 < 4; i2++)
                        {
                          int c1 = (unsigned char) p[8 + i2];

                          if (c1 >= '0' && c1 <= '9')
                            m = (m << 4) + (c1 - '0');
                          else if (c1 >= 'A' && c1 <= 'F')
                            m = (m << 4) + (c1 - 'A' + 10);
                          else if (c1 >= 'a' && c1 <= 'f')
                            m = (m << 4) + (c1 - 'a' + 10);
                          else
                            goto just_one_byte;
                        }

                      if (i2 == 4 && (m >= 0xdc00 && m < 0xe000))
                        {
                          /* Combine two UTF-16 words to a character.  */
                          uc = 0x10000 + ((n - 0xd800) << 10) + (m - 0xdc00);
                          p += 12;
                        }
                      else
                        goto just_one_byte;
                    }
                  else
                    goto just_one_byte;
                }
              else
                {
                  uc = n;
                  p += 6;
                }

              q += u8_uctomb (q, uc, 6);
              continue;
            }
        }
      just_one_byte:
        *q++ = (unsigned char) *p++;
    }
  *q = '\0';
  return string;
}


/* Phase 4: Read the next single byte or UTF-16 code point,
   treating CR/LF like a single LF, with handling of continuation lines
   and of \uxxxx sequences.  */

/* Return value of phase 4 when EOF is reached.  */
#define P4_EOF 0xffff

/* Convert an UTF-16 code point to a return value that can be distinguished
   from a single-byte return value.  */
#define UNICODE(code) (0x10000 + (code))

/* Test a return value of phase 4 whether it designates an UTF-16 code
   point.  */
#define IS_UNICODE(p4_result) ((p4_result) >= 0x10000)

/* Extract the UTF-16 code of a return value that satisfies IS_UNICODE.  */
#define UTF16_VALUE(p4_result) ((unsigned short) ((p4_result) - 0x10000))

static int
phase4_getuc (abstract_catalog_reader_ty *catr)
{
  int c = phase3_getc ();

  if (c == EOF)
    return P4_EOF;
  if (c == '\\')
    {
      int c2 = phase3_getc ();

      if (c2 == 't')
        return '\t';
      if (c2 == 'n')
        return '\n';
      if (c2 == 'r')
        return '\r';
      if (c2 == 'f')
        return '\f';
      if (c2 == 'u')
        {
          unsigned int n = 0;

          for (int i = 0; i < 4; i++)
            {
              int c1 = phase3_getc ();

              if (c1 >= '0' && c1 <= '9')
                n = (n << 4) + (c1 - '0');
              else if (c1 >= 'A' && c1 <= 'F')
                n = (n << 4) + (c1 - 'A' + 10);
              else if (c1 >= 'a' && c1 <= 'f')
                n = (n << 4) + (c1 - 'a' + 10);
              else
                {
                  phase3_ungetc (c1);
                  catr->xeh->xerror (CAT_SEVERITY_ERROR, NULL,
                                     real_file_name, pos.line_number, (size_t)(-1),
                                     false,
                                     _("warning: invalid \\uxxxx syntax for Unicode character"));
                  return 'u';
                }
            }
          return UNICODE (n);
        }

      return c2;
    }
  else
    return c;
}


/* Reads a key or value string.
   Returns the string in UTF-8 encoding, or NULL if the end of the logical
   line is reached.
   Parsing ends:
     - when returning NULL, after the end of the logical line,
     - otherwise, if in_key is true, after the whitespace and possibly the
       separator that follows after the string,
     - otherwise, if in_key is false, after the end of the logical line. */

static char *
read_escaped_string (abstract_catalog_reader_ty *catr, bool in_key)
{
  /* The part of the string that has already been converted to UTF-8.  */
  static unsigned char *utf8_buffer;
  static size_t utf8_buflen;
  static size_t utf8_allocated;
  /* The first half of an UTF-16 surrogate character.  */
  unsigned short utf16_surr;
  /* Line in which this surrogate character occurred.  */
  size_t utf16_surr_line;

  /* Ensures utf8_buffer has room for N bytes.  N must be <= 10.  */
  #define utf8_buffer_ensure_available(n)  \
    do                                                                        \
      {                                                                       \
        if (utf8_buflen + (n) > utf8_allocated)                               \
          {                                                                   \
            utf8_allocated = 2 * utf8_allocated + 10;                         \
            utf8_buffer =                                                     \
              (unsigned char *) xrealloc (utf8_buffer, utf8_allocated);       \
          }                                                                   \
      }                                                                       \
    while (0)

  /* Appends a lone surrogate to utf8_buffer.  */
  /* Note: A half surrogate is invalid in UTF-8:
     - RFC 3629 says
         "The definition of UTF-8 prohibits encoding character
          numbers between U+D800 and U+DFFF".
     - Unicode 4.0 chapter 3
       <https://www.unicode.org/versions/Unicode4.0.0/ch03.pdf>
       section 3.9, p.77, says
         "Because surrogate code points are not Unicode scalar
          values, any UTF-8 byte sequence that would otherwise
          map to code points D800..DFFF is ill-formed."
       and in table 3-6, p. 78, does not mention D800..DFFF.
     - The unicode.org FAQ question "How do I convert an unpaired
       UTF-16 surrogate to UTF-8?" has the answer
         "By representing such an unpaired surrogate on its own
          as a 3-byte sequence, the resulting UTF-8 data stream
          would become ill-formed."
     So use U+FFFD instead.  */
  #define utf8_buffer_append_lone_surrogate(uc, line) \
    do                                                                        \
      {                                                                       \
        catr->xeh->xerror (CAT_SEVERITY_ERROR, NULL,                          \
                           real_file_name, (line), (size_t)(-1), false,       \
                           xasprintf (_("warning: lone surrogate U+%04X"),    \
                                      (uc)));                                 \
        utf8_buffer_ensure_available (3);                                     \
        utf8_buffer[utf8_buflen++] = 0xef;                                    \
        utf8_buffer[utf8_buflen++] = 0xbf;                                    \
        utf8_buffer[utf8_buflen++] = 0xbd;                                    \
      }                                                                       \
    while (0)

  int c;

  /* Skip whitespace before the string.  */
  do
    c = phase3_getc ();
  while (c == ' ' || c == '\t' || c == '\r' || c == '\f');

  if (c == EOF || c == '\n')
    /* Empty string.  */
    return NULL;

  /* Start accumulating the string.  */
  utf8_buflen = 0;
  utf16_surr = 0;
  utf16_surr_line = 0;
  for (;;)
    {
      if (in_key && (c == '=' || c == ':'
                     || c == ' ' || c == '\t' || c == '\r' || c == '\f'))
        {
          /* Skip whitespace after the string.  */
          while (c == ' ' || c == '\t' || c == '\r' || c == '\f')
            c = phase3_getc ();
          /* Skip '=' or ':' separator.  */
          if (!(c == '=' || c == ':'))
            phase3_ungetc (c);
          break;
        }

      phase3_ungetc (c);

      /* Read the next byte or UTF-16 code point.  */
      c = phase4_getuc (catr);
      if (c == P4_EOF)
        break;

      /* Append it to the buffer.  */
      if (IS_UNICODE (c))
        {
          /* Append an UTF-16 code point.  */
          /* Test whether this character and the previous one form a Unicode
             surrogate pair.  */
          if (utf16_surr != 0
              && (c >= UNICODE (0xdc00) && c < UNICODE (0xe000)))
            {
              unsigned short utf16buf[2];
              utf16buf[0] = utf16_surr;
              utf16buf[1] = UTF16_VALUE (c);

              ucs4_t uc;
              if (u16_mbtouc (&uc, utf16buf, 2) != 2)
                abort ();

              utf8_buffer_ensure_available (6);
              int len = u8_uctomb (utf8_buffer + utf8_buflen, uc, 6);
              if (len < 0)
                catr->xeh->xerror (CAT_SEVERITY_ERROR, NULL,
                                   real_file_name, pos.line_number, (size_t)(-1),
                                   false,
                                   _("warning: invalid Unicode character"));
              else
                utf8_buflen += len;

              utf16_surr = 0;
            }
          else
            {
              if (utf16_surr != 0)
                {
                  utf8_buffer_append_lone_surrogate (utf16_surr, utf16_surr_line);
                  utf16_surr = 0;
                }

              if (c >= UNICODE (0xd800) && c < UNICODE (0xdc00))
                {
                  utf16_surr = UTF16_VALUE (c);
                  utf16_surr_line = pos.line_number;
                }
              else if (c >= UNICODE (0xdc00) && c < UNICODE (0xe000))
                utf8_buffer_append_lone_surrogate (UTF16_VALUE (c), pos.line_number);
              else
                {
                  ucs4_t uc = UTF16_VALUE (c);

                  utf8_buffer_ensure_available (3);
                  int len = u8_uctomb (utf8_buffer + utf8_buflen, uc, 3);
                  if (len < 0)
                    catr->xeh->xerror (CAT_SEVERITY_ERROR, NULL,
                                       real_file_name, pos.line_number, (size_t)(-1),
                                       false,
                                       _("warning: invalid Unicode character"));
                  else
                    utf8_buflen += len;
                }
            }
        }
      else
        {
          /* Append a single byte.  */
          if (utf16_surr != 0)
            {
              utf8_buffer_append_lone_surrogate (utf16_surr, utf16_surr_line);
              utf16_surr = 0;
            }

          if (assume_utf8)
            {
              /* No conversion needed.  */
              utf8_buffer_ensure_available (1);
              utf8_buffer[utf8_buflen++] = c;
            }
          else
            {
              /* Convert the byte from ISO-8859-1 to UTF-8 on the fly.  */
              ucs4_t uc = c;

              utf8_buffer_ensure_available (2);
              int len = u8_uctomb (utf8_buffer + utf8_buflen, uc, 2);
              if (len < 0)
                abort ();
              utf8_buflen += len;
            }
        }

      c = phase3_getc ();
      if (c == EOF || c == '\n')
        {
          if (in_key)
            phase3_ungetc (c);
          break;
        }
    }
  if (utf16_surr != 0)
    utf8_buffer_append_lone_surrogate (utf16_surr, utf16_surr_line);

  /* Return the result.  */
  {
    unsigned char *utf8_string = XNMALLOC (utf8_buflen + 1, unsigned char);
    if (utf8_buflen > 0)
      memcpy (utf8_string, utf8_buffer, utf8_buflen);
    utf8_string[utf8_buflen] = '\0';

    return (char *) utf8_string;
  }
  #undef utf8_buffer_append_lone_surrogate
  #undef utf8_buffer_ensure_available
}


/* Read a .properties file from a stream, and dispatch to the various
   abstract_catalog_reader_class_ty methods.  */
static void
properties_parse (abstract_catalog_reader_ty *catr, FILE *file,
                  const char *real_filename, const char *logical_filename,
                  bool is_pot_role, string_list_ty *arena)
{
  /* Read the file into memory.  */
  contents = fread_file (file, 0, &contents_length);
  if (contents == NULL)
    {
      int err = errno;
      catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                         xstrerror (xasprintf (_("error while reading \"%s\""),
                                               real_filename),
                                    err));
      return;
    }

  /* Test whether it's valid UTF-8.  */
  assume_utf8 = (u8_check ((uint8_t *) contents, contents_length) == NULL);

  position = 0;
  real_file_name = real_filename;
  pos.file_name = xstrdup (real_file_name);
  pos.line_number = 1;

  for (;;)
    {
      int c;

      c = phase2_getc ();

      if (c == EOF)
        break;

      bool comment = false;
      bool hidden = false;
      if (c == '#')
        comment = true;
      else if (c == '!')
        {
          /* For compatibility with write-properties.c, we treat '!' not
             followed by space as a fuzzy or untranslated message.  */
          int c2 = phase2_getc ();
          if (c2 == ' ' || c2 == '\n' || c2 == EOF)
            comment = true;
          else
            hidden = true;
          phase2_ungetc (c2);
        }
      else
        phase2_ungetc (c);

      if (comment)
        {
          /* A comment line.  */
          struct string_buffer buffer;
          sb_init (&buffer);
          for (;;)
            {
              c = phase2_getc ();
              if (c == EOF || c == '\n')
                break;

              sb_xappend1 (&buffer, c);
            }
          char *contents = sb_xdupfree_c (&buffer);

          catalog_reader_seen_generic_comment (
            catr,
            conv_from_java (
              assume_utf8 ? contents : conv_from_iso_8859_1 (contents)));
        }
      else
        {
          /* A key/value pair.  */
          lex_pos_ty msgid_pos = pos;
          char *msgid = read_escaped_string (catr, true);
          if (msgid == NULL)
            /* Skip blank line.  */
            ;
          else
            {
              lex_pos_ty msgstr_pos = pos;
              char *msgstr = read_escaped_string (catr, false);
              if (msgstr == NULL)
                msgstr = xstrdup ("");

              /* Be sure to make the message fuzzy if it was commented out
                 and if it is not already header/fuzzy/untranslated.  */
              bool force_fuzzy = (hidden && msgid[0] != '\0' && msgstr[0] != '\0');

              catalog_reader_seen_message (catr,
                                           NULL, msgid, &msgid_pos, NULL,
                                           msgstr, strlen (msgstr) + 1, &msgstr_pos,
                                           NULL, NULL, NULL,
                                           force_fuzzy, false);
            }
        }
    }

  free (contents);
  contents = NULL;
  real_file_name = NULL;
  pos.line_number = 0;
}

const struct catalog_input_format input_format_properties =
{
  properties_parse,                     /* parse */
  true                                  /* produces_utf8 */
};
