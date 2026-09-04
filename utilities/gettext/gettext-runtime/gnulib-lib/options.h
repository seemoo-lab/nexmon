/* Parsing program options.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>.  */

#ifndef _OPTIONS_H
#define _OPTIONS_H

/* This file uses _GL_GNUC_PREREQ.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

/* This file provides a more convenient API to parsing program options,
   based on GNU getopt_long() and thus compatible with the option parsing
   conventions for GNU programs
   <https://www.gnu.org/prep/standards/html_node/Command_002dLine-Interfaces.html>.

   Instead of writing

     static struct option const long_options[] =
     {
       { "width", required_argument, NULL, 'w' },
       { "help", no_argument, &show_help, 1 },
       { "version", no_argument, &show_version, 1 },
       { NULL, 0, NULL, 0 }
     };

     while ((optchar = getopt_long (argc, argv, "w:xhV", long_options, NULL))
            != -1)
       switch (optchar)
         {
         case '\0':      // Long option with flag != NULL.
           break;
         case 'w':
           set_width (optarg);
           break;
         case 'x':
           do_x = true;
           break;
         case 'h':
           show_help = 1;
           break;
         case 'V':
           show_version = 1;
           break;
         default:
           usage (EXIT_FAILURE);
         }

   you write

     BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
     static struct program_option const options[] =
     {
       { "width",   'w', required_argument },
       { NULL,      'x', no_argument       },
       { "help",    'h', no_argument,      &show_help, 1 },
       { "version", 'V', no_argument,      &show_version, 1 },
     };
     END_ALLOW_OMITTING_FIELD_INITIALIZERS

     start_options (argc, argv, options, MOVE_OPTIONS_FIRST, 0);
     while ((optchar = get_next_option ()) != -1)
       switch (optchar)
         {
         case '\0':      // Long option with key == 0.
           break;
         case 'w':
           set_width (optarg);
           break;
         case 'x':
           do_x = true;
           break;
         case 'h':
         case 'V':
           break;
         default:
           usage (EXIT_FAILURE);
         }

   This API fixes the following shortcomings of the getopt_long() API:

     * The information whether an option takes a required vs. optional argument
       needs to be specified twice: in the option[] array for the long option
       and in the string argument for the short option.
       It is too easy to forget to add the ":" or "::" part in the string
       argument and thus get inconsistent behaviour between the long option and
       the short option.

     * This information needs to be specified twice, but in different ways:
         In the array        In the string
         ------------        -------------
         no_argument         ""
         required_argument   ":"
         optional_argument   "::"

     * For an action that merely sets an 'int'-typed variable to a value, you
       can specify this action in the options[] array, and thus omit the
       handling in the 'switch' statement.  But this works only for options
       that are long options without a corresponding short option.  As soon
       as the option has a corresponding short option, you *do* need to handle
       it in the 'switch' statement.  Here again, there is the opportunity
       for inconsistent behaviour between the long option and the short option.

     * The 'val' field in a 'struct option' has different meanings, depending
       on another field:  If the 'flag' field is non-NULL, 'val' is a value to
       be stored in a variable.  If the 'flag' field is NULL, 'val' is a key
       to be returned from getopt_long() and subject to the 'switch' statement.

     * The handling of non-option arguments is specified by prepending a
       certain character ('+' or '-') to the string argument.  This is not
       one of the usual ways to specify things in an API.  The conventional
       way in an API is an argument of 'enum' type, or a flags word.

     * The handling of errors consists of two independent flags, and each of
       the flags has to be specified in a different way: one flag is specified
       by prepending a certain character (':') to the string argument; the
       other flag is specified through the global variable 'opterr'.

     * The 'struct option' is a misnomer: It cannot encode short options.
       Therefore, it would have better been called 'struct long_option'.

     * The getopt_long() function is expected to receive the same arguments in
       each call, in the 'while' loop.  The effects are undefined if you
       don't follow this (unwritten!) constraint.

     * The fifth argument to getopt_long(), indexptr, is redundant, because
       when the 'flag' is non-NULL, the switch statement does not need to
       handle the option, and when the 'flag' is NULL, getopt_long returns
       the value of 'val', as a way to identify which option was seen.

   It keeps the following properties of the getopt_long() API:

     * The programmer writes in actions directly in the main() function.
       That is, the actions don't go into separate callback functions
       (like with argp).  Such callback functions are a fine thing in languages
       with nested function (and implicit closures), like Lisp and C++, but not
       in C.

     * The option processing does not require dynamic memory allocation.  That
       is, you don't need to worry about out-of-memory situations here.
 */

