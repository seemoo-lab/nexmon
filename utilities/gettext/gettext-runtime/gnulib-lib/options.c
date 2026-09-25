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

#include <config.h>

/* Specification.  */
#include "options.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* State for communicating between _gl_start_options and get_next_option.  */
static struct
  {
    int argc;
    char **argv;
    const struct program_option *options;
    size_t n_options;
    struct option *long_options;
    char *short_options;
  }
state;

void
_gl_start_options (int argc, char **argv,
                   const struct program_option *options, size_t n_options,
                   struct option *long_options, char *short_options,
                   enum non_option_handling nonopt_handling,
                   unsigned int error_handling)
{
  /* Construct the long_options array.  */
  {
    struct option *p = long_options;

    for (size_t i = 0; i < n_options; i++)
      if (options[i].name != NULL)
        {
          if (options[i].key == 0 && options[i].variable == NULL)
            fprintf (stderr,
                     "start_options: warning: Option '--%s' has no action. Use the 'key' or the 'variable' field to specify an action.\n",
                     options[i].name);

          p->name = options[i].name;
          p->has_arg = options[i].has_arg;
          if (options[i].key == 0 && options[i].variable != NULL)
            {
              p->flag = options[i].variable;
              p->val = options[i].value;
            }
          else
            {
              p->flag = NULL;
              p->val = options[i].key;
            }
          p++;
        }

    p->name = NULL;
    p->has_arg = 0;
    p->flag = NULL;
    p->val = 0;
    p++;
    /* Verify that we haven't exceeded its allocated size.  */
    if (!(p - long_options <= _GL_LONG_OPTIONS_SIZE (n_options)))
      abort ();
  }

  /* Construct the short_options string.  */
  {
    char *p = short_options;

    if (nonopt_handling == NON_OPTION_TERMINATES_OPTIONS)
      *p++ = '+';
    else if (nonopt_handling == PROCESS_NON_OPTIONS)
      *p++ = '-';

    if (error_handling & OPTIONS_MISSING_IS_COLON)
      *p++ = ':';

    for (size_t i = 0; i < n_options; i++)
      if (options[i].key != 0 && options[i].key <= CHAR_MAX)
        {
          *p++ = options[i].key;
          if (options[i].has_arg != no_argument)
            {
              *p++ = ':';
              if (options[i].has_arg == optional_argument)
                *p++ = ':';
            }
        }

    *p++ = '\0';
    /* Verify that we haven't exceeded its allocated size.  */
    if (!(p - short_options <= _GL_SHORT_OPTIONS_SIZE (n_options)))
      abort ();
  }

  state.argc = argc;
  state.argv = argv;
  state.options = options;
  state.n_options = n_options;
  state.long_options = long_options;
  state.short_options = short_options;
  opterr = (error_handling & OPTIONS_ERRORS_SILENT) == 0;
}

int
#ifdef __MINGW32__
_gl_get_next_option (int *optind_p, char **optarg_p, int *optopt_p)
#else
get_next_option (void)
#endif
{
  if (state.argv == NULL)
    {
      fprintf (stderr, "fatal: start_options has not been invoked\n");
      abort ();
    }
#ifdef __MINGW32__
  /* See below for a general explanation.  optind is not only an output of
     getopt_long(), but also an input.  */
  optind = *optind_p;
#endif
  int ret = getopt_long (state.argc, state.argv,
                         state.short_options, state.long_options, NULL);
  if (ret > 1)
    {
      const struct program_option *options = state.options;
      size_t n_options = state.n_options;
      for (size_t i = 0; i < n_options; i++)
        if (ret == options[i].key)
          {
            if (options[i].variable != NULL)
              *(options[i].variable) = options[i].value;
          }
    }
#ifdef __MINGW32__
  /* On mingw, when this file is compiled into a shared library, it pulls
     mingw's getopt.o file (that defines getopt_long, opterr, optind, optarg,
     optopt) into the same shared library.  Since these variables are declared
     and defined without any __declspec(dllexport) or __declspec(dllimport),
     the effect is that there are two copies of the variables: one in the
     shared library and one in the executable.  Upon return from this function,
     we need to copy the values of the output variables (optind, optarg, optopt)
     from the shared library into the executable, where the main() function will
     pick them up.  */
  *optind_p = optind;
  *optarg_p = optarg;
  *optopt_p = optopt;
#endif
  return ret;
}
