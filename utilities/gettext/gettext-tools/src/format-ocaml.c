/* OCaml format strings.
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
#include <string.h>

#include "format.h"
#include "gettext.h"
#include "xalloc.h"
#include "format-invalid.h"
#include "c-ctype.h"
#include "xvasprintf.h"

#define _(str) gettext (str)

/* The OCaml format strings are described in the OCaml reference manual,
   at https://ocaml.org/manual/5.3/api/Printf.html#VALfprintf .
   They are implemented in ocaml-5.3.0/stdlib/scanf.ml.

   A directive
   - starts with '%',
   - [in msgstr only] is optionally followed by
       a positive integer m, then '$'
   - is optionally followed by a sequence of flags, each being one of
       '+', '-', ' ', '0', '#',
   - is optionally followed by a width specification:
       a positive integer, or
       '*', or
       [in msgstr only] '*', then a positive integer, then '$',
   - is optionally followed by a precision specification:
       '.' then optionally:
         a positive integer, or
         '*', or
         [in msgstr only] '*', then a positive integer, then '$',
   - is finished by a specifier
       - 'd', 'i', 'u', 'x', 'X', 'o', that need an integer argument,
       - 'l' then 'd', 'i', 'u', 'x', 'X', 'o', that need an int32 argument,
       - 'n' then 'd', 'i', 'u', 'x', 'X', 'o', that need an nativeint argument,
       - 'L' then 'd', 'i', 'u', 'x', 'X', 'o', that need an int64 argument,
       - 's', that needs a string argument,
       - 'S', that needs a string argument and outputs it in OCaml syntax,
       - 'c', that needs a character argument,
       - 'C', that needs a character argument and outputs it in OCaml syntax,
       - 'f', 'e', 'E', 'g', 'G', 'h', 'H', that need a floating-point argument,
       - 'F', that needs a floating-point argument and outputs it in OCaml syntax,
       - 'B', that needs a boolean argument,
       - 'a', that takes a function (of type : out_channel -> unit) argument,
       - 't', that takes two arguments: a function (of type : out_channel -> <T> -> unit)
              and a <T>,
       - '{' FMT '%}', that takes a format string argument without msgstr
         extensions, expected to have the same signature as FMT, effectively
         ignores it, and instead outputs the minimal format string with the
         same signature as FMT: a concatenation of
           - "%i" for an integer argument,
           - "%li" for an int32 argument,
           - "%ni" for a nativeint argument,
           - "%Li" for an int64 argument,
           - "%s" for a string argument,
           - "%c" for a character argument,
           - "%f" for a floating-point argument,
           - "%B" for a boolean argument,
           - "%a" for a function argument,
           - "%t" for two arguments, as described above,
       - '(' FMT '%)', that takes a format string argument without msgstr
         extensions, expected to have the same signature as FMT, and a set
         of arguments suitable for FMT,
       - '!', '%', '@', ',', that take no argument.
   Numbered ('%m$' or '*m$') and unnumbered argument specifications cannot
   be used in the same string.
 */

enum format_arg_type
{
  FAT_NONE              = 0,
  /* Basic types */
  FAT_INTEGER           = 1,
  FAT_INT32             = 2,
  FAT_NATIVEINT         = 3,
  FAT_INT64             = 4,
  FAT_STRING            = 5,
  FAT_CHARACTER         = 6,
  FAT_FLOATINGPOINT     = 7,
  FAT_BOOLEAN           = 8,
  FAT_FUNCTION_A        = 9,
  FAT_FUNCTION_T        = 10, /* first argument for %t */
  FAT_FUNCTION_T2       = 11, /* second argument for %t */
  FAT_FORMAT_STRING     = 12,
  /* Flags */
  FAT_OCAML_SYNTAX          = 1 << 4,
  FAT_OPTIONAL_OCAML_SYNTAX = 1 << 5,
  /* Bitmasks */
  FAT_BASIC_MASK        = (FAT_INTEGER | FAT_INT32 | FAT_NATIVEINT | FAT_INT64
                           | FAT_STRING | FAT_CHARACTER | FAT_FLOATINGPOINT
                           | FAT_BOOLEAN | FAT_FUNCTION_A | FAT_FUNCTION_T
                           | FAT_FUNCTION_T2 | FAT_FORMAT_STRING)
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
  char *signature;        /* for type == FAT_FORMAT_STRING */
};

