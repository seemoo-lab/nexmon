/* Formatted output with a POSIX compatible format string.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

/* Specification.  */
#include "printf-command.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include <error.h>
#include "attribute.h"
#include "c-ctype.h"
#include "strnlen1.h"
#include "c-strtod.h"
#include "xstrtod.h"
#include "quote.h"
#include "xalloc.h"
#include "gettext.h"

#define _(str) gettext (str)

/* The argument type consumed by a directive.  */
enum format_arg_type
{
  FAT_CHARACTER,
  FAT_STRING,
  FAT_INTEGER,
  FAT_UNSIGNED_INTEGER,
  FAT_FLOAT
};

/* A piece of output.  */
struct format_piece
{
  /* For plain text, directives that take no argument, and escape sequences:  */
  const char *text_start;
  size_t text_length;
  /* For directives that take an argument:  */
  enum format_arg_type arg_type;
  size_t arg_number; /* > 0 */
  const char *arg_fmt;
};

/* The entire format string.  */
struct format_string
{
  struct format_piece *pieces;
  size_t npieces;
};

/* Parses the format string.
   Returns the number of arguments that it consumes.  */
static size_t
parse_format_string (struct format_string *fmts, const char *format)
{
  struct format_piece *pieces = NULL;
  size_t npieces = 0;
  size_t npieces_allocated = 0;

  size_t directives = 0;
  size_t numbered_arg_count = 0;
  size_t unnumbered_arg_count = 0;
  size_t max_numbered_arg = 0;
  const char *current_piece_start = NULL;

  for (;;)
    {
      /* Invariant: numbered_arg_count == 0 || unnumbered_arg_count == 0.  */
      /* Invariant: current_piece_start == NULL || current_piece_start < format.  */
      if (*format == '\0' || *format == '%' || *format == '\\')
        {
          if (current_piece_start != NULL)
            {
              if (npieces == npieces_allocated)
                {
                  npieces_allocated = 2 * npieces_allocated + 1;
                  pieces = (struct format_piece *) xrealloc (pieces, npieces_allocated * sizeof (struct format_piece));
                }
              pieces[npieces].text_start = current_piece_start;
              pieces[npieces].text_length = format - current_piece_start;
              npieces++;
              current_piece_start = NULL;
            }
        }
      else
        {
          if (current_piece_start == NULL)
            current_piece_start = format;
        }

      if (*format == '\0')
        break;

      if (*format == '%')
        {
          /* A directive.  */
          format++;
          directives++;

          if (*format == '%')
            {
              /* "%%" produces a literal '%'.  */
              if (npieces == npieces_allocated)
                {
                  npieces_allocated = 2 * npieces_allocated + 1;
                  pieces = (struct format_piece *) xrealloc (pieces, npieces_allocated * sizeof (struct format_piece));
                }
              pieces[npieces].text_start = "%";
              pieces[npieces].text_length = 1;
              npieces++;
            }
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
                        error (EXIT_FAILURE, 0,
                               _("In the directive number %zu, the argument number 0 is not a positive integer."),
                               directives);
                      number = m;
                      format = ++f;
                    }
                }

              /* Parse flags.  */
              bool have_space_flag = false;
              bool have_plus_flag = false;
              bool have_minus_flag = false;
              bool have_hash_flag = false;
              bool have_zero_flag = false;
              for (;; format++)
                {
                  switch (*format)
                    {
                    case ' ':
                      have_space_flag = true;
                      continue;
                    case '+':
                      have_plus_flag = true;
                      continue;
                    case '-':
                      have_minus_flag = true;
                      continue;
                    case '#':
                      have_hash_flag = true;
                      continue;
                    case '0':
                      have_zero_flag = true;
                      continue;
                    default:
                      break;
                    }
                  break;
                }

              /* Parse width.  */
              const char *width_start = NULL;
              size_t width_length = 0;
              if (c_isdigit (*format))
                {
                  width_start = format;
                  do format++; while (c_isdigit (*format));
                  width_length = format - width_start;
                }

              /* Parse precision.  */
              const char *precision_start = NULL;
              size_t precision_length = 0;
              if (*format == '.')
                {
                  format++;

                  precision_start = format;
                  while (c_isdigit (*format))
                    format++;
                  precision_length = format - precision_start;
                }

              enum format_arg_type type;
              switch (*format)
                {
                case 'c':
                  type = FAT_CHARACTER;
                  break;
                case 's':
                  type = FAT_STRING;
                  break;
                case 'i': case 'd':
                  type = FAT_INTEGER;
                  break;
                case 'u': case 'o': case 'x': case 'X':
                  type = FAT_UNSIGNED_INTEGER;
                  break;
                case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
                case 'a': case 'A':
                  type = FAT_FLOAT;
                  break;
                default:
                  if (*format == '\0')
                    error (EXIT_FAILURE, 0,
                           _("The string ends in the middle of a directive."));
                  else
                    {
                      if (c_isprint (*format))
                        error (EXIT_FAILURE, 0,
                               _("In the directive number %zu, the character '%c' is not a valid conversion specifier."),
                               directives, *format);
                      else
                        error (EXIT_FAILURE, 0,
                               _("The character that terminates the directive number %zu is not a valid conversion specifier."),
                               directives);
                   }
                }

              if (have_hash_flag
                  && (*format == 'c' || *format == 's'
                      || *format == 'i' || *format == 'd' || *format == 'u'))
                error (EXIT_FAILURE, 0,
                       _("In the directive number %zu, the flag '%c' is invalid for the conversion '%c'."),
                       directives, '#', *format);
              if (have_zero_flag && (*format == 'c' || *format == 's'))
                error (EXIT_FAILURE, 0,
                       _("In the directive number %zu, the flag '%c' is invalid for the conversion '%c'."),
                       directives, '0', *format);

              if (npieces == npieces_allocated)
                {
                  npieces_allocated = 2 * npieces_allocated + 1;
                  pieces = (struct format_piece *) xrealloc (pieces, npieces_allocated * sizeof (struct format_piece));
                }
              pieces[npieces].text_start = NULL;
              pieces[npieces].text_length = 0;
              pieces[npieces].arg_type = type;

              if (number)
                {
                  /* Numbered argument.  */

                  /* Numbered and unnumbered specifications are exclusive.  */
                  if (unnumbered_arg_count > 0)
                    error (EXIT_FAILURE, 0,
                           _("The string refers to arguments both through absolute argument numbers and through unnumbered argument specifications."));

                  pieces[npieces].arg_number = number;
                  numbered_arg_count++;
                  if (max_numbered_arg < number)
                    max_numbered_arg = number;
                }
              else
                {
                  /* Unnumbered argument.  */

                  /* Numbered and unnumbered specifications are exclusive.  */
                  if (numbered_arg_count > 0)
                    error (EXIT_FAILURE, 0,
                           _("The string refers to arguments both through absolute argument numbers and through unnumbered argument specifications."));

                  pieces[npieces].arg_number = unnumbered_arg_count + 1;
                  unnumbered_arg_count++;
                }

              if (fmts != NULL)
                {
                  char *arg_fmt = (char *) xmalloc (1 + 5 + width_length + 1 + precision_length + 2 + 1);
                  {
                    char *f = arg_fmt;
                    *f++ = '%';
                    if (have_space_flag)
                      *f++ = ' ';
                    if (have_plus_flag)
                      *f++ = '+';
                    if (have_minus_flag)
                      *f++ = '-';
                    if (have_hash_flag)
                      *f++ = '#';
                    if (have_zero_flag)
                      *f++ = '0';
                    if (width_start != NULL)
                      {
                        memcpy (f, width_start, width_length);
                        f += width_length;
                      }
                    if (precision_start != NULL)
                      {
                        *f++ = '.';
                        memcpy (f, precision_start, precision_length);
                        f += precision_length;
                      }
                    switch (type)
                      {
                      case FAT_INTEGER:
                      case FAT_UNSIGNED_INTEGER:
                        *f++ = 'j';
                        break;
                      case FAT_FLOAT:
                        *f++ = 'L';
                        break;
                      default:
                        break;
                      }
                    *f++ = (*format == 'c' ? 's' : *format);
                    *f = '\0';
                  }
                  pieces[npieces].arg_fmt = arg_fmt;
                }

              npieces++;
            }

          format++;
        }
      else if (*format == '\\')
        {
          /* An escape sequence.  */
          format++;

          const char *one_char;
          switch (*format)
            {
            case '\\': one_char = "\\"; format++; break;
            case 'a':  one_char = "\a"; format++; break;
            case 'b':  one_char = "\b"; format++; break;
            case 'f':  one_char = "\f"; format++; break;
            case 'n':  one_char = "\n"; format++; break;
            case 'r':  one_char = "\r"; format++; break;
            case 't':  one_char = "\t"; format++; break;
            case 'v':  one_char = "\v"; format++; break;

            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7':
              {
                unsigned int n = (*format - '0');
                format++;
                if (*format >= '0' && *format <= '7')
                  {
                    n =  (n << 3) + (*format - '0');
                    format++;
                    if (*format >= '0' && *format <= '7')
                      {
                        n =  (n << 3) + (*format - '0');
                        format++;
                      }
                  }
                if (fmts != NULL)
                  {
                    char *text = (char *) xmalloc (1);
                    *text = (unsigned char) n;
                    one_char = text;
                  }
                else
                  one_char = ""; /* just a dummy */
              }
              break;

            default:
              if (*format == '\0')
                error (EXIT_FAILURE, 0,
                       _("The string ends in the middle of an escape sequence."));
              else
                {
                  if (c_isprint (*format))
                    error (EXIT_FAILURE, 0,
                           (*format == 'c'
                            || *format == 'x'
                            || *format == 'u' || *format == 'U'
                            ? _("The escape sequence '%c%c' is unsupported (not in POSIX).")
                            : _("The escape sequence '%c%c' is invalid.")),
                           '\\', *format);
                  else
                    error (EXIT_FAILURE, 0,
                           _("This escape sequence is invalid."));
                }
            }

          if (npieces == npieces_allocated)
            {
              npieces_allocated = 2 * npieces_allocated + 1;
              pieces = (struct format_piece *) xrealloc (pieces, npieces_allocated * sizeof (struct format_piece));
            }
          pieces[npieces].text_start = one_char;
          pieces[npieces].text_length = 1;
          npieces++;
        }
      else
        format++;
    }

  if (fmts != NULL)
    {
      fmts->pieces = pieces;
      fmts->npieces = npieces;
    }
  else
    free (pieces);

  /* The number of consumed arguments:  */
  return (numbered_arg_count > 0 ? max_numbered_arg : unnumbered_arg_count);
}

