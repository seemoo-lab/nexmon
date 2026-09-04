/* Reading C# .resources files.
   Copyright (C) 2003-2026 Free Software Foundation, Inc.

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

/* Specification.  */
#include "read-resources.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <error.h>
#include "msgunfmt.h"
#include "relocatable.h"
#include "csharpexec.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "read-catalog.h"
#include "read-po.h"
#include "str-list.h"
#include "xerror-handler.h"
#include "message.h"
#include "concat-filename.h"
#include "cygpath.h"
#include "gettext.h"

#define _(str) gettext (str)


/* A .resources file has such a complex format that it's most easily read
   through the C# class ResourceReader.  So we start a C# process to execute
   the DumpResource program, and read its output, which is .po format without
   comments.  */

struct locals
{
  /* OUT */
  msgdomain_list_ty *mdlp;
};

static bool
execute_and_read_po_output (const char *progname,
                            const char *prog_path,
                            const char * const *prog_argv,
                            void *private_data)
{
  struct locals *l = (struct locals *) private_data;

  /* Open a pipe to the C# execution engine.  */
  int fd[1];
  pid_t child = create_pipe_in (progname, prog_path, prog_argv, NULL, NULL,
                                NULL, false, true, true, fd);

  FILE *fp = fdopen (fd[0], "r");
  if (fp == NULL)
    error (EXIT_FAILURE, errno, _("fdopen() failed"));

  /* Read the message list.  */
  string_list_ty arena;
  string_list_init (&arena);
  l->mdlp = read_catalog_stream (fp, "(pipe)", "(pipe)", &input_format_po,
                                 textmode_xerror_handler, &arena);

  fclose (fp);

  /* Remove zombie process from process list, and retrieve exit status.  */
  int exitstatus =
    wait_subprocess (child, progname, false, false, true, true, NULL);
  if (exitstatus != 0)
    error (EXIT_FAILURE, 0, _("%s subprocess failed with exit code %d"),
           progname, exitstatus);

  return false;
}


void
read_resources_file (message_list_ty *mlp, const char *filename)
{
  char *filename_converted = cygpath_w (filename);

  /* Prepare arguments.  */
  const char *args[2];
  args[0] = filename_converted;
  args[1] = NULL;

  /* Make it possible to override the .exe location.  This is
     necessary for running the testsuite before "make install".  */
  const char *gettextexedir = getenv ("GETTEXTCSHARPEXEDIR");
  if (gettextexedir == NULL || gettextexedir[0] == '\0')
    gettextexedir = relocate (LIBDIR "/gettext");

  /* Make it possible to override the .dll location.  This is
     necessary for running the testsuite before "make install".  */
  const char *gettextlibdir = getenv ("GETTEXTCSHARPLIBDIR");
  if (gettextlibdir == NULL || gettextlibdir[0] == '\0')
    gettextlibdir = relocate (LIBDIR);

  /* Dump the resource and retrieve the resulting output.  */
  char *assembly_path =
    xconcatenated_filename (gettextexedir, "msgunfmt.net", ".exe");
  const char *libdirs[1];
  libdirs[0] = gettextlibdir;
  struct locals locals;
  if (execute_csharp_program (assembly_path, libdirs, 1,
                              args,
                              verbose, false,
                              execute_and_read_po_output, &locals))
    /* An error message should already have been provided.  */
    exit (EXIT_FAILURE);

  /* Add the output to mlp.  */
  {
    message_list_ty *read_mlp = locals.mdlp->item[0]->messages;

    for (size_t j = 0; j < read_mlp->nitems; j++)
      message_list_append (mlp, read_mlp->item[j]);
  }

  free (assembly_path);
  free (filename_converted);
}
