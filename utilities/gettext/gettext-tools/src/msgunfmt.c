/* msgunfmt - converts binary .mo files to Uniforum style .po files
   Copyright (C) 1995-2026 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper and Bruno Haible.  */

#include <config.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include <textstyle.h>

#include <error.h>
#include "options.h"
#include "noreturn.h"
#include "closeout.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename-lgpl.h"
#include "message.h"
#include "msgunfmt.h"
#include "read-mo.h"
#include "read-java.h"
#include "read-csharp.h"
#include "read-resources.h"
#include "read-tcl.h"
#include "write-catalog.h"
#include "write-po.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "xerror-handler.h"
#include "propername.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Be more verbose.  */
bool verbose;

/* Java mode input file specification.  */
static bool java_mode;
static const char *java_resource_name;
static const char *java_locale_name;

/* C# mode input file specification.  */
static bool csharp_mode;
static const char *csharp_resource_name;
static const char *csharp_locale_name;
static const char *csharp_base_directory;

/* C# resources mode input file specification.  */
static bool csharp_resources_mode;

/* Tcl mode input file specification.  */
static bool tcl_mode;
static const char *tcl_locale_name;
static const char *tcl_base_directory;

/* Force output of PO file even if empty.  */
static int force_po;


/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void usage (int status);
static void read_one_file (message_list_ty *mlp, const char *filename);


