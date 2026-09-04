/* GNU gettext - internationalization aids
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

/* Written by Peter Miller, Ulrich Drepper, and Bruno Haible.  */

#include <config.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#ifdef _OPENMP
# include <omp.h>
#endif

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
#include "write-catalog.h"
#include "write-po.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "xalloc.h"
#include "msgl-equal.h"
#include "msgl-merge.h"
#include "xerror-handler.h"
#include "backupfile.h"
#include "copy-file.h"
#include "propername.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Force output of PO file even if empty.  */
static int force_po;

/* Update mode.  */
static bool update_mode = false;
static const char *version_control_string;
static const char *backup_suffix_string;


/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void usage (int status);
static void compendium (const char *filename);
static void msgdomain_list_stablesort_by_obsolete (msgdomain_list_ty *mdlp);


int
main (int argc, char **argv)
{
  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;
  gram_max_allowed_errors = UINT_MAX;

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
  verbosity_level = 0;
  quiet = false;
  char *output_file = NULL;
  char *color = NULL;
  catalog_input_format_ty input_syntax = &input_format_po;
  catalog_output_format_ty output_syntax = &output_format_po;
  bool sort_by_filepos = false;
  bool sort_by_msgid = false;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "add-location",       CHAR_MAX + 'n', optional_argument },
    { NULL,                 'n',            no_argument       },
    { "backup",             CHAR_MAX + 1,   required_argument },
    { "color",              CHAR_MAX + 9,   optional_argument },
    { "compendium",         'C',            required_argument },
    { "directory",          'D',            required_argument },
    { "escape",             'E',            no_argument       },
    { "for-msgfmt",         CHAR_MAX + 12,  no_argument       },
    { "force-po",           0,              no_argument,      &force_po, 1 },
    { "help",               'h',            no_argument       },
    { "indent",             'i',            no_argument       },
    { "lang",               CHAR_MAX + 8,   required_argument },
    { "multi-domain",       'm',            no_argument       },
    { "no-escape",          'e',            no_argument       },
    { "no-fuzzy-matching",  'N',            no_argument       },
    { "no-location",        CHAR_MAX + 11,  no_argument       },
    { "no-wrap",            CHAR_MAX + 4,   no_argument       },
    { "output-file",        'o',            required_argument },
    { "previous",           CHAR_MAX + 7,   no_argument       },
    { "properties-input",   'P',            no_argument       },
    { "properties-output",  'p',            no_argument       },
    { "quiet",              'q',            no_argument       },
    { "sort-by-file",       'F',            no_argument       },
    { "sort-output",        's',            no_argument       },
    { "silent",             'q',            no_argument       },
    { "strict",             CHAR_MAX + 2,   no_argument       },
    { "stringtable-input",  CHAR_MAX + 5,   no_argument       },
    { "stringtable-output", CHAR_MAX + 6,   no_argument       },
    { "style",              CHAR_MAX + 10,  required_argument },
    { "suffix",             CHAR_MAX + 3,   required_argument },
    { "update",             'U',            no_argument       },
    { "verbose",            'v',            no_argument       },
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

        case 'C':
          compendium (optarg);
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

        case 'm':
          multi_domain_mode = true;
          break;

        case 'n':            /* -n */
        case CHAR_MAX + 'n': /* --add-location[={full|yes|file|never|no}] */
          if (handle_filepos_comment_option (optarg))
            usage (EXIT_FAILURE);
          break;

        case 'N':
          use_fuzzy_matching = false;
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

        case 'q':
          quiet = true;
          break;

        case 's':
          sort_by_msgid = true;
          break;

        case 'U':
          update_mode = true;
          break;

        case 'v':
          ++verbosity_level;
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

        case CHAR_MAX + 1: /* --backup */
          version_control_string = optarg;
          break;

        case CHAR_MAX + 2: /* --strict */
          message_print_style_uniforum ();
          break;

        case CHAR_MAX + 3: /* --suffix */
          backup_suffix_string = optarg;
          break;

        case CHAR_MAX + 4: /* --no-wrap */
          message_page_width_ignore ();
          break;

        case CHAR_MAX + 5: /* --stringtable-input */
          input_syntax = &input_format_stringtable;
          break;

        case CHAR_MAX + 6: /* --stringtable-output */
          output_syntax = &output_format_stringtable;
          break;

        case CHAR_MAX + 7: /* --previous */
          keep_previous = true;
          break;

        case CHAR_MAX + 8: /* --lang */
          catalogname = optarg;
          break;

        case CHAR_MAX + 9: /* --color */
          if (handle_color_option (optarg) || color_test_mode)
            usage (EXIT_FAILURE);
          color = optarg;
          break;

        case CHAR_MAX + 10: /* --style */
          handle_style_option (optarg);
          break;

        case CHAR_MAX + 11: /* --no-location */
          message_print_style_filepos (filepos_comment_none);
          break;

        case CHAR_MAX + 12: /* --for-msgfmt */
          for_msgfmt = true;
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
              proper_name ("Peter Miller"), proper_name ("Bruno Haible"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test whether we have an .po file name as argument.  */
  if (optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input files given"));
      usage (EXIT_FAILURE);
    }
  if (optind + 2 != argc)
    {
      error (EXIT_SUCCESS, 0, _("exactly 2 input files required"));
      usage (EXIT_FAILURE);
    }

  /* Verify selected options.  */
  if (update_mode)
    {
      if (output_file != NULL)
        {
          error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
                 "--update", "--output-file");
        }
      if (for_msgfmt)
        {
          error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
                 "--update", "--for-msgfmt");
        }
      if (color != NULL)
        {
          error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
                 "--update", "--color");
        }
      if (style_file_name != NULL)
        {
          error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
                 "--update", "--style");
        }
    }
  else
    {
      if (version_control_string != NULL)
        {
          error (EXIT_SUCCESS, 0, _("%s is only valid with %s"),
                 "--backup", "--update");
          usage (EXIT_FAILURE);
        }
      if (backup_suffix_string != NULL)
        {
          error (EXIT_SUCCESS, 0, _("%s is only valid with %s"),
                 "--suffix", "--update");
          usage (EXIT_FAILURE);
        }
    }

  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--sort-output", "--sort-by-file");

  /* Warn when deprecated options are used.  */
  if (sort_by_msgid)
    error (EXIT_SUCCESS, 0, _("The option '%s' is deprecated."),
           "--sort-output");

  /* In update mode, --properties-input implies --properties-output.  */
  if (update_mode && input_syntax == &input_format_properties)
    output_syntax = &output_format_properties;
  /* In update mode, --stringtable-input implies --stringtable-output.  */
  if (update_mode && input_syntax == &input_format_stringtable)
    output_syntax = &output_format_stringtable;

  if (for_msgfmt)
    {
      /* With --for-msgfmt, no fuzzy matching.  */
      use_fuzzy_matching = false;

      /* With --for-msgfmt, merging is fast, therefore no need for a progress
         indicator.  */
      quiet = true;

      /* With --for-msgfmt, no need for comments.  */
      message_print_style_comment (false);

      /* With --for-msgfmt, no need for source location lines.  */
      message_print_style_filepos (filepos_comment_none);
    }

  /* Initialize OpenMP.  */
  #ifdef _OPENMP
  openmp_init ();
  #endif

  /* Merge the two files.  */
  msgdomain_list_ty *def;
  msgdomain_list_ty *result =
    merge (argv[optind], input_syntax, argv[optind + 1], input_syntax, &def);

  /* Sort the results.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (result);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (result);

  if (update_mode)
    {
      /* Before comparing result with def, sort the result into the same order
         as would be done implicitly by output_syntax->print.  */
      if (output_syntax->sorts_obsoletes_to_end)
        msgdomain_list_stablesort_by_obsolete (result);

      /* Do nothing if the original file and the result are equal.  Also do
         nothing if the original file and the result differ only by the
         POT-Creation-Date in the header entry; this is needed for projects
         which don't put the .pot file under CVS.  */
      if (!msgdomain_list_equal (def, result, true))
        {
          output_file = argv[optind];

          /* Back up def.po.  */
          if (backup_suffix_string == NULL)
            {
              backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");
              if (backup_suffix_string != NULL
                  && backup_suffix_string[0] == '\0')
                backup_suffix_string = NULL;
            }
          if (backup_suffix_string != NULL)
            simple_backup_suffix = backup_suffix_string;

          enum backup_type backup_type =
            xget_version (_("backup type"), version_control_string);
          if (backup_type != none)
            {
              char *backup_file =
                find_backup_file_name (output_file, backup_type);
              xcopy_file_preserving (output_file, backup_file);
            }

          /* Write the merged message list out.  */
          msgdomain_list_print (result, output_file, output_syntax,
                                textmode_xerror_handler, true, false);
        }
    }
  else
    {
      /* Write the merged message list out.  */
      msgdomain_list_print (result, output_file, output_syntax,
                            textmode_xerror_handler,
                            for_msgfmt || force_po, false);
    }

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
Usage: %s [OPTION] def.po ref.pot\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Merges two Uniforum style .po files together.  The def.po file is an\n\
existing PO file with translations which will be taken over to the newly\n\
created file as long as they still match; comments will be preserved,\n\
but extracted comments and file positions will be discarded.  The ref.pot\n\
file is the last created PO file with up-to-date source references but\n\
old translations, or a PO Template file (generally created by xgettext);\n\
any translations or comments in the file will be discarded, however dot\n\
comments and file positions will be preserved.  Where an exact match\n\
cannot be found, fuzzy matching is used to produce better results.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  def.po                      translations referring to old sources\n"));
      printf (_("\
  ref.pot                     references to new sources\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
  -C, --compendium=FILE       additional library of message translations,\n\
                              may be specified more than once\n"));
      printf ("\n");
      printf (_("\
Operation mode:\n"));
      printf (_("\
  -U, --update                update def.po,\n\
                              do nothing if def.po already up to date\n"));
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
Output file location in update mode:\n"));
      printf (_("\
The result is written back to def.po.\n"));
      printf (_("\
      --backup=CONTROL        make a backup of def.po\n"));
      printf (_("\
      --suffix=SUFFIX         override the usual backup suffix\n"));
      printf (_("\
The version control method may be selected via the --backup option or through\n\
the VERSION_CONTROL environment variable.  Here are the values:\n\
  none, off       never make backups (even if --backup is given)\n\
  numbered, t     make numbered backups\n\
  existing, nil   numbered if numbered backups exist, simple otherwise\n\
  simple, never   always make simple backups\n"));
      printf (_("\
The backup suffix is '~', unless set with --suffix or the SIMPLE_BACKUP_SUFFIX\n\
environment variable.\n\
"));
      printf ("\n");
      printf (_("\
Operation modifiers:\n"));
      printf (_("\
  -m, --multi-domain          apply ref.pot to each of the domains in def.po\n"));
      printf (_("\
      --for-msgfmt            produce output for '%s', not for a translator\n"),
              "msgfmt");
      printf (_("\
  -N, --no-fuzzy-matching     do not use fuzzy matching\n"));
      printf (_("\
      --previous              keep previous msgids of translated messages\n"));
      printf ("\n");
      printf (_("\
Input file syntax:\n"));
      printf (_("\
  -P, --properties-input      input files are in Java .properties syntax\n"));
      printf (_("\
      --stringtable-input     input files are in NeXTstep/GNUstep .strings\n\
                              syntax\n"));
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
  -s, --sort-output           generate sorted output (deprecated)\n"));
      printf (_("\
  -F, --sort-by-file          sort output by file location\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf (_("\
  -v, --verbose               increase verbosity level\n"));
      printf (_("\
  -q, --quiet, --silent       suppress progress indicators\n"));
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
compendium (const char *filename)
{
  msgdomain_list_ty *mdlp = read_catalog_file (filename, &input_format_po);
  if (compendiums == NULL)
    {
      compendiums = message_list_list_alloc ();
      compendium_filenames = string_list_alloc ();
    }
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      message_list_list_append (compendiums, mdlp->item[k]->messages);
      string_list_append (compendium_filenames, filename);
    }
}


/* Sorts obsolete messages to the end, for every domain.  */
static void
msgdomain_list_stablesort_by_obsolete (msgdomain_list_ty *mdlp)
{
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      /* Sort obsolete messages to the end.  */
      if (mlp->nitems > 0)
        {
          message_ty **l1 = XNMALLOC (mlp->nitems, message_ty *);
          message_ty **l2 = XNMALLOC (mlp->nitems, message_ty *);

          /* Sort the non-obsolete messages into l1 and the obsolete messages
             into l2.  */
          size_t n1 = 0;
          size_t n2 = 0;
          for (size_t j = 0; j < mlp->nitems; j++)
            {
              message_ty *mp = mlp->item[j];

              if (mp->obsolete)
                l2[n2++] = mp;
              else
                l1[n1++] = mp;
            }
          if (n1 > 0 && n2 > 0)
            {
              memcpy (mlp->item, l1, n1 * sizeof (message_ty *));
              memcpy (mlp->item + n1, l2, n2 * sizeof (message_ty *));
            }
          free (l2);
          free (l1);
        }
    }
}
