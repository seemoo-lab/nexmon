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
#include "windows-cygpath.h"

#include "xalloc.h"
#include "gettext.h"

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)

#if defined _WIN32 && !defined __CYGWIN__

/* Since Cygwin has its own notion of mount points (which can be defined by the
   user), it would be wrong to blindly convert '/cygdrive/c/foo' to 'C:\foo'.
   We really need to use the Cygwin API or the 'cygpath' program.
   Since under native Windows, the Cygwin API is not available, we need to
   invoke the 'cygpath' program.  */

/* Documentation:
   https://cygwin.com/cygwin-ug-net/cygpath.html  */

# include <errno.h>
# include <error.h>
# include <stdio.h>
# include <stdlib.h>

# include "spawn-pipe.h"
# include "wait-process.h"

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
windows_cygpath_w (const char *filename)
{
  if (filename[0] == '/')
    {
      /* It's an absolute POSIX-style file name.  */
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
  else
    {
      /* It's a relative file name, or an absolute native Windows file name.
         All we need to do is to convert slashes to backslahes, e.g.
         'C:/Users' -> 'C:\Users'.  */
      size_t len = strlen (filename) + 1;
      char *copy = XNMALLOC (len, char);
      for (size_t i = 0; i < len; i++)
        copy[i] = (filename[i] == '/' ? '\\' : filename[i]);
      return copy;
    }
}

#else

char *
windows_cygpath_w (const char *filename)
{
  return xstrdup (filename);
}

#endif
