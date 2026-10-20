/* GCC internal format strings.
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

/* GCC internal format strings consist of language frontend independent
   format directives, implemented in gcc-15.1.0/gcc/pretty-print.cc (method
   pretty_printer::format), plus some frontend dependent extensions, activated
   through set_format_decoder invocations:
     - for frontend independent diagnostics
       in gcc-15.1.0/gcc/tree-diagnostic.cc (function default_tree_printer)
     - for the C/ObjC frontend
       in gcc-15.1.0/gcc/c/c-objc-common.cc (function c_tree_printer)
     - for the C++ frontend
       in gcc-15.1.0/gcc/cp/error.cc (function cp_printer)
     - for the Fortran frontend
       in gcc-15.1.0/gcc/fortran/error.cc (function gfc_format_decoder)
   Taking these together, GCC internal format strings are specified as follows.

   A directive
   - starts with '%',
   - either is finished by one of these:
       - '%', '<', '>', '}', "'", 'R', that need no argument,
       - 'm', that needs no argument but looks at an err_no variable,
   - or is continued like this:
       - optionally 'm$' where m is a positive integer,
       - optionally any number of flags:
         'q' (once only),
         'l' (up to twice) or 'w' (once only) or 'z' (once only)
             or 't' (once only) (exclusive),
         '+' (once only),
         '#' (once only),
       - finished by a specifier

           - 'r', that needs a color argument and expects a '%R' later,
           - '{', that needs a URL argument and expects a '%}' later,
           - 'c', that needs a character argument,
           - 's', that needs a string argument,
           - '.NNNs', where NNN is a nonempty digit sequence, that needs a
             string argument,
           - '.*NNN$s' where NNN is a positive integer and NNN = m - 1, that
             needs a signed integer argument at position NNN and a string
             argument,
           - '.*s', that needs a signed integer argument and a string argument,
           - 'i', 'd', that need a signed integer argument of the specified
             size,
           - 'o', 'u', 'x', that need an unsigned integer argument of the
             specified size,
           - 'f', that needs a floating-point argument,
           - 'p', that needs a 'void *' argument,
           - '@', that needs a 'diagnostic_event_id_t *' argument,
           - 'e', that needs a 'pp_element *' argument,
           - 'Z', that needs an 'int *' argument and an 'unsigned int' argument,
             [see gcc/pretty-print.cc]

           - 'D', that needs a general declaration argument,
           - 'E', that needs an expression argument,
           - 'F', that needs a function declaration argument,
           - 'T', that needs a type argument,
             [see gcc/tree-diagnostic.cc and gcc/c/c-objc-common.cc and
              gcc/cp/error.cc]

           - 'V', that needs a tree argument with a list of type qualifiers,
             [see gcc/c/c-objc-common.cc and gcc/cp/error.cc]

           - 'v', that needs a list of type qualifiers,
             [see gcc/c/c-objc-common.cc]

           - 'A', that needs a function argument list argument,
           - 'H', that needs a type argument and prints it specially
                  (as first type of a type difference),
           - 'I', that needs a type argument and prints it specially
                  (as second type of a type difference),
           - 'O', that needs a binary operator argument,
           - 'P', that needs a function parameter index argument,
           - 'Q', that needs an assignment operator argument,
           - 'S', that needs a substitution,
           - 'X', that needs an exception,
             [see gcc/cp/error.cc]

           - 'C', that needs a tree code argument in the cp/ front end [IGNORE!]
                  but no argument in the fortran/ front end,
           - 'L', that needs a language identifier argument in the cp/ front end
                  or a locus argument in the fortran/ front end,
             [see gcc/cp/error.cc and gcc/fortran/error.cc]

   Directives %< and %> should come in pairs; these pairs cannot be nested.
   Directives %r and %R should come in pairs; these pairs cannot be nested.
   Directives %{ and %} should come in pairs; these pairs cannot be nested.

   Numbered ('%m$' or '*m$') and unnumbered argument specifications cannot
   be used in the same string.  */