struct spec
{
  size_t directives;
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

/* Frees the memory held by *spec.  */
static void
destroy_spec (struct spec *spec)
{
  if (spec->numbered != NULL)
    {
      for (size_t i = spec->numbered_arg_count; i > 0; )
        {
          --i;
          if (spec->numbered[i].type == FAT_FORMAT_STRING)
            free (spec->numbered[i].signature);
        }
      free (spec->numbered);
    }
}

/* Returns the signature of a format string
   as a freshly allocated string.  */
static char *
format_string_signature (const struct spec *spec)
{
  size_t len;
  {
    size_t i;
    const struct numbered_arg *p;
    len = spec->numbered_arg_count;
    for (i = 0, p = spec->numbered; i < spec->numbered_arg_count; i++, p++)
      if ((p->type & FAT_BASIC_MASK) == FAT_FORMAT_STRING)
        len += strlen (p->signature) + 1;
  }
  char *signature = (char *) xmalloc (len + 1);
  {
    size_t i;
    const struct numbered_arg *p;
    char *s;
    for (i = 0, p = spec->numbered, s = signature;
         i < spec->numbered_arg_count;
         i++, p++)
      switch (p->type & FAT_BASIC_MASK)
        {
        case FAT_INTEGER:
          *s++ = 'i';
          break;
        case FAT_INT32:
          *s++ = 'l';
          break;
        case FAT_NATIVEINT:
          *s++ = 'n';
          break;
        case FAT_INT64:
          *s++ = 'L';
          break;
        case FAT_STRING:
          *s++ = 's';
          break;
        case FAT_CHARACTER:
          *s++ = 'c';
          break;
        case FAT_FLOATINGPOINT:
          *s++ = 'f';
          break;
        case FAT_BOOLEAN:
          *s++ = 'B';
          break;
        case FAT_FUNCTION_A:
          *s++ = 'a';
          break;
        case FAT_FUNCTION_T:
          *s++ = 't';
          break;
        case FAT_FUNCTION_T2:
          break;
        case FAT_FORMAT_STRING:
          *s++ = '(';
          memcpy (s, p->signature, strlen (p->signature));
          s += strlen (p->signature);
          *s++ = ')';
          break;
        default:
          abort ();
        }
    *s = '\0';
  }
  return signature;
}

/* When a type is specified via format string substitution, e.g. "%(%s%)", both
   the variant without OCaml syntax "%s" and the variant with OCaml syntax "%S"
   are allowed.  */
static format_arg_type_t
type_without_translator_constraint (format_arg_type_t type)
{
  switch (type & FAT_BASIC_MASK)
    {
    case FAT_STRING:
    case FAT_CHARACTER:
    case FAT_FLOATINGPOINT:
      return (type & FAT_BASIC_MASK) | FAT_OPTIONAL_OCAML_SYNTAX;
    default:
      return type;
    }
}

/* Parse a piece of format string, until the matching terminating format
   directive is encountered.
   spec is the global struct spec.
   format is the remainder of the format string.
   It is updated upon valid return.
   terminator is '\0' at the top-level, otherwise '}' or ')'.
   translated is true when msgstr extensions should be accepted.
   fdi is an array to be filled with format directive indicators, or NULL.
   If the format string is invalid, false is returned and *invalid_reason is
   set to an error message explaining why.  */
static bool
parse_upto (struct spec *spec,
            const char **formatp,
            char terminator, bool translated,
            char *fdi, char **invalid_reason)
{
  const char *format = *formatp;
  const char *const format_start = format;

  spec->directives = 0;
  spec->numbered_arg_count = 0;
  spec->numbered = NULL;
  size_t numbered_allocated = 0;
  size_t unnumbered_arg_count = 0;

  for (; *format != '\0';)
    /* Invariant: spec->numbered_arg_count == 0 || unnumbered_arg_count == 0.  */
    if (*format++ == '%')
      {
        /* A directive.  */
        FDI_SET (format - 1, FMTDIR_START);
        spec->directives++;

        size_t number = 0;
        if (translated && c_isdigit (*format))
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
                    *invalid_reason = INVALID_ARGNO_0 (spec->directives);
                    FDI_SET (f, FMTDIR_ERROR);
                    goto bad_format;
                  }
                number = m;
                format = ++f;
              }
          }

        /* Parse flags.  */
        while (*format == ' ' || *format == '+' || *format == '-'
               || *format == '#' || *format == '0')
          format++;

        /* Parse width.  */
        if (*format == '*')
          {
            format++;

            size_t width_number = 0;
            if (translated && c_isdigit (*format))
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
                        *invalid_reason =
                          INVALID_WIDTH_ARGNO_0 (spec->directives);
                        FDI_SET (f, FMTDIR_ERROR);
                        goto bad_format;
                      }
                    width_number = m;
                    format = ++f;
                  }
              }

            if (width_number)
              {
                /* Numbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (unnumbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (numbered_allocated == spec->numbered_arg_count)
                  {
                    numbered_allocated = 2 * numbered_allocated + 1;
                    spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                  }
                spec->numbered[spec->numbered_arg_count].number = width_number;
                spec->numbered[spec->numbered_arg_count].type = FAT_INTEGER;
                spec->numbered_arg_count++;
              }
            else
              {
                /* Unnumbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (spec->numbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (numbered_allocated == unnumbered_arg_count)
                  {
                    numbered_allocated = 2 * numbered_allocated + 1;
                    spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                  }
                spec->numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                spec->numbered[unnumbered_arg_count].type = FAT_INTEGER;
                unnumbered_arg_count++;
              }
          }
        else if (c_isdigit (*format))
          {
            do format++; while (c_isdigit (*format));
          }

        /* Parse precision.  */
        if (*format == '.')
          {
            format++;

            if (*format == '*')
              {
                format++;

                size_t precision_number = 0;
                if (translated && c_isdigit (*format))
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
                            *invalid_reason =
                              INVALID_PRECISION_ARGNO_0 (spec->directives);
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

                    if (numbered_allocated == spec->numbered_arg_count)
                      {
                        numbered_allocated = 2 * numbered_allocated + 1;
                        spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                      }
                    spec->numbered[spec->numbered_arg_count].number = precision_number;
                    spec->numbered[spec->numbered_arg_count].type = FAT_INTEGER;
                    spec->numbered_arg_count++;
                  }
                else
                  {
                    /* Unnumbered argument.  */

                    /* Numbered and unnumbered specifications are exclusive.  */
                    if (spec->numbered_arg_count > 0)
                      {
                        *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                        FDI_SET (format - 1, FMTDIR_ERROR);
                        goto bad_format;
                      }

                    if (numbered_allocated == unnumbered_arg_count)
                      {
                        numbered_allocated = 2 * numbered_allocated + 1;
                        spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                      }
                    spec->numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                    spec->numbered[unnumbered_arg_count].type = FAT_INTEGER;
                    unnumbered_arg_count++;
                  }
              }
            else if (c_isdigit (*format))
              {
                do format++; while (c_isdigit (*format));
              }
          }

        /* Parse the specifier.  */
        enum format_arg_type integer_type = FAT_INTEGER;
        if (*format == 'l')
          {
            integer_type = FAT_INT32;
            format++;
          }
        else if (*format == 'n')
          {
            integer_type = FAT_NATIVEINT;
            format++;
          }
        else if (*format == 'L')
          {
            integer_type = FAT_INT64;
            format++;
          }

        format_arg_type_t type;
        char *signature = NULL;
        switch (*format)
          {
          case 'd':
          case 'i':
          case 'u':
          case 'x': case 'X':
          case 'o':
            type = integer_type;
            break;
          default:
            if (integer_type != FAT_INTEGER)
              --format;
            switch (*format)
              {
              case 's':
                type = FAT_STRING;
                break;
              case 'S':
                type = FAT_STRING | FAT_OCAML_SYNTAX;
                break;
              case 'c':
                type = FAT_CHARACTER;
                break;
              case 'C':
                type = FAT_CHARACTER | FAT_OCAML_SYNTAX;
                break;
              case 'f':
              case 'e': case 'E':
              case 'g': case 'G':
              case 'h': case 'H':
                type = FAT_FLOATINGPOINT;
                break;
              case 'F':
                type = FAT_FLOATINGPOINT | FAT_OCAML_SYNTAX;
                break;
              case 'B':
                type = FAT_BOOLEAN;
                break;
              case 'a':
                type = FAT_FUNCTION_A;
                break;
              case 't':
                type = FAT_FUNCTION_T;
                break;
              case '{':
                {
                  struct spec sub_spec;
                  *formatp = format;
                  if (!parse_upto (&sub_spec, formatp, '}', false,
                                   fdi, invalid_reason))
                    {
                      FDI_SET (**formatp == '\0' ? *formatp - 1 : *formatp,
                               FMTDIR_ERROR);
                      goto bad_format;
                    }
                  format = *formatp;
                  type = FAT_FORMAT_STRING;
                  signature = format_string_signature (&sub_spec);
                  destroy_spec (&sub_spec);
                }
                break;
              case '}':
                if (terminator != '}')
                  {
                    *invalid_reason =
                      xasprintf (_("Found '%s' without matching '%s'."), "%}", "%{");
                    FDI_SET (format - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }
                spec->directives--;
                goto done;
              case '(':
                {
                  struct spec sub_spec;
                  *formatp = format;
                  if (!parse_upto (&sub_spec, formatp, ')', false,
                                   fdi, invalid_reason))
                    {
                      FDI_SET (**formatp == '\0' ? *formatp - 1 : *formatp,
                               FMTDIR_ERROR);
                      goto bad_format;
                    }
                  format = *formatp;
                  type = FAT_FORMAT_STRING;
                  signature = format_string_signature (&sub_spec);

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

                      size_t new_numbered_arg_count =
                        spec->numbered_arg_count + 1 + sub_spec.numbered_arg_count;
                      if (numbered_allocated < new_numbered_arg_count)
                        {
                          numbered_allocated = 2 * numbered_allocated + 1;
                          if (numbered_allocated < new_numbered_arg_count)
                            numbered_allocated = new_numbered_arg_count;
                          spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                        }
                      spec->numbered[spec->numbered_arg_count].number = number;
                      spec->numbered[spec->numbered_arg_count].type = type;
                      spec->numbered[spec->numbered_arg_count].signature = signature;
                      spec->numbered_arg_count++;
                      for (size_t i = 0; i < sub_spec.numbered_arg_count; i++)
                        {
                          spec->numbered[spec->numbered_arg_count].number = number + sub_spec.numbered[i].number;
                          spec->numbered[spec->numbered_arg_count].type =
                            type_without_translator_constraint (sub_spec.numbered[i].type);
                          if (sub_spec.numbered[i].type == FAT_FORMAT_STRING)
                            spec->numbered[spec->numbered_arg_count].signature = sub_spec.numbered[i].signature;
                          spec->numbered_arg_count++;
                        }
                    }
                  else
                    {
                      /* Unnumbered argument.  */

                      /* Numbered and unnumbered specifications are exclusive.  */
                      if (spec->numbered_arg_count > 0)
                        {
                          *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                          FDI_SET (format, FMTDIR_ERROR);
                          goto bad_format;
                        }

                      size_t new_unnumbered_arg_count =
                        unnumbered_arg_count + 1 + sub_spec.numbered_arg_count;
                      if (numbered_allocated < new_unnumbered_arg_count)
                        {
                          numbered_allocated = 2 * numbered_allocated + 1;
                          if (numbered_allocated < new_unnumbered_arg_count)
                            numbered_allocated = new_unnumbered_arg_count;
                          spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                        }
                      spec->numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                      spec->numbered[unnumbered_arg_count].type = type;
                      spec->numbered[unnumbered_arg_count].signature = signature;
                      unnumbered_arg_count++;
                      for (size_t i = 0; i < sub_spec.numbered_arg_count; i++)
                        {
                          spec->numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                          spec->numbered[unnumbered_arg_count].type =
                            type_without_translator_constraint (sub_spec.numbered[i].type);
                          if (sub_spec.numbered[i].type == FAT_FORMAT_STRING)
                            spec->numbered[unnumbered_arg_count].signature = sub_spec.numbered[i].signature;
                          unnumbered_arg_count++;
                        }
                    }

                  free (sub_spec.numbered);
                }
                goto done_specifier;
              case ')':
                if (terminator != ')')
                  {
                    *invalid_reason =
                      xasprintf (_("Found '%s' without matching '%s'."), "%)", "%(");
                    FDI_SET (format - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }
                spec->directives--;
                goto done;
              case '!':
              case '%':
              case '@':
              case ',':
                type = FAT_NONE;
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
                      INVALID_CONVERSION_SPECIFIER (spec->directives, *format);
                    FDI_SET (format, FMTDIR_ERROR);
                  }
                goto bad_format;
              }
            break;
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

                size_t new_numbered_arg_count =
                  spec->numbered_arg_count + 1 + (type == FAT_FUNCTION_T);
                if (numbered_allocated < new_numbered_arg_count)
                  {
                    numbered_allocated = 2 * numbered_allocated + 1;
                    if (numbered_allocated < new_numbered_arg_count)
                      numbered_allocated = new_numbered_arg_count;
                    spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                  }
                spec->numbered[spec->numbered_arg_count].number = number;
                spec->numbered[spec->numbered_arg_count].type = type;
                if (type == FAT_FORMAT_STRING)
                  spec->numbered[spec->numbered_arg_count].signature = signature;
                spec->numbered_arg_count++;
                if (type == FAT_FUNCTION_T)
                  {
                    spec->numbered[spec->numbered_arg_count].number = number + 1;
                    spec->numbered[spec->numbered_arg_count].type = FAT_FUNCTION_T2;
                    spec->numbered_arg_count++;
                  }
              }
            else
              {
                /* Unnumbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (spec->numbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }

                size_t new_unnumbered_arg_count =
                  unnumbered_arg_count + 1 + (type == FAT_FUNCTION_T);
                if (numbered_allocated < new_unnumbered_arg_count)
                  {
                    numbered_allocated = 2 * numbered_allocated + 1;
                    if (numbered_allocated < new_unnumbered_arg_count)
                      numbered_allocated = new_unnumbered_arg_count;
                    spec->numbered = (struct numbered_arg *) xrealloc (spec->numbered, numbered_allocated * sizeof (struct numbered_arg));
                  }
                spec->numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                spec->numbered[unnumbered_arg_count].type = type;
                if (type == FAT_FORMAT_STRING)
                  spec->numbered[unnumbered_arg_count].signature = signature;
                unnumbered_arg_count++;
                if (type == FAT_FUNCTION_T)
                  {
                    spec->numbered[unnumbered_arg_count].number = unnumbered_arg_count + 1;
                    spec->numbered[unnumbered_arg_count].type = FAT_FUNCTION_T2;
                    unnumbered_arg_count++;
                  }
              }
          }

       done_specifier:
        FDI_SET (format, FMTDIR_END);

        format++;
      }

  if (terminator != '\0')
    {
      *invalid_reason = xasprintf (_("Found '%%%c' without matching '%%%c'."),
                                   terminator == '}' ? '{' : '(', terminator);
      goto bad_format;
    }

 done:
  /* Convert the unnumbered argument array to numbered arguments.  */
  if (unnumbered_arg_count > 0)
    spec->numbered_arg_count = unnumbered_arg_count;
  /* Sort the numbered argument array, and eliminate duplicates.  */
  else if (spec->numbered_arg_count > 1)
    {
      qsort (spec->numbered, spec->numbered_arg_count,
             sizeof (struct numbered_arg), numbered_arg_compare);

      /* Remove duplicates: Copy from i to j, keeping 0 <= j <= i.  */
      bool err = false;
      size_t i, j;
      for (i = j = 0; i < spec->numbered_arg_count; i++)
        if (j > 0 && spec->numbered[i].number == spec->numbered[j-1].number)
          {
            format_arg_type_t type1 = spec->numbered[i].type;
            format_arg_type_t type2 = spec->numbered[j-1].type;

            format_arg_type_t type_both;
            if (((type1 == type2)
                 && (type1 != FAT_FORMAT_STRING
                     || strcmp (spec->numbered[i].signature,
                                spec->numbered[j-1].signature) == 0))
                || (((type1 | type2) & FAT_OPTIONAL_OCAML_SYNTAX) != 0
                    && (((type1 & ~FAT_OPTIONAL_OCAML_SYNTAX) | FAT_OCAML_SYNTAX)
                        == ((type2 & ~FAT_OPTIONAL_OCAML_SYNTAX) | FAT_OCAML_SYNTAX))))
              type_both = (type1 | type2) & ~FAT_OPTIONAL_OCAML_SYNTAX;
            else
              {
                /* Incompatible types.  */
                type_both = FAT_NONE;
                if (!err)
                  *invalid_reason =
                    INVALID_INCOMPATIBLE_ARG_TYPES (spec->numbered[i].number);
                err = true;
              }

            spec->numbered[j-1].type = type_both;
            if (type_both == FAT_FORMAT_STRING)
              free (spec->numbered[i].signature);
          }
        else
          {
            if (j < i)
              {
                spec->numbered[j].number = spec->numbered[i].number;
                spec->numbered[j].type = spec->numbered[i].type;
                if (spec->numbered[j].type == FAT_FORMAT_STRING)
                  spec->numbered[j].signature = spec->numbered[i].signature;
              }
            j++;
          }
      spec->numbered_arg_count = j;
      if (err)
        /* *invalid_reason has already been set above.  */
        goto bad_format;
    }

  *formatp = format;
  return true;

 bad_format:
  destroy_spec (spec);
  return false;
}