static int status;

/* Applies the format string to the array of remaining arguments.  */
static void
apply_format_string (const struct format_string *fmts,
                     size_t argc, char *argv[])
{
  size_t npieces = fmts->npieces;
  for (size_t i = 0; i < npieces; i++)
    {
      struct format_piece *piece = &fmts->pieces[i];

      if (piece->text_start != NULL)
        {
          /* Print some fixed text.  */
          if (fwrite (piece->text_start, 1, piece->text_length, stdout)
              < piece->text_length)
            error (EXIT_FAILURE, 0, _("write error"));
        }
      else
        {
          /* Convert and print an argument.  */
          char zero[2] = { '0', '\0' };
          char *empty = zero + 1;

          char *arg;
          if (piece->arg_number - 1 < argc)
            arg = argv[piece->arg_number - 1];
          else
            {
              /* <https://pubs.opengroup.org/onlinepubs/9799919799/utilities/printf.html>
                 point 11 suggests that we make "%1$x" behave differently from
                 "%x".  We don't do this, because translators are free to switch
                 from unnumbered arguments to numbered arguments or vice versa.  */
              arg = (piece->arg_type == FAT_CHARACTER
                     || piece->arg_type == FAT_STRING
                     ? empty
                     : zero);
            }

          switch (piece->arg_type)
            {
            case FAT_CHARACTER:
              /* <https://pubs.opengroup.org/onlinepubs/9799919799/utilities/printf.html>
                 point 13 suggests to print the first *byte* of arg.  But this
                 is not appropriate in multibyte locales.   Therefore, print the
                 first multibyte character instead, if arg starts with a valid
                 multibyte character.  */
              {
                mbstate_t state;
                mbszero (&state);

                char32_t wc;
                size_t ret = mbrtoc32 (&wc, arg, strnlen1 (arg, MB_CUR_MAX), &state);
                arg[(int) ret >= 0 ? ret : 1] = '\0';
              }
              FALLTHROUGH;
            case FAT_STRING:
              errno = 0;
              if (fzprintf (stdout, piece->arg_fmt, arg) < 0)
                {
                  if (errno == ENOMEM)
                    xalloc_die ();
                  error (EXIT_FAILURE, 0, _("write error"));
                }
              break;

            case FAT_INTEGER:
              {
                intmax_t arg_value;
                if (*arg == '\'' || *arg == '"')
                  {
                    /* POSIX says: "If the leading character is a single-quote
                       or double-quote, the value shall be the numeric value
                       in the underlying codeset of the character following the
                       single-quote or double-quote."
                       Use the first first multibyte character, if arg starts
                       with a valid multibyte character.  */
                    mbstate_t state;
                    mbszero (&state);

                    char32_t wc;
                    size_t ret = mbrtoc32 (&wc, arg + 1, strnlen1 (arg + 1, MB_CUR_MAX), &state);
                    if ((int) ret > 0)
                      arg_value = wc;
                    else if (arg[1] != '\0')
                      arg_value = (unsigned char) arg[1];
                    else
                      {
                        arg_value = 0;
                        error (EXIT_SUCCESS, 0,
                               _("%s: expected a numeric value"),
                               quote (arg));
                        status = EXIT_FAILURE;
                      }
                  }
                else
                  {
                    /* xstrtoimax is a nicer API than strtoimax.
                       Let's hope that I don't make a mistake with strtoimax's
                       horrible calling convention here.  */
                    char *ptr;
                    arg_value = (errno = 0, strtoimax (arg, &ptr, 0));
                    bool parsed = (ptr != arg && errno == 0);

                    if (parsed && *ptr == '\0')
                      /* Successful parse of arg.  */
                      ;
                    else
                      {
                        if (parsed)
                          error (EXIT_SUCCESS, 0,
                                 _("%s: value not completely converted"),
                                 quote (arg));
                        else
                          {
                            arg_value = 0;
                            error (EXIT_SUCCESS, 0,
                                   _("%s: expected a numeric value"),
                                   quote (arg));
                          }
                        status = EXIT_FAILURE;
                      }
                  }

                errno = 0;
                if (fzprintf (stdout, piece->arg_fmt, arg_value) < 0)
                  {
                    if (errno == ENOMEM)
                      xalloc_die ();
                    error (EXIT_FAILURE, 0, _("write error"));
                  }
              }
              break;

            case FAT_UNSIGNED_INTEGER:
              {
                uintmax_t arg_value;
                if (*arg == '\'' || *arg == '"')
                  {
                    /* POSIX says: "If the leading character is a single-quote
                       or double-quote, the value shall be the numeric value
                       in the underlying codeset of the character following the
                       single-quote or double-quote."
                       Use the first first multibyte character, if arg starts
                       with a valid multibyte character.  */
                    mbstate_t state;
                    mbszero (&state);

                    char32_t wc;
                    size_t ret = mbrtoc32 (&wc, arg + 1, strnlen1 (arg + 1, MB_CUR_MAX), &state);
                    if ((int) ret > 0)
                      arg_value = wc;
                    else if (arg[1] != '\0')
                      arg_value = (unsigned char) arg[1];
                    else
                      {
                        arg_value = 0;
                        error (EXIT_SUCCESS, 0,
                               _("%s: expected a numeric value"),
                               quote (arg));
                        status = EXIT_FAILURE;
                      }
                  }
                else
                  {
                    /* xstrtoumax is a nicer API than strtoumax.
                       But here, we need to accept a leading '-' sign, as in
                       "-3" or " -3".
                       Let's hope that I don't make a mistake with strtoumax's
                       horrible calling convention here.  */
                    char *ptr;
                    arg_value = (errno = 0, strtoumax (arg, &ptr, 0));
                    bool parsed = (ptr != arg && errno == 0);

                    if (parsed && *ptr == '\0')
                      /* Successful parse of arg.  */
                      ;
                    else
                      {
                        if (parsed)
                          error (EXIT_SUCCESS, 0,
                                 _("%s: value not completely converted"),
                                 quote (arg));
                        else
                          {
                            arg_value = 0;
                            error (EXIT_SUCCESS, 0,
                                   _("%s: expected a numeric value"),
                                   quote (arg));
                          }
                        status = EXIT_FAILURE;
                      }
                  }

                errno = 0;
                if (fzprintf (stdout, piece->arg_fmt, arg_value) < 0)
                  {
                    if (errno == ENOMEM)
                      xalloc_die ();
                    error (EXIT_FAILURE, 0, _("write error"));
                  }
              }
              break;

            case FAT_FLOAT:
              /* <https://pubs.opengroup.org/onlinepubs/9799919799/utilities/printf.html>
                 suggests to use strtod(), i.e. a 'double'.  We prefer a
                 'long double', because it has higher precision.  */
              /* Try interpreting the argument as a number in the current locale
                 and, if that fails, in the "C" locale.  Like coreutils 'printf'
                 does.  */
              {
                long double arg_value;
                const char *ptr;
                bool parsed = xstrtold (arg, &ptr, &arg_value, strtold);
                if (parsed && *ptr == '\0')
                  /* Successful parse of arg in the current locale.  */
                  ;
                else
                  {
                    long double arg_value2;
                    const char *ptr2;
                    bool parsed2 = xstrtold (arg, &ptr2, &arg_value2, c_strtold);
                    if (parsed2 && *ptr2 == '\0')
                      {
                        /* Successful parse of arg in the "C" locale.  */
                        arg_value = arg_value2;
                      }
                    else
                      {
                        if (parsed2 && (!parsed || ptr2 > ptr))
                          arg_value = arg_value2;
                        if (parsed || parsed2)
                          error (EXIT_SUCCESS, 0,
                                 _("%s: value not completely converted"),
                                 quote (arg));
                        else
                          {
                            arg_value = 0.0L;
                            error (EXIT_SUCCESS, 0,
                                   _("%s: expected a numeric value"),
                                   quote (arg));
                          }
                        status = EXIT_FAILURE;
                      }
                  }

                errno = 0;
                if (fzprintf (stdout, piece->arg_fmt, arg_value) < 0)
                  {
                    if (errno == ENOMEM)
                      xalloc_die ();
                    error (EXIT_FAILURE, 0, _("write error"));
                  }
              }
              break;
            }
        }
    }
}