enum format_arg_type
{
  FAT_NONE              = 0,
  /* Basic types */
  FAT_INTEGER           = 1,
  FAT_CHAR              = 2,
  FAT_FLOAT             = 3,
  FAT_STRING            = 4,
  FAT_POINTER           = 5,
  FAT_TREE              = 6,
  FAT_TREE_CODE         = 7,
  FAT_EVENT_ID          = 8,
  FAT_ELEMENT           = 9,
  FAT_LANGUAGE_OR_LOCUS = 10,
  FAT_CV                = 11,
  FAT_INT_ARRAY_PART1   = 12,
  FAT_INT_ARRAY_PART2   = 13,
  FAT_COLOR             = 14,
  FAT_URL               = 15,
  /* Flags */
  FAT_UNSIGNED          = 1 << 4,
  FAT_SIZE_LONG         = 1 << 5,
  FAT_SIZE_LONGLONG     = 2 << 5,
  FAT_SIZE_WIDE         = 3 << 5,
  FAT_SIZE_SIZE         = 4 << 5,
  FAT_SIZE_PTRDIFF      = 5 << 5,
  FAT_TREE_DECL         = 1 << 8,
  FAT_TREE_STATEMENT    = 2 << 8,
  FAT_TREE_FUNCDECL     = 3 << 8,
  FAT_TREE_TYPE         = 4 << 8,
  FAT_TREE_TYPE_DIFF1   = 5 << 8,
  FAT_TREE_TYPE_DIFF2   = 6 << 8,
  FAT_TREE_ARGUMENT     = 7 << 8,
  FAT_TREE_EXPRESSION   = 8 << 8,
  FAT_TREE_CV           = 9 << 8,
  FAT_TREE_SUBSTITUTION = 10 << 8,
  FAT_TREE_EXCEPTION    = 11 << 8,
  FAT_TREE_CODE_BINOP   = 1 << 12,
  FAT_TREE_CODE_ASSOP   = 2 << 12,
  FAT_FUNCPARAM         = 1 << 14,
  /* Bitmasks */
  FAT_SIZE_MASK         = (FAT_SIZE_LONG | FAT_SIZE_LONGLONG
                           | FAT_SIZE_WIDE | FAT_SIZE_SIZE | FAT_SIZE_PTRDIFF)
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

struct spec
{
  size_t directives;
  size_t numbered_arg_count;
  struct numbered_arg *numbered;
  bool uses_err_no;
  bool uses_current_locus;
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
  spec.numbered_arg_count = 0;
  spec.numbered = NULL;
  spec.uses_err_no = false;
  spec.uses_current_locus = false;
  size_t numbered_allocated = 0;
  size_t unnumbered_arg_count = 0;
  size_t in_quote_group = 0;
  size_t in_color_group = 0;
  size_t in_url_group = 0;

