/* Return the version-control based modification time of a file.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

#include <config.h>

/* Specification.  */
#include "vc-mtime.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <error.h>
#include "spawn-pipe.h"
#include "wait-process.h"
#include "execute.h"
#include "safe-read.h"
#include "xstrtol.h"
#include "stat-time.h"
#include "windows-cygpath.h"
#include "filename.h"
#include "xalloc.h"
#include "xgetcwd.h"
#include "xvasprintf.h"
#include "gl_map.h"
#include "gl_xmap.h"
#include "gl_hash_map.h"
#include "hashkey-string.h"
#if USE_UNLOCKED_IO
# include "unlocked-io.h"
#endif
#include "gettext.h"

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)


/* ========================================================================== */

static const char *git_version;

/* Determines whether git is present, and sets git_version if so.  */
static bool
is_git_present (void)
{
  static bool git_tested;
  static bool git_present;

  if (!git_tested)
    {
      /* Test for presence of git:
         "git --version 2>/dev/null"  */
      const char *argv[3];
      argv[0] = "git";
      argv[1] = "--version";
      argv[2] = NULL;

      int fd[1];
      pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                    DEV_NULL, true, true, false, fd);
      if (child == -1)
        git_present = false;
      else
        {
          /* Retrieve its result.  */
          FILE *fp = fdopen (fd[0], "r");
          if (fp == NULL)
            error (EXIT_FAILURE, errno, _("fdopen() failed"));

          char *line = NULL;
          size_t linesize = 0;
          size_t linelen = getline (&line, &linesize, fp);
          if (linelen == (size_t)(-1))
            {
              fclose (fp);
              wait_subprocess (child, "git", true, true, true, false, NULL);
              git_present = false;
            }
          else
            {
              if (linelen > 0 && line[linelen - 1] == '\n')
                line[linelen - 1] = '\0';

              /* Read until EOF (otherwise the child process may get a SIGPIPE
                 signal).  */
              while (getc (fp) != EOF)
                ;

              fclose (fp);

              /* Remove zombie process from process list, and retrieve exit
                 status.  */
              int exitstatus =
                wait_subprocess (child, "git", true, true, true, false, NULL);
              if (exitstatus != 0)
                {
                  free (line);
                  git_present = false;
                }
              else
                {
                  /* The version starts at the first digit in the line.  */
                  const char *p = line;
                  for (; *p != '0'; p++)
                    if (*p >= '0' && *p <= '9')
                      break;
                  git_version = p;
                  git_present = true;
                }
            }
        }
      git_tested = true;
    }

  return git_present;
}

/* Determines whether the specified file is under version control.  */
static bool
git_vc_controlled (const char *filename)
{
  /* Run "git ls-files FILENAME" and return true if the exit code is 0
     and the output is non-empty.  */
  const char *argv[4];
  argv[0] = "git";
  argv[1] = "ls-files";
  argv[2] = filename;
  argv[3] = NULL;

  int fd[1];
  pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                DEV_NULL, true, true, false, fd);
  if (child == -1)
    return false;

  /* Read the subprocess output, and test whether it is non-empty.  */
  ptrdiff_t count = 0;

  for (;;)
    {
      char buf[1024];
      ptrdiff_t n = safe_read (fd[0], buf, sizeof (buf));
      if (n > 0)
        count += n;
      else
        break;
    }

  close (fd[0]);

  /* Remove zombie process from process list, and retrieve exit status.  */
  int exitstatus =
    wait_subprocess (child, "git", false, true, true, false, NULL);
  return (exitstatus == 0 && count > 0);
}

/* Determines whether the specified file is unmodified, compared to the
   last version in version control.  */
static bool
git_unmodified (const char *filename)
{
  /* Run "git diff --quiet -- HEAD FILENAME"
     (or "git diff --quiet HEAD FILENAME")
     and return true if the exit code is 0.
     The '--' option is for the case that the specified file was removed.  */
  const char *argv[7];
  argv[0] = "git";
  argv[1] = "diff";
  argv[2] = "--quiet";
  argv[3] = "--";
  argv[4] = "HEAD";
  argv[5] = filename;
  argv[6] = NULL;

  int exitstatus = execute ("git", "git", argv, NULL, NULL,
                            false, false, true, true,
                            true, false, NULL);
  return (exitstatus == 0);
}

