/* Reading textual message catalogs (such as PO files), abstract class.
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
#include "read-catalog-abstract.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>
#include "po-charset.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "xerror-handler.h"
#include "gettext.h"


/* ========================================================================= */
/* Allocating and freeing instances of abstract_catalog_reader_ty.  */


abstract_catalog_reader_ty *
catalog_reader_alloc (abstract_catalog_reader_class_ty *method_table,
                      xerror_handler_ty xerror_handler)
{
  abstract_catalog_reader_ty *catr =
    (abstract_catalog_reader_ty *) xmalloc (method_table->size);
  catr->methods = method_table;
  catr->xeh = xerror_handler;
  catr->pass_comments = false;
  catr->pass_obsolete_entries = false;
  catr->po_lex_isolate_start = NULL;
  catr->po_lex_isolate_end = NULL;
  if (method_table->constructor)
    method_table->constructor (catr);
  return catr;
}


void
catalog_reader_free (abstract_catalog_reader_ty *catr)
{
  if (catr->methods->destructor)
    catr->methods->destructor (catr);
  free (catr);
}


/* ========================================================================= */
/* Inline functions to invoke the methods.  */


static inline void
call_parse_brief (abstract_catalog_reader_ty *catr)
{
  if (catr->methods->parse_brief)
    catr->methods->parse_brief (catr);
}

static inline void
call_parse_debrief (abstract_catalog_reader_ty *catr)
{
  if (catr->methods->parse_debrief)
    catr->methods->parse_debrief (catr);
}

static inline void
call_directive_domain (abstract_catalog_reader_ty *catr,
                       char *name, lex_pos_ty *name_pos)
{
  if (catr->methods->directive_domain)
    catr->methods->directive_domain (catr, name, name_pos);
}

static inline void
call_directive_message (abstract_catalog_reader_ty *catr,
                        char *msgctxt,
                        char *msgid,
                        lex_pos_ty *msgid_pos,
                        char *msgid_plural,
                        char *msgstr, size_t msgstr_len,
                        lex_pos_ty *msgstr_pos,
                        char *prev_msgctxt,
                        char *prev_msgid,
                        char *prev_msgid_plural,
                        bool force_fuzzy, bool obsolete)
{
  if (catr->methods->directive_message)
    catr->methods->directive_message (catr, msgctxt,
                                      msgid, msgid_pos, msgid_plural,
                                      msgstr, msgstr_len, msgstr_pos,
                                      prev_msgctxt,
                                      prev_msgid,
                                      prev_msgid_plural,
                                      force_fuzzy, obsolete);
}

static inline void
call_comment (abstract_catalog_reader_ty *catr, const char *s)
{
  if (catr->methods->comment != NULL)
    catr->methods->comment (catr, s);
}

static inline void
call_comment_dot (abstract_catalog_reader_ty *catr, const char *s)
{
  if (catr->methods->comment_dot != NULL)
    catr->methods->comment_dot (catr, s);
}

static inline void
call_comment_filepos (abstract_catalog_reader_ty *catr,
                      const char *file_name, size_t line_number)
{
  if (catr->methods->comment_filepos)
    catr->methods->comment_filepos (catr, file_name, line_number);
}

static inline void
call_comment_special (abstract_catalog_reader_ty *catr, const char *s)
{
  if (catr->methods->comment_special != NULL)
    catr->methods->comment_special (catr, s);
}


/* ========================================================================= */
/* Exported functions.  */


void
catalog_reader_parse (abstract_catalog_reader_ty *catr, FILE *fp,
                      const char *real_filename, const char *logical_filename,
                      bool is_pot_role,
                      catalog_input_format_ty input_syntax,
                      string_list_ty *arena)
{
  *(catr->xeh->error_message_count_p) = 0;

  /* Parse the stream's content.  */
  call_parse_brief (catr);
  input_syntax->parse (catr, fp, real_filename, logical_filename,
                       is_pot_role, arena);
  call_parse_debrief (catr);

  unsigned int num_errors = *(catr->xeh->error_message_count_p);
  if (num_errors > 0)
    catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL,
                       /*real_filename*/ NULL, (size_t)(-1), (size_t)(-1),
                       false,
                       xasprintf (ngettext ("found %u fatal error",
                                            "found %u fatal errors",
                                            num_errors),
                                  num_errors));
}


/* ========================================================================= */
/* Callbacks used by read-po-gram.y, read-properties.c, read-stringtable.c,
   indirectly from catalog_reader_parse.  */


void
catalog_reader_seen_domain (abstract_catalog_reader_ty *catr,
                            char *name, lex_pos_ty *name_pos)
{
  call_directive_domain (catr, name, name_pos);
}


void
catalog_reader_seen_message (abstract_catalog_reader_ty *catr,
                             char *msgctxt,
                             char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
                             char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
                             char *prev_msgctxt,
                             char *prev_msgid,
                             char *prev_msgid_plural,
                             bool force_fuzzy, bool obsolete)
{
  call_directive_message (catr, msgctxt,
                          msgid, msgid_pos, msgid_plural,
                          msgstr, msgstr_len, msgstr_pos,
                          prev_msgctxt, prev_msgid, prev_msgid_plural,
                          force_fuzzy, obsolete);
}