  for (; *format != '\0';)
    /* Invariant: spec.numbered_arg_count == 0 || unnumbered_arg_count == 0.  */
    if (*format++ == '%')
      {
        /* A directive.  */
        FDI_SET (format - 1, FMTDIR_START);
        spec.directives++;

        if (*format == '%' || *format == '\'')
          ;
        else if (*format == '<')
          {
            if (in_quote_group)
              {
                *invalid_reason = xasprintf (_("The directive number %zu opens a quote group, but the previous one is not terminated."), spec.directives);
                FDI_SET (format, FMTDIR_ERROR);
                goto bad_format;
              }
            in_quote_group = spec.directives;
          }
        else if (*format == '>')
          {
            if (!in_quote_group)
              {
                *invalid_reason = xasprintf (_("The directive number %zu does not match a preceding '%%%c'."), spec.directives, '<');
                FDI_SET (format, FMTDIR_ERROR);
                goto bad_format;
              }
            in_quote_group = 0;
          }
        else if (*format == 'R')
          {
            if (!in_color_group)
              {
                *invalid_reason = xasprintf (_("The directive number %zu does not match a preceding '%%%c'."), spec.directives, 'r');
                FDI_SET (format, FMTDIR_ERROR);
                goto bad_format;
              }
            in_color_group = 0;
          }
        else if (*format == '}')
          {
            if (!in_url_group)
              {
                *invalid_reason = xasprintf (_("The directive number %zu does not match a preceding '%%%c'."), spec.directives, '{');
                FDI_SET (format, FMTDIR_ERROR);
                goto bad_format;
              }
            in_url_group = 0;
          }
        else if (*format == 'm')
          spec.uses_err_no = true;
        else if (*format == 'C')
          spec.uses_current_locus = true;
        else
          {
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

            /* Parse flags and size.  */
            unsigned int flag_q = 0;
            unsigned int flag_l = 0;
            unsigned int flag_w = 0;
            unsigned int flag_z = 0;
            unsigned int flag_t = 0;
            unsigned int flag_plus = 0;
            unsigned int flag_sharp = 0;
            for (;; format++)
              {
                switch (*format)
                  {
                  case 'q':
                    if (flag_q > 0)
                      goto invalid_flags;
                    flag_q = 1;
                    continue;
                  case 'l':
                    if (flag_l > 1 || flag_w || flag_z || flag_t)
                      goto invalid_flags;
                    flag_l++;
                    continue;
                  case 'w':
                    if (flag_l || flag_w || flag_z || flag_t)
                      goto invalid_flags;
                    flag_w = 1;
                    continue;
                  case 'z':
                    if (flag_l || flag_w || flag_z || flag_t)
                      goto invalid_flags;
                    flag_z = 1;
                    continue;
                  case 't':
                    if (flag_l || flag_w || flag_z || flag_t)
                      goto invalid_flags;
                    flag_t = 1;
                    continue;
                  case '+':
                    if (flag_plus > 0)
                      goto invalid_flags;
                    flag_plus = 1;
                    continue;
                  case '#':
                    if (flag_sharp > 0)
                      goto invalid_flags;
                    flag_sharp = 1;
                    continue;
                  invalid_flags:
                    *invalid_reason = xasprintf (_("In the directive number %zu, the flags combination is invalid."), spec.directives);
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  default:
                    break;
                  }
                break;
              }
            format_arg_type_t size =
              (flag_l == 2 ? FAT_SIZE_LONGLONG :
               flag_l == 1 ? FAT_SIZE_LONG :
               flag_w ? FAT_SIZE_WIDE :
               flag_z ? FAT_SIZE_SIZE :
               flag_t ? FAT_SIZE_PTRDIFF :
               0);

            format_arg_type_t type;
            if (*format == 'c')
              type = FAT_CHAR;
            else if (*format == 's')
              type = FAT_STRING;
            else if (*format == '.')
              {
                format++;

                if (c_isdigit (*format))
                  {
                    do
                      format++;
                    while (c_isdigit (*format));

                    if (*format != 's')
                      {
                        if (*format == '\0')
                          {
                            *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
                            FDI_SET (format - 1, FMTDIR_ERROR);
                          }
                        else
                          {
                            *invalid_reason =
                              xasprintf (_("In the directive number %zu, a precision is not allowed before '%c'."), spec.directives, *format);
                            FDI_SET (format, FMTDIR_ERROR);
                          }
                        goto bad_format;
                      }

                    type = FAT_STRING;
                  }
                else if (*format == '*')
                  {
                    format++;

                    size_t precision_number = 0;
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
                                *invalid_reason = INVALID_WIDTH_ARGNO_0 (spec.directives);
                                FDI_SET (f, FMTDIR_ERROR);
                                goto bad_format;
                              }
                            if (unnumbered_arg_count > 0 || number == 0)
                              {
                                *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                                FDI_SET (f, FMTDIR_ERROR);
                                goto bad_format;
                              }
                            if (m != number - 1)
                              {
                                *invalid_reason = xasprintf (_("In the directive number %zu, the argument number for the precision must be equal to %zu."), spec.directives, number - 1);
                                FDI_SET (f, FMTDIR_ERROR);
                                goto bad_format;
                              }
                            precision_number = m;
                            format = ++f;
                          }
                      }

                    if (precision_number)
                      {
                        /* Numbered argument.  */

                        /* Numbered and unnumbered specifications are exclusive.  */
                        if (unnumbered_arg_count > 0)
                          {
                            *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                            FDI_SET (format - 1, FMTDIR_ERROR);
                            goto bad_format;
                          }

                        if (numbered_allocated == spec.numbered_arg_count)
                          {
                            numbered_allocated = 2 * numbered_allocated + 1;
                            spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, numbered_allocated * sizeof (struct numbered_arg));
                          }
                        spec.numbered[spec.numbered_arg_count].number = precision_number;
                        spec.numbered[spec.numbered_arg_count].type = FAT_INTEGER;
                        spec.numbered_arg_count++;
                      }
                    else
                      {
                        /* Unnumbered argument.  */

                        /* Numbered and unnumbered specifications are exclusive.  */
                        if (spec.numbered_arg_count > 0)
                          {
                            *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                            FDI_SET (format - 1, FMTDIR_ERROR);
                            goto bad_format;
                          }

                        if (numbered_allocated == unnumbered_arg_count)
                          {
                            numbered_allocated = 2 * numbered_allocated + 1;
                            spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, numbered_allocated * sizeof (struct numbered_arg));
                          }
                        spec.numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                        spec.numbered[unnumbered_arg_count].type = FAT_INTEGER;
                        unnumbered_arg_count++;
                      }