/* Stores in *MTIME the time of last modification in version control of the
   specified file, and returns 0.
   Upon failure, it returns -1.  */
static int
git_mtime (struct timespec *mtime, const char *filename)
{
  /* Run "git log -1 --format=%ct -- FILENAME".  It prints the time of last
     modification (the 'CommitDate', not the 'AuthorDate' which merely
     represents the time at which the author locally committed the first version
     of the change), as the number of seconds since the Epoch.
     The '--' option is for the case that the specified file was removed.  */
  const char *argv[7];
  argv[0] = "git";
  argv[1] = "log";
  argv[2] = "-1";
  argv[3] = "--format=%ct";
  argv[4] = "--";
  argv[5] = filename;
  argv[6] = NULL;

  int fd[1];
  pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                DEV_NULL, true, true, false, fd);
  if (child == -1)
    return -1;

  /* Retrieve its result.  */
  FILE *fp = fdopen (fd[0], "r");
  if (fp == NULL)
    error (EXIT_FAILURE, errno, _("fdopen() failed"));

  char *line = NULL;
  size_t linesize = 0;
  size_t linelen = getline (&line, &linesize, fp);
  if (linelen == (size_t)(-1))
    {
      error (0, 0, _("%s subprocess I/O error"), "git");
      fclose (fp);
      wait_subprocess (child, "git", true, false, true, false, NULL);
    }
  else
    {
      if (linelen > 0 && line[linelen - 1] == '\n')
        line[linelen - 1] = '\0';

      fclose (fp);

      /* Remove zombie process from process list, and retrieve exit status.  */
      int exitstatus =
        wait_subprocess (child, "git", true, false, true, false, NULL);
      if (exitstatus == 0)
        {
          char *endptr;
          unsigned long git_log_time;
          if (xstrtoul (line, &endptr, 10, &git_log_time, NULL) == LONGINT_OK
              && endptr == line + strlen (line))
            {
              mtime->tv_sec = git_log_time;
              mtime->tv_nsec = 0;
              free (line);
              return 0;
            }
        }
    }
  free (line);
  return -1;
}

int
vc_mtime (struct timespec *mtime, const char *filename)
{
  if (is_git_present ()
      && git_vc_controlled (filename)
      && git_unmodified (filename))
    {
      if (git_mtime (mtime, filename) == 0)
        return 0;
    }
  struct stat statbuf;
  if (stat (filename, &statbuf) == 0)
    {
      *mtime = get_stat_mtime (&statbuf);
      return 0;
    }
  return -1;
}

/* ========================================================================== */

/* Maximum length of a command that is guaranteed to work.  */
#if defined _WIN32 || defined __CYGWIN__
/* Windows */
# define MAX_COMMAND_LENGTH 8192
#else
/* Unix platforms */
# define MAX_COMMAND_LENGTH 32768
#endif
/* Keep some safe distance to this maximum.  */
#define MAX_CMD_LEN ((int) (MAX_COMMAND_LENGTH * 0.8))

/* The directory separator in canonicalized file names.  */
#if defined _WIN32 && !defined __CYGWIN__
# define SLASH '\\'
#else
# define SLASH '/'
#endif

/* Returns the directory name of the git checkout that contains tha current
   directory, as an absolute file name, or NULL if the current directory is
   not in a git checkout.  */
static char *
abs_git_checkout (void)
{
  /* Run "git rev-parse --show-toplevel 2>/dev/null" and return its output,
     without the trailing newline.  */
  const char *argv[4];
  argv[0] = "git";
  argv[1] = "rev-parse";
  argv[2] = "--show-toplevel";
  argv[3] = NULL;

  int fd[1];
  pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                DEV_NULL, true, true, false, fd);

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
      fclose (fp);
      wait_subprocess (child, "git", true, true, true, false, NULL);
      return NULL;
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
        wait_subprocess (child, "git", true, true, true, false, NULL);
      if (exitstatus == 0)
        {
          char *line_converted = windows_cygpath_w (line);
          free (line);
          return line_converted;
        }
    }
  free (line);
  return NULL;
}

