/* JavaScript format strings.
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

/* Written by Bruno Haible, Andreas Stricker, and Daiki Ueno.  */

#include <config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "format.h"
#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "format-invalid.h"
#include "gettext.h"

#define _(str) gettext (str)

/* JavaScript format strings are not in the language specification,
   but there are several implementations which provide the printf-like
   feature.  Here, we provide a permissive parser which at least accepts
   format strings supported by Gjs version 1.40, where:

   A directive
   - starts with '%' or '%m$' where m is a positive integer,
   - is optionally followed by any of the characters '0', '-', ' ',
     or 'I', each of which acts as a flag,
   - is optionally followed by a width specification: a nonempty digit
     sequence,
   - is optionally followed by '.' and a precision specification: a nonempty
     digit sequence,
   - is finished by a specifier
       - 's', that needs a string argument,
       - 'b', 'd', 'u', 'o', 'x', 'X', that need an integer argument,
       - 'f', that need a floating-point argument,
       - 'c', that needs a character argument.
       - 'j', that needs an argument of any type.
   Additionally there is the directive '%%', which takes no argument.  */

enum format_arg_type
{
  FAT_NONE,
  FAT_ANY,
  FAT_CHARACTER,
  FAT_STRING,
  FAT_INTEGER,
  FAT_FLOAT
};

struct numbered_arg
{
  size_t number;
  enum format_arg_type type;
};

struct spec
{
  size_t directives;
  /* We consider a directive as "likely intentional" if it does not contain a
     space.  This prevents xgettext from flagging strings like "100% complete"
     as 'javascript-format' if they don't occur in a context that requires a format
     string.  */
  size_t likely_intentional_directives;
  size_t numbered_arg_count;
  struct numbered_arg *numbered;
};


static int
numbered_arg_compare (const void *p1, const void *p2)
{
  size_t n1 = ((const struct numbered_arg *) p1)->number;
  size_t n2 = ((const struct numbered_arg *) p2)->number;

  return (n1 > n2 ? 1 : n1 < n2 ? -1 : 0);
}

