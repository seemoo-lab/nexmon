/* Reading Java .properties files.
   Copyright (C) 2003, 2005-2007, 2009, 2015-2016 Free Software Foundation,
   Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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
# include <config.h>
#endif

/* Specification.  */
#include "read-properties.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "error-progname.h"
#include "message.h"
#include "read-catalog-abstract.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "po-xerror.h"
#include "msgl-ascii.h"
#include "unistr.h"
#include "gettext.h"

#define _(str) gettext (str)

/* For compiling this file in C++ mode.  */
#ifdef __cplusplus
# define this thiss
#endif


/* The format of the Java .properties files is documented in the JDK
   documentation for class java.util.Properties.  In the case of .properties
   files for PropertyResourceBundle, each non-comment line contains a
   key/value pair in the form "key = value" or "key : value" or "key value",
   where the key is the msgid and the value is the msgstr.  Messages with
   plurals are not supported in this format.  */

/* Handling of comments: We copy all comments from the .properties file to
   the PO file. This is not really needed; it's a service for translators
   who don't like PO files and prefer to maintain the .properties file.  */

/* Real filename, used in error messages about the input file.  */
static const char *real_file_name;

/* File name and line number.  */
extern lex_pos_ty gram_pos;

/* The input file stream.  */
static FILE *fp;


/* Phase 1: Read an ISO-8859-1 character.
   Max. 1 pushback character.  */

static int
phase1_getc ()
{
  int c;

  c = getc (fp);

  if (c == EOF)
    {
      if (ferror (fp))
        {
          const char *errno_description = strerror (errno);
          po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                     xasprintf ("%s: %s",
                                xasprintf (_("error while reading \"%s\""),
                                           real_file_name),
                                errno_description));
        }
      return EOF;
    }

  return c;
}

static inline void
phase1_ungetc (int c)
{
  if (c != EOF)
    ungetc (c, fp);
}


/* Phase 2: Read an ISO-8859-1 character, treating CR/LF like a single LF.
   Max. 2 pushback characters.  */

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
    gram_pos.line_number++;

  return c;
}

static void
phase2_ungetc (int c)
{
  if (c == '\n')
    --gram_pos.line_number;
  if (c != EOF)
    phase2_pushback[phase2_pushback_length++] = c;
}


/* Phase 3: Read an ISO-8859-1 character, treating CR/LF like a single LF,
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


/* Phase 4: Read an UTF-16 codepoint, treating CR/LF like a single LF,
   with handling of continuation lines and of \uxxxx sequences.  */

static int
phase4_getuc ()
{
  int c = phase3_getc ();

  if (c == EOF)
    return -1;
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
          int i;

          for (i = 0; i < 4; i++)
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
                  po_xerror (PO_SEVERITY_ERROR, NULL,
                             real_file_name, gram_pos.line_number, (size_t)(-1),
                             false, _("warning: invalid \\uxxxx syntax for Unicode character"));
                  return 'u';
                }
            }
          return n;
        }

      return c2;
    }
  else
    return c;
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

                      for (i = 0; i < 4; i++)
                        {
                          int c1 = (unsigned char) p[8 + i];

                          if (c1 >= '0' && c1 <= '9')
                            m = (m << 4) + (c1 - '0');
                          else if (c1 >= 'A' && c1 <= 'F')
                            m = (m << 4) + (c1 - 'A' + 10);
                          else if (c1 >= 'a' && c1 <= 'f')
                            m = (m << 4) + (c1 - 'a' + 10);
                          else
                            goto just_one_byte;
                        }

                      if (i == 4 && (m >= 0xdc00 && m < 0xe000))
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


/* Reads a key or value string.
   Returns the string in UTF-8 encoding, or NULL if the end of the logical
   line is reached.
   Parsing ends:
     - when returning NULL, after the end of the logical line,
     - otherwise, if in_key is true, after the whitespace and possibly the
       separator that follows after the string,
     - otherwise, if in_key is false, after the end of the logical line. */

