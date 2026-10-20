/* Test program for program options.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

#include <config.h>

/* Specification.  */
#include "options.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Display usage information and exit.  */
static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, "Try 'foo --help' for more information.\n");
  else
    {
      printf ("\
Usage: foo [OPTION] STRING\n\
");
      printf ("\n");
      printf ("\
Does something with the STRING.\n");
      printf ("\n");
      printf ("\
Options and arguments:\n");
      printf ("\
  -w, --width=WIDTH         specify line width\n");
      printf ("\
  STRING                    a string\n");
      printf ("\
Informative output:\n");
      printf ("\
  -h, --help                display this help and exit\n");
      printf ("\
  -V, --version             display version information and exit\n");
    }

  exit (status);
}

/* Default values for command line options.  */
static int show_help = 0;
static int show_version = 0;
static int width = 80;
static bool do_x = false;

static void
set_width (const char *arg)
{
  width = atoi (arg);
}

int
main (int argc, char *argv[])
{
  /* Parse command line options.  */
  {
    static struct program_option const options[] =
    {
      { "width",   'w', required_argument },
      { NULL,      'x', no_argument       },
      { "help",    'h', no_argument,      &show_help, 1 },
      { "version", 'V', no_argument,      &show_version, 1 },
    };

    start_options (argc, argv, options, MOVE_OPTIONS_FIRST, 0);
    int optchar;
    while ((optchar = get_next_option ()) != -1)
      switch (optchar)
        {
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
  }

  /* Version information is requested.  */
  if (show_version)
    {
      printf ("foo 0.0\n");
      printf ("Copyright (C) %s Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <%s>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
",
              "2025", "https://gnu.org/licenses/gpl.html");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (show_help)
    usage (EXIT_SUCCESS);

  /* The STRING argument is the first non-option argument.  */
  if (!(argc - optind >= 1))
    {
      fprintf (stderr, "missing argument\n");
      usage (EXIT_FAILURE);
    }
  const char *string = argv[optind++];
  if (!(argc == optind))
    {
      fprintf (stderr, "too many arguments\n");
      usage (EXIT_FAILURE);
    }

  printf ("Width:  %d\n", width);
  printf ("x:      %s\n", do_x ? "true" : "false");
  printf ("String: %s\n", string);

  exit (EXIT_SUCCESS);
}
