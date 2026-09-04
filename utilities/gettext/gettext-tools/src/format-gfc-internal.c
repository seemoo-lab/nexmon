/* GFC (GNU Fortran Compiler) internal format strings.
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

#include <stdbool.h>
#include <stdlib.h>

#include "format.h"
#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "format-invalid.h"
#include "gettext.h"

#define _(str) gettext (str)

/* GFC internal format strings consist of format directives that are specific
   to the GNU Fortran Compiler frontend of GCC, implemented in
   gcc-4.3.3/gcc/fortran/error.c (function error_print).

   A directive
   - starts with '%',
   - either is finished by '%', that needs no argument,
   - or is continued like this:
       - optionally 'm$' where m is a positive integer,
       - finished by a specifier
           - 'C', that needs no argument but uses a particular variable
                  (but for the purposes of 'm$' numbering it consumes an
                  argument nevertheless, of 'void' type),
           - 'L', that needs a 'locus *' argument,
           - 'i', 'd', that need a signed integer argument,
           - 'u', that needs an unsigned integer argument,
           - 'li', 'ld', that need a signed long integer argument,
           - 'lu', that needs an unsigned long integer argument,
           - 'c', that needs a character argument,
           - 's', that needs a string argument.

   Numbered ('%m$') and unnumbered argument specifications can be used in the
   same string. The effect of '%m$' is to set the current argument number to
   m. The current argument number is incremented after processing a directive.

   When numbered argument specifications are used, specifying the Nth argument
   requires that all the leading arguments, from the first to the (N-1)th, are
   specified in the format string.

   These gfc-internal format strings were removed from GCC on 2024-10-11 (for
   GCC 15.1.0).  */

enum format_arg_type
{
  FAT_NONE              = 0,
  /* Basic types */
  FAT_VOID              = 1,
  FAT_INTEGER           = 2,
  FAT_CHAR              = 3,
  FAT_STRING            = 4,
  FAT_LOCUS             = 5,
  /* Flags */
  FAT_UNSIGNED          = 1 << 3,
  FAT_SIZE_LONG         = 1 << 4,
  /* Bitmasks */
  FAT_SIZE_MASK         = FAT_SIZE_LONG
};
#ifdef __cplusplus
typedef int format_arg_type_t;
#else
typedef enum format_arg_type format_arg_type_t;
#endif

struct numbered_arg
{
  size_t number;
  format_arg_type_t type;
};

struct unnumbered_arg
{
  format_arg_type_t type;
};

struct spec
{
  size_t directives;
  size_t unnumbered_arg_count;
  struct unnumbered_arg *unnumbered;
  bool uses_currentloc;
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
  size_t numbered_arg_count = 0;
  size_t numbered_allocated = 0;
  struct numbered_arg *numbered = NULL;
  spec.uses_currentloc = false;
  size_t number = 1;