static char *
read_escaped_string (bool in_key)
{
  static unsigned short *buffer;
  static size_t bufmax;
  static size_t buflen;
  int c;

  /* Skip whitespace before the string.  */
  do
    c = phase3_getc ();
  while (c == ' ' || c == '\t' || c == '\r' || c == '\f');

  if (c == EOF || c == '\n')
    /* Empty string.  */
    return NULL;

  /* Start accumulating the string.  We store the string in UTF-16 before
     converting it to UTF-8.  Why not converting every character directly to
     UTF-8? Because a string can contain surrogates like \uD800\uDF00, and
     we must combine them to a single UTF-8 character.  */
  buflen = 0;
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

      /* Read the next UTF-16 codepoint.  */
      c = phase4_getuc ();
      if (c < 0)
        break;
      /* Append it to the buffer.  */
      if (buflen >= bufmax)
        {
          bufmax += 100;
          buffer = xrealloc (buffer, bufmax * sizeof (unsigned short));
        }
      buffer[buflen++] = c;

      c = phase3_getc ();
      if (c == EOF || c == '\n')
        {
          if (in_key)
            phase3_ungetc (c);
          break;
        }
    }

  /* Now convert from UTF-16 to UTF-8.  */
  {
    size_t pos;
    unsigned char *utf8_string;
    unsigned char *q;

    /* Each UTF-16 word needs 3 bytes at worst.  */
    utf8_string = XNMALLOC (3 * buflen + 1, unsigned char);
    for (pos = 0, q = utf8_string; pos < buflen; )
      {
        ucs4_t uc;
        int n;

        pos += u16_mbtouc (&uc, buffer + pos, buflen - pos);
        n = u8_uctomb (q, uc, 6);
        assert (n > 0);
        q += n;
      }
    *q = '\0';
    assert (q - utf8_string <= 3 * buflen);

    return (char *) utf8_string;
  }
}


/* Read a .properties file from a stream, and dispatch to the various
   abstract_catalog_reader_class_ty methods.  */
static void
properties_parse (abstract_catalog_reader_ty *this, FILE *file,
                  const char *real_filename, const char *logical_filename)
{
  fp = file;
  real_file_name = real_filename;
  gram_pos.file_name = xstrdup (real_file_name);
  gram_pos.line_number = 1;

  for (;;)
    {
      int c;
      bool comment;
      bool hidden;

      c = phase2_getc ();

      if (c == EOF)
        break;

      comment = false;
      hidden = false;
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
          static char *buffer;
          static size_t bufmax;
          static size_t buflen;

          buflen = 0;
          for (;;)
            {
              c = phase2_getc ();

              if (buflen >= bufmax)
                {
                  bufmax += 100;
                  buffer = xrealloc (buffer, bufmax);
                }

              if (c == EOF || c == '\n')
                break;

              buffer[buflen++] = c;
            }
          buffer[buflen] = '\0';

          po_callback_comment_dispatcher (conv_from_java (conv_from_iso_8859_1 (buffer)));
        }
      else
        {
          /* A key/value pair.  */
          char *msgid;
          lex_pos_ty msgid_pos;

          msgid_pos = gram_pos;
          msgid = read_escaped_string (true);
          if (msgid == NULL)
            /* Skip blank line.  */
            ;
          else
            {
              char *msgstr;
              lex_pos_ty msgstr_pos;
              bool force_fuzzy;

              msgstr_pos = gram_pos;
              msgstr = read_escaped_string (false);
              if (msgstr == NULL)
                msgstr = xstrdup ("");

              /* Be sure to make the message fuzzy if it was commented out
                 and if it is not already header/fuzzy/untranslated.  */
              force_fuzzy = (hidden && msgid[0] != '\0' && msgstr[0] != '\0');

              po_callback_message (NULL, msgid, &msgid_pos, NULL,
                                   msgstr, strlen (msgstr) + 1, &msgstr_pos,
                                   NULL, NULL, NULL,
                                   force_fuzzy, false);
            }
        }
    }

  fp = NULL;
  real_file_name = NULL;
  gram_pos.line_number = 0;
}

const struct catalog_input_format input_format_properties =
{
  properties_parse,                     /* parse */
  true                                  /* produces_utf8 */
};
