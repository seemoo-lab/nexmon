/* t-spawn.c - Check the spawn functions.
 * Copyright (C) 2024 g10 Code GmbH
 *
 * This file is part of libgpg-error.
 *
 * libgpg-error is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * libgpg-error is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#define PGM "t-spawn"

#include "t-common.h"

#define MAXLINE 1024

static void
run_test (const char *progname)
{
  gpg_err_code_t rc;
  int r;
  gpgrt_spawn_actions_t act;
  gpgrt_process_t process;
  gpgrt_stream_t fp_out;
  const char *const envchange[] = {
    "ADD=0",              /* Add     */
    "REPLACE=1",          /* Replace */
    "GNUPGHOME",          /* Remove  */
    NULL,
  };
  const char *argv1[] = {
    "--child",
    NULL,
  };
  char line[MAXLINE];
  int ok_add, ok_replace, ok_remove;

  /* Make sure ADD is not defined in the parent process.  */
  rc = gpgrt_setenv ("ADD", NULL, 1);
  if (rc)
    fail ("gpgrt_setenv failed at %d: %s", __LINE__, gpg_strerror (rc));

  /* Set REPLACE and GNUPGHOME in the parent process.  */
  rc = gpgrt_setenv ("REPLACE", "0", 1);
  if (rc)
    fail ("gpgrt_setenv failed at %d: %s", __LINE__, gpg_strerror (rc));
  rc = gpgrt_setenv ("GNUPGHOME", "/tmp/test", 1);
  if (rc)
    fail ("gpgrt_setenv failed at %d: %s", __LINE__, gpg_strerror (rc));

  rc = gpgrt_spawn_actions_new (&act);
  if (rc)
    fail ("gpgrt_spawn_actions_new failed at %d: %s",
          __LINE__, gpg_strerror (rc));

  gpgrt_spawn_actions_set_env_rev (act, envchange);

  rc = gpgrt_process_spawn (progname, argv1,
                            (GPGRT_PROCESS_STDIN_KEEP
                             | GPGRT_PROCESS_STDOUT_PIPE
                             | GPGRT_PROCESS_STDERR_KEEP),
                            act, &process);
  if (rc)
    fail ("gpgrt_process_spawn failed at %d: %s", __LINE__, gpg_strerror (rc));

  gpgrt_spawn_actions_release (act);

  rc = gpgrt_process_get_streams (process, 0, NULL, &fp_out, NULL);
  if (rc)
    fail ("gpgrt_process_get_streams failed at %d: %s",
          __LINE__, gpg_strerror (rc));

  ok_add = ok_replace = 0;
  ok_remove = 1;

  while (gpgrt_fgets (line, DIM (line), fp_out))
    {
#ifdef HAVE_W32_SYSTEM
      { /* Take care of CRLF.  */
        char *p = strchr (line, '\r');
        if (p)
          *p = '\n';
      }
#endif
      if (!strncmp (line, "ADD=", 3))
        {
          if (!strncmp (line+4, "0\n", 2))
            ok_add = 1;
        }
      if (!strncmp (line, "REPLACE=", 8))
        {
          if (!strncmp (line+8, "1\n", 2))
            ok_replace = 1;
        }
      if (!strncmp (line, "GNUPGHOME=", 10))
        ok_remove = 0;
    }

  r = gpgrt_fclose (fp_out);
  if (r)
    fail ("gpgrt_fclose failed at %d: %d", __LINE__, r);

  rc = gpgrt_process_wait (process, 1);
  if (rc)
    fail ("gpgrt_process_wait failed at %d: %s", __LINE__, gpg_strerror (rc));

  gpgrt_process_release (process);

  if (verbose)
    show ("Result: add=%s, replace=%s, remove=%s\n",
          ok_add? "OK" : "FAIL", ok_replace? "OK" : "FAIL",
          ok_remove? "OK" : "FAIL");

  if (!ok_add)
    fail ("ADD failed");
  if (!ok_replace)
    fail ("REPLACE failed");
  if (!ok_remove)
    fail ("GNUPGHOME failed");
}


int
main (int argc, char **argv)
{
  int last_argc = -1;
  int child = 0;
  const char *progname = argv[0];

  if (argc)
    {
      argc--; argv++;
    }
  while (argc && last_argc != argc )
    {
      last_argc = argc;
      if (!strcmp (*argv, "--help"))
        {
          puts (
"usage: ./t-spawn [options]\n"
"\n"
"Options:\n"
"  --verbose      Show what is going on\n"
"  --debug        Flyswatter\n"
"  --child        Internal use for the child process\n"
);
          exit (0);
        }
      if (!strcmp (*argv, "--verbose"))
        {
          verbose = 1;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--debug"))
        {
          verbose = debug = 1;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--child"))
        {
          child = 1;
          argc--; argv++;
        }
    }

  if (!gpg_error_check_version (GPG_ERROR_VERSION))
    {
      die ("gpg_error_check_version returned an error");
      errorcount++;
    }

  if (child)
    {
      char *e;

      e = gpgrt_getenv ("ADD");
      if (e)
        {
          printf ("ADD=%s\n", e);
          gpgrt_free (e);
        }
      e = gpgrt_getenv ("REPLACE");
      if (e)
        {
          printf ("REPLACE=%s\n", e);
          gpgrt_free (e);
        }
      e = gpgrt_getenv ("GNUPGHOME");
      if (e)
        {
          printf ("GNUPGHOME=%s\n", e);
          gpgrt_free (e);
        }
      return 0;
    }

  run_test (progname);
  return errorcount ? 1 : 0;
}
