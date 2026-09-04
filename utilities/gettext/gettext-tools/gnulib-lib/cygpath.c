/* Convert file names between Cygwin syntax and Windows syntax.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

/* Specification.  */
#include "cygpath.h"

#include <stdlib.h>

#include "xalloc.h"
#include "gettext.h"

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)

#ifdef __CYGWIN__

/* Since Cygwin has its own notion of mount points (which can be defined by the
   user), it would be wrong to blindly convert '/cygdrive/c/foo' to 'C:\foo'.
   We really need to use the Cygwin API or the 'cygpath' program.  */

# include <errno.h>
# include <error.h>

# if 1

/* Documentation:
   https://cygwin.com/cygwin-api/func-cygwin-conv-path.html  */

#  include <sys/cygwin.h>

char *
cygpath_w (const char *filename)
{
  /* Try several times, since there is a small time window during which the
     size returned by the previous call may not be sufficient, namely when a
     directory gets renamed.  */
  for (int repeat = 3; repeat > 0; repeat--)
    {
      ssize_t size = cygwin_conv_path (CCP_POSIX_TO_WIN_A | CCP_RELATIVE, filename, NULL, 0);
      if (size < 0)
        error (EXIT_FAILURE, errno, _("cannot convert file name '%s' to Windows syntax"), filename);
      char *result = (char *) xmalloc (size);
      if (cygwin_conv_path (CCP_POSIX_TO_WIN_A | CCP_RELATIVE, filename, result, size) == 0)
        return result;
      free (result);
    }
  error (EXIT_FAILURE, errno, _("cygwin_conv_path failed"));
  return NULL;
}

# else
/* Alternative implementation (slower)  */

/* Documentation:
   https://cygwin.com/cygwin-ug-net/cygpath.html  */

#  include <stdio.h>

#  include "spawn-pipe.h"
#  include "wait-process.h"

/* Executes a program.
   Returns the first line of its output, as a freshly allocated string, or
   NULL.  */
static char *
execute_and_read_line (const char *progname,
                       const char *prog_path, const char * const *prog_argv)
{
  /* Open a pipe to the program.  */
  int fd[1];
  pid_t child = create_pipe_in (progname, prog_path, prog_argv, NULL, NULL,
                                DEV_NULL, false, true, false, fd);

  if (child == -1)
    return NULL;

  /* Retrieve its result.  */
  FILE *fp = fdopen (fd[0], "r");
  if (fp == NULL)
    error (EXIT_FAILURE, errno, _("fdopen() failed"));

  char *line = NULL;
  size_t linesize = 0;
  size_t linelen = getline (&line, &linesize, fp);
  if (linelen == (size_t)(-1))
    {
      error (0, 0, _("%s subprocess I/O error"), progname);
      fclose (fp);
      wait_subprocess (child, progname, true, false, true, false, NULL);
    }
  else
    {
      if (linelen > 0 && line[linelen - 1] == '\n')
        line[linelen - 1] = '\0';

      /* Read until EOF (otherwise the child process may get a SIGPIPE signal).  */
      while (getc (fp) != EOF)
        ;

      fclose (fp);

      /* Remove zombie process from process list, and retrieve exit status.  */
      int exitstatus =
        wait_subprocess (child, progname, true, false, true, false, NULL);
      if (exitstatus == 0)
        return line;
    }
  free (line);
  return NULL;
}

char *
cygpath_w (const char *filename)
{
  const char *argv[4];
  argv[0] = "cygpath";
  argv[1] = "-w";
  argv[2] = filename;
  argv[3] = NULL;

  char *line = execute_and_read_line ("cygpath", "cygpath", argv);
  if (line == NULL || line[0] == '\0')
    error (EXIT_FAILURE, 0, _("%s invocation failed"), "cygpath");
  return line;
}

# endif

#else

char *
cygpath_w (const char *filename)
{
  return xstrdup (filename);
}

#endif