/* Given an absolute canonicalized directory DIR1 and an absolute canonicalized
   directory DIR2, returns N where DIR1 = DIR2 "/.." ... "/.." with N times
   "/..", or -1 if DIR1 is not an ancestor directory of DIR2.  */
static long
ancestor_level (const char *dir1, const char *dir2)
{
  if (streq (dir1, "/"))
    dir1 = "";
  if (streq (dir2, "/"))
    dir2 = "";
  size_t dir1_len = strlen (dir1);
  if (strncmp (dir1, dir2, dir1_len) == 0)
    {
      /* DIR2 starts with DIR1.  */
      const char *p = dir2 + dir1_len;
      if (*p == '\0')
        /* DIR2 and DIR1 are the same.  */
        return 0;
      if (ISSLASH (*p))
        {
          /* Return the number of slashes in the tail of DIR2 that starts
             at P.  */
          long n = 1;
          p++;
          for (; *p != '\0'; p++)
            if (ISSLASH (*p))
              n++;
          return n;
        }
    }
  return -1;
}

/* Given an absolute canolicalized FILENAME that starts with DIR1, returns the
   same file name relative to DIR2, where DIR1 = DIR2 "/.." ... "/.." with
   N times "/..", as a freshly allocated string.  */
static char *
relativize (const char *filename,
            unsigned long n, const char *dir1, const char *dir2)
{
  if (streq (dir1, "/"))
    dir1 = "";
  size_t dir1_len = strlen (dir1);
  if (!(strncmp (filename, dir1, dir1_len) == 0
        && (filename[dir1_len] == '\0' || ISSLASH (filename[dir1_len]))))
    /* Invalid argument.  */
    abort ();
  if (streq (dir2, "/"))
    dir2 = "";

  dir2 += dir1_len;
  filename += dir1_len;
  for (;;)
    {
      /* Invariant: The result will be N times "../" followed by FILENAME.  */
      if (*filename == '\0')
        break;
      if (!ISSLASH (*filename))
        abort ();
      filename++;
      if (*dir2 == '\0')
        break;
      if (!ISSLASH (*dir2))
        abort ();
      dir2++;
      /* Skip one component in DIR2.  */
      const char *dir2_s;
      for (dir2_s = dir2; *dir2_s != '\0'; dir2_s++)
        if (ISSLASH (*dir2_s))
          break;
      /* Skip one component in FILENAME, at P.  */
      const char *filename_s;
      for (filename_s = filename; *filename_s != '\0'; filename_s++)
        if (ISSLASH (*filename_s))
          break;
      /* Did the components match?  */
      if (!(filename_s - filename == dir2_s - dir2
            && memeq (filename, dir2, dir2_s - dir2)))
        break;
      dir2 = dir2_s;
      filename = filename_s;
      n--;
    }

  if (n == 0 && *filename == '\0')
    return xstrdup (".");

  char *result = (char *) xmalloc (3 * n + strlen (filename) + 1);
  {
    char *q = result;
    for (; n > 0; n--)
      {
        q[0] = '.'; q[1] = '.'; q[2] = '/'; q += 3;
      }
    strcpy (q, filename);
  }
  return result;
}

/* Accumulating mtimes.  */
struct accumulator
{
  bool has_some_mtimes;
  struct timespec max_of_mtimes;
};

static void
accumulate (struct accumulator *accu, struct timespec mtime)
{
  if (accu->has_some_mtimes)
    {
      /* Compute the maximum of accu->max_of_mtimes and mtime.  */
      if (accu->max_of_mtimes.tv_sec < mtime.tv_sec
          || (accu->max_of_mtimes.tv_sec == mtime.tv_sec
              && accu->max_of_mtimes.tv_nsec < mtime.tv_nsec))
       accu->max_of_mtimes = mtime;
    }
  else
    {
      accu->max_of_mtimes = mtime;
      accu->has_some_mtimes = true;
    }
}

