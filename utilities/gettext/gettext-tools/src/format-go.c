/* Go format strings.
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

/* Go format strings are described in
   https://pkg.go.dev/fmt
   and are implemented in gcc-14.2.0/libgo/go/fmt/print.go .
   A format string consists of literal text and directives.
   A directive
   - starts with '%',
   - is optionally followed by any of the characters '#', '0', '-', ' ', '+',
     each of which acts as a flag,
   - is optionally followed by a width specification: '*' (reads an argument)
     or '[m]*' (moves to the m-th argument and reads that argument) or a digit
     sequence with value <= 1000000,
   - is optionally followed by '.' and a precision specification: '*' (reads an
     argument) or '[m]*' (moves to the m-th argument and reads that argument) or
     optionally a nonempty digit sequence with value <= 1000000,
   - is optionally followed by '[m]', where m is a digit sequence with a
     positive value <= 1000000,
   - is finished by a specifier
       - '%', that needs no argument,
       - 'v', that need a generic value argument,
       - 'T', that need a generic value argument to take the type of,
       - 't', that need a boolean argument,
       - 'c', that need a character argument,
       - 'U', that need a character argument to print with U+nnnn escapes,
       - 's', that need a string argument,
       - 'q', that need a character or string argument to print as a
         character-literal or string-literal, respectively, with escapes,
       - 'e', 'E', 'f', 'F', 'g', 'G', that need a floating-point argument,
       - 'O', that need an integer argument,
       - 'd', 'o', that need an integer or pointer argument,
       - 'b', that need an integer or floating-point or pointer argument,
       - 'x', 'X', that need an integer or floating-point or string or pointer
         argument.
   Numbered ('[m]' or '[m]*') and unnumbered argument specifications can be
   used in the same string.  The effect of '[m]' is to set the current argument
   number to m.  The current argument number is incremented after processing an
   argument.
 */

enum format_arg_type
{
  FAT_NONE           = 0,
  FAT_BOOLEAN        = 1 << 0,
  FAT_CHARACTER      = 1 << 1,
  FAT_STRING         = 1 << 2,
  FAT_FLOATINGPOINT  = 1 << 3,
  FAT_INTEGER        = 1 << 4,
  FAT_POINTER        = 1 << 5,
  FAT_ANYVALUE       = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)
                       | (1 << 4) | (1 << 5) | (1 << 6),
  FAT_ANYVALUE_TYPE  = 1 << 7
};
typedef unsigned int format_arg_type_t;

struct numbered_arg
{
  size_t number;
  format_arg_type_t type;
};

struct spec
{
  size_t directives;
  /* We consider a directive as "likely intentional" if it does not contain a
     space.  This prevents xgettext from flagging strings like "100% complete"
     as 'go-format' if they don't occur in a context that requires a format
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

#define INVALID_ARGNO_TOO_LARGE(directive_number) \
  xasprintf (_("In the directive number %zu, the argument number is too large."), directive_number)

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
  size_t allocated = 0;
  size_t number = 1;

  for (; *format != '\0';)
    if (*format++ == '%')
      {
        /* A directive.  */
        FDI_SET (format - 1, FMTDIR_START);
        spec.directives++;
        bool likely_intentional = true;

        /* Parse flags.  */
        while (*format == ' ' || *format == '+' || *format == '-'
               || *format == '#' || *format == '0')
          {
            if (*format == ' ')
              likely_intentional = false;
            format++;
          }

