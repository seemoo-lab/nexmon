/* Reading tcl/msgcat .msg files.
   Copyright (C) 2002-2026 Free Software Foundation, Inc.

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
#include <alloca.h>

/* Specification.  */
#include "read-tcl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <error.h>
#include "msgunfmt.h"
#include "relocatable.h"
#include "concat-filename.h"
#include "sh-quote.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "read-catalog.h"
#include "read-po.h"
#include "str-list.h"
#include "xerror-handler.h"
#include "xmalloca.h"
#include "gettext.h"

#define _(str) gettext (str)


/* A Tcl .msg file contains Tcl commands.  It is best interpreted by Tcl
   itself.  But we redirect the msgcat::mcset function so that it passes
   the msgid/msgstr pair to us, instead of storing it in the hash table.  */

msgdomain_list_ty *
msgdomain_read_tcl (const char *locale_name, const char *directory)
{
  /* Make it possible to override the msgunfmt.tcl location.  This is
     necessary for running the testsuite before "make install".  */
  const char *gettextdatadir = getenv ("GETTEXTTCLDIR");
  if (gettextdatadir == NULL || gettextdatadir[0] == '\0')
    gettextdatadir = relocate (GETTEXTDATADIR);

  char *tclscript = xconcatenated_filename (gettextdatadir, "msgunfmt.tcl", NULL);

  /* Convert the locale name to lowercase and remove any encoding.  */
  size_t len = strlen (locale_name);
  char *frobbed_locale_name = (char *) xmalloca (len + 1);
  {
    memcpy (frobbed_locale_name, locale_name, len + 1);
    for (char *p = frobbed_locale_name; *p != '\0'; p++)
      if (*p >= 'A' && *p <= 'Z')
        *p = *p - 'A' + 'a';
      else if (*p == '.')
        {
          *p = '\0';
          break;
        }
  }

  char *file_name = xconcatenated_filename (directory, frobbed_locale_name, ".msg");

  freea (frobbed_locale_name);

  /* Prepare arguments.  */
  const char *argv[4];
  argv[0] = "tclsh";
  argv[1] = tclscript;
  argv[2] = file_name;
  argv[3] = NULL;

  if (verbose)
    {
      char *command = shell_quote_argv (argv);
      printf ("%s\n", command);
      free (command);
    }

  /* Open a pipe to the Tcl interpreter.  */
  int fd[1];
  pid_t child = create_pipe_in ("tclsh", "tclsh", argv, NULL, NULL,
                                DEV_NULL, false, true, true, fd);

  FILE *fp = fdopen (fd[0], "r");
  if (fp == NULL)
    error (EXIT_FAILURE, errno, _("fdopen() failed"));

  /* Read the message list.  */
  string_list_ty arena;
  string_list_init (&arena);
  msgdomain_list_ty *mdlp =
    read_catalog_stream (fp, "(pipe)", "(pipe)", &input_format_po,
                         textmode_xerror_handler, &arena);

  fclose (fp);

  /* Remove zombie process from process list, and retrieve exit status.  */
  int exitstatus =
    wait_subprocess (child, "tclsh", false, false, true, true, NULL);
  if (exitstatus != 0)
    {
      if (exitstatus == 2)
        /* Special exitcode provided by msgunfmt.tcl.  */
        error (EXIT_FAILURE, ENOENT,
               _("error while opening \"%s\" for reading"), file_name);
      else
        error (EXIT_FAILURE, 0, _("%s subprocess failed with exit code %d"),
               "tclsh", exitstatus);
    }

  free (tclscript);

  /* Move the header entry to the beginning.  */
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      for (size_t j = 0; j < mlp->nitems; j++)
        if (is_header (mlp->item[j]))
          {
            /* Found the header entry.  */
            if (j > 0)
              {
                message_ty *header = mlp->item[j];

                for (size_t i = j; i > 0; i--)
                  mlp->item[i] = mlp->item[i - 1];
                mlp->item[0] = header;
              }
            break;
          }
    }

  return mdlp;
}