static void *
format_parse (const char *format, bool translated, char *fdi,
              char **invalid_reason)
{
  const char *const format_start = format;

  struct spec spec;
  spec.directives = 0;
  spec.likely_intentional_directives = 0;
  spec.numbered_arg_count = 0;
  spec.numbered = NULL;
  size_t numbered_allocated = 0;
  size_t unnumbered_arg_count = 0;

  for (; *format != '\0';)
    /* Invariant: spec.numbered_arg_count == 0 || unnumbered_arg_count == 0.  */
    if (*format++ == '%')
      {
        /* A directive.  */
        FDI_SET (format - 1, FMTDIR_START);
        spec.directives++;
        bool likely_intentional = true;

        size_t number = 0;
        if (c_isdigit (*format))
          {
            const char *f = format;
            size_t m = 0;

            do
              {
                m = 10 * m + (*f - '0');
                f++;
              }
            while (c_isdigit (*f));

            if (*f == '$')
              {
                if (m == 0)
                  {
                    *invalid_reason = INVALID_ARGNO_0 (spec.directives);
                    FDI_SET (f, FMTDIR_ERROR);
                    goto bad_format;
                  }
                number = m;
                format = ++f;
              }
          }

        /* Parse flags.  */
        while (*format == '-' || *format == '+' || *format == ' '
               || *format == '0' || *format == 'I')
          {
            if (*format == ' ')
              likely_intentional = false;
            format++;
          }

        /* Parse width.  */
        while (c_isdigit (*format))
          format++;

        /* Parse precision.  */
        if (*format == '.')
          {
            format++;

            if (!c_isdigit (*format))
              {
                if (*format == '\0')
                  {
                    *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
                    FDI_SET (format - 1, FMTDIR_ERROR);
                  }
                else
                  {
                    *invalid_reason = INVALID_PRECISION_MISSING (spec.directives);
                    FDI_SET (format, FMTDIR_ERROR);
                  }
                goto bad_format;
              }

            do format++; while (c_isdigit (*format));
          }

        enum format_arg_type type;
        switch (*format)
          {
          case '%':
            type = FAT_NONE;
            break;
          case 'c':
            type = FAT_CHARACTER;
            break;
          case 's':
            type = FAT_STRING;
            break;
          case 'b': case 'd': case 'o': case 'x': case 'X':
            type = FAT_INTEGER;
            break;
          case 'f':
            type = FAT_FLOAT;
            break;
          case 'j':
            type = FAT_ANY;
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

        if (type != FAT_NONE)
          {
            if (number)
              {
                /* Numbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (unnumbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (numbered_allocated == spec.numbered_arg_count)
                  {
                    numbered_allocated = 2 * numbered_allocated + 1;
                    spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, numbered_allocated * sizeof (struct numbered_arg));
                  }
                spec.numbered[spec.numbered_arg_count].number = number;
                spec.numbered[spec.numbered_arg_count].type = type;
                spec.numbered_arg_count++;
              }
            else
              {
                /* Unnumbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (spec.numbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (numbered_allocated == unnumbered_arg_count)
                  {
                    numbered_allocated = 2 * numbered_allocated + 1;
                    spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, numbered_allocated * sizeof (struct numbered_arg));
                  }
                spec.numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                spec.numbered[unnumbered_arg_count].type = type;
                unnumbered_arg_count++;
              }
          }

        if (likely_intentional)
          spec.likely_intentional_directives++;
        FDI_SET (format, FMTDIR_END);

        format++;
      }

  /* Convert the unnumbered argument array to numbered arguments.  */
  if (unnumbered_arg_count > 0)
    spec.numbered_arg_count = unnumbered_arg_count;
  /* Sort the numbered argument array, and eliminate duplicates.  */
  else if (spec.numbered_arg_count > 1)
    {
      qsort (spec.numbered, spec.numbered_arg_count,
             sizeof (struct numbered_arg), numbered_arg_compare);

      /* Remove duplicates: Copy from i to j, keeping 0 <= j <= i.  */
      bool err = false;
      size_t i, j;
      for (i = j = 0; i < spec.numbered_arg_count; i++)
        if (j > 0 && spec.numbered[i].number == spec.numbered[j-1].number)
          {
            enum format_arg_type type1 = spec.numbered[i].type;
            enum format_arg_type type2 = spec.numbered[j-1].type;

            enum format_arg_type type_both;
            if (type1 == type2)
              type_both = type1;
            else
              {
                /* Incompatible types.  */
                type_both = FAT_NONE;
                if (!err)
                  *invalid_reason =
                    INVALID_INCOMPATIBLE_ARG_TYPES (spec.numbered[i].number);
                err = true;
              }

            spec.numbered[j-1].type = type_both;
          }
        else
          {
            if (j < i)
              {
                spec.numbered[j].number = spec.numbered[i].number;
                spec.numbered[j].type = spec.numbered[i].type;
              }
            j++;
          }
      spec.numbered_arg_count = j;
      if (err)
        /* *invalid_reason has already been set above.  */
        goto bad_format;
    }

  struct spec *result = XMALLOC (struct spec);
  *result = spec;
  return result;

 bad_format:
  if (spec.numbered != NULL)
    free (spec.numbered);
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->numbered != NULL)
    free (spec->numbered);
  free (spec);
}

static int
format_get_number_of_directives (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->directives;
}

static bool
format_is_unlikely_intentional (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->likely_intentional_directives == 0;
}

static bool
format_check (void *msgid_descr, void *msgstr_descr, bool equality,
              formatstring_error_logger_t error_logger, void *error_logger_data,
              const char *pretty_msgid, const char *pretty_msgstr)
{
  struct spec *spec1 = (struct spec *) msgid_descr;
  struct spec *spec2 = (struct spec *) msgstr_descr;
  bool err = false;

  if (spec1->numbered_arg_count + spec2->numbered_arg_count > 0)
    {
      size_t n1 = spec1->numbered_arg_count;
      size_t n2 = spec2->numbered_arg_count;

      /* Check that the argument numbers are the same.
         Both arrays are sorted.  We search for the first difference.  */
      {
        size_t i, j;
        for (i = 0, j = 0; i < n1 || j < n2; )
          {
            int cmp = (i >= n1 ? 1 :
                       j >= n2 ? -1 :
                       spec1->numbered[i].number > spec2->numbered[j].number ? 1 :
                       spec1->numbered[i].number < spec2->numbered[j].number ? -1 :
                       0);
            if (cmp > 0)
              {
                if (error_logger)
                  error_logger (error_logger_data,
                                _("a format specification for argument %zu, as in '%s', doesn't exist in '%s'"),
                                spec2->numbered[j].number, pretty_msgstr,
                                pretty_msgid);
                err = true;
                break;
              }
            else if (cmp < 0)
              {
                if (equality)
                  {
                    if (error_logger)
                      error_logger (error_logger_data,
                                    _("a format specification for argument %zu doesn't exist in '%s'"),
                                    spec1->numbered[i].number, pretty_msgstr);
                    err = true;
                    break;
                  }
                else
                  i++;
              }
            else
              j++, i++;
          }
      }
      /* Check the argument types are the same.  */
      if (!err)
        {
          size_t i, j;
          for (i = 0, j = 0; j < n2; )
            {
              if (spec1->numbered[i].number == spec2->numbered[j].number)
                {
                  if (!(spec1->numbered[i].type == spec2->numbered[j].type
                        || (!equality
                            && (spec1->numbered[i].type == FAT_ANY
                                || spec2->numbered[i].type == FAT_ANY))))
                    {
                      if (error_logger)
                        error_logger (error_logger_data,
                                      _("format specifications in '%s' and '%s' for argument %zu are not the same"),
                                      pretty_msgid, pretty_msgstr,
                                      spec2->numbered[j].number);
                      err = true;
                      break;
                    }
                  j++, i++;
                }
              else
                i++;
            }
        }
    }

  return err;
}


struct formatstring_parser formatstring_javascript =
{
  format_parse,
  format_free,
  format_get_number_of_directives,
  format_is_unlikely_intentional,
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
  for (size_t i = 0; i < spec->numbered_arg_count; i++)
    {
      if (i > 0)
        printf (" ");
      switch (spec->numbered[i].type)
        {
        case FAT_ANY:
          printf ("*");
          break;
        case FAT_CHARACTER:
          printf ("c");
          break;
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_FLOAT:
          printf ("f");
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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-javascript.c ../gnulib-lib/libgettextlib.la"
 * End:
 */
#endif /* TEST */
