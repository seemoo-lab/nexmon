/* xgettext RST backend.
   Copyright (C) 2001-2003, 2005-2009, 2015-2016 Free Software Foundation, Inc.

   This file was written by Bruno Haible <haible@clisp.cons.org>, 2001.

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
#include "x-rst.h"

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "c-ctype.h"
#include "message.h"
#include "xgettext.h"
#include "error.h"
#include "error-progname.h"
#include "xalloc.h"
#include "gettext.h"

#define _(s) gettext(s)

/* RST stands for Resource String Table.

   An RST file consists of several string definitions.  A string definition
   starts at the beginning of a line and looks like this:
       ModuleName.ConstName=StringExpression
   A StringExpression consists of string pieces of the form 'xyz',
   single characters of the form #nnn (decimal integer), and +
   at the end of the line to designate continuation on the next line.
   String definitions can be separated by blank lines or comment lines
   beginning with '#'.

   This backend attempts to be functionally equivalent to the 'rstconv'
   program, part of the Free Pascal run time library, written by
   Sebastian Guenther.  Except that the locations are output as
   "ModuleName.ConstName", not "ModuleName:ConstName".
 */

void
extract_rst (FILE *f,
             const char *real_filename, const char *logical_filename,
             flag_context_list_table_ty *flag_table,
             msgdomain_list_ty *mdlp)
{
  static char *buffer;
  static int bufmax;
  message_list_ty *mlp = mdlp->item[0]->messages;
  int line_number;

  line_number = 1;
  for (;;)
    {
      int c;
      int bufpos;
      char *location;
      char *msgid;
      lex_pos_ty pos;

      c = getc (f);
      if (c == EOF)
        break;

      /* Ignore blank line.  */
      if (c == '\n')
        {
          line_number++;
          continue;
        }

      /* Ignore comment line.  */
      if (c == '#')
        {
          do
            c = getc (f);
          while (c != EOF && c != '\n');
          if (c == EOF)
            break;
          line_number++;
          continue;
        }

      /* Read ModuleName.ConstName.  */
      bufpos = 0;
      for (;;)
        {
          if (c == EOF || c == '\n')
            {
              error_with_progname = false;
              error (EXIT_FAILURE, 0, _("%s:%d: invalid string definition"),
                     logical_filename, line_number);
              error_with_progname = true;
            }
          if (bufpos >= bufmax)
            {
              bufmax = 2 * bufmax + 10;
              buffer = xrealloc (buffer, bufmax);
            }
          if (c == '=')
            break;
          buffer[bufpos++] = c;
          c = getc (f);
          if (c == EOF && ferror (f))
            goto bomb;
        }
      buffer[bufpos] = '\0';
      location = xstrdup (buffer);

      /* Read StringExpression.  */
      bufpos = 0;
      for (;;)
        {
          c = getc (f);
          if (c == EOF)
            break;
          else if (c == '\n')
            {
              line_number++;
              break;
            }
          else if (c == '\'')
            {
              for (;;)
                {
                  c = getc (f);
                  /* Embedded single quotes like 'abc''def' don't occur.
                     See fpc-1.0.4/compiler/cresstr.pas.  */
                  if (c == EOF || c == '\n' || c == '\'')
                    break;
                  if (bufpos >= bufmax)
                    {
                      bufmax = 2 * bufmax + 10;
                      buffer = xrealloc (buffer, bufmax);
                    }
                  buffer[bufpos++] = c;
                }
              if (c == EOF)
                break;
              else if (c == '\n')
                {
                  line_number++;
                  break;
                }
            }
          else if (c == '#')
            {
              int n;
              c = getc (f);
              if (c == EOF && ferror (f))
                goto bomb;
              if (c == EOF || !c_isdigit (c))
                {
                  error_with_progname = false;
                  error (EXIT_FAILURE, 0, _("%s:%d: missing number after #"),
                         logical_filename, line_number);
                  error_with_progname = true;
                }
              n = (c - '0');
              for (;;)
                {
                  c = getc (f);
                  if (c == EOF || !c_isdigit (c))
                    break;
                  n = n * 10 + (c - '0');
                }
              if (bufpos >= bufmax)
                {
                  bufmax = 2 * bufmax + 10;
                  buffer = xrealloc (buffer, bufmax);
                }
              buffer[bufpos++] = (unsigned char) n;
              if (c == EOF)
                break;
              ungetc (c, f);
            }
          else if (c == '+')
            {
              c = getc (f);
              if (c == EOF)
                break;
              if (c == '\n')
                line_number++;
              else
                ungetc (c, f);
            }
          else
            {
              error_with_progname = false;
              error (EXIT_FAILURE, 0, _("%s:%d: invalid string expression"),
                     logical_filename, line_number);
              error_with_progname = true;
            }
        }
      if (bufpos >= bufmax)
        {
          bufmax = 2 * bufmax + 10;
          buffer = xrealloc (buffer, bufmax);
        }
      buffer[bufpos] = '\0';
      msgid = xstrdup (buffer);

      pos.file_name = location;
      pos.line_number = (size_t)(-1);

      remember_a_message (mlp, NULL, msgid, null_context, &pos, NULL, NULL);

      /* Here c is the last read character: EOF or '\n'.  */
      if (c == EOF)
        break;
    }

  if (ferror (f))
    {
    bomb:
      error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
             real_filename);
    }
}
