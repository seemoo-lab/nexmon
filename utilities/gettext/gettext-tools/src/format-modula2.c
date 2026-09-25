/* Modula-2 format strings.
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

#include <stdbool.h>
#include <stdlib.h>

#include "format.h"
#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "format-invalid.h"
#include "gettext.h"

#define _(str) gettext (str)

/* The GNU Modula-2 format strings are implemented in
   gcc-14.2.0/gcc/m2/gm2-libs/FormatStrings.mod.

   A directive
   - starts with '%',
   - is optionally followed by the flag character '-',
   - is optionally followed by the flag character '0',
   - is optionally followed by a width specification: a nonempty digit
     sequence,
   - is finished by a specifier
       - 's', that needs a String argument,
       - 'c', that needs a CHAR argument,
       - 'd', that needs an INTEGER argument and converts it to decimal,
       - 'u', that needs a CARDINAL argument and converts it to decimal,
       - 'x', that needs a CARDINAL argument and converts it to hexadecimal.
   Additionally there is the directive '%%', which takes no argument.

   Also, escape sequences in the format string are processed, as documented in
   <https://gcc.gnu.org/onlinedocs/gcc-14.2.0/gm2/gm2-libs_002fFormatStrings.html>:
   \a, \b, \e, \f, \n, \r, \x[hex], \[octal], \[other character]
 */

enum format_arg_type
{
  FAT_STRING,
  FAT_CHAR,
  FAT_INTEGER,
  FAT_CARDINAL
};

struct spec
{
  size_t directives;
  size_t arg_count;
  enum format_arg_type *args;
};

static void *
format_parse (const char *format, bool translated, char *fdi,
              char **invalid_reason)
{
  const char *const format_start = format;

  struct spec spec;
  spec.directives = 0;
  spec.arg_count = 0;
  spec.args = NULL;
  size_t args_allocated = 0;

  for (; *format != '\0';)
    {
      if (*format == '\\')
        format++;
      if (*format != '\0')
        {
          if (*format++ == '%')
            {
              FDI_SET (format - 1, FMTDIR_START);
              spec.directives++;

              if (*format != '%')
                {
                  /* Parse flags.  */
                  if (*format == '-')
                    format++;
                  if (*format == '0')
                    format++;

                  /* Parse width.  */
                  while (c_isdigit (*format))
                    format++;

                  enum format_arg_type type;
                  switch (*format)
                    {
                    case 's':
                      type = FAT_STRING;
                      break;
                    case 'c':
                      type = FAT_CHAR;
                      break;
                    case 'd':
                      type = FAT_INTEGER;
                      break;
                    case 'u': case 'x':
                      type = FAT_CARDINAL;
                      break;
                    default:
                      if (*format == '\0')
                        {
                          *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
                          FDI_SET (format - 1, FMTDIR_ERROR);
                        }
                      else
                        {
                          *invalid_reason =
                            INVALID_CONVERSION_SPECIFIER (spec.directives, *format);
                          FDI_SET (format, FMTDIR_ERROR);
                        }
                      goto bad_format;
                    }

                  if (spec.arg_count == args_allocated)
                    {
                      args_allocated = 2 * args_allocated + 10;
                      spec.args =
                        (enum format_arg_type *)
                        xrealloc (spec.args, args_allocated * sizeof (enum format_arg_type));
                    }
                  spec.args[spec.arg_count++] = type;
                }
              FDI_SET (format, FMTDIR_END);
              format++;
            }
        }
    }

  struct spec *result = XMALLOC (struct spec);
  *result = spec;
  return result;

 bad_format:
  if (spec.args != NULL)
    free (spec.args);
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->args != NULL)
    free (spec->args);
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

  if (spec1->arg_count + spec2->arg_count > 0)
    {
      size_t n1 = spec1->arg_count;
      size_t n2 = spec2->arg_count;

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
                if (spec1->args[i] != spec2->args[i])
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


struct formatstring_parser formatstring_modula2 =
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
  for (size_t i = 0; i < spec->arg_count; i++)
    {
      if (i > 0)
        printf (" ");
      switch (spec->args[i])
        {
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_CHAR:
          printf ("c");
          break;
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_CARDINAL:
          printf ("u");
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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-modula2.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