static void *
format_parse (const char *format, bool translated, char *fdi,
              char **invalid_reason)
{
  struct spec spec;

  if (!parse_upto (&spec, &format, '\0', translated, fdi, invalid_reason))
    return NULL;

  struct spec *result = XMALLOC (struct spec);
  *result = spec;
  return result;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  destroy_spec (spec);
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
      /* Check that the argument types are essentially the same.  */
      if (!err)
        {
          size_t i, j;
          for (i = 0, j = 0; j < n2; )
            {
              if (spec1->numbered[i].number == spec2->numbered[j].number)
                {
                  format_arg_type_t type1 = spec1->numbered[i].type;
                  format_arg_type_t type2 = spec2->numbered[j].type;

                  if (!(((type1 == type2)
                         && (type1 != FAT_FORMAT_STRING
                             || strcmp (spec1->numbered[i].signature,
                                        spec2->numbered[j].signature) == 0))
                        || ((type2 & FAT_OPTIONAL_OCAML_SYNTAX) != 0
                            && (type2 & ~FAT_OPTIONAL_OCAML_SYNTAX)
                               == (type1 & ~FAT_OCAML_SYNTAX))))
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


struct formatstring_parser formatstring_ocaml =
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
      switch (spec->numbered[i].type & FAT_BASIC_MASK)
        {
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_INT32:
          printf ("l");
          break;
        case FAT_NATIVEINT:
          printf ("n");
          break;
        case FAT_INT64:
          printf ("L");
          break;
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_CHARACTER:
          printf ("c");
          break;
        case FAT_FLOATINGPOINT:
          printf ("f");
          break;
        case FAT_BOOLEAN:
          printf ("B");
          break;
        case FAT_FUNCTION_A:
          printf ("a");
          break;
        case FAT_FUNCTION_T:
          printf ("t1");
          break;
        case FAT_FUNCTION_T2:
          printf ("t2");
          break;
        case FAT_FORMAT_STRING:
          printf ("\"%s\"", spec->numbered[i].signature);
          break;
        default:
          abort ();
        }
      if (spec->numbered[i].type & FAT_OCAML_SYNTAX)
        printf ("!");
      if (spec->numbered[i].type & FAT_OPTIONAL_OCAML_SYNTAX)
        printf ("?");
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
      void *descr = format_parse (line, true, NULL, &invalid_reason);

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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-ocaml.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