                    if (*format == 's')
                      type = FAT_STRING;
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
                              xasprintf (_("In the directive number %zu, a precision specification is not allowed before '%c'."), spec.directives, *format);
                            FDI_SET (format, FMTDIR_ERROR);
                          }
                        goto bad_format;
                      }
                  }
                else
                  {
                    *invalid_reason = xasprintf (_("In the directive number %zu, the precision specification is invalid."), spec.directives);
                    FDI_SET (*format == '\0' ? format - 1 : format,
                             FMTDIR_ERROR);
                    goto bad_format;
                  }
              }
            else if (*format == 'i' || *format == 'd')
              type = FAT_INTEGER | size;
            else if (*format == 'o' || *format == 'u' || *format == 'x')
              type = FAT_INTEGER | FAT_UNSIGNED | size;
            else if (*format == 'f')
              type = FAT_FLOAT;
            else if (*format == 'p')
              type = FAT_POINTER;
            else if (*format == '@')
              type = FAT_EVENT_ID;
            else if (*format == 'e')
              type = FAT_ELEMENT;
            else if (*format == 'v')
              type = FAT_CV;
            else if (*format == 'Z')
              type = FAT_INT_ARRAY_PART1;
            else if (*format == 'r')
              {
                if (in_color_group)
                  {
                    *invalid_reason = xasprintf (_("The directive number %zu opens a color group, but the previous one is not terminated."), spec.directives);
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }
                in_color_group = spec.directives;
                type = FAT_COLOR;
              }
            else if (*format == '{')
              {
                if (in_url_group)
                  {
                    *invalid_reason = xasprintf (_("The directive number %zu opens a URL group, but the previous one is not terminated."), spec.directives);
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }
                in_url_group = spec.directives;
                type = FAT_URL;
              }
            else if (*format == 'D')
              type = FAT_TREE | FAT_TREE_DECL;
            else if (*format == 'F')
              type = FAT_TREE | FAT_TREE_FUNCDECL;
            else if (*format == 'T')
              type = FAT_TREE | FAT_TREE_TYPE;
            else if (*format == 'H')
              type = FAT_TREE | FAT_TREE_TYPE_DIFF1;
            else if (*format == 'I')
              type = FAT_TREE | FAT_TREE_TYPE_DIFF2;
            else if (*format == 'E')
              type = FAT_TREE | FAT_TREE_EXPRESSION;
            else if (*format == 'A')
              type = FAT_TREE | FAT_TREE_ARGUMENT;
            else if (*format == 'C')
              type = FAT_TREE_CODE;
            else if (*format == 'L')
              type = FAT_LANGUAGE_OR_LOCUS;
            else if (*format == 'O')
              type = FAT_TREE_CODE | FAT_TREE_CODE_BINOP;
            else if (*format == 'P')
              type = FAT_INTEGER | FAT_FUNCPARAM;
            else if (*format == 'Q')
              type = FAT_TREE_CODE | FAT_TREE_CODE_ASSOP;
            else if (*format == 'V')
              type = FAT_TREE | FAT_TREE_CV;
            else if (*format == 'S')
              type = FAT_TREE | FAT_TREE_SUBSTITUTION;
            else if (*format == 'X')
              type = FAT_TREE | FAT_TREE_EXCEPTION;
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

                if (type == FAT_INT_ARRAY_PART1)
                  {
                    if (numbered_allocated == spec.numbered_arg_count)
                      {
                        numbered_allocated = 2 * numbered_allocated + 1;
                        spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, numbered_allocated * sizeof (struct numbered_arg));
                      }
                    spec.numbered[spec.numbered_arg_count].number = number + 1;
                    spec.numbered[spec.numbered_arg_count].type = FAT_INT_ARRAY_PART2;
                    spec.numbered_arg_count++;
                  }
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

                if (type == FAT_INT_ARRAY_PART1)
                  {
                    if (numbered_allocated == unnumbered_arg_count)
                      {
                        numbered_allocated = 2 * numbered_allocated + 1;
                        spec.numbered = (struct numbered_arg *) xrealloc (spec.numbered, numbered_allocated * sizeof (struct numbered_arg));
                      }
                    spec.numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                    spec.numbered[unnumbered_arg_count].type = FAT_INT_ARRAY_PART2;
                    unnumbered_arg_count++;
                  }
              }
          }

        FDI_SET (format, FMTDIR_END);

        format++;
      }

  if (in_quote_group)
    {
      *invalid_reason = xasprintf (_("The quote group opened by the directive number %zu is not terminated."), in_quote_group);
      goto bad_format;
    }
  if (in_color_group)
    {
      *invalid_reason = xasprintf (_("The color group opened by the directive number %zu is not terminated."), in_color_group);
      goto bad_format;
    }
  if (in_url_group)
    {
      *invalid_reason = xasprintf (_("The URL group opened by the directive number %zu is not terminated."), in_url_group);
      goto bad_format;
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
            format_arg_type_t type1 = spec.numbered[i].type;
            format_arg_type_t type2 = spec.numbered[j-1].type;

            format_arg_type_t type_both;
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

  /* Check that the use of err_no is the same.  */
  if (spec1->uses_err_no != spec2->uses_err_no)
    {
      if (error_logger)
        {
          if (spec1->uses_err_no)
            error_logger (error_logger_data,
                          _("'%s' uses %%m but '%s' doesn't"),
                          pretty_msgid, pretty_msgstr);
          else
            error_logger (error_logger_data,
                          _("'%s' does not use %%m but '%s' uses %%m"),
                          pretty_msgid, pretty_msgstr);
        }
      err = true;
    }

  /* Check that the use of current_locus is the same.  */
  if (spec1->uses_current_locus != spec2->uses_current_locus)
    {
      if (error_logger)
        {
          if (spec1->uses_current_locus)
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


struct formatstring_parser formatstring_gcc_internal =
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
      if (spec->numbered[i].type & FAT_UNSIGNED)
        printf ("[unsigned]");
      switch (spec->numbered[i].type & FAT_SIZE_MASK)
        {
        case 0:
          break;
        case FAT_SIZE_LONG:
          printf ("[long]");
          break;
        case FAT_SIZE_LONGLONG:
          printf ("[long long]");
          break;
        case FAT_SIZE_WIDE:
          printf ("[host-wide]");
          break;
        case FAT_SIZE_SIZE:
          printf ("[host-size_t]");
          break;
        case FAT_SIZE_PTRDIFF:
          printf ("[host-ptrdiff_t]");
          break;
        default:
          abort ();
        }
      switch (spec->numbered[i].type & ~(FAT_UNSIGNED | FAT_SIZE_MASK))
        {
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_INTEGER | FAT_FUNCPARAM:
          printf ("P");
          break;
        case FAT_CHAR:
          printf ("c");
          break;
        case FAT_FLOAT:
          printf ("f");
          break;
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_POINTER:
          printf ("p");
          break;
        case FAT_EVENT_ID:
          printf ("@");
          break;
        case FAT_ELEMENT:
          printf ("e");
          break;
        case FAT_CV:
          printf ("v");
          break;
        case FAT_INT_ARRAY_PART1:
          printf ("Z1");
          break;
        case FAT_INT_ARRAY_PART2:
          printf ("Z2");
          break;
        case FAT_COLOR:
          printf ("r");
          break;
        case FAT_URL:
          printf ("{");
          break;
        case FAT_TREE | FAT_TREE_DECL:
          printf ("D");
          break;
        case FAT_TREE | FAT_TREE_STATEMENT:
          printf ("K");
          break;
        case FAT_TREE | FAT_TREE_FUNCDECL:
          printf ("F");
          break;
        case FAT_TREE | FAT_TREE_TYPE:
          printf ("T");
          break;
        case FAT_TREE | FAT_TREE_TYPE_DIFF1:
          printf ("H");
          break;
        case FAT_TREE | FAT_TREE_TYPE_DIFF2:
          printf ("I");
          break;
        case FAT_TREE | FAT_TREE_ARGUMENT:
          printf ("A");
          break;
        case FAT_TREE | FAT_TREE_EXPRESSION:
          printf ("E");
          break;
        case FAT_TREE | FAT_TREE_CV:
          printf ("V");
          break;
        case FAT_TREE | FAT_TREE_SUBSTITUTION:
          printf ("S");
          break;
        case FAT_TREE | FAT_TREE_EXCEPTION:
          printf ("X");
          break;
        case FAT_TREE_CODE:
          printf ("C");
          break;
        case FAT_TREE_CODE | FAT_TREE_CODE_BINOP:
          printf ("O");
          break;
        case FAT_TREE_CODE | FAT_TREE_CODE_ASSOP:
          printf ("Q");
          break;
        case FAT_LANGUAGE_OR_LOCUS:
          printf ("L");
          break;
        default:
          abort ();
        }
      last = number + 1;
    }
  printf (")");
  if (spec->uses_err_no)
    printf (" ERR_NO");
  if (spec->uses_current_locus)
    printf (" CURRENT_LOCUS");
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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-gcc-internal.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