/* Get no_argument, required_argument, optional_argument.  */
#include <getopt.h>
/* Get size_t.  */
#include <stddef.h>
/* Get countof.  */
#include <stdcountof.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Description of an option (long option or short option or both).  */
struct program_option
{
  /* The name of the long option, or NULL for a short-option-only.  */
  const char *name;
  /* A key that uniquely identifies the option.
     If the option has a short option, use the character of that short option.
     Otherwise, use an arbitrary unique value > CHAR_MAX.
     Don't use '\0' or '\1' here, because they have a special meaning.  */
  int key;

  /* One of: no_argument, required_argument, optional_argument.  */
  int has_arg;

  /* For an action that consists in assigned a value to an 'int'-typed variable,
     put the variable and the value here.
     Otherwise, use NULL and 0, or omit these fields.  */
  int *variable;
  int value;
};

/* These macros silence '-Wmissing-field-initializers' warnings from GCC and
   clang in the definition of a 'struct program_option' array.
   To be placed before and after the declaration of a 'struct program_option'
   array.  */
#if _GL_GNUC_PREREQ (4, 6) || defined __clang__
# define BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")
# define END_ALLOW_OMITTING_FIELD_INITIALIZERS \
    _Pragma("GCC diagnostic pop")
#else
# define BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
# define END_ALLOW_OMITTING_FIELD_INITIALIZERS
#endif

/* Handling of non-option arguments.  */
enum non_option_handling {
  /* Move options before non-option arguments.
     This is suitable for most programs.  */
  MOVE_OPTIONS_FIRST,
  /* Option processing stops when the first non-option argument is encountered.
     This is suitable for programs that pass an entire argument list to another
     program (such as 'env'), or for programs that accept normal arguments that
     start with '-' (such as 'expr' and 'printf').  */
  NON_OPTION_TERMINATES_OPTIONS,
  /* Process non-option arguments as if they were options, associated with the
     key '\1'.  */
  PROCESS_NON_OPTIONS
};

/* Handling of errors: A bit mask.  */
#define OPTIONS_ERRORS_SILENT     0x1
#define OPTIONS_MISSING_IS_COLON  0x2

/* Starts the processing of options.  */
#define start_options(argc, argv, options, nonopt_handling, error_handling)    \
  /* Allocate room for the long options and short options.  */                 \
  struct option _gl_long_options[_GL_LONG_OPTIONS_SIZE (countof (options))];   \
  char _gl_short_options[_GL_SHORT_OPTIONS_SIZE (countof (options))];          \
  _gl_start_options (argc, argv,                                               \
                     options, countof (options),                               \
                     _gl_long_options, _gl_short_options,                      \
                     nonopt_handling, error_handling)
#define _GL_LONG_OPTIONS_SIZE(count) ((count) + 1)
#define _GL_SHORT_OPTIONS_SIZE(count) (3 * (count) + 3)

extern void _gl_start_options (int argc, /*const*/ char **argv,
                               const struct program_option *options,
                               size_t n_options,
                               struct option *long_options, char *short_options,
                               enum non_option_handling nonopt_handling,
                               unsigned int error_handling);

/* Processes the next option (or, if PROCESS_NON_OPTIONS was specified,
   non-option).
   Requires a prior 'start_options' invocation in the same scope or an outer
   scope.
   If the option has a 'variable' field, that variable is assigned the 'value'
   field.
   Returns the key of the option, or '\1' when returning a non-option argument.
   If the option lacks an argument, it returns '?'.
   If the option is unknown, it returns '?' or (if OPTIONS_MISSING_IS_COLON was
   specified) ':'.
   If the processing is terminated, it returns -1.  */
extern int get_next_option (void);
#ifdef __MINGW32__
extern int _gl_get_next_option (int *optind_p, char **optarg_p, int *optopt_p);
# define get_next_option() _gl_get_next_option (&optind, &optarg, &optopt)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OPTIONS_H */
