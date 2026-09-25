/* gettext - retrieve text string from message catalog and print it.
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
#include <string.h>
#include <locale.h>

#include <error.h>
#include "options.h"
#include "attribute.h"
#include "noreturn.h"
#include "closeout.h"
#include "progname.h"
#include "relocatable.h"
#include "basename-lgpl.h"
#include "xalloc.h"
#include "propername.h"
#include "xsetenv.h"
#include "glthread/thread.h"
#include "../../gettext-runtime/src/escapes.h"

/* Make sure we use the included libintl, not the system's one. */
#undef _LIBINTL_H
#include "libgnuintl.h"

#if defined _WIN32 && !defined __CYGWIN__
# undef setlocale
# define setlocale fake_setlocale
extern char *setlocale (int category, const char *locale);
#endif

#define _(str) gettext (str)

/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void *worker_thread (void *arg);
_GL_NORETURN_FUNC static void usage (int status);

/* Argument passed to the worker_thread.  */
struct worker_context
{
  int argc;
  char **argv;
  bool do_shell;
  const char *domain;
  const char *domaindir;
  /* If false, add newline after last string.  This makes only sense in
     the 'echo' emulation mode.  */
  bool inhibit_added_newline;
  /* If true, expand escape sequences in strings before looking in the
     message catalog.  */
  bool do_expand;
};

