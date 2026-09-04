/* Manipulates attributes of messages in translation catalogs.
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
#include "write-catalog.h"
#include "write-po.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "xerror-handler.h"
#include "propername.h"
#include "xalloc.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Force output of PO file even if empty.  */
static int force_po;

/* Bit mask of subsets to remove.  */
enum
{
  REMOVE_UNTRANSLATED   = 1 << 0,
  REMOVE_TRANSLATED     = 1 << 1,
  REMOVE_FUZZY          = 1 << 2,
  REMOVE_NONFUZZY       = 1 << 3,
  REMOVE_OBSOLETE       = 1 << 4,
  REMOVE_NONOBSOLETE    = 1 << 5
};
static int to_remove;

/* Bit mask of actions to perform on all messages.  */
enum
{
  SET_FUZZY             = 1 << 0,
  RESET_FUZZY           = 1 << 1,
  SET_OBSOLETE          = 1 << 2,
  RESET_OBSOLETE        = 1 << 3,
  REMOVE_PREV           = 1 << 4,
  ADD_PREV              = 1 << 5,
  REMOVE_TRANSLATION    = 1 << 6
};
static int to_change;


/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void usage (int status);
static msgdomain_list_ty *process_msgdomain_list (msgdomain_list_ty *mdlp,
                                                  msgdomain_list_ty *only_mdlp,
                                                msgdomain_list_ty *ignore_mdlp);


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
  const char *input_file = NULL;
  const char *only_file = NULL;
  const char *ignore_file = NULL;
  catalog_input_format_ty input_syntax = &input_format_po;
  catalog_output_format_ty output_syntax = &output_format_po;
  bool sort_by_msgid = false;
  bool sort_by_filepos = false;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "add-location",       CHAR_MAX + 'n', optional_argument },
    { NULL,                 'n',            no_argument       },
    { "clear-fuzzy",        CHAR_MAX + 8,   no_argument       },
    { "clear-obsolete",     CHAR_MAX + 10,  no_argument       },
    { "clear-previous",     CHAR_MAX + 18,  no_argument       },
    { "empty",              CHAR_MAX + 23,  no_argument       },
    { "color",              CHAR_MAX + 19,  optional_argument },
    { "directory",          'D',            required_argument },
    { "escape",             'E',            no_argument       },
    { "force-po",           0,              no_argument,      &force_po, 1 },
    { "fuzzy",              CHAR_MAX + 11,  no_argument       },
    { "help",               'h',            no_argument       },
    { "ignore-file",        CHAR_MAX + 15,  required_argument },
    { "indent",             'i',            no_argument       },
    { "no-escape",          'e',            no_argument       },
    { "no-fuzzy",           CHAR_MAX + 3,   no_argument       },
    { "no-location",        CHAR_MAX + 22,  no_argument       },
    { "no-obsolete",        CHAR_MAX + 5,   no_argument       },
    { "no-wrap",            CHAR_MAX + 13,  no_argument       },
    { "obsolete",           CHAR_MAX + 12,  no_argument       },
    { "only-file",          CHAR_MAX + 14,  required_argument },
    { "only-fuzzy",         CHAR_MAX + 4,   no_argument       },
    { "only-obsolete",      CHAR_MAX + 6,   no_argument       },
    { "output-file",        'o',            required_argument },
    { "previous",           CHAR_MAX + 21,  no_argument       },
    { "properties-input",   'P',            no_argument       },
    { "properties-output",  'p',            no_argument       },
    { "set-fuzzy",          CHAR_MAX + 7,   no_argument       },
    { "set-obsolete",       CHAR_MAX + 9,   no_argument       },
    { "sort-by-file",       'F',            no_argument       },
    { "sort-output",        's',            no_argument       },
    { "stringtable-input",  CHAR_MAX + 16,  no_argument       },
    { "stringtable-output", CHAR_MAX + 17,  no_argument       },
    { "strict",             CHAR_MAX + 24,  no_argument       },
    { "style",              CHAR_MAX + 20,  required_argument },
    { "translated",         CHAR_MAX + 1,   no_argument       },
    { "untranslated",       CHAR_MAX + 2,   no_argument       },
    { "version",            'V',            no_argument       },
    { "width",              'w',            required_argument },
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

        case CHAR_MAX + 24: /* --strict */
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

        case CHAR_MAX + 1: /* --translated */
          to_remove |= REMOVE_UNTRANSLATED;
          break;

        case CHAR_MAX + 2: /* --untranslated */
          to_remove |= REMOVE_TRANSLATED;
          break;

        case CHAR_MAX + 3: /* --no-fuzzy */
          to_remove |= REMOVE_FUZZY;
          break;

        case CHAR_MAX + 4: /* --only-fuzzy */
          to_remove |= REMOVE_NONFUZZY;
          break;

        case CHAR_MAX + 5: /* --no-obsolete */
          to_remove |= REMOVE_OBSOLETE;
          break;

        case CHAR_MAX + 6: /* --only-obsolete */
          to_remove |= REMOVE_NONOBSOLETE;
          break;

        case CHAR_MAX + 7: /* --set-fuzzy */
          to_change |= SET_FUZZY;
          break;

        case CHAR_MAX + 8: /* --clear-fuzzy */
          to_change |= RESET_FUZZY;
          break;

        case CHAR_MAX + 9: /* --set-obsolete */
          to_change |= SET_OBSOLETE;
          break;

        case CHAR_MAX + 10: /* --clear-obsolete */
          to_change |= RESET_OBSOLETE;
          break;

        case CHAR_MAX + 11: /* --fuzzy */
          to_remove |= REMOVE_NONFUZZY;
          to_change |= RESET_FUZZY;
          break;

        case CHAR_MAX + 12: /* --obsolete */
          to_remove |= REMOVE_NONOBSOLETE;
          to_change |= RESET_OBSOLETE;
          break;

        case CHAR_MAX + 13: /* --no-wrap */
          message_page_width_ignore ();
          break;

        case CHAR_MAX + 14: /* --only-file */
          only_file = optarg;
          break;

        case CHAR_MAX + 15: /* --ignore-file */
          ignore_file = optarg;
          break;

        case CHAR_MAX + 16: /* --stringtable-input */
          input_syntax = &input_format_stringtable;
          break;

        case CHAR_MAX + 17: /* --stringtable-output */
          output_syntax = &output_format_stringtable;
          break;

        case CHAR_MAX + 18: /* --clear-previous */
          to_change |= REMOVE_PREV;
          break;

        case CHAR_MAX + 19: /* --color */
          if (handle_color_option (optarg) || color_test_mode)
            usage (EXIT_FAILURE);
          break;

        case CHAR_MAX + 20: /* --style */
          handle_style_option (optarg);
          break;

        case CHAR_MAX + 21: /* --previous */
          to_change |= ADD_PREV;
          break;

        case CHAR_MAX + 22: /* --no-location */
          message_print_style_filepos (filepos_comment_none);
          break;

        case CHAR_MAX + 23: /* --empty */
          to_change |= REMOVE_TRANSLATION;
          break;

        default:
          usage (EXIT_FAILURE);
          /* NOTREACHED */
        }
  }

  /* Version information requested.  */
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
  if (optind == argc)
    input_file = "-";
  else if (optind + 1 == argc)
    input_file = argv[optind];
  else
    {
      error (EXIT_SUCCESS, 0, _("at most one input file allowed"));
      usage (EXIT_FAILURE);
    }

  /* Verify selected options.  */
  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--sort-output", "--sort-by-file");

  /* Read input file.  */
  msgdomain_list_ty *result = read_catalog_file (input_file, input_syntax);

  /* Read optional files that limit the extent of the attribute changes.  */
  msgdomain_list_ty *only_mdlp =
    (only_file != NULL
     ? read_catalog_file (only_file, input_syntax)
     : NULL);
  msgdomain_list_ty *ignore_mdlp =
    (ignore_file != NULL
     ? read_catalog_file (ignore_file, input_syntax)
     : NULL);

  /* Filter the messages and manipulate the attributes.  */
  result = process_msgdomain_list (result, only_mdlp, ignore_mdlp);

  /* Sorting the list of messages.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (result);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (result);

  /* Write the PO file.  */
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
Usage: %s [OPTION] [INPUTFILE]\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Filters the messages of a translation catalog according to their attributes,\n\
and manipulates the attributes.\n"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  INPUTFILE                   input PO file\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
If no input file is given or if it is -, standard input is read.\n"));
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
Message selection:\n"));
      printf (_("\
      --translated            keep translated, remove untranslated messages\n"));
      printf (_("\
      --untranslated          keep untranslated, remove translated messages\n"));
      printf (_("\
      --no-fuzzy              remove 'fuzzy' marked messages\n"));
      printf (_("\
      --only-fuzzy            keep 'fuzzy' marked messages\n"));
      printf (_("\
      --no-obsolete           remove obsolete #~ messages\n"));
      printf (_("\
      --only-obsolete         keep obsolete #~ messages\n"));
      printf ("\n");
      printf (_("\
Attribute manipulation:\n"));
      printf (_("\
      --set-fuzzy             set all messages 'fuzzy'\n"));
      printf (_("\
      --clear-fuzzy           set all messages non-'fuzzy'\n"));
      printf (_("\
      --set-obsolete          set all messages obsolete\n"));
      printf (_("\
      --clear-obsolete        set all messages non-obsolete\n"));
      printf (_("\
      --previous              when setting 'fuzzy', keep previous msgids\n\
                              of translated messages.\n"));
      printf (_("\
      --clear-previous        remove the \"previous msgid\" from all messages\n"));
      printf (_("\
      --empty                 when removing 'fuzzy', also set msgstr empty\n"));
      printf (_("\
      --only-file=FILE.po     manipulate only entries listed in FILE.po\n"));
      printf (_("\
      --ignore-file=FILE.po   manipulate only entries not listed in FILE.po\n"));
      printf (_("\
      --fuzzy                 synonym for --only-fuzzy --clear-fuzzy\n"));
      printf (_("\
      --obsolete              synonym for --only-obsolete --clear-obsolete\n"));
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
  -i, --indent                write the .po file using indented style\n"));
      printf (_("\
      --no-location           do not write '#: filename:line' lines\n"));
      printf (_("\
  -n, --add-location          generate '#: filename:line' lines (default)\n"));
      printf (_("\
      --strict                write out strict Uniforum conforming .po file\n"));
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