int
main (int argc, char **argv)
{
  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  bindtextdomain ("gnulib", relocate (GNULIB_LOCALEDIR));
  bindtextdomain ("bison-runtime", relocate (BISON_LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  /* Default values for command line options.  */
  bool do_help = false;
  bool do_version = false;
  const char *output_file = "-";
  catalog_output_format_ty output_syntax = &output_format_po;
  bool sort_by_msgid = false;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "color",              CHAR_MAX + 6, optional_argument },
    { "csharp",             CHAR_MAX + 4, no_argument       },
    { "csharp-resources",   CHAR_MAX + 5, no_argument       },
    { "escape",             'E',          no_argument       },
    { "force-po",           0,            no_argument,      &force_po, 1 },
    { "help",               'h',          no_argument       },
    { "indent",             'i',          no_argument       },
    { "java",               'j',          no_argument       },
    { "locale",             'l',          required_argument },
    { "no-escape",          'e',          no_argument       },
    { "no-wrap",            CHAR_MAX + 2, no_argument       },
    { "output-file",        'o',          required_argument },
    { "properties-output",  'p',          no_argument       },
    { "resource",           'r',          required_argument },
    { "sort-output",        's',          no_argument       },
    { "strict",             CHAR_MAX + 8, no_argument       },
    { "stringtable-output", CHAR_MAX + 3, no_argument       },
    { "style",              CHAR_MAX + 7, required_argument },
    { "tcl",                CHAR_MAX + 1, no_argument       },
    { "verbose",            'v',          no_argument       },
    { "version",            'V',          no_argument       },
    { "width",              'w',          required_argument },
    { NULL,                 'd',          required_argument },
  };
  END_ALLOW_OMITTING_FIELD_INITIALIZERS
  start_options (argc, argv, options, MOVE_OPTIONS_FIRST, 0);
  {
    int optchar;
    while ((optchar = get_next_option ()) != -1)
      switch (optchar)
        {
        case '\0':                /* Long option with key == 0.  */
          break;

        case 'd':
          csharp_base_directory = optarg;
          tcl_base_directory = optarg;
          break;

        case 'e':
          message_print_style_escape (false);
          break;

        case 'E':
          message_print_style_escape (true);
          break;

        case 'h':
          do_help = true;
          break;

        case 'i':
          message_print_style_indent ();
          break;

        case 'j':
          java_mode = true;
          break;

        case 'l':
          java_locale_name = optarg;
          csharp_locale_name = optarg;
          tcl_locale_name = optarg;
          break;

        case 'o':
          output_file = optarg;
          break;

        case 'p':
          output_syntax = &output_format_properties;
          break;

        case 'r':
          java_resource_name = optarg;
          csharp_resource_name = optarg;
          break;

        case 's':
          sort_by_msgid = true;
          break;

        case CHAR_MAX + 8: /* --strict */
          message_print_style_uniforum ();
          break;

        case 'v':
          verbose = true;
          break;

        case 'V':
          do_version = true;
          break;

        case 'w':
          {
            char *endp;
            int value = strtol (optarg, &endp, 10);
            if (endp != optarg)
              message_page_width_set (value);
          }
          break;

        case CHAR_MAX + 1: /* --tcl */
          tcl_mode = true;
          break;

        case CHAR_MAX + 2: /* --no-wrap */
          message_page_width_ignore ();
          break;

        case CHAR_MAX + 3: /* --stringtable-output */
          output_syntax = &output_format_stringtable;
          break;

        case CHAR_MAX + 4: /* --csharp */
          csharp_mode = true;
          break;

        case CHAR_MAX + 5: /* --csharp-resources */
          csharp_resources_mode = true;
          break;

        case CHAR_MAX + 6: /* --color */
          if (handle_color_option (optarg) || color_test_mode)
            usage (EXIT_FAILURE);
          break;

        case CHAR_MAX + 7: /* --style */
          handle_style_option (optarg);
          break;

        default:
          usage (EXIT_FAILURE);
          break;
        }
  }

  /* Version information is requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", last_component (program_name),
              PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <%s>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
"),
              "1995-2026", "https://gnu.org/licenses/gpl.html");
      printf (_("Written by %s and %s.\n"),
              proper_name ("Ulrich Drepper"), proper_name ("Bruno Haible"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Check for contradicting options.  */
  {
    unsigned int modes =
      (java_mode ? 1 : 0)
      | (csharp_mode ? 2 : 0)
      | (csharp_resources_mode ? 4 : 0)
      | (tcl_mode ? 8 : 0);
    static const char *mode_options[] =
      { "--java", "--csharp", "--csharp-resources", "--tcl" };
    /* More than one bit set?  */
    if (modes & (modes - 1))
      {
        const char *first_option;
        const char *second_option;
        {
          unsigned int i;
          for (i = 0; ; i++)
            if (modes & (1 << i))
              break;
          first_option = mode_options[i];
          for (i = i + 1; ; i++)
            if (modes & (1 << i))
              break;
          second_option = mode_options[i];
        }
        error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
               first_option, second_option);
      }
  }
  if (java_mode)
    {
      if (optind < argc)
        {
          error (EXIT_FAILURE, 0,
                 _("%s and explicit file names are mutually exclusive"),
                 "--java");
        }
    }
  else if (csharp_mode)
    {
      if (optind < argc)
        {
          error (EXIT_FAILURE, 0,
                 _("%s and explicit file names are mutually exclusive"),
                 "--csharp");
        }
      if (csharp_locale_name == NULL)
        {
          error (EXIT_SUCCESS, 0,
                 _("%s requires a \"-l locale\" specification"),
                 "--csharp");
          usage (EXIT_FAILURE);
        }
      if (csharp_base_directory == NULL)
        {
          error (EXIT_SUCCESS, 0,
                 _("%s requires a \"-d directory\" specification"),
                 "--csharp");
          usage (EXIT_FAILURE);
        }
    }
  else if (tcl_mode)
    {
      if (optind < argc)
        {
          error (EXIT_FAILURE, 0,
                 _("%s and explicit file names are mutually exclusive"),
                 "--tcl");
        }
      if (tcl_locale_name == NULL)
        {
          error (EXIT_SUCCESS, 0,
                 _("%s requires a \"-l locale\" specification"),
                 "--tcl");
          usage (EXIT_FAILURE);
        }
      if (tcl_base_directory == NULL)
        {
          error (EXIT_SUCCESS, 0,
                 _("%s requires a \"-d directory\" specification"),
                 "--tcl");
          usage (EXIT_FAILURE);
        }
    }
  else
    {
      if (java_resource_name != NULL)
        {
          error (EXIT_SUCCESS, 0, _("%s is only valid with %s or %s"),
                 "--resource", "--java", "--csharp");
          usage (EXIT_FAILURE);
        }
      if (java_locale_name != NULL)
        {
          error (EXIT_SUCCESS, 0, _("%s is only valid with %s or %s"),
                 "--locale", "--java", "--csharp");
          usage (EXIT_FAILURE);
        }
    }

  /* Read the given .mo file. */
  msgdomain_list_ty *result;
  if (java_mode)
    {
      result = msgdomain_read_java (java_resource_name, java_locale_name);
    }
  else if (csharp_mode)
    {
      result = msgdomain_read_csharp (csharp_resource_name, csharp_locale_name,
                                      csharp_base_directory);
    }
  else if (tcl_mode)
    {
      result = msgdomain_read_tcl (tcl_locale_name, tcl_base_directory);
    }
  else
    {
      message_list_ty *mlp = message_list_alloc (false);
      if (optind < argc)
        {
          do
            read_one_file (mlp, argv[optind]);
          while (++optind < argc);
        }
      else
        read_one_file (mlp, "-");

      result = msgdomain_list_alloc (false);
      result->item[0]->messages = mlp;
    }

  /* Sorting the list of messages.  */
  if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (result);

  /* Write the resulting message list to the given .po file.  */
  msgdomain_list_print (result, output_file, output_syntax,
                        textmode_xerror_handler, force_po, false);

  /* No problems.  */
  exit (EXIT_SUCCESS);
}


