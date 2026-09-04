/* Reading Java ResourceBundles.
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

/* Specification.  */
#include "read-java.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <error.h>
#include "msgunfmt.h"
#include "relocatable.h"
#include "javaexec.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "read-catalog.h"
#include "read-po.h"
#include "str-list.h"
#include "xerror-handler.h"
#include "gettext.h"

#define _(str) gettext (str)


/* A Java resource name can only be manipulated by a Java virtual machine.
   So we start a JVM to execute the DumpResource program, and read its
   output, which is .po format without comments.  */

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

  /* Open a pipe to the JVM.  */
  int fd[1];
  pid_t child = create_pipe_in (progname, prog_path, prog_argv, NULL, NULL,
                                DEV_NULL, false, true, true, fd);

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


msgdomain_list_ty *
msgdomain_read_java (const char *resource_name, const char *locale_name)
{
  const char *class_name = "gnu.gettext.DumpResource";

  /* Make it possible to override the gettext.jar location.  This is
     necessary for running the testsuite before "make install".  */
  const char *gettextjar = getenv ("GETTEXTJAR");
  if (gettextjar == NULL || gettextjar[0] == '\0')
    gettextjar = relocate (GETTEXTJAR);

  /* Assign a default value to the resource name.  */
  if (resource_name == NULL)
    resource_name = "Messages";

  /* Prepare arguments.  */
  const char *args[3];
  args[0] = resource_name;
  if (locale_name != NULL)
    {
      args[1] = locale_name;
      args[2] = NULL;
    }
  else
    args[1] = NULL;

  /* Dump the resource and retrieve the resulting output.
     Here we use the user's CLASSPATH, not a minimal one, so that the
     resource can be found.  */
  struct locals locals;
  if (execute_java_class (class_name, &gettextjar, 1, false, NULL,
                          args,
                          verbose, false,
                          execute_and_read_po_output, &locals))
    /* An error message should already have been provided.  */
    exit (EXIT_FAILURE);

  return locals.mdlp;
}
