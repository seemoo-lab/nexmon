/* Lua format strings.
   Copyright (C) 2012-2026 Free Software Foundation, Inc.

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

/* Written by Ľubomír Remák and Bruno Haible.  */

#include <config.h>

#include <stdbool.h>
#include <stdlib.h>

#include "format.h"
#include "gettext.h"
#include "xalloc.h"
#include "format-invalid.h"
#include "c-ctype.h"
#include "xvasprintf.h"

#define _(str) gettext (str)

/* The Lua format strings are described in the Lua manual,
   which can be found at:
   https://www.lua.org/manual/5.2/manual.html
   They are implemented in lua-5.2.4/src/lstrlib.c.

   A directive
   - starts with '%',
   - is optionally followed by any of the characters '0', '-', ' ', or
     each of which acts as a flag,
   - is optionally followed by a width specification: a nonempty digit
     sequence with at most 2 digits,
   - is optionally followed by '.' and a precision specification: an optional
     nonempty digit sequence with at most 2 digits,
   - is finished by a specifier
       - 's', 'q', that need a string argument,
       - 'd', 'i', 'o', 'u', 'X', 'x', that need an integer argument,
       - 'A', 'a', 'E', 'e', 'f', 'G', 'g', that need a floating-point argument,
       - 'c', that needs a character argument.
   Additionally there is the directive '%%', which takes no argument.

   Note: Lua does not distinguish between integer, floating-point
   and character arguments, since it has a number data type only.
   However, we should not allow users to use %d instead of %c.
   The same applies to %s and %q - we should not allow intermixing them.
 */

enum format_arg_type
{
  FAT_INTEGER,
  FAT_CHARACTER,
  FAT_FLOAT,
  FAT_STRING,
  FAT_ESCAPED_STRING
};

struct spec
{
  size_t directives;
  size_t format_args_count;
  enum format_arg_type *format_args;
};

static void *
format_parse (const char *format, bool translated, char *fdi,
              char **invalid_reason)
{

  const char *format_start = format;
  const char *fatstr = format;

  struct spec spec;
  spec.directives = 0;
  spec.format_args_count = 0;
  spec.format_args = NULL;
  size_t format_args_allocated = 0;

  for (; *fatstr != '\0';)
    {
      if (*fatstr++ == '%')
        {
          FDI_SET (fatstr - 1, FMTDIR_START);
          spec.directives++;

          if (*fatstr != '%')
            {
              /* Parse width. */
              if (c_isdigit (*fatstr))
                {
                  fatstr++;
                  if (c_isdigit (*fatstr))
                    fatstr++;
                }

              if (*fatstr == '.')
                {
                  fatstr++;

                  /* Parse precision. */
                  if (c_isdigit (*fatstr))
                    {
                      fatstr++;
                      if (c_isdigit (*fatstr))
                        fatstr++;
                    }
                }

              enum format_arg_type type;
              switch (*fatstr)
                {
                case 'c':
                  type = FAT_CHARACTER;
                  break;
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'X':
                case 'x':
                  type = FAT_INTEGER;
                  break;
                case 'a':
                case 'A':
                case 'E':
                case 'e':
                case 'f':
                case 'g':
                case 'G':
                  type = FAT_FLOAT;
                  break;
                case 's':
                  type = FAT_STRING;
                  break;
                case 'q':
                  type = FAT_ESCAPED_STRING;
                  break;
                default:
                  if (*fatstr == '\0')
                    {
                      *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
                      FDI_SET (fatstr - 1, FMTDIR_ERROR);
                    }
                  else
                    {
                      *invalid_reason =
                        INVALID_CONVERSION_SPECIFIER (spec.format_args_count + 1,
                                                      *fatstr);
                      FDI_SET (fatstr, FMTDIR_ERROR);
                    }
                  goto bad_format;
                }

              if (spec.format_args_count == format_args_allocated)
                {
                  format_args_allocated = 2 * format_args_allocated + 10;
                  spec.format_args =
                    xrealloc (spec.format_args,
                              format_args_allocated *
                              sizeof (enum format_arg_type));
                }
              spec.format_args[spec.format_args_count++] = type;
            }
          FDI_SET (fatstr, FMTDIR_END);
          fatstr++;
        }
    }

  struct spec *result = XMALLOC (struct spec);
  *result = spec;
  return result;

 bad_format:
  if (spec.format_args != NULL)
    free (spec.format_args);
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->format_args != NULL)
    free (spec->format_args);
  free (spec);
}

static int
format_get_number_of_directives (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->directives;
}

static bool
format_check (void *msgid_descr, void *msgstr_descr, bool equality,
              formatstring_error_logger_t error_logger, void *error_logger_data,
              const char *pretty_msgid, const char *pretty_msgstr)
{
  struct spec *spec1 = (struct spec *) msgid_descr;
  struct spec *spec2 = (struct spec *) msgstr_descr;
  bool err = false;

  if (spec1->format_args_count + spec2->format_args_count > 0)
    {
      size_t n1 = spec1->format_args_count;
      size_t n2 = spec2->format_args_count;

      /* Check that the argument counts are the same.  */
      if (n1 < n2)
        {
          if (error_logger)
            error_logger (error_logger_data,
                          _("a format specification for argument %zu, as in '%s', doesn't exist in '%s'"),
                          n1 + 1, pretty_msgstr, pretty_msgid);
          err = true;
        }
      else if (n1 > n2 && equality)
        {
          if (error_logger)
            error_logger (error_logger_data,
                          _("a format specification for argument %zu doesn't exist in '%s'"),
                          n2 + 1, pretty_msgstr);
          err = true;
        }
      else
        {
          /* Check that the argument types are the same.  */
          if (!err)
            for (size_t i = 0; i < n2; i++)
              {
                if (spec1->format_args[i] != spec2->format_args[i])
                  {
                    if (error_logger)
                      error_logger (error_logger_data,
                                    _("format specifications in '%s' and '%s' for argument %zu are not the same"),
                                    pretty_msgid, pretty_msgstr, i + 1);
                    err = true;
                    break;
                  }
              }
        }
    }

  return err;
}

struct formatstring_parser formatstring_lua =
{
  format_parse,
  format_free,
  format_get_number_of_directives,
  NULL,
  format_check
};


#ifdef TEST

/* Test program: Print the argument list specification returned by
   format_parse for strings read from standard input.  */

#include <stdio.h>

static void
format_print (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec == NULL)
    {
      printf ("INVALID");
      return;
    }

  printf ("(");
  for (size_t i = 0; i < spec->format_args_count; i++)
    {
      if (i > 0)
        printf (" ");
      switch (spec->format_args[i])
        {
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_FLOAT:
          printf ("f");
          break;
        case FAT_CHARACTER:
          printf ("c");
          break;
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_ESCAPED_STRING:
          printf ("q");
          break;
        default:
          abort ();
        }
    }
  printf (")");
}

int
main ()
{
  for (;;)
    {
      char *line = NULL;
      size_t line_size = 0;
      int line_len = getline (&line, &line_size, stdin);
      if (line_len < 0)
        break;
      if (line_len > 0 && line[line_len - 1] == '\n')
        line[--line_len] = '\0';

      char *invalid_reason = NULL;
      void *descr = format_parse (line, false, NULL, &invalid_reason);

      format_print (descr);
      printf ("\n");
      if (descr == NULL)
        printf ("%s\n", invalid_reason);

      free (invalid_reason);
      free (line);
    }

  return 0;
}

/*
 * For Emacs M-x compile
 * Local Variables:
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-lua.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