size_t
printf_consumed_arguments (const char *format)
{
  return parse_format_string (NULL, format);
}

void
printf_command (const char *format, size_t args_each_round,
                size_t argc, char *argv[])
{
  /* Parse the format string, and bail out early if it is invalid.  */
  struct format_string fmts;
  size_t consumed_arguments = parse_format_string (&fmts, format);

  /* Validate consumed_arguments against args_each_round.  */
  if (consumed_arguments > args_each_round)
    error (EXIT_FAILURE, 0,
           _("The translated format string consumes %zu arguments, whereas the original format string consumes only %zu arguments."),
           consumed_arguments, args_each_round);
  /* Here consumed_arguments <= args_each_round.
     It is OK if consumed_arguments < args_each_round; this happens for example
     in 'printf_ngettext', when the chosen format string applies only to a
     single value.  */

  /* Repeatedly apply the format string to the remaining arguments.  */
  if (args_each_round == 0 && argc > 0)
    {
      error (0, 0,
             _("warning: ignoring excess arguments, starting with %s"),
             quote(argv[0]));
      argc = 0;
    }
  status = EXIT_SUCCESS;
  for (;;)
    {
      apply_format_string (&fmts, argc, argv);
      if (argc <= args_each_round)
        break;
      argc -= args_each_round;
      argv += args_each_round;
    }

  if (status != EXIT_SUCCESS)
    exit (status);
}