/* Display usage information and exit.  */
static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try '%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION] [FILE]...\n\
"), program_name);
      printf ("\n");
      printf (_("\
Convert binary message catalog to Uniforum style .po file.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Operation mode:\n"));
      printf (_("\
  -j, --java                  Java mode: input is a Java ResourceBundle class\n"));
      printf (_("\
      --csharp                C# mode: input is a .NET .dll file\n"));
      printf (_("\
      --csharp-resources      C# resources mode: input is a .NET .resources file\n"));
      printf (_("\
      --tcl                   Tcl mode: input is a tcl/msgcat .msg file\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  FILE ...                    input .mo files\n"));
      printf (_("\
If no input file is given or if it is -, standard input is read.\n"));
      printf ("\n");
      printf (_("\
Input file location in Java mode:\n"));
      printf (_("\
  -r, --resource=RESOURCE     resource name\n"));
      printf (_("\
  -l, --locale=LOCALE         locale name, either language or language_COUNTRY\n"));
      printf (_("\
The class name is determined by appending the locale name to the resource name,\n\
separated with an underscore.  The class is located using the CLASSPATH.\n\
"));
      printf ("\n");
      printf (_("\
Input file location in C# mode:\n"));
      printf (_("\
  -r, --resource=RESOURCE     resource name\n"));
      printf (_("\
  -l, --locale=LOCALE         locale name, either language or language_COUNTRY\n"));
      printf (_("\
  -d DIRECTORY                base directory for locale dependent .dll files\n"));
      printf (_("\
The -l and -d options are mandatory.  The .dll file is located in a\n\
subdirectory of the specified directory whose name depends on the locale.\n"));
      printf ("\n");
      printf (_("\
Input file location in Tcl mode:\n"));
      printf (_("\
  -l, --locale=LOCALE         locale name, either language or language_COUNTRY\n"));
      printf (_("\
  -d DIRECTORY                base directory of .msg message catalogs\n"));
      printf (_("\
The -l and -d options are mandatory.  The .msg file is located in the\n\
specified directory.\n"));
      printf ("\n");
      printf (_("\
Output file location:\n"));
      printf (_("\
  -o, --output-file=FILE      write output to specified file\n"));
      printf (_("\
The results are written to standard output if no output file is specified\n\
or if it is -.\n"));
      printf ("\n");
      printf (_("\
Output details:\n"));
      printf (_("\
      --color                 use colors and other text attributes always\n\
      --color=WHEN            use colors and other text attributes if WHEN.\n\
                              WHEN may be 'always', 'never', 'auto', or 'html'.\n"));
      printf (_("\
      --style=STYLEFILE       specify CSS style rule file for --color\n"));
      printf (_("\
  -e, --no-escape             do not use C escapes in output (default)\n"));
      printf (_("\
  -E, --escape                use C escapes in output, no extended chars\n"));
      printf (_("\
      --force-po              write PO file even if empty\n"));
      printf (_("\
  -i, --indent                write indented output style\n"));
      printf (_("\
      --strict                write strict uniforum style\n"));
      printf (_("\
  -p, --properties-output     write out a Java .properties file\n"));
      printf (_("\
      --stringtable-output    write out a NeXTstep/GNUstep .strings file\n"));
      printf (_("\
  -w, --width=NUMBER          set output page width\n"));
      printf (_("\
      --no-wrap               do not break long message lines, longer than\n\
                              the output page width, into several lines\n"));
      printf (_("\
  -s, --sort-output           generate sorted output\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf (_("\
  -v, --verbose               increase verbosity level\n"));
      printf ("\n");
      /* TRANSLATORS: The first placeholder is the web address of the Savannah
         project of this package.  The second placeholder is the bug-reporting
         email address for this package.  Please add _another line_ saying
         "Report translation bugs to <...>\n" with the address for translation
         bugs (typically your translation team's web or email address).  */
      printf (_("\
Report bugs in the bug tracker at <%s>\n\
or by email to <%s>.\n"),
             "https://savannah.gnu.org/projects/gettext",
             "bug-gettext@gnu.org");
    }

  exit (status);
}


static void
read_one_file (message_list_ty *mlp, const char *filename)
{
  if (csharp_resources_mode)
    read_resources_file (mlp, filename);
  else
    read_mo_file (mlp, filename);
}