  for (; *format != '\0';)
    if (*format++ == '%')
      {
        /* A directive.  */
        FDI_SET (format - 1, FMTDIR_START);
        spec.directives++;

        if (*format != '%')
          {
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

            format_arg_type_t type;
            if (*format == 'C')
              {
                type = FAT_VOID;
                spec.uses_currentloc = true;
              }
            else if (*format == 'L')
              type = FAT_LOCUS;
            else if (*format == 'c')
              type = FAT_CHAR;
            else if (*format == 's')
              type = FAT_STRING;
            else
              {
                format_arg_type_t size = 0;

                if (*format == 'l')
                  {
                    ++format;
                    size = FAT_SIZE_LONG;
                  }

                if (*format == 'i' || *format == 'd')
                  type = FAT_INTEGER | size;
                else if (*format == 'u')
                  type = FAT_INTEGER | FAT_UNSIGNED | size;
                else
                  {
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
              }

            if (numbered_allocated == numbered_arg_count)
              {
                numbered_allocated = 2 * numbered_allocated + 1;
                numbered = (struct numbered_arg *) xrealloc (numbered, numbered_allocated * sizeof (struct numbered_arg));
              }
            numbered[numbered_arg_count].number = number;
            numbered[numbered_arg_count].type = type;
            numbered_arg_count++;

            number++;
          }

        FDI_SET (format, FMTDIR_END);

        format++;
      }

  /* Sort the numbered argument array, and eliminate duplicates.  */
  if (numbered_arg_count > 1)
    {
      qsort (numbered, numbered_arg_count,
             sizeof (struct numbered_arg), numbered_arg_compare);

      /* Remove duplicates: Copy from i to j, keeping 0 <= j <= i.  */
      bool err = false;
      size_t i, j;
      for (i = j = 0; i < numbered_arg_count; i++)
        if (j > 0 && numbered[i].number == numbered[j-1].number)
          {
            format_arg_type_t type1 = numbered[i].type;
            format_arg_type_t type2 = numbered[j-1].type;

            format_arg_type_t type_both;
            if (type1 == type2)
              type_both = type1;
            else
              {
                /* Incompatible types.  */
                type_both = FAT_NONE;
                if (!err)
                  *invalid_reason =
                    INVALID_INCOMPATIBLE_ARG_TYPES (numbered[i].number);
                err = true;
              }

            numbered[j-1].type = type_both;
          }
        else
          {
            if (j < i)
              {
                numbered[j].number = numbered[i].number;
                numbered[j].type = numbered[i].type;
              }
            j++;
          }
      numbered_arg_count = j;
      if (err)
        /* *invalid_reason has already been set above.  */
        goto bad_format;
    }

  /* Verify that the format string uses all arguments up to the highest
     numbered one.  */
  for (size_t i = 0; i < numbered_arg_count; i++)
    if (numbered[i].number != i + 1)
      {
        *invalid_reason =
          xasprintf (_("The string refers to argument number %zu but ignores argument number %zu."), numbered[i].number, i + 1);
        goto bad_format;
      }

  /* So now the numbered arguments array is equivalent to a sequence
     of unnumbered arguments.  Eliminate the FAT_VOID placeholders.  */
  {
    spec.unnumbered_arg_count = 0;
    for (size_t i = 0; i < numbered_arg_count; i++)
      if (numbered[i].type != FAT_VOID)
        spec.unnumbered_arg_count++;

    if (spec.unnumbered_arg_count > 0)
      {
        spec.unnumbered = XNMALLOC (spec.unnumbered_arg_count, struct unnumbered_arg);
        size_t j = 0;
        for (size_t i = 0; i < numbered_arg_count; i++)
          if (numbered[i].type != FAT_VOID)
            spec.unnumbered[j++].type = numbered[i].type;
      }
    else
      spec.unnumbered = NULL;
  }
  free (numbered);

  struct spec *result = XMALLOC (struct spec);
  *result = spec;
  return result;

 bad_format:
  if (numbered != NULL)
    free (numbered);
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->unnumbered != NULL)
    free (spec->unnumbered);
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

  /* Check the argument types are the same.  */
  if (equality
      ? spec1->unnumbered_arg_count != spec2->unnumbered_arg_count
      : spec1->unnumbered_arg_count < spec2->unnumbered_arg_count)
    {
      if (error_logger)
        error_logger (error_logger_data,
                      _("number of format specifications in '%s' and '%s' does not match"),
                      pretty_msgid, pretty_msgstr);
      err = true;
    }
  else
    for (size_t i = 0; i < spec2->unnumbered_arg_count; i++)
      if (spec1->unnumbered[i].type != spec2->unnumbered[i].type)
        {
          if (error_logger)
            error_logger (error_logger_data,
                          _("format specifications in '%s' and '%s' for argument %zu are not the same"),
                          pretty_msgid, pretty_msgstr, i + 1);
          err = true;
        }

  /* Check that the use of currentloc is the same.  */
  if (spec1->uses_currentloc != spec2->uses_currentloc)
    {
      if (error_logger)
        {
          if (spec1->uses_currentloc)
            error_logger (error_logger_data,
                          _("'%s' uses %%C but '%s' doesn't"),
                          pretty_msgid, pretty_msgstr);
          else
            error_logger (error_logger_data,
                          _("'%s' does not use %%C but '%s' uses %%C"),
                          pretty_msgid, pretty_msgstr);
        }
      err = true;
    }

  return err;
}


struct formatstring_parser formatstring_gfc_internal =
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
  for (size_t i = 0; i < spec->unnumbered_arg_count; i++)
    {
      if (i > 0)
        printf (" ");
      if (spec->unnumbered[i].type & FAT_UNSIGNED)
        printf ("[unsigned]");
      switch (spec->unnumbered[i].type & FAT_SIZE_MASK)
        {
        case 0:
          break;
        case FAT_SIZE_LONG:
          printf ("[long]");
          break;
        default:
          abort ();
        }
      switch (spec->unnumbered[i].type & ~(FAT_UNSIGNED | FAT_SIZE_MASK))
        {
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_CHAR:
          printf ("c");
          break;
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_LOCUS:
          printf ("L");
          break;
        default:
          abort ();
        }
    }
  printf (")");
  if (spec->uses_currentloc)
    printf (" C");
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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-gfc-internal.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