void
catalog_reader_seen_comment (abstract_catalog_reader_ty *catr, const char *s)
{
  call_comment (catr, s);
}


void
catalog_reader_seen_comment_dot (abstract_catalog_reader_ty *catr,
                                 const char *s)
{
  call_comment_dot (catr, s);
}


void
catalog_reader_seen_comment_filepos (abstract_catalog_reader_ty *catr,
                                     const char *file_name, size_t line_number)
{
  call_comment_filepos (catr, file_name, line_number);
}


void
catalog_reader_seen_comment_special (abstract_catalog_reader_ty *catr,
                                     const char *s)
{
  call_comment_special (catr, s);
}


/* Parse a GNU style file comment.
   Syntax: an arbitrary number of
             STRING COLON NUMBER
           or
             STRING
   The latter style, without line number, occurs in PO files converted e.g.
   from Pascal .rst files or from OpenOffice resource files.
   The STRING is either
             FILENAME
           or
             U+2068 FILENAME U+2069.
   Call catalog_reader_seen_comment_filepos for each of them.  */
static void
parse_comment_filepos (abstract_catalog_reader_ty *catr, const char *s)
{
  while (*s != '\0')
    {
      while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
      if (*s != '\0')
        {
          bool isolated_filename =
            (catr->po_lex_isolate_start != NULL
             && str_startswith (s, catr->po_lex_isolate_start));
          if (isolated_filename)
            s += strlen (catr->po_lex_isolate_start);

          const char *filename_start = s;

          const char *filename_end;
          if (isolated_filename)
            {
              for (;; s++)
                {
                  if (*s == '\0' || *s == '\n')
                    {
                      filename_end = s;
                      break;
                    }
                  if (str_startswith (s, catr->po_lex_isolate_end))
                    {
                      filename_end = s;
                      s += strlen (catr->po_lex_isolate_end);
                      break;
                    }
                }
            }
          else
            {
              do
                s++;
              while (!(*s == '\0' || *s == ' ' || *s == '\t' || *s == '\n'));
              filename_end = s;
            }

          /* See if there is a COLON and NUMBER after the STRING, separated
             through optional spaces.  */
          {
            const char *p = s;

            while (*p == ' ' || *p == '\t' || *p == '\n')
              p++;

            if (*p == ':')
              {
                p++;

                while (*p == ' ' || *p == '\t' || *p == '\n')
                  p++;

                if (*p >= '0' && *p <= '9')
                  {
                    /* Accumulate a number.  */
                    size_t n = 0;

                    do
                      {
                        n = n * 10 + (*p - '0');
                        p++;
                      }
                    while (*p >= '0' && *p <= '9');

                    if (*p == '\0' || *p == ' ' || *p == '\t' || *p == '\n')
                      {
                        /* Parsed a GNU style file comment with spaces.  */
                        size_t filename_length = filename_end - filename_start;

                        char *filename = XNMALLOC (filename_length + 1, char);
                        memcpy (filename, filename_start, filename_length);
                        filename[filename_length] = '\0';

                        catalog_reader_seen_comment_filepos (catr, filename, n);

                        free (filename);

                        s = p;
                        continue;
                      }
                  }
              }
          }

          /* See if there is a COLON at the end of STRING and a NUMBER after
             it, separated through optional spaces.  */
          if (s[-1] == ':')
            {
              const char *p = s;

              while (*p == ' ' || *p == '\t' || *p == '\n')
                p++;

              if (*p >= '0' && *p <= '9')
                {
                  /* Accumulate a number.  */
                  size_t n = 0;

                  do
                    {
                      n = n * 10 + (*p - '0');
                      p++;
                    }
                  while (*p >= '0' && *p <= '9');

                  if (*p == '\0' || *p == ' ' || *p == '\t' || *p == '\n')
                    {
                      /* Parsed a GNU style file comment with spaces.  */
                      filename_end = s - 1;
                      size_t filename_length = filename_end - filename_start;

                      char *filename = XNMALLOC (filename_length + 1, char);
                      memcpy (filename, filename_start, filename_length);
                      filename[filename_length] = '\0';

                      catalog_reader_seen_comment_filepos (catr, filename, n);

                      free (filename);

                      s = p;
                      continue;
                    }
                }
            }

          /* See if there is a COLON and NUMBER at the end of the STRING,
             without separating spaces.  */
          {
            const char *p = s;

            while (p > filename_start)
              {
                p--;
                if (!(*p >= '0' && *p <= '9'))
                  {
                    p++;
                    break;
                  }
              }

            /* p now points to the beginning of the trailing digits segment
               at the end of STRING.  */

            if (p < s
                && p > filename_start + 1
                && p[-1] == ':')
              {
                /* Parsed a GNU style file comment without spaces.  */
                const char *string_end = p - 1;

                /* Accumulate a number.  */
                {
                  size_t n = 0;

                  do
                    {
                      n = n * 10 + (*p - '0');
                      p++;
                    }
                  while (p < s);

                  {
                    filename_end = string_end;
                    size_t filename_length = filename_end - filename_start;

                    char *filename = XNMALLOC (filename_length + 1, char);
                    memcpy (filename, filename_start, filename_length);
                    filename[filename_length] = '\0';

                    catalog_reader_seen_comment_filepos (catr, filename, n);

                    free (filename);

                    continue;
                  }
                }
              }
          }

          /* Parsed a file comment without line number.  */
          {
            size_t filename_length = filename_end - filename_start;

            char *filename = XNMALLOC (filename_length + 1, char);
            memcpy (filename, filename_start, filename_length);
            filename[filename_length] = '\0';

            catalog_reader_seen_comment_filepos (catr, filename, (size_t)(-1));

            free (filename);
          }
        }
    }
}


