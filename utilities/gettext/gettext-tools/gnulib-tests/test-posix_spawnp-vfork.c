/* Test of posix_spawnp() function.
   Copyright (C) 2020-2026 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <spawn.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "macros.h"

/* Check an invocation via vfork() with a large $PATH (ca. 4 KB).
   When configured with gl_cv_func_posix_spawnp_secure_exec=no, this test used
   to crash on Linux/SPARC, Linux/ppc64, Linux/ppc64le, and Solaris/SPARC.  */

int
main ()
{
#if defined _WIN32 && !defined __CYGWIN__
  /* There is no 'false' program on this platform, but no vfork() either.  */
  fprintf (stderr, "Skipping test: no vfork() on this platform.\n");
  return 77;
#else

  /* Components of file names are limited to ca. 255 characters on most
     platforms.  (Cf. NAME_MAX in <limits.h>.)
     Here is one with 250 characters.  */
# define OOO "oooooooooooooooooooooooooooooooooooooooooooooooooo" \
             "oooooooooooooooooooooooooooooooooooooooooooooooooo" \
             "oooooooooooooooooooooooooooooooooooooooooooooooooo" \
             "oooooooooooooooooooooooooooooooooooooooooooooooooo" \
             "oooooooooooooooooooooooooooooooooooooooooooooooooo"
  /* File names are limited to ca. 1024 characters on many platforms,
     to ca. 4096 characters only on Linux and Cygwin, and unlimited only
     on GNU/Hurd.  (Cf. PATH_MAX in <limits.h>.)
     We prepend several directory names below this 1024 limit.  */
# define ADD "/usr/bin/p" OOO "/" OOO "/" OOO "/" OOO ":" \
             "/usr/bin/q" OOO "/" OOO "/" OOO "/" OOO ":" \
             "/usr/bin/r" OOO "/" OOO "/" OOO "/" OOO ":" \
             "/usr/bin/s" OOO "/" OOO "/" OOO "/" OOO ":"
  const char *old_path = getenv ("PATH");
  char *new_path = (char *) malloc (strlen (ADD) + strlen (old_path) + 1);
  ASSERT (new_path != NULL);
  memcpy (new_path, ADD, strlen (ADD));
  strcpy (new_path + strlen (ADD), old_path);
  ASSERT (setenv ("PATH", new_path, 1) == 0);

  posix_spawnattr_t attr;
  ASSERT (posix_spawnattr_init (&attr) == 0);
  ASSERT (posix_spawnattr_setflags (&attr, POSIX_SPAWN_USEVFORK) == 0);

  if (test_exit_status != EXIT_SUCCESS)
    return test_exit_status;

  /* Can't use 'false' here, because its exit status is only guaranteed to be
     non-zero, and is in fact 255 on Solaris.  */
  const char *prog = "sh";
  const char *prog_argv[4] = { prog, "-c", "exit 1", NULL };
  pid_t child;
  int err = posix_spawnp (&child, prog, NULL, &attr, (char **) prog_argv, environ);
  if (err != 0)
    {
      errno = err;
      perror ("posix_spawn");
      return 1;
    }

  /* Wait for child.  */
  int status = 0;
  while (waitpid (child, &status, 0) != child)
    ;
  if (!WIFEXITED (status))
    {
      fprintf (stderr, "subprocess terminated with unexpected wait status %d\n", status);
      return 1;
    }
  int exitstatus = WEXITSTATUS (status);
  if (exitstatus != 1)
    {
      fprintf (stderr, "subprocess terminated with unexpected exit status %d\n", exitstatus);
      return 1;
    }

  return test_exit_status;

#endif
}