int
main (int argc, char *argv[])
{
  /* Default values for command line options.  */
  bool do_help = false;
  bool do_thread = false;
  bool do_version = false;
  bool environ_changed = false;
  struct worker_context context;
  context.argc = argc;
  context.argv = argv;
  context.do_shell = false;
  context.domain = getenv ("TEXTDOMAIN");
  context.domaindir = getenv ("TEXTDOMAINDIR");
  context.inhibit_added_newline = false;
  context.do_expand = false;

  /* Set program name for message texts.  */
  set_program_name (argv[0]);

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "domain",       'd',          required_argument },
    { "env",          CHAR_MAX + 1, required_argument },
    { "help",         'h',          no_argument       },
    { "shell-script", 's',          no_argument       },
    { "thread",       't',          no_argument       },
    { "version",      'V',          no_argument       },
    { NULL,           'e',          no_argument       },
    { NULL,           'E',          no_argument       },
    { NULL,           'n',          no_argument       },
  };
  END_ALLOW_OMITTING_FIELD_INITIALIZERS
  start_options (argc, argv, options, NON_OPTION_TERMINATES_OPTIONS, 0);
  int optchar;
  while ((optchar = get_next_option ()) != -1)
    switch (optchar)
      {
      case '\0':          /* Long option with key == 0.  */
        break;
      case 'd':
        context.domain = optarg;
        break;
      case 'e':
        context.do_expand = true;
        break;
      case 'E':
        /* Ignore.  Just for compatibility.  */
        break;
      case 'h':
        do_help = true;
        break;
      case 'n':
        context.inhibit_added_newline = true;
        break;
      case 's':
        context.do_shell = true;
        break;
      case 't':
        do_thread = true;
        break;
      case 'V':
        do_version = true;
        break;
      case CHAR_MAX + 1: /* --env */
        {
          /* Undocumented option --env sets an environment variable.  */
          char *separator = strchr (optarg, '=');
          if (separator != NULL)
            {
              *separator = '\0';
              xsetenv (optarg, separator + 1, 1);
              environ_changed = true;
              break;
            }
        }
        FALLTHROUGH;
      default:
        usage (EXIT_FAILURE);
      }

  if (environ_changed)
    /* Set locale again via LC_ALL.  */
    setlocale (LC_ALL, "");

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
              "1995-2023", "https://gnu.org/licenses/gpl.html");
      printf (_("Written by %s.\n"), proper_name ("Ulrich Drepper"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  if (do_thread)
    {
      gl_thread_t thread = gl_thread_create (worker_thread, &context);
      void *retval;
      gl_thread_join (thread, &retval);
    }
  else
    worker_thread (&context);
}

static void *
worker_thread (void *arg)
{
  struct worker_context *context = arg;
  int argc = context->argc;
  char **argv = context->argv;

  const char *msgid;

  /* We have two major modes: use following Uniforum spec and as
     internationalized 'echo' program.  */
  if (!context->do_shell)
    {
      /* We have to write a single strings translation to stdout.  */

      /* Get arguments.  */
      switch (argc - optind)
        {
          default:
            error (EXIT_FAILURE, 0, _("too many arguments"));

          case 2:
            context->domain = argv[optind++];
            FALLTHROUGH;

          case 1:
            break;

          case 0:
            error (EXIT_FAILURE, 0, _("missing arguments"));
        }

      msgid = argv[optind++];

      /* Expand escape sequences if enabled.  */
      if (context->do_expand)
        msgid = expand_escapes (msgid, &context->inhibit_added_newline);

      /* If no domain name is given we don't translate.  */
      if (context->domain == NULL || context->domain[0] == '\0')
        {
          fputs (msgid, stdout);
        }
      else
        {
          /* Bind domain to appropriate directory.  */
          if (context->domaindir != NULL && context->domaindir[0] != '\0')
            bindtextdomain (context->domain, context->domaindir);

          /* Write out the result.  */
          fputs (dgettext (context->domain, msgid), stdout);
        }
    }
  else
    {
      if (optind < argc)
        {
          /* If no domain name is given we print the original string.
             We mark this assigning NULL to domain.  */
          if (context->domain == NULL || context->domain[0] == '\0')
            context->domain = NULL;
          else
            /* Bind domain to appropriate directory.  */
            if (context->domaindir != NULL && context->domaindir[0] != '\0')
              bindtextdomain (context->domain, context->domaindir);

          /* We have to simulate 'echo'.  All arguments are strings.  */
          do
            {
              msgid = argv[optind++];

              /* Expand escape sequences if enabled.  */
              if (context->do_expand)
                msgid = expand_escapes (msgid, &context->inhibit_added_newline);

              /* Write out the result.  */
              fputs (context->domain == NULL
                     ? msgid
                     : dgettext (context->domain, msgid),
                     stdout);

              /* We separate the arguments by a single ' '.  */
              if (optind < argc)
                fputc (' ', stdout);
            }
          while (optind < argc);
        }

      /* If not otherwise told: add trailing newline.  */
      if (!context->inhibit_added_newline)
        fputc ('\n', stdout);
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
      /* xgettext: no-wrap */
      printf (_("\
Usage: %s [OPTION] [[TEXTDOMAIN] MSGID]\n\
or:    %s [OPTION] -s [MSGID]...\n\
"), program_name, program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Display native language translation of a textual message.\n"));
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
  -d, --domain=TEXTDOMAIN   retrieve translated messages from TEXTDOMAIN\n\
  -e                        enable expansion of some escape sequences\n\
  -E                        (ignored for compatibility)\n\
  -h, --help                display this help and exit\n\
  -n                        suppress trailing newline\n\
  -V, --version             display version information and exit\n\
  [TEXTDOMAIN] MSGID        retrieve translated message corresponding\n\
                            to MSGID from TEXTDOMAIN\n"));
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
If the TEXTDOMAIN parameter is not given, the domain is determined from the\n\
environment variable TEXTDOMAIN.  If the message catalog is not found in the\n\
regular directory, another location can be specified with the environment\n\
variable TEXTDOMAINDIR.\n\
When used with the -s option the program behaves like the 'echo' command.\n\
But it does not simply copy its arguments to stdout.  Instead those messages\n\
found in the selected catalog are translated.\n\
Standard search directory: %s\n"),
              getenv ("IN_HELP2MAN") == NULL ? LOCALEDIR : "@localedir@");
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