/* Return true if a message should be kept.  */
static bool
is_message_selected (const message_ty *mp)
{
  /* Always keep the header entry.  */
  if (is_header (mp))
    return true;

  if ((to_remove & (REMOVE_UNTRANSLATED | REMOVE_TRANSLATED))
      && (mp->msgstr[0] == '\0'
          ? to_remove & REMOVE_UNTRANSLATED
          : to_remove & REMOVE_TRANSLATED))
    return false;

  if ((to_remove & (REMOVE_FUZZY | REMOVE_NONFUZZY))
      && (mp->is_fuzzy
          ? to_remove & REMOVE_FUZZY
          : to_remove & REMOVE_NONFUZZY))
    return false;

  if ((to_remove & (REMOVE_OBSOLETE | REMOVE_NONOBSOLETE))
      && (mp->obsolete
          ? to_remove & REMOVE_OBSOLETE
          : to_remove & REMOVE_NONOBSOLETE))
    return false;

  return true;
}


static void
process_message_list (message_list_ty *mlp,
                      message_list_ty *only_mlp, message_list_ty *ignore_mlp)
{
  /* Keep only the selected messages.  */
  message_list_remove_if_not (mlp, is_message_selected);

  /* Change the attributes.  */
  if (to_change)
    {
      for (size_t j = 0; j < mlp->nitems; j++)
        {
          message_ty *mp = mlp->item[j];

          /* Attribute changes only affect messages listed in --only-file
             and not listed in --ignore-file.  */
          if ((only_mlp
               ? message_list_search (only_mlp, mp->msgctxt, mp->msgid) != NULL
               : true)
              && (ignore_mlp
                  ? message_list_search (ignore_mlp, mp->msgctxt, mp->msgid) == NULL
                  : true))
            {
              if (to_change & SET_FUZZY)
                {
                  if ((to_change & ADD_PREV) && !is_header (mp)
                      && !mp->is_fuzzy && mp->msgstr[0] != '\0')
                    {
                      mp->prev_msgctxt =
                        (mp->msgctxt != NULL ? xstrdup (mp->msgctxt) : NULL);
                      mp->prev_msgid =
                        (mp->msgid != NULL ? xstrdup (mp->msgid) : NULL);
                      mp->prev_msgid_plural =
                        (mp->msgid_plural != NULL
                         ? xstrdup (mp->msgid_plural)
                         : NULL);
                    }
                  mp->is_fuzzy = true;
                }

              if (to_change & RESET_FUZZY)
                {
                  if ((to_change & REMOVE_TRANSLATION)
                      && mp->is_fuzzy && !mp->obsolete)
                    {
                      unsigned long int nplurals = 0;
                      for (size_t pos = 0; pos < mp->msgstr_len; ++pos)
                        if (!mp->msgstr[pos])
                          ++nplurals;

                      free ((char *) mp->msgstr);

                      char *msgstr = XNMALLOC (nplurals, char);
                      memset (msgstr, '\0', nplurals);
                      mp->msgstr = msgstr;
                      mp->msgstr_len = nplurals;
                    }
                  mp->is_fuzzy = false;
                }
              /* Always keep the header entry non-obsolete.  */
              if ((to_change & SET_OBSOLETE) && !is_header (mp))
                mp->obsolete = true;
              if (to_change & RESET_OBSOLETE)
                mp->obsolete = false;
              if (to_change & REMOVE_PREV)
                {
                  mp->prev_msgctxt = NULL;
                  mp->prev_msgid = NULL;
                  mp->prev_msgid_plural = NULL;
                }
            }
        }
    }
}


static msgdomain_list_ty *
process_msgdomain_list (msgdomain_list_ty *mdlp,
                        msgdomain_list_ty *only_mdlp,
                        msgdomain_list_ty *ignore_mdlp)
{
  for (size_t k = 0; k < mdlp->nitems; k++)
    process_message_list (mdlp->item[k]->messages,
                          only_mdlp
                          ? msgdomain_list_sublist (only_mdlp,
                                                    mdlp->item[k]->domain,
                                                    true)
                          : NULL,
                          ignore_mdlp
                          ? msgdomain_list_sublist (ignore_mdlp,
                                                    mdlp->item[k]->domain,
                                                    false)
                          : NULL);

  return mdlp;
}
