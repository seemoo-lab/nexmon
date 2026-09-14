/* Creates an English translation catalog.
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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include <textstyle.h>

#include <error.h>
#include "options.h"
#include "noreturn.h"
#include "closeout.h"
#include "dir-list.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename-lgpl.h"
#include "message.h"
#include "read-catalog-file.h"
#include "read-po.h"
#include "read-properties.h"
#include "read-stringtable.h"
#include "msgl-english.h"
#include "msgl-ascii.h"
#include "msgl-header.h"
#include "c-strstr.h"
#include "xalloc.h"
#include "write-catalog.h"
#include "write-po.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "xerror-handler.h"
#include "propername.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Force output of PO file even if empty.  */
static int force_po;


/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void usage (int status);
static msgdomain_list_ty *fill_header (msgdomain_list_ty *mdlp);


int
main (int argc, char **argv)
{
  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;
  gram_max_allowed_errors = 20;

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
  char *output_file = NULL;
  catalog_input_format_ty input_syntax = &input_format_po;
  catalog_output_format_ty output_syntax = &output_format_po;
  bool sort_by_filepos = false;
  bool sort_by_msgid = false;
  /* Language (ISO-639 code) and optional territory (ISO-3166 code).  */
  const char *catalogname = NULL;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "add-location",       CHAR_MAX + 'n', optional_argument },
    { NULL,                 'n',            no_argument       },
    { "color",              CHAR_MAX + 5,   optional_argument },
    { "directory",          'D',            required_argument },
    { "escape",             'E',            no_argument       },
    { "force-po",           0,              no_argument,      &force_po, 1 },
    { "help",               'h',            no_argument       },
    { "indent",             'i',            no_argument       },
    { "lang",               CHAR_MAX + 4,   required_argument },
    { "no-escape",          'e',            no_argument       },
    { "no-location",        CHAR_MAX + 7,   no_argument       },
    { "no-wrap",            CHAR_MAX + 1,   no_argument       },
    { "output-file",        'o',            required_argument },
    { "properties-input",   'P',            no_argument       },
    { "properties-output",  'p',            no_argument       },
    { "sort-by-file",       'F',            no_argument       },
    { "sort-output",        's',            no_argument       },
    { "strict",             CHAR_MAX + 8,   no_argument       },
    { "stringtable-input",  CHAR_MAX + 2,   no_argument       },
    { "stringtable-output", CHAR_MAX + 3,   no_argument       },
    { "style",              CHAR_MAX + 6,   required_argument },
    { "version",            'V',            no_argument       },
    { "width",              'w',            required_argument },
  };
  END_ALLOW_OMITTING_FIELD_INITIALIZERS
  start_options (argc, argv, options, MOVE_OPTIONS_FIRST, 0);
  {
    int opt;
    while ((opt = get_next_option ()) != -1)
      switch (opt)
        {
        case '\0':                /* Long option with key == 0.  */
          break;

        case 'D':
          dir_list_append (optarg);
          break;

        case 'e':
          message_print_style_escape (false);
          break;

        case 'E':
          message_print_style_escape (true);
          break;

        case 'F':
          sort_by_filepos = true;
          break;

        case 'h':
          do_help = true;
          break;

        case 'i':
          message_print_style_indent ();
          break;

        case 'n':            /* -n */
        case CHAR_MAX + 'n': /* --add-location[={full|yes|file|never|no}] */
          if (handle_filepos_comment_option (optarg))
            usage (EXIT_FAILURE);
          break;

        case 'o':
          output_file = optarg;
          break;

        case 'p':
          output_syntax = &output_format_properties;
          break;

        case 'P':
          input_syntax = &input_format_properties;
          break;

        case 's':
          sort_by_msgid = true;
          break;

        case CHAR_MAX + 8: /* --strict */
          message_print_style_uniforum ();
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

        case CHAR_MAX + 1: /* --no-wrap */
          message_page_width_ignore ();
          break;

        case CHAR_MAX + 2: /* --stringtable-input */
          input_syntax = &input_format_stringtable;
          break;

        case CHAR_MAX + 3: /* --stringtable-output */
          output_syntax = &output_format_stringtable;
          break;

        case CHAR_MAX + 4: /* --lang */
          catalogname = optarg;
          break;

        case CHAR_MAX + 5: /* --color */
          if (handle_color_option (optarg) || color_test_mode)
            usage (EXIT_FAILURE);
          break;

        case CHAR_MAX + 6: /* --style */
          handle_style_option (optarg);
          break;

        case CHAR_MAX + 7: /* --no-location */
          message_print_style_filepos (filepos_comment_none);
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
              "2001-2026", "https://gnu.org/licenses/gpl.html");
      printf (_("Written by %s.\n"), proper_name ("Bruno Haible"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test whether we have an .po file name as argument.  */
  if (optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input file given"));
      usage (EXIT_FAILURE);
    }
  if (optind + 1 != argc)
    {
      error (EXIT_SUCCESS, 0, _("exactly one input file required"));
      usage (EXIT_FAILURE);
    }

  /* Verify selected options.  */
  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--sort-output", "--sort-by-file");

  /* Read input file.  */
  msgdomain_list_ty *result = read_catalog_file (argv[optind], input_syntax);

  if (!output_syntax->requires_utf8)
    /* Fill the header entry.  */
    result = fill_header (result);

  /* Add English translations.  */
  result = msgdomain_list_english (result);

  /* Set the Language field in the header.  */
  if (catalogname != NULL)
    msgdomain_list_set_header_field (result, "Language:", catalogname);

  /* Sort the results.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (result);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (result);

  /* Write the merged message list out.  */
  msgdomain_list_print (result, output_file, output_syntax,
                        textmode_xerror_handler, force_po, false);

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
Usage: %s [OPTION] INPUTFILE\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Creates an English translation catalog.  The input file is the last\n\
created English PO file, or a PO Template file (generally created by\n\
xgettext).  Untranslated entries are assigned a translation that is\n\
identical to the msgid.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  INPUTFILE                   input PO or POT file\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
If input file is -, standard input is read.\n"));
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
Input file syntax:\n"));
      printf (_("\
  -P, --properties-input      input file is in Java .properties syntax\n"));
      printf (_("\
      --stringtable-input     input file is in NeXTstep/GNUstep .strings syntax\n"));
      printf ("\n");
      printf (_("\
Output details:\n"));
      printf (_("\
      --lang=CATALOGNAME      set 'Language' field in the header entry\n"));
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
  -i, --indent                indented output style\n"));
      printf (_("\
      --no-location           suppress '#: filename:line' lines\n"));
      printf (_("\
  -n, --add-location          preserve '#: filename:line' lines (default)\n"));
      printf (_("\
      --strict                strict Uniforum output style\n"));
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
      printf (_("\
  -F, --sort-by-file          sort output by file location\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
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

/* Fill the templates in the most essential fields of the header entry,
   namely to force a charset name.  */
static msgdomain_list_ty *
fill_header (msgdomain_list_ty *mdlp)
{
  if (mdlp->encoding == NULL
      && is_ascii_msgdomain_list (mdlp))
    mdlp->encoding = "ASCII";

  if (mdlp->encoding != NULL)
    for (size_t k = 0; k < mdlp->nitems; k++)
      {
        message_list_ty *mlp = mdlp->item[k]->messages;

        if (mlp->nitems > 0)
          {
            message_ty *header_mp = NULL;

            /* Search the header entry.  */
            for (size_t j = 0; j < mlp->nitems; j++)
              if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
                {
                  header_mp = mlp->item[j];
                  break;
                }

            /* If it wasn't found, provide one.  */
            if (header_mp == NULL)
              {
                static lex_pos_ty pos = { __FILE__, __LINE__ };
                const char *msgstr =
                  "Content-Type: text/plain; charset=CHARSET\n"
                  "Content-Transfer-Encoding: 8bit\n";

                header_mp =
                  message_alloc (NULL, "", NULL, msgstr, strlen (msgstr), &pos);
                message_list_prepend (mlp, header_mp);
              }

            /* Fill in the charset name.  */
            {
              const char *header = header_mp->msgstr;
              const char *charsetstr = c_strstr (header, "charset=");
              if (charsetstr != NULL)
                {
                  charsetstr += strlen ("charset=");
                  header_set_charset (header_mp, charsetstr, mdlp->encoding);
                }
            }

            /* Finally remove the fuzzy attribute.  */
            header_mp->is_fuzzy = false;
          }
      }

  return mdlp;
}