        if (*format == '[')
          {
            if (c_isdigit (format[1]))
              {
                const char *f = format + 1;
                size_t m = 0;

                do
                  {
                    if (m <= 1000000)
                      m = 10 * m + (*f - '0');
                    f++;
                  }
                while (c_isdigit (*f));

                if (*f == ']')
                  {
                    if (m == 0)
                      {
                        *invalid_reason = INVALID_ARGNO_0 (spec.directives);
                        FDI_SET (f, FMTDIR_ERROR);
                        goto bad_format;
                      }
                    if (m > 1000000)
                      {
                        *invalid_reason = INVALID_ARGNO_TOO_LARGE (spec.directives);
                        FDI_SET (f, FMTDIR_ERROR);
                        goto bad_format;
                      }
                    number = m;
                    format = ++f;

                    /* Parse width.  */
                    if (*format == '*')
                      {
                        size_t width_number = number;

                        if (allocated == spec.numbered_arg_count)
                          {
                            allocated = 2 * allocated + 1;
                            spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, allocated * sizeof (struct numbered_arg));
                          }
                        spec.numbered[spec.numbered_arg_count].number = width_number;
                        spec.numbered[spec.numbered_arg_count].type = FAT_INTEGER;
                        spec.numbered_arg_count++;

                        number++;
                        format++;
                        goto parse_precision;
                      }

                    goto parse_specifier;
                  }
              }
          }

        /* Parse width other than [m]*.  */
        if (c_isdigit (*format))
          {
            const char *f = format;
            size_t width = 0;

            do
              {
                if (width <= 1000000)
                  width = 10 * width + (*f - '0');
                f++;
              }
            while (c_isdigit (*f));

            if (width > 1000000)
              {
                *invalid_reason =
                  xasprintf (_("In the directive number %zu, the width is too large."),
                             spec.directives);
                FDI_SET (f - 1, FMTDIR_ERROR);
                goto bad_format;
              }
            format = f;
          }
        else if (*format == '*')
          {
            size_t width_number = number;

            if (allocated == spec.numbered_arg_count)
              {
                allocated = 2 * allocated + 1;
                spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, allocated * sizeof (struct numbered_arg));
              }
            spec.numbered[spec.numbered_arg_count].number = width_number;
            spec.numbered[spec.numbered_arg_count].type = FAT_INTEGER;
            spec.numbered_arg_count++;

            number++;
            format++;
          }

       parse_precision:
        /* Parse precision.  */
        if (*format == '.')
          {
            format++;
            if (*format == '[')
              {
                if (c_isdigit (format[1]))
                  {
                    const char *f = format + 1;
                    size_t m = 0;

                    do
                      {
                        if (m <= 1000000)
                          m = 10 * m + (*f - '0');
                        f++;
                      }
                    while (c_isdigit (*f));

                    if (*f == ']')
                      {
                        if (m == 0)
                          {
                            *invalid_reason = INVALID_ARGNO_0 (spec.directives);
                            FDI_SET (f, FMTDIR_ERROR);
                            goto bad_format;
                          }
                        if (m > 1000000)
                          {
                            *invalid_reason = INVALID_ARGNO_TOO_LARGE (spec.directives);
                            FDI_SET (f, FMTDIR_ERROR);
                            goto bad_format;
                          }
                        number = m;
                        format = ++f;

                        /* Finish parsing precision.  */
                        if (*format == '*')
                          {
                            size_t precision_number = number;

                            if (allocated == spec.numbered_arg_count)
                              {
                                allocated = 2 * allocated + 1;
                                spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, allocated * sizeof (struct numbered_arg));
                              }
                            spec.numbered[spec.numbered_arg_count].number = precision_number;
                            spec.numbered[spec.numbered_arg_count].type = FAT_INTEGER;
                            spec.numbered_arg_count++;

                            number++;
                            format++;
                            goto parse_value;
                          }

                        goto parse_specifier;
                      }

                    /* The precision was empty, means zero.  */
                    goto parse_specifier;
                  }
              }

            /* Parse precision other than [m]*.  */
            if (c_isdigit (*format))
              {
                const char *f = format;
                size_t precision = 0;

                do
                  {
                    if (precision <= 1000000)
                      precision = 10 * precision + (*f - '0');
                    f++;
                  }
                while (c_isdigit (*f));

                if (precision > 1000000)
                  {
                    *invalid_reason =
                      xasprintf (_("In the directive number %zu, the precision is too large."),
                                 spec.directives);
                    FDI_SET (f - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }
                format = f;
              }
            else if (*format == '*')
              {
                size_t precision_number = number;

                if (allocated == spec.numbered_arg_count)
                  {
                    allocated = 2 * allocated + 1;
                    spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, allocated * sizeof (struct numbered_arg));
                  }
                spec.numbered[spec.numbered_arg_count].number = precision_number;
                spec.numbered[spec.numbered_arg_count].type = FAT_INTEGER;
                spec.numbered_arg_count++;

                number++;
                format++;
              }
          }

       parse_value:
        if (*format == '[')
          {
            if (c_isdigit (format[1]))
              {
                const char *f = format + 1;
                size_t m = 0;

                do
                  {
                    if (m <= 1000000)
                      m = 10 * m + (*f - '0');
                    f++;
                  }
                while (c_isdigit (*f));

                if (*f == ']')
                  {
                    if (m == 0)
                      {
                        *invalid_reason = INVALID_ARGNO_0 (spec.directives);
                        FDI_SET (f, FMTDIR_ERROR);
                        goto bad_format;
                      }
                    if (m > 1000000)
                      {
                        *invalid_reason = INVALID_ARGNO_TOO_LARGE (spec.directives);
                        FDI_SET (f, FMTDIR_ERROR);
                        goto bad_format;
                      }
                    number = m;
                    format = ++f;
                    goto parse_specifier;
                  }
              }
          }

       parse_specifier: ;
        /* Parse the specifier.  */
        enum format_arg_type type;
        switch (*format)
          {
          case '%':
            type = FAT_NONE;
            break;
          case 'v':
            type = FAT_ANYVALUE;
            break;
          case 'T':
            type = FAT_ANYVALUE_TYPE;
            break;
          case 't':
            type = FAT_BOOLEAN;
            break;
          case 'c':
          case 'U':
            type = FAT_CHARACTER;
            break;
          case 's':
            type = FAT_STRING;
            break;
          case 'q':
            type = FAT_CHARACTER | FAT_STRING;
            break;
          case 'e': case 'E':
          case 'f': case 'F':
          case 'g': case 'G':
            type = FAT_FLOATINGPOINT;
            break;
          case 'O':
            type = FAT_INTEGER;
            break;
          case 'd':
          case 'o':
            type = FAT_INTEGER | FAT_POINTER;
            break;
          case 'b':
            type = FAT_INTEGER | FAT_FLOATINGPOINT | FAT_POINTER;
            break;
          case 'x': case 'X':
            type = FAT_INTEGER | FAT_FLOATINGPOINT | FAT_STRING | FAT_POINTER;
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
            if (allocated == spec.numbered_arg_count)
              {
                allocated = 2 * allocated + 1;
                spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, allocated * sizeof (struct numbered_arg));
              }
            spec.numbered[spec.numbered_arg_count].number = number;
            spec.numbered[spec.numbered_arg_count].type = type;
            spec.numbered_arg_count++;

            number++;
          }

        if (likely_intentional)
          spec.likely_intentional_directives++;
        FDI_SET (format, FMTDIR_END);

        format++;
      }

  /* Sort the numbered argument array, and eliminate duplicates.  */
  if (spec.numbered_arg_count > 1)
    {
      qsort (spec.numbered, spec.numbered_arg_count,
             sizeof (struct numbered_arg), numbered_arg_compare);

      /* Remove duplicates: Copy from i to j, keeping 0 <= j <= i.  */
      bool err = false;
      size_t i, j;
      for (i = j = 0; i < spec.numbered_arg_count; i++)
        if (j > 0 && spec.numbered[i].number == spec.numbered[j-1].number)
          {
            format_arg_type_t type1 = spec.numbered[i].type;
            format_arg_type_t type2 = spec.numbered[j-1].type;

            format_arg_type_t type_both = type1 & type2;
            if (type_both == FAT_NONE)
              {
                /* Incompatible types.  */
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

  /* The formatting functions in the Go package "fmt" treat an unused argument
     as an error.  Therefore here the translator must not omit some of the
     arguments.  */
  equality = true;

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
                  if (spec1->numbered[i].type != spec2->numbered[j].type)
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


struct formatstring_parser formatstring_go =
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
  size_t last = 1;
  for (size_t i = 0; i < spec->numbered_arg_count; i++)
    {
      size_t number = spec->numbered[i].number;

      if (i > 0)
        printf (" ");
      if (number < last)
        abort ();
      for (; last < number; last++)
        printf ("_ ");

      format_arg_type_t type = spec->numbered[i].type;
      if (type == FAT_NONE)
        abort ();
      if ((type & FAT_BOOLEAN) != 0)
        printf ("b");
      if ((type & FAT_CHARACTER) != 0)
        printf ("c");
      if ((type & FAT_STRING) != 0)
        printf ("s");
      if ((type & FAT_FLOATINGPOINT) != 0)
        printf ("f");
      if ((type & FAT_INTEGER) != 0)
        printf ("i");
      if ((type & FAT_POINTER) != 0)
        printf ("p");
      if ((type & (1 << 6)) != 0)
        printf ("*");
      if ((type & (1 << 7)) != 0)
        printf ("T");

      last = number + 1;
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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-go.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