int
max_vc_mtime (struct timespec *max_of_mtimes,
              size_t nfiles, const char * const *filenames)
{
  if (nfiles == 0)
    /* Invalid argument.  */
    abort ();

  struct accumulator accu = { false };

  /* Determine which of the specified files are under version control,
     and which are duplicates.  (The case of duplicates is rare, but it needs
     special attention, because 'git ls-files' eliminates duplicates.)
     vc_controlled[n] = 1 means that filenames[n] is under version control.
     vc_controlled[n] = 0 means that filenames[n] is not under version control.
     vc_controlled[n] = -1 means that filenames[n] is a duplicate.  */
  signed char *vc_controlled = XNMALLOC (nfiles, signed char);
  for (size_t n = 0; n < nfiles; n++)
    vc_controlled[n] = 0;

  if (is_git_present ())
    {
      /* Since 'git ls-files' produces an error when at least one of the files
         is outside the git checkout that contains tha current directory, we
         need to filter out such files.  This is most easily done by converting
         each file name to a canonical file name first and then comparing with
         the directory name of said git checkout.  */
      char *git_checkout = abs_git_checkout ();
      if (git_checkout != NULL)
        {
          char *currdir = xgetcwd ();
          if (currdir != NULL)
            {
              /* git_checkout is expected to be an ancestor directory of the
                 current directory.  */
              long ancestor = ancestor_level (git_checkout, currdir);
              if (ancestor >= 0)
                {
                  char **canonical_filenames = XNMALLOC (nfiles, char *);
                  for (size_t n = 0; n < nfiles; n++)
                    {
                      char *canonical = canonicalize_file_name (filenames[n]);
                      if (canonical == NULL)
                        {
                          if (errno == ENOMEM)
                            xalloc_die ();
                          /* The file filenames[n] does not exist.  */
                          for (size_t k = n; k > 0; )
                            free (canonical_filenames[--k]);
                          free (canonical_filenames);
                          free (currdir);
                          free (git_checkout);
                          free (vc_controlled);
                          return -1;
                        }
                      canonical_filenames[n] = canonical;
                    }

                  /* Test which of these absolute file names are outside of the
                     git_checkout.  */
                  char *git_checkout_slash =
                    (streq (git_checkout, "/")
                     ? xstrdup (git_checkout)
                     : xasprintf ("%s%c", git_checkout, SLASH));

                  char **checkout_relative_filenames = XNMALLOC (nfiles, char *);
                  char **currdir_relative_filenames = XNMALLOC (nfiles, char *);
                  for (size_t n = 0; n < nfiles; n++)
                    {
                      if (str_startswith (canonical_filenames[n], git_checkout_slash))
                        {
                          vc_controlled[n] = 1;
                          checkout_relative_filenames[n] =
                            relativize (canonical_filenames[n],
                                        0, git_checkout, git_checkout);
                          currdir_relative_filenames[n] =
                            relativize (canonical_filenames[n],
                                        ancestor, git_checkout, currdir);
                        }
                      else
                        {
                          vc_controlled[n] = 0;
                          checkout_relative_filenames[n] = NULL;
                          currdir_relative_filenames[n] = NULL;
                        }
                    }

                  /* Room for passing arguments to git commands.  */
                  const char **argv = XNMALLOC (7 + nfiles + 1, const char *);

                  {
                    /* Put the relative file names into a hash table.  This is needed
                       because 'git ls-files' returns the files in a different order
                       than the one we provide in the command.  */
                    gl_map_t relative_filenames_ht =
                      gl_map_create_empty (GL_HASH_MAP,
                                           hashkey_string_equals, hashkey_string_hash,
                                           NULL, NULL);
                    for (size_t n = 0; n < nfiles; n++)
                      if (currdir_relative_filenames[n] != NULL)
                        {
                          if (gl_map_get (relative_filenames_ht, currdir_relative_filenames[n]) != NULL)
                            {
                              /* It's already in the table.  */
                              vc_controlled[n] = -1;
                            }
                          else
                            gl_map_put (relative_filenames_ht, currdir_relative_filenames[n], &vc_controlled[n]);
                        }

                    /* Run "git ls-files -c -o -t -z FILE1..." for as many files as
                       possible, and inspect the output.  */
                    size_t n0 = 0;
                    do
                      {
                        size_t i = 0;
                        argv[i++] = "git";
                        argv[i++] = "ls-files";
                        argv[i++] = "-c";
                        argv[i++] = "-o";
                        argv[i++] = "-t";
                        argv[i++] = "-z";
                        size_t i0 = i;

                        size_t n = n0;
                        size_t cmd_len = 25;
                        for (; n < nfiles; n++)
                          if (vc_controlled[n] == 1)
                            {
                              if (cmd_len + strlen (currdir_relative_filenames[n]) >= MAX_CMD_LEN
                                  && i > i0)
                                break;
                              argv[i++] = currdir_relative_filenames[n];
                              cmd_len += 1 + strlen (currdir_relative_filenames[n]);
                            }
                        if (i > i0)
                          {
                            argv[i] = NULL;
                            int fd[1];
                            pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                                          DEV_NULL, true, true, false, fd);
                            if (child == -1)
                              break;

                            /* Read the subprocess output.  It is expected to be of the form
                                 T1 <space> <currdir_relative_filename1> NUL
                                 T2 <space> <currdir_relative_filename2> NUL
                                 ...
                               where the relative filenames correspond to the given file
                               names (because we have already relativized them).  */
                            FILE *fp = fdopen (fd[0], "r");
                            if (fp == NULL)
                              error (EXIT_FAILURE, errno, _("fdopen() failed"));

                            char *fn = NULL;
                            size_t fn_size = 0;
                            for (;;)
                              {
                                int status = fgetc (fp);
                                if (status == EOF)
                                  break;
                                /* status is a status tag, as documented in
                                   "man git-ls-files".  */

                                int space = fgetc (fp);
                                if (space != ' ')
                                  {
                                    fprintf (stderr, "vc-mtime: git ls-files output not as expected\n");
                                    break;
                                  }

                                if (getdelim (&fn, &fn_size, '\0', fp) == -1)
                                  {
                                    if (errno == ENOMEM)
                                      xalloc_die ();
                                    fprintf (stderr, "vc-mtime: failed to read git ls-files output\n");
                                    break;
                                  }
                                signed char *vc_controlled_p =
                                  (signed char *) gl_map_get (relative_filenames_ht, fn);
                                if (vc_controlled_p == NULL)
                                  fprintf (stderr, "vc-mtime: git ls-files returned an unexpected file name: %s\n", fn);
                                else
                                  *vc_controlled_p = (status == 'H' ? 1 : 0);
                              }

                            free (fn);
                            fclose (fp);

                            /* Remove zombie process from process list, and retrieve exit status.  */
                            int exitstatus =
                              wait_subprocess (child, "git", false, true, true, false, NULL);
                            if (exitstatus != 0)
                              fprintf (stderr, "vc-mtime: git ls-files failed with exit code %d\n", exitstatus);
                          }
                        n0 = n;
                      }
                    while (n0 < nfiles);

                    gl_map_free (relative_filenames_ht);
                  }

                  {
                    /* Put the relative file names into a hash table.  This is needed
                       because 'git diff' returns the files in a different order
                       than the one we provide in the command.  */
                    gl_map_t relative_filenames_ht =
                      gl_map_create_empty (GL_HASH_MAP,
                                           hashkey_string_equals, hashkey_string_hash,
                                           NULL, NULL);
                    for (size_t n = 0; n < nfiles; n++)
                      if (vc_controlled[n] == 1)
                        {
                          /* No need to test for duplicates here.  We have already set
                             vc_controlled[n] to -1 for duplicates, above.  */
                          gl_map_put (relative_filenames_ht, checkout_relative_filenames[n], &vc_controlled[n]);
                        }

                    /* Run "git diff --name-only --no-relative -z HEAD -- FILE1..." for
                       as many files as possible, and inspect the output.  */
                    size_t n0 = 0;
                    do
                      {
                        size_t i = 0;
                        argv[i++] = "git";
                        argv[i++] = "diff";
                        argv[i++] = "--name-only";
                        /* With git versions >= 2.28, we pass option --no-relative,
                           in order to neutralize any possible customization of the
                           "diff.relative" property.  With git versions < 2.28, this
                           is not needed, and the option --no-relative does not
                           exist.  */
                        if (!(git_version[0] <= '1'
                              || (git_version[0] == '2' && git_version[1] == '.'
                                  && ((git_version[2] >= '0' && git_version[2] <= '9'
                                       && !(git_version[3] >= '0' && git_version[3] <= '9'))
                                      || (((git_version[2] == '1'
                                            && git_version[3] >= '0' && git_version[3] <= '9')
                                           || (git_version[2] == '2'
                                               && git_version[3] >= '0' && git_version[3] <= '7'))
                                          && !(git_version[4] >= '0' && git_version[4] <= '9'))))))
                          argv[i++] = "--no-relative";
                        argv[i++] = "-z";
                        argv[i++] = "HEAD";
                        argv[i++] = "--";
                        size_t i0 = i;

                        size_t n = n0;
                        size_t cmd_len = 46;
                        for (; n < nfiles; n++)
                          if (vc_controlled[n] == 1)
                            {
                              if (cmd_len + strlen (currdir_relative_filenames[n]) >= MAX_CMD_LEN
                                  && i > i0)
                                break;
                              argv[i++] = currdir_relative_filenames[n];
                              cmd_len += 1 + strlen (currdir_relative_filenames[n]);
                            }
                        if (i > i0)
                          {
                            argv[i] = NULL;
                            int fd[1];
                            pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                                          DEV_NULL, true, true, false, fd);
                            if (child == -1)
                              break;

                            /* Read the subprocess output.  It is expected to be of the form
                                 <checkout_relative_filename1> NUL
                                 <checkout_relative_filename2> NUL
                                 ...
                               where the relative filenames are relative to the git
                               checkout dir, not to currdir!  */
                            FILE *fp = fdopen (fd[0], "r");
                            if (fp == NULL)
                              error (EXIT_FAILURE, errno, _("fdopen() failed"));

                            char *fn = NULL;
                            size_t fn_size = 0;
                            for (;;)
                              {
                                /* Test for EOF.  */
                                int c = fgetc (fp);
                                if (c == EOF)
                                  break;
                                ungetc (c, fp);

                                if (getdelim (&fn, &fn_size, '\0', fp) == -1)
                                  {
                                    if (errno == ENOMEM)
                                      xalloc_die ();
                                    fprintf (stderr, "vc-mtime: failed to read git diff output\n");
                                    break;
                                  }
                                signed char *vc_controlled_p =
                                  (signed char *) gl_map_get (relative_filenames_ht, fn);
                                if (vc_controlled_p == NULL)
                                  fprintf (stderr, "vc-mtime: git diff returned an unexpected file name: %s\n", fn);
                                else
                                  /* filenames[n] is under version control but is modified.
                                     Treat it like a file not under version control.  */
                                  *vc_controlled_p = 0;
                              }

                            free (fn);
                            fclose (fp);

                            /* Remove zombie process from process list, and retrieve exit status.  */
                            int exitstatus =
                              wait_subprocess (child, "git", false, true, true, false, NULL);
                            if (exitstatus != 0)
                              fprintf (stderr, "vc-mtime: git diff failed with exit code %d\n", exitstatus);
                          }
                        n0 = n;
                      }
                    while (n0 < nfiles);

                    gl_map_free (relative_filenames_ht);
                  }

                  {
                    /* Run "git log -1 --format=%ct -- FILE1...".  It prints the
                       time of last modification (the 'CommitDate', not the
                       'AuthorDate' which merely represents the time at which the
                       author locally committed the first version of the change),
                       as the number of seconds since the Epoch.  The '--' option
                       is for the case that the specified file was removed.  */
                    size_t n0 = 0;
                    do
                      {
                        size_t i = 0;
                        argv[i++] = "git";
                        argv[i++] = "log";
                        argv[i++] = "-1";
                        argv[i++] = "--format=%ct";
                        argv[i++] = "--";
                        size_t i0 = i;

                        size_t n = n0;
                        size_t cmd_len = 27;
                        for (; n < nfiles; n++)
                          if (vc_controlled[n] == 1)
                            {
                              if (cmd_len + strlen (currdir_relative_filenames[n]) >= MAX_CMD_LEN
                                  && i > i0)
                                break;
                              argv[i++] = currdir_relative_filenames[n];
                              cmd_len += 1 + strlen (currdir_relative_filenames[n]);
                            }
                        if (i > i0)
                          {
                            argv[i] = NULL;
                            int fd[1];
                            pid_t child = create_pipe_in ("git", "git", argv, NULL, NULL,
                                                          DEV_NULL, true, true, false, fd);
                            if (child == -1)
                              break;

                            /* Read the subprocess output.  It is expected to be a
                               single line, containing a positive integer.  */
                            FILE *fp = fdopen (fd[0], "r");
                            if (fp == NULL)
                              error (EXIT_FAILURE, errno, _("fdopen() failed"));

                            char *line = NULL;
                            size_t linesize = 0;
                            size_t linelen = getline (&line, &linesize, fp);
                            if (linelen == (size_t)(-1))
                              {
                                if (errno == ENOMEM)
                                  xalloc_die ();
                                fprintf (stderr, "vc-mtime: failed to read git log output\n");
                               git_log_fail1:
                                free (line);
                                fclose (fp);
                                wait_subprocess (child, "git", true, false, true, false, NULL);
                               git_log_fail2:
                                free (argv);
                                for (size_t k = nfiles; k > 0; )
                                  free (currdir_relative_filenames[--k]);
                                free (currdir_relative_filenames);
                                for (size_t k = nfiles; k > 0; )
                                  free (checkout_relative_filenames[--k]);
                                free (checkout_relative_filenames);
                                free (git_checkout_slash);
                                for (size_t k = nfiles; k > 0; )
                                  free (canonical_filenames[--k]);
                                free (canonical_filenames);
                                free (currdir);
                                free (git_checkout);
                                free (vc_controlled);
                                return -1;
                              }
                            if (linelen > 0 && line[linelen - 1] == '\n')
                              line[linelen - 1] = '\0';

                            char *endptr;
                            unsigned long git_log_time;
                            if (!(xstrtoul (line, &endptr, 10, &git_log_time, NULL) == LONGINT_OK
                                  && endptr == line + strlen (line)))
                              {
                                fprintf (stderr, "vc-mtime: git log output not as expected\n");
                                goto git_log_fail1;
                              }

                            struct timespec mtime;
                            mtime.tv_sec = git_log_time;
                            mtime.tv_nsec = 0;
                            accumulate (&accu, mtime);

                            free (line);
                            fclose (fp);

                            /* Remove zombie process from process list, and retrieve exit status.  */
                            int exitstatus =
                              wait_subprocess (child, "git", false, true, true, false, NULL);
                            if (exitstatus != 0)
                              {
                                fprintf (stderr, "vc-mtime: git log failed with exit code %d\n", exitstatus);
                                goto git_log_fail2;
                              }
                          }
                        n0 = n;
                      }
                    while (n0 < nfiles);
                  }

                  free (argv);
                  for (size_t k = nfiles; k > 0; )
                    free (currdir_relative_filenames[--k]);
                  free (currdir_relative_filenames);
                  for (size_t k = nfiles; k > 0; )
                    free (checkout_relative_filenames[--k]);
                  free (checkout_relative_filenames);
                  free (git_checkout_slash);
                  for (size_t k = nfiles; k > 0; )
                    free (canonical_filenames[--k]);
                  free (canonical_filenames);
                }
              free (currdir);
            }
          free (git_checkout);
        }
    }

  /* For the files that are not under version control, or that are modified
     compared to HEAD, use the file's time stamp.  */
  for (size_t n = 0; n < nfiles; n++)
    if (vc_controlled[n] == 0)
      {
        struct stat statbuf;
        if (stat (filenames[n], &statbuf) < 0)
          {
            free (vc_controlled);
            return -1;
          }

        struct timespec mtime = get_stat_mtime (&statbuf);
        accumulate (&accu, mtime);
      }

  free (vc_controlled);

  /* Since nfiles > 0, we must have accumulated at least one mtime.  */
  if (!accu.has_some_mtimes)
    abort ();
  *max_of_mtimes = accu.max_of_mtimes;
  return 0;
}

