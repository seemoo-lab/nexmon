/* Pretranslate using machine translation.
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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <textstyle.h>

#include <error.h>
#include "options.h"
#include "noreturn.h"
#include "closeout.h"
#include "dir-list.h"
#include "xvasprintf.h"
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
#include "msgl-charset.h"
#include "xalloc.h"
#include "findprog.h"
#include "pipe-filter.h"
#include "msgl-iconv.h"
#include "xerror-handler.h"
#include "po-charset.h"
#include "c-strstr.h"
#include "propername.h"
#include "gettext.h"

#define _(str) gettext (str)


/* We use the 'spit' program as a child process, and communicate through
   a bidirectional pipe.  */


/* Force output of PO file even if empty.  */
static int force_po;

/* Keep the fuzzy messages unmodified.  */
static int keep_fuzzy;

/* Name of the subprogram.  */
static const char *sub_name;

/* Pathname of the subprogram.  */
static const char *sub_path;

/* Argument list for the subprogram.  */
static const char **sub_argv;
static int sub_argc;

/* If true do not print unneeded messages.  */
static bool quiet;


/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void usage (int status);
static void generic_filter (const char *str, size_t len, char **resultp, size_t *lengthp);
static msgdomain_list_ty *process_msgdomain_list (msgdomain_list_ty *mdlp);


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
  const char *species = "ollama";
  const char *url = "http://localhost:11434";
  const char *model = NULL;
  const char *to_language = NULL;
  const char *prompt = NULL;
  const char *postprocess = NULL;
  catalog_input_format_ty input_syntax = &input_format_po;
  catalog_output_format_ty output_syntax = &output_format_po;
  bool sort_by_filepos = false;
  bool sort_by_msgid = false;
  quiet = false;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "add-location",       CHAR_MAX + 'n', optional_argument },
    { NULL,                 'n',            no_argument       },
    { "color",              CHAR_MAX + 6,   optional_argument },
    { "directory",          'D',            required_argument },
    { "force-po",           0,              no_argument,      &force_po, 1 },
    { "help",               'h',            no_argument       },
    { "indent",             CHAR_MAX + 8,   no_argument       },
    { "input",              'i',            required_argument },
    { "keep-fuzzy",         0,              no_argument,      &keep_fuzzy, 1 },
    { "model",              'm',            required_argument },
    { "no-location",        CHAR_MAX + 9,   no_argument       },
    { "no-wrap",            CHAR_MAX + 12,  no_argument       },
    { "output-file",        'o',            required_argument },
    { "postprocess",        CHAR_MAX + 4,   required_argument },
    { "prompt",             CHAR_MAX + 3,   required_argument },
    { "properties-input",   'P',            no_argument       },
    { "properties-output",  'p',            no_argument       },
    { "quiet",              'q',            no_argument       },
    { "silent",             'q',            no_argument       },
    { "sort-by-file",       'F',            no_argument       },
    { "sort-output",        's',            no_argument       },
    { "species",            CHAR_MAX + 1,   required_argument },
    { "strict",             CHAR_MAX + 10,  no_argument       },
    { "stringtable-input",  CHAR_MAX + 5,   no_argument       },
    { "stringtable-output", CHAR_MAX + 11,  no_argument       },
    { "style",              CHAR_MAX + 7,   required_argument },
    { "url",                CHAR_MAX + 2,   required_argument },
    { "version",            'V',            no_argument       },
    { "width",              'w',            required_argument },
  };
  END_ALLOW_OMITTING_FIELD_INITIALIZERS
  /* The flag NON_OPTION_TERMINATES_OPTIONS causes option parsing to terminate
     when the first non-option, i.e. the subprogram name, is encountered.  */
  start_options (argc, argv, options, NON_OPTION_TERMINATES_OPTIONS, 0);
  {
    int opt;
    while ((opt = get_next_option ()) != -1)
      switch (opt)
        {
        case '\0':                /* Long option with key == 0.  */
          break;

        case 'i':
          if (input_file != NULL)
            {
              error (EXIT_SUCCESS, 0, _("at most one input file allowed"));
              usage (EXIT_FAILURE);
            }
          input_file = optarg;
          break;

        case 'D':
          dir_list_append (optarg);
          break;

        case 'o':
          output_file = optarg;
          break;

        case CHAR_MAX + 1: /* --species */
          species = optarg;
          break;

        case CHAR_MAX + 2: /* --url */
          url = optarg;
          break;

        case 'm': /* --model */
          model = optarg;
          break;

        case CHAR_MAX + 3: /* --prompt */
          prompt = optarg;
          break;

        case CHAR_MAX + 4: /* --postprocess */
          postprocess = optarg;
          break;

        case 'P':
          input_syntax = &input_format_properties;
          break;

        case CHAR_MAX + 5: /* --stringtable-input */
          input_syntax = &input_format_stringtable;
          break;

        case CHAR_MAX + 6: /* --color */
          if (handle_color_option (optarg) || color_test_mode)
            usage (EXIT_FAILURE);
          break;

        case CHAR_MAX + 7: /* --style */
          handle_style_option (optarg);
          break;

        case CHAR_MAX + 8: /* --indent */
          message_print_style_indent ();
          break;

        case CHAR_MAX + 9: /* --no-location */
          message_print_style_filepos (filepos_comment_none);
          break;

        case 'n':            /* -n */
        case CHAR_MAX + 'n': /* --add-location[={full|yes|file|never|no}] */
          if (handle_filepos_comment_option (optarg))
            usage (EXIT_FAILURE);
          break;

        case CHAR_MAX + 10: /* --strict */
          message_print_style_uniforum ();
          break;

        case 'p':
          output_syntax = &output_format_properties;
          break;

        case CHAR_MAX + 11: /* --stringtable-output */
          output_syntax = &output_format_stringtable;
          break;

        case 'w':
          {
            char *endp;
            int value = strtol (optarg, &endp, 10);
            if (endp != optarg)
              message_page_width_set (value);
          }
          break;

        case CHAR_MAX + 12: /* --no-wrap */
          message_page_width_ignore ();
          break;

        case 's':
          sort_by_msgid = true;
          break;

        case 'F':
          sort_by_filepos = true;
          break;

        case 'h':
          do_help = true;
          break;

        case 'V':
          do_version = true;
          break;

        case 'q': /* --quiet, --silent */
          quiet = true;
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

  /* Test for extraneous arguments.  */
  if (optind != argc)
    error (EXIT_FAILURE, 0, _("too many arguments"));

  /* Check --species option.  */
  if (strcmp (species, "ollama") != 0)
    error (EXIT_FAILURE, 0, _("invalid value for %s option: %s"),
           "--species", species);

  /* Check --model option.  */
  if (model == NULL)
    error (EXIT_FAILURE, 0, _("missing %s option"),
           "--model");

  /* Verify selected options.  */
  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--sort-output", "--sort-by-file");

  /* By default, input comes from standard input.  */
  if (input_file == NULL)
    input_file = "-";

  /* Read input file.  */
  msgdomain_list_ty *result = read_catalog_file (input_file, input_syntax);

  /* Convert the input to UTF-8 first.  */
  result = iconv_msgdomain_list (result, po_charset_utf8, true, input_file,
                                 textmode_xerror_handler);

  /* Warn if the current locale is not suitable for this PO file.  */
  compare_po_locale_charsets (result);

  /* Extract the target language from the header entry.  */
  if (prompt == NULL)
    {
      bool header_found = false;
      for (size_t k = 0; k < result->nitems; k++)
        {
          message_list_ty *mlp = result->item[k]->messages;
          message_ty *header = message_list_search (mlp, NULL, "");
          if (header != NULL && !header->obsolete)
            {
              header_found = true;
              const char *nullentry = header->msgstr;
              const char *language = c_strstr (nullentry, "Language: ");
              if (language != NULL)
                {
                  language += 10;

                  size_t len = strcspn (language, " \t\n");
                  if (len > 0)
                    {
                      char *memory = (char *) malloc (len + 1);
                      memcpy (memory, language, len);
                      memory[len] = '\0';

                      to_language = memory;
                      break;
                    }
                }
            }

          if (to_language != NULL)
            break;
        }

      if (!header_found)
        error (EXIT_FAILURE, 0, _("The input does not have a header entry."));

      if (to_language == NULL)
        error (EXIT_FAILURE, 0,
               _("The input's header entry does not contain the '%s' header field."),
               "Language");
    }

  /* The name of the subprogram.  */
  sub_name = "spit";

  /* Attempt to locate the subprogram.
     This is an optimization, to avoid that spawn/exec searches the PATH
     on every call.  */
  sub_path = find_in_path (sub_name);

  /* Build the argument list for the subprogram.  */
  sub_argv = (const char **) XNMALLOC (7, const char *);
  {
    sub_argv[0] = sub_path;
    size_t i = 1;

    if (species != NULL)
      sub_argv[i++] = xasprintf ("--species=%s", species);

    if (url != NULL)
      sub_argv[i++] = xasprintf ("--url=%s", url);

    sub_argv[i++] = xasprintf ("--model=%s", model);

    if (prompt != NULL)
      sub_argv[i++] = xasprintf ("--prompt=%s", prompt);
    else
      sub_argv[i++] = xasprintf ("--to=%s", to_language);

    if (postprocess != NULL)
      sub_argv[i++] = xasprintf ("--postprocess=%s", postprocess);

    sub_argv[i] = NULL;
    sub_argc = i;
  }

  /* Apply the subprogram.  */
  result = process_msgdomain_list (result);

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
Usage: %s [OPTION...]\n\
"), program_name);
      printf ("\n");
      printf (_("\
Pretranslates a translation catalog.\n\
"));
      printf ("\n");
      printf (_("\
Warning: The pretranslations might not be what you expect.\n\
They might be of the wrong form, be of poor quality, or reflect some biases.\n"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  -i, --input=INPUTFILE       input PO file\n"));
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
      --keep-fuzzy            Keep fuzzy messages unmodified.\n\
                              Pretranslate only untranslated messages.\n"));
      printf ("\n");
      printf (_("\
Large Language Model (LLM) options:\n"));
      printf (_("\
      --species=TYPE          Specifies the type of LLM.  The default and only\n\
                              valid value is '%s'.\n"),
              "ollama");
      printf (_("\
      --url=URL               Specifies the URL of the server that runs the LLM.\n"));
      printf (_("\
  -m, --model=MODEL           Specifies the model to use.\n"));
      printf (_("\
      --prompt=TEXT           Specifies the prompt to use before standard input.\n"));
      printf (_("\
      --postprocess=COMMAND   Specifies a command to post-process the output.\n"));
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
      --force-po              write PO file even if empty\n"));
      printf (_("\
      --indent                indented output style\n"));
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


/* Callbacks called from pipe_filter_ii_execute.  */

struct locals
{
  /* String being written.  */
  const char *str;
  size_t len;
  /* String being read and accumulated.  */
  char *result;
  size_t allocated;
  size_t length;
};

static const void *
prepare_write (size_t *num_bytes_p, void *private_data)
{
  struct locals *l = (struct locals *) private_data;

  if (l->len > 0)
    {
      *num_bytes_p = l->len;
      return l->str;
    }
  else
    return NULL;
}

static void
done_write (void *data_written, size_t num_bytes_written, void *private_data)
{
  struct locals *l = (struct locals *) private_data;

  l->str += num_bytes_written;
  l->len -= num_bytes_written;
}

static void *
prepare_read (size_t *num_bytes_p, void *private_data)
{
  struct locals *l = (struct locals *) private_data;

  if (l->length == l->allocated)
    {
      l->allocated = l->allocated + (l->allocated >> 1) + 1;
      l->result = (char *) xrealloc (l->result, l->allocated);
    }
  *num_bytes_p = l->allocated - l->length;
  return l->result + l->length;
}

static void
done_read (void *data_read, size_t num_bytes_read, void *private_data)
{
  struct locals *l = (struct locals *) private_data;

  l->length += num_bytes_read;
}


/* Process a string STR of size LEN bytes through the subprogram.
   Store the freshly allocated result at *RESULTP and its length at *LENGTHP.
 */
static void
generic_filter (const char *str, size_t len, char **resultp, size_t *lengthp)
{
  struct locals l;
  l.str = str;
  l.len = len;
  l.allocated = len + (len >> 2) + 1;
  l.result = XNMALLOC (l.allocated, char);
  l.length = 0;

  pipe_filter_ii_execute (sub_name, sub_path, sub_argv, false, true,
                          prepare_write, done_write, prepare_read, done_read,
                          &l);

  *resultp = l.result;
  *lengthp = l.length;
}


/* Process a string STR of size LEN bytes, then remove NUL bytes.
   Store the freshly allocated result at *RESULTP and its length at *LENGTHP.
 */
static void
process_string (const char *str, size_t len, char **resultp, size_t *lengthp)
{
  char *result;
  size_t length;
  generic_filter (str, len, &result, &length);

  /* Remove NUL bytes from result.  */
  {
    char *p = result;
    char *pend = result + length;

    for (; p < pend; p++)
      if (*p == '\0')
        {
          char *q = p;
          for (; p < pend; p++)
            if (*p != '\0')
              *q++ = *p;
          length = q - result;
          break;
        }
  }

  *resultp = result;
  *lengthp = length;
}


/* Number of messages processed so far.  */
static size_t messages_processed;


static void
process_message (message_ty *mp)
{
  /* Keep the header entry unmodified.  */
  if (is_header (mp))
    return;

  /* Ignore obsolete messages.  */
  if (mp->obsolete)
    return;

  /* Translate only untranslated or, if --keep-fuzzy is not specified, fuzzy
     messages.  */
  if (!(mp->msgstr[0] == '\0'
        || (mp->is_fuzzy && !keep_fuzzy)))
    return;

  /* Because querying a Large Language Model can take a while
     we print something to signal we are not dead.  */
  if (!quiet)
    {
      fputc ('.', stderr);
      messages_processed++;
    }

  /* Take the msgid.
     For a plural message, take the msgid_plural and repeat its translation
     for each of the plural forms.  Let the translator work out the plural
     forms.  */
  const char *msgid = (mp->msgid_plural != NULL ? mp->msgid_plural : mp->msgid);

  char *result;
  size_t length;
  process_string (msgid, strlen (msgid), &result, &length);

  /* Avoid an error later, during "msgfmt --check", due to a trailing newline.  */
  if (strlen (msgid) > 0 && msgid[strlen (msgid) - 1] == '\n')
    {
      /* msgid ends in a newline.  Ensure that the result ends in a newline
         as well.  */
      if (!(length > 0 && result[length - 1] == '\n'))
        {
          result = (char *) xrealloc (result, length + 1);
          result[length] = '\n';
          length++;
        }
    }
  else
    {
      /* msgid does not end in a newline.  Ensure that the same holds for the
         result.  */
      while (length > 0 && result[length - 1] == '\n')
        length--;
    }

  /* Count the number of plural forms.  */
  size_t nplurals;
  {
    const char *msgstr = mp->msgstr;
    size_t msgstr_len = mp->msgstr_len;
    nplurals = 0;
    for (const char *p = msgstr; p < msgstr + msgstr_len; p += strlen (p) + 1)
      nplurals++;
  }

  /* Produce nplurals copies of the result, each with an added NUL.  */
  size_t msgstr_len = nplurals * (length + 1);
  char *msgstr = XNMALLOC (msgstr_len, char);
  {
    char *p;
    size_t k;
    for (p = msgstr, k = 0; k < nplurals; k++)
      {
        memcpy (p, result, length);
        p += length;
        *p++ = '\0';
      }
  }

  mp->msgstr = msgstr;
  mp->msgstr_len = msgstr_len;

  /* Mark the message as fuzzy, so that the translator can review it.  */
  mp->is_fuzzy = (msgstr_len > 0);
}


static void
process_message_list (message_list_ty *mlp)
{
  for (size_t j = 0; j < mlp->nitems; j++)
    process_message (mlp->item[j]);
}


static msgdomain_list_ty *
process_msgdomain_list (msgdomain_list_ty *mdlp)
{
  messages_processed = 0;

  for (size_t k = 0; k < mdlp->nitems; k++)
    process_message_list (mdlp->item[k]->messages);

  if (messages_processed > 0)
    fputc ('\n', stderr);

  return mdlp;
}