/* Parse a SunOS or Solaris style file comment.
   Syntax of SunOS style:
     FILE_KEYWORD COLON STRING COMMA LINE_KEYWORD COLON NUMBER
   Syntax of Solaris style:
     FILE_KEYWORD COLON STRING COMMA LINE_KEYWORD NUMBER_KEYWORD COLON NUMBER
   where
     FILE_KEYWORD ::= "file" | "File"
     COLON ::= ":"
     COMMA ::= ","
     LINE_KEYWORD ::= "line"
     NUMBER_KEYWORD ::= "number"
     NUMBER ::= [0-9]+
   Return true if parsed, false if not a comment of this form. */
static bool
parse_comment_solaris_filepos (abstract_catalog_reader_ty *catr, const char *s)
{
  if (s[0] == ' '
      && (s[1] == 'F' || s[1] == 'f')
      && s[2] == 'i' && s[3] == 'l' && s[4] == 'e'
      && s[5] == ':')
    {
      const char *string_start;

      {
        const char *p = s + 6;

        while (*p == ' ' || *p == '\t')
          p++;
        string_start = p;
      }

      for (const char *string_end = string_start; *string_end != '\0'; string_end++)
        {
          const char *p = string_end;

          while (*p == ' ' || *p == '\t')
            p++;

          if (*p == ',')
            {
              p++;

              while (*p == ' ' || *p == '\t')
                p++;

              if (p[0] == 'l' && p[1] == 'i' && p[2] == 'n' && p[3] == 'e')
                {
                  p += 4;

                  while (*p == ' ' || *p == '\t')
                    p++;

                  if (p[0] == 'n' && p[1] == 'u' && p[2] == 'm'
                      && p[3] == 'b' && p[4] == 'e' && p[5] == 'r')
                    {
                      p += 6;
                      while (*p == ' ' || *p == '\t')
                        p++;
                    }

                  if (*p == ':')
                    {
                      p++;

                      if (*p >= '0' && *p <= '9')
                        {
                          /* Accumulate a number.  */
                          size_t n = 0;

                          do
                            {
                              n = n * 10 + (*p - '0');
                              p++;
                            }
                          while (*p >= '0' && *p <= '9');

                          while (*p == ' ' || *p == '\t' || *p == '\n')
                            p++;

                          if (*p == '\0')
                            {
                              /* Parsed a Sun style file comment.  */
                              size_t string_length = string_end - string_start;

                              char *string =
                                XNMALLOC (string_length + 1, char);
                              memcpy (string, string_start, string_length);
                              string[string_length] = '\0';

                              catalog_reader_seen_comment_filepos (catr, string, n);

                              free (string);
                              return true;
                            }
                        }
                    }
                }
            }
        }
    }

  return false;
}


/* This callback is called whenever a generic comment line has been seen.
   It parses s and invokes the appropriate method: call_comment,
   call_comment_dot, call_comment_filepos (via parse_comment_filepos), or
   call_comment_special.  */
void
catalog_reader_seen_generic_comment (abstract_catalog_reader_ty *catr,
                                     const char *s)
{
  if (*s == '.')
    {
      s++;
      /* There is usually a space before the comment.  People don't
         consider it part of the comment, therefore remove it here.  */
      if (*s == ' ')
        s++;
      catalog_reader_seen_comment_dot (catr, s);
    }
  else if (*s == ':')
    {
      /* Parse the file location string.  The appropriate callback will be
         invoked.  */
      parse_comment_filepos (catr, s + 1);
    }
  else if (*s == ',' || *s == '=' || *s == '!')
    {
      /* Get all entries in the special comment line.  */
      catalog_reader_seen_comment_special (catr, s + 1);
    }
  else
    {
      /* It looks like a plain vanilla comment, but Solaris-style file
         position lines do, too.  Try to parse the lot.  If the parse
         succeeds, the appropriate callback will be invoked.  */
      if (parse_comment_solaris_filepos (catr, s))
        /* Do nothing, it is a Sun-style file pos line.  */ ;
      else
        {
          /* There is usually a space before the comment.  People don't
             consider it part of the comment, therefore remove it here.  */
          if (*s == ' ')
            s++;
          catalog_reader_seen_comment (catr, s);
        }
    }
}