/* ========================================================================== */

#ifdef TEST

#include <assert.h>
#include <stdio.h>
#include <time.h>

/* Some unit tests for internal functions.  */

static void
test_ancestor_level (void)
{
  assert (ancestor_level ("/home/user/projects/gnulib", "/home/user/projects/gnulib") == 0);
  assert (ancestor_level ("/", "/") == 0);

  assert (ancestor_level ("/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto") == 2);
  assert (ancestor_level ("/", "/home/user") == 2);

  assert (ancestor_level ("/home/user/.local", "/home/user/projects/gnulib") == -1);
  assert (ancestor_level ("/.local", "/home/user") == -1);
  assert (ancestor_level ("/.local", "/") == -1);
}

static void
test_relativize (void)
{
  assert (streq (relativize ("/home/user/projects/gnulib",
                             0, "/home/user/projects/gnulib", "/home/user/projects/gnulib"),
                 "."));
  assert (streq (relativize ("/home/user/projects/gnulib/NEWS",
                             0, "/home/user/projects/gnulib", "/home/user/projects/gnulib"),
                 "NEWS"));
  assert (streq (relativize ("/home/user/projects/gnulib/doc/Makefile",
                             0, "/home/user/projects/gnulib", "/home/user/projects/gnulib"),
                 "doc/Makefile"));

  assert (streq (relativize ("/",
                             0, "/", "/"),
                 "."));
  assert (streq (relativize ("/swapfile",
                             0, "/", "/"),
                 "swapfile"));
  assert (streq (relativize ("/etc/passwd",
                             0, "/", "/"),
                 "etc/passwd"));

  assert (streq (relativize ("/home/user/projects/gnulib",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../../"));
  assert (streq (relativize ("/home/user/projects/gnulib/lib",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../"));
  assert (streq (relativize ("/home/user/projects/gnulib/lib/crypto",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "."));
  assert (streq (relativize ("/home/user/projects/gnulib/lib/malloc",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../malloc"));
  assert (streq (relativize ("/home/user/projects/gnulib/lib/cr",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../cr"));
  assert (streq (relativize ("/home/user/projects/gnulib/lib/cryptography",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../cryptography"));
  assert (streq (relativize ("/home/user/projects/gnulib/doc",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../../doc"));
  assert (streq (relativize ("/home/user/projects/gnulib/doc/Makefile",
                             2, "/home/user/projects/gnulib", "/home/user/projects/gnulib/lib/crypto"),
                 "../../doc/Makefile"));

  assert (streq (relativize ("/",
                             2, "/", "/home/user"),
                 "../../"));
  assert (streq (relativize ("/home",
                             2, "/", "/home/user"),
                 "../"));
  assert (streq (relativize ("/home/user",
                             2, "/", "/home/user"),
                 "."));
  assert (streq (relativize ("/home/root",
                             2, "/", "/home/user"),
                 "../root"));
  assert (streq (relativize ("/home/us",
                             2, "/", "/home/user"),
                 "../us"));
  assert (streq (relativize ("/home/users",
                             2, "/", "/home/user"),
                 "../users"));
  assert (streq (relativize ("/etc",
                             2, "/", "/home/user"),
                 "../../etc"));
  assert (streq (relativize ("/etc/passwd",
                             2, "/", "/home/user"),
                 "../../etc/passwd"));
}

/* Usage: ./a.out FILE[...]
 */
int
main (int argc, char *argv[])
{
  test_ancestor_level ();
  test_relativize ();

  if (argc == 1)
    {
      fprintf (stderr, "Usage: ./a.out FILE[...]\n");
      return 1;
    }
  struct timespec mtime;
  int ret = max_vc_mtime (&mtime, argc - 1, (const char **) argv + 1);
  if (ret == 0)
    {
      time_t t = mtime.tv_sec;
      struct tm *gmt = gmtime (&t);
      printf ("mtime = %04d-%02d-%02d %02d:%02d:%02d UTC\n",
              gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday,
              gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
      return 0;
    }
  else
    {
      printf ("failed\n");
      return 1;
    }
}

/*
 * Local Variables:
 * compile-command: "gcc -ggdb -DTEST -Wall -I. -I.. vc-mtime.c libgnu.a"
 * End:
 */

#endif
