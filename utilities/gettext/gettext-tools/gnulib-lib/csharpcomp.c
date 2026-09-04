/* Compile a C# program.
   Copyright (C) 2003-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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

#include <config.h>
#include <alloca.h>

/* Specification.  */
#include "csharpcomp.h"

#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>
#include "concat-filename.h"
#include "cygpath.h"
#include "execute.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "sh-quote.h"
#include "safe-read.h"
#include "xmalloca.h"
#include "xvasprintf.h"
#include "gettext.h"

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)


/* Survey of C# compilers.

   Program          from

   mcs              mono
   dotnet csc.dll   dotnet
   csc              MSVC
   csc              sscli  (non-free)

   We try the C# compilers in the following order:
     1. "mcs", because it is nowadays the "traditional" C# system on Unix.
     2. "dotnet" or "csc" from MSVC, because it is another (newer) free C#
        system.
     3. "csc", although it is not free, because it is a kind of "reference
        implementation" of C#.
   But the order can be changed through the --enable-csharp configuration
   option.
 */

static int
name_is_dll (const struct dirent *d)
{
  if (d->d_name[0] != '.')
    {
      size_t d_name_len = strlen (d->d_name);
      if (d_name_len > 4 && memeq (d->d_name + d_name_len - 4, ".dll", 4))
        /* Filter out files that start with a lowercase letter and files that
           contain the substring ".Native.", since on Windows these files
           produce an error "PE image doesn't contain managed metadata".  */
        if (d->d_name[0] >= 'A' && d->d_name[0] <= 'Z'
            && strstr (d->d_name, ".Native.") == NULL)
          return 1;
    }
  return 0;
}

static int
compile_csharp_using_mono (const char * const *sources,
                           unsigned int sources_count,
                           const char * const *libdirs,
                           unsigned int libdirs_count,
                           const char * const *libraries,
                           unsigned int libraries_count,
                           const char *output_file, bool output_is_library,
                           bool optimize, bool debug,
                           bool verbose)
{
  static bool mcs_tested;
  static bool mcs_present;

  if (!mcs_tested)
    {
      /* Test for presence of mcs:
         "mcs --version >/dev/null 2>/dev/null"
         and (to exclude an unrelated 'mcs' program on QNX 6)
         "mcs --version 2>/dev/null | grep Mono >/dev/null"  */
      const char *argv[3];
      argv[0] = "mcs";
      argv[1] = "--version";
      argv[2] = NULL;

      int fd[1];
      pid_t child = create_pipe_in ("mcs", "mcs", argv, NULL, NULL,
                                    DEV_NULL, true, true, false, fd);
      mcs_present = false;
      if (child != -1)
        {
          /* Read the subprocess output, and test whether it contains the
             string "Mono".  */
          char c[4];
          size_t count = 0;

          while (safe_read (fd[0], &c[count], 1) > 0)
            {
              count++;
              if (count == 4)
                {
                  if (memeq (c, "Mono", 4))
                    mcs_present = true;
                  c[0] = c[1]; c[1] = c[2]; c[2] = c[3];
                  count--;
                }
            }

          close (fd[0]);

          /* Remove zombie process from process list, and retrieve exit
             status.  */
          int exitstatus =
            wait_subprocess (child, "mcs", false, true, true, false, NULL);
          if (exitstatus != 0)
            mcs_present = false;
        }
      mcs_tested = true;
    }

  if (mcs_present)
    {
      unsigned int argc =
        1 + (output_is_library ? 1 : 0) + 1 + libdirs_count + libraries_count
        + (debug ? 1 : 0) + sources_count;
      const char **argv =
        (const char **) xmalloca ((argc + 1) * sizeof (const char *));
      {
        const char **argp = argv;
        *argp++ = "mcs";
        if (output_is_library)
          *argp++ = "-target:library";
        {
          char *option = (char *) xmalloca (5 + strlen (output_file) + 1);
          memcpy (option, "-out:", 5);
          strcpy (option + 5, output_file);
          *argp++ = option;
        }
        for (unsigned int i = 0; i < libdirs_count; i++)
          {
            char *option = (char *) xmalloca (5 + strlen (libdirs[i]) + 1);
            memcpy (option, "-lib:", 5);
            strcpy (option + 5, libdirs[i]);
            *argp++ = option;
          }
        for (unsigned int i = 0; i < libraries_count; i++)
          {
            char *option = (char *) xmalloca (11 + strlen (libraries[i]) + 4 + 1);
            memcpy (option, "-reference:", 11);
            memcpy (option + 11, libraries[i], strlen (libraries[i]));
            strcpy (option + 11 + strlen (libraries[i]), ".dll");
            *argp++ = option;
          }
        if (debug)
          *argp++ = "-debug";
        for (unsigned int i = 0; i < sources_count; i++)
          {
            const char *source_file = sources[i];
            if (strlen (source_file) >= 10
                && memeq (source_file + strlen (source_file) - 10,
                          ".resources", 10))
              {
                char *option = (char *) xmalloca (10 + strlen (source_file) + 1);

                memcpy (option, "-resource:", 10);
                strcpy (option + 10, source_file);
                *argp++ = option;
              }
            else
              *argp++ = source_file;
          }
        *argp = NULL;
        /* Ensure argv length was correctly calculated.  */
        if (argp - argv != argc)
          abort ();
      }

      if (verbose)
        {
          char *command = shell_quote_argv (argv);
          printf ("%s\n", command);
          free (command);
        }

      int fd[1];
      pid_t child = create_pipe_in ("mcs", "mcs", argv, NULL, NULL,
                                    NULL, false, true, true, fd);

      /* Read the subprocess output, copying it to stderr.  Drop the last
         line if it starts with "Compilation succeeded".  */
      FILE *fp = fdopen (fd[0], "r");
      if (fp == NULL)
        error (EXIT_FAILURE, errno, _("fdopen() failed"));
      char *line[2];
      size_t linesize[2];
      line[0] = NULL; linesize[0] = 0;
      line[1] = NULL; linesize[1] = 0;
      unsigned int l = 0;
      size_t linelen[2];
      for (;;)
        {
          linelen[l] = getline (&line[l], &linesize[l], fp);
          if (linelen[l] == (size_t)(-1))
            break;
          l = (l + 1) % 2;
          if (line[l] != NULL)
            fwrite (line[l], 1, linelen[l], stderr);
        }
      l = (l + 1) % 2;
      if (line[l] != NULL
          && !(linelen[l] >= 21
               && memeq (line[l], "Compilation succeeded", 21)))
        fwrite (line[l], 1, linelen[l], stderr);
      if (line[0] != NULL)
        free (line[0]);
      if (line[1] != NULL)
        free (line[1]);
      fclose (fp);

      /* Remove zombie process from process list, and retrieve exit status.  */
      int exitstatus =
        wait_subprocess (child, "mcs", false, false, true, true, NULL);

      for (unsigned int i = 1 + (output_is_library ? 1 : 0);
           i < 1 + (output_is_library ? 1 : 0)
               + 1 + libdirs_count + libraries_count;
           i++)
        freea ((char *) argv[i]);
      for (unsigned int i = 0; i < sources_count; i++)
        if (argv[argc - sources_count + i] != sources[i])
          freea ((char *) argv[argc - sources_count + i]);
      freea (argv);

      return (exitstatus != 0);
    }
  else
    return -1;
}

static int
compile_csharp_using_dotnet (const char * const *sources,
                             unsigned int sources_count,
                             const char * const *libdirs,
                             unsigned int libdirs_count,
                             const char * const *libraries,
                             unsigned int libraries_count,
                             const char *output_file, bool output_is_library,
                             bool optimize, bool debug,
                             bool verbose)
{
  static bool dotnet_tested;
  static bool dotnet_present;

  if (!dotnet_tested)
    {
      /* Test for presence of dotnet:
         dotnet --list-runtimes >/dev/null 2>/dev/null
         && test -n "`dotnet --list-sdks 2>/dev/null`"  */
      int exitstatus1;
      {
        const char *argv[3];
        argv[0] = "dotnet";
        argv[1] = "--list-runtimes";
        argv[2] = NULL;
        exitstatus1 = execute ("dotnet", "dotnet", argv, NULL, NULL,
                               false, false, true, true,
                               true, false, NULL);
      }
      if (exitstatus1 == 0)
        {
          const char *argv[3];
          argv[0] = "dotnet";
          argv[1] = "--list-sdks";
          argv[2] = NULL;

          int fd[1];
          pid_t child = create_pipe_in ("dotnet", "dotnet", argv, NULL, NULL,
                                        DEV_NULL, true, true, false, fd);
          if (child != -1)
            {
              /* Read the subprocess output, and test whether it is
                 non-empty.  */
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

              /* Remove zombie process from process list, and retrieve exit
                 status.  */
              int exitstatus =
                wait_subprocess (child, "dotnet", false, true, true, false,
                                 NULL);
              dotnet_present = (exitstatus == 0 && count > 0);
            }
          else
            dotnet_present = false;
        }
      else
        dotnet_present = false;
      dotnet_tested = true;
    }

  if (dotnet_present)
    {
      bool err = false;

      /* Invoke 'dotnet --list-runtimes' and extract the .NET runtime dir
         from its output.  */
      char *dotnet_runtime_dir;
      {
        dotnet_runtime_dir = NULL;

        const char *argv[3];
        argv[0] = "dotnet";
        argv[1] = "--list-runtimes";
        argv[2] = NULL;

        /* Open a pipe to the program.  */
        int fd[1];
        pid_t child = create_pipe_in ("dotnet", "dotnet", argv, NULL, NULL,
                                      DEV_NULL, false, true, false, fd);
        if (child == -1)
          {
            error (0, 0, _("%s subprocess I/O error"), "dotnet");
            err = true;
          }
        else
          {
            /* Retrieve its result.  */
            FILE *fp = fdopen (fd[0], "r");
            if (fp == NULL)
              error (EXIT_FAILURE, errno, _("fdopen() failed"));

            char *line = NULL;
            size_t linesize = 0;

            for (;;)
              {
                size_t linelen = getline (&line, &linesize, fp);
                if (linelen == (size_t)(-1))
                  {
                    error (0, 0, _("%s subprocess I/O error"), "dotnet");
                    err = true;
                    break;
                  }
                if (linelen > 0 && line[linelen - 1] == '\n')
                  {
                    line[linelen - 1] = '\0';
                    linelen--;
                    if (linelen > 0 && line[linelen - 1] == '\r')
                      {
                        line[linelen - 1] = '\0';
                        linelen--;
                      }
                  }
                /* The line has the structure
                     Microsoft.SUBSYSTEM VERSION [DIRECTORY]  */
                if (linelen > 22
                    && memeq (line, "Microsoft.NETCore.App ", 22))
                  {
                    char *version = line + 22;
                    char *version_end = strchr (version, ' ');
                    if (version_end != NULL && version_end[1] == '[')
                      {
                        *version_end = '\0';
                        char *dir = version_end + 2;
                        char *dir_end = strchr (dir, ']');
                        if (dir_end != NULL)
                          {
                            *dir_end = '\0';
                            dotnet_runtime_dir =
                              xasprintf ("%s/%s", dir, version);
                            break;
                          }
                      }
                  }
              }

            free (line);

            /* Read until EOF (otherwise the child process may get a
               SIGPIPE signal).  */
            while (getc (fp) != EOF)
              ;

            fclose (fp);

            /* Remove zombie process from process list, and retrieve
               exit status.  */
            int exitstatus =
              wait_subprocess (child, "dotnet", true, false, true, false, NULL);
            if (exitstatus != 0)
              {
                error (0, 0, _("%s subprocess failed"), "dotnet");
                err = true;
              }
          }
      }

      /* Invoke 'dotnet --list-sdks' and extract the .NET SDK dir
         from its output.  */
      char *dotnet_sdk_dir;
      {
        dotnet_sdk_dir = NULL;

        const char *argv[3];
        argv[0] = "dotnet";
        argv[1] = "--list-sdks";
        argv[2] = NULL;

        /* Open a pipe to the program.  */
        int fd[1];
        pid_t child = create_pipe_in ("dotnet", "dotnet", argv, NULL, NULL,
                                      DEV_NULL, false, true, false, fd);
        if (child == -1)
          {
            error (0, 0, _("%s subprocess I/O error"), "dotnet");
            err = true;
          }
        else
          {
            /* Retrieve its result.  */
            FILE *fp = fdopen (fd[0], "r");
            if (fp == NULL)
              error (EXIT_FAILURE, errno, _("fdopen() failed"));

            char *line = NULL;
            size_t linesize = 0;

            for (;;)
              {
                size_t linelen = getline (&line, &linesize, fp);
                if (linelen == (size_t)(-1))
                  {
                    error (0, 0, _("%s subprocess I/O error"), "dotnet");
                    err = true;
                    break;
                  }
                if (linelen > 0 && line[linelen - 1] == '\n')
                  {
                    line[linelen - 1] = '\0';
                    linelen--;
                    if (linelen > 0 && line[linelen - 1] == '\r')
                      {
                        line[linelen - 1] = '\0';
                        linelen--;
                      }
                  }
                /* The line has the structure
                     VERSION [DIRECTORY]  */
                char *version = line;
                char *version_end = strchr (version, ' ');
                if (version_end != NULL && version_end[1] == '[')
                  {
                    *version_end = '\0';
                    char *dir = version_end + 2;
                    char *dir_end = strchr (dir, ']');
                    if (dir_end != NULL)
                      {
                        *dir_end = '\0';
                        dotnet_sdk_dir = xasprintf ("%s/%s", dir, version);
                        break;
                      }
                  }
              }

            free (line);

            /* Read until EOF (otherwise the child process may get a
               SIGPIPE signal).  */
            while (getc (fp) != EOF)
              ;

            fclose (fp);

            /* Remove zombie process from process list, and retrieve
               exit status.  */
            int exitstatus =
              wait_subprocess (child, "dotnet", true, false, true, false, NULL);
            if (exitstatus != 0)
              {
                error (0, 0, _("%s subprocess failed"), "dotnet");
                err = true;
              }
          }
      }

      if (err)
        return 1;

      /* Get a list of all *.dll files in dotnet_runtime_dir.  */
      struct dirent **dlls;
      int num_dlls = scandir (dotnet_runtime_dir, &dlls, name_is_dll, alphasort);
      if (num_dlls < 0 && errno == ENOMEM)
        xalloc_die ();
      if (num_dlls <= 0)
        {
          dlls = NULL;
          num_dlls = 0;
        }

      /* Construct the file name of the 'csc' compiler.  */
      char *roslyn_dir = xconcatenated_filename (dotnet_sdk_dir, "Roslyn", NULL);
      char *roslyn_bin_dir = xconcatenated_filename (roslyn_dir, "bincore", NULL);
      char *csc = xconcatenated_filename (roslyn_bin_dir, "csc.dll", NULL);
      char *csc_converted = cygpath_w (csc);

      /* Here, we assume that 'csc' is a native Windows program, therefore
         we need to use cygpath_w.  */
      char **malloced =
        (char **)
        xmalloca ((1 + libdirs_count + sources_count * 2 + 1) * sizeof (char *));
      char **mallocedp = malloced;

      unsigned int argc =
        3 + 1 + 1 + libdirs_count + libraries_count
        + (optimize ? 1 : 0) + (debug ? 1 : 0) + sources_count + 1 + num_dlls;
      const char **argv =
        (const char **) xmalloca ((argc + 1) * sizeof (const char *));
      {
        const char **argp = argv;
        *argp++ = "dotnet";
        *argp++ = csc_converted;
        *argp++ = "-nologo";
        *argp++ = (output_is_library ? "-target:library" : "-target:exe");
        {
          char *output_file_converted = cygpath_w (output_file);
          *mallocedp++ = output_file_converted;
          char *option = (char *) xmalloca (5 + strlen (output_file_converted) + 1);
          memcpy (option, "-out:", 5);
          strcpy (option + 5, output_file_converted);
          *argp++ = option;
        }
        for (unsigned int i = 0; i < libdirs_count; i++)
          {
            const char *libdir = libdirs[i];
            char *libdir_converted = cygpath_w (libdir);
            *mallocedp++ = libdir_converted;
            char *option = (char *) xmalloca (5 + strlen (libdir_converted) + 1);
            memcpy (option, "-lib:", 5);
            strcpy (option + 5, libdir_converted);
            *argp++ = option;
          }
        for (unsigned int i = 0; i < libraries_count; i++)
          {
            char *option = (char *) xmalloca (11 + strlen (libraries[i]) + 4 + 1);
            memcpy (option, "-reference:", 11);
            memcpy (option + 11, libraries[i], strlen (libraries[i]));
            strcpy (option + 11 + strlen (libraries[i]), ".dll");
            *argp++ = option;
          }
        if (optimize)
          *argp++ = "-optimize+";
        if (debug)
          *argp++ = "-debug+";
        for (unsigned int i = 0; i < sources_count; i++)
          {
            const char *source_file = sources[i];
            char *source_file_converted = cygpath_w (source_file);
            *mallocedp++ = source_file_converted;
            if (strlen (source_file_converted) >= 10
                && memeq ((source_file_converted
                           + strlen (source_file_converted) - 10),
                          ".resources", 10))
              {
                char *option =
                  (char *) xmalloc (10 + strlen (source_file_converted) + 1);
                memcpy (option, "-resource:", 10);
                strcpy (option + 10, source_file_converted);
                *mallocedp++ = option;
                *argp++ = option;
              }
            else
              *argp++ = source_file_converted;
          }
        /* Add -lib and -reference options, so that the compiler finds
           Object, Console, String, etc.  */
        {
          char *dotnet_runtime_dir_converted = cygpath_w (dotnet_runtime_dir);
          *mallocedp++ = dotnet_runtime_dir_converted;
          char *option =
            (char *) xmalloca (5 + strlen (dotnet_runtime_dir_converted) + 1);
          memcpy (option, "-lib:", 5);
          strcpy (option + 5, dotnet_runtime_dir_converted);
          *argp++ = option;
        }
        for (unsigned int i = 0; i < num_dlls; i++)
          {
            char *option = (char *) xmalloca (11 + strlen (dlls[i]->d_name) + 1);
            memcpy (option, "-reference:", 11);
            strcpy (option + 11, dlls[i]->d_name);
            *argp++ = option;
          }
        *argp = NULL;
        /* Ensure argv length was correctly calculated.  */
        if (argp - argv != argc)
          abort ();
      }

      if (verbose)
        {
          char *command = shell_quote_argv (argv);
          printf ("%s\n", command);
          free (command);
        }

      int exitstatus = execute ("dotnet", "dotnet", argv, NULL, NULL,
                                false, false, false, false,
                                true, true, NULL);

      for (unsigned int i = 4; i < 5 + libdirs_count + libraries_count; i++)
        freea ((char *) argv[i]);
      for (unsigned int i = 0; i < 1 + num_dlls; i++)
        freea ((char *) argv[argc - (1 + num_dlls) + i]);
      while (mallocedp > malloced)
        free (*--mallocedp);
      freea (argv);
      freea (malloced);
      free (csc_converted);
      free (csc);
      free (roslyn_bin_dir);
      free (roslyn_dir);
      for (unsigned int i = 0; i < num_dlls; i++)
        free (dlls[i]);
      free (dlls);

      return (exitstatus != 0);
    }
  else
    {
      static bool csc_tested;
      static bool csc_present;

      if (!csc_tested)
        {
          /* Test for presence of csc:
             "csc -help 2>/dev/null | grep -i analyzer >/dev/null \
              && ! { csc -help 2>/dev/null | grep -i chicken > /dev/null; }"  */
          const char *argv[3];
          argv[0] = "csc";
          argv[1] = "-help";
          argv[2] = NULL;

          int fd[1];
          pid_t child = create_pipe_in ("csc", "csc", argv, NULL, NULL,
                                        DEV_NULL, true, true, false, fd);
          if (child != -1)
            {
              /* Read the subprocess output, and test whether it contains the
                 string "analyzer" or the string "chicken".  */
              char c[8];
              size_t count = 0;
              bool seen_analyzer = false;
              bool seen_chicken = false;

              csc_present = true;
              while (safe_read (fd[0], &c[count], 1) > 0)
                {
                  if (c[count] >= 'A' && c[count] <= 'Z')
                    c[count] += 'a' - 'A';
                  count++;
                  if (count >= 7)
                    {
                      if (memeq (c, "chicken", 7))
                        seen_chicken = true;
                    }
                  if (count == 8)
                    {
                      if (memeq (c, "analyzer", 8))
                        seen_analyzer = true;
                      c[0] = c[1]; c[1] = c[2]; c[2] = c[3]; c[3] = c[4];
                      c[4] = c[5]; c[5] = c[6]; c[6] = c[7];
                      count--;
                    }
                }

              close (fd[0]);

              /* Remove zombie process from process list, and retrieve exit
                 status.  */
              int exitstatus =
                wait_subprocess (child, "csc", false, true, true, false, NULL);
              csc_present = (exitstatus == 0 && seen_analyzer && !seen_chicken);
            }
          else
            csc_present = false;
          csc_tested = true;
        }

      if (csc_present)
        {
          /* Here, we assume that 'csc' is a native Windows program, therefore
             we need to use cygpath_w.  */
          char **malloced =
            (char **)
            xmalloca ((1 + libdirs_count + sources_count * 2) * sizeof (char *));
          char **mallocedp = malloced;

          unsigned int argc =
            2 + 1 + 1 + libdirs_count + libraries_count
            + (optimize ? 1 : 0) + (debug ? 1 : 0) + sources_count;
          const char **argv =
            (const char **) xmalloca ((argc + 1) * sizeof (const char *));
          {
            const char **argp = argv;
            *argp++ = "csc";
            *argp++ = "-nologo";
            *argp++ = (output_is_library ? "-target:library" : "-target:exe");
            {
              char *output_file_converted = cygpath_w (output_file);
              *mallocedp++ = output_file_converted;
              char *option = (char *) xmalloca (5 + strlen (output_file_converted) + 1);
              memcpy (option, "-out:", 5);
               strcpy (option + 5, output_file_converted);
              *argp++ = option;
            }
            for (unsigned int i = 0; i < libdirs_count; i++)
              {
                const char *libdir = libdirs[i];
                char *libdir_converted = cygpath_w (libdir);
                *mallocedp++ = libdir_converted;
                char *option = (char *) xmalloca (5 + strlen (libdir_converted) + 1);
                memcpy (option, "-lib:", 5);
                strcpy (option + 5, libdir_converted);
                *argp++ = option;
              }
            for (unsigned int i = 0; i < libraries_count; i++)
              {
                char *option = (char *) xmalloca (11 + strlen (libraries[i]) + 4 + 1);
                memcpy (option, "-reference:", 11);
                memcpy (option + 11, libraries[i], strlen (libraries[i]));
                strcpy (option + 11 + strlen (libraries[i]), ".dll");
                *argp++ = option;
              }
            if (optimize)
              *argp++ = "-optimize+";
            if (debug)
              *argp++ = "-debug+";
            for (unsigned int i = 0; i < sources_count; i++)
              {
                const char *source_file = sources[i];
                char *source_file_converted = cygpath_w (source_file);
                *mallocedp++ = source_file_converted;
                if (strlen (source_file_converted) >= 10
                    && memeq ((source_file_converted
                               + strlen (source_file_converted) - 10),
                              ".resources", 10))
                  {
                    char *option =
                      (char *) xmalloc (10 + strlen (source_file_converted) + 1);
                    memcpy (option, "-resource:", 10);
                    strcpy (option + 10, source_file_converted);
                    *mallocedp++ = option;
                    *argp++ = option;
                  }
                else
                  *argp++ = source_file_converted;
              }
            *argp = NULL;
            /* Ensure argv length was correctly calculated.  */
            if (argp - argv != argc)
              abort ();
          }

          if (verbose)
            {
              char *command = shell_quote_argv (argv);
              printf ("%s\n", command);
              free (command);
            }

          int exitstatus = execute ("csc", "csc", argv, NULL, NULL,
                                    false, false, false, false,
                                    true, true, NULL);

          for (unsigned int i = 3; i < 4 + libdirs_count + libraries_count; i++)
            freea ((char *) argv[i]);
          while (mallocedp > malloced)
            free (*--mallocedp);
          freea (argv);
          freea (malloced);

          return (exitstatus != 0);
        }
      else
        return -1;
    }
}

static int
compile_csharp_using_sscli (const char * const *sources,
                            unsigned int sources_count,
                            const char * const *libdirs,
                            unsigned int libdirs_count,
                            const char * const *libraries,
                            unsigned int libraries_count,
                            const char *output_file, bool output_is_library,
                            bool optimize, bool debug,
                            bool verbose)
{
  static bool csc_tested;
  static bool csc_present;

  if (!csc_tested)
    {
      /* Test for presence of csc:
         "csc -help >/dev/null 2>/dev/null \
          && ! { csc -help 2>/dev/null | grep -i chicken > /dev/null; }"  */
      const char *argv[3];
      argv[0] = "csc";
      argv[1] = "-help";
      argv[2] = NULL;

      int fd[1];
      pid_t child = create_pipe_in ("csc", "csc", argv, NULL, NULL,
                                    DEV_NULL, true, true, false, fd);
      csc_present = false;
      if (child != -1)
        {
          /* Read the subprocess output, and test whether it contains the
             string "chicken".  */
          char c[7];
          size_t count = 0;

          csc_present = true;
          while (safe_read (fd[0], &c[count], 1) > 0)
            {
              if (c[count] >= 'A' && c[count] <= 'Z')
                c[count] += 'a' - 'A';
              count++;
              if (count == 7)
                {
                  if (memeq (c, "chicken", 7))
                    csc_present = false;
                  c[0] = c[1]; c[1] = c[2]; c[2] = c[3];
                  c[3] = c[4]; c[4] = c[5]; c[5] = c[6];
                  count--;
                }
            }

          close (fd[0]);

          /* Remove zombie process from process list, and retrieve exit
             status.  */
          int exitstatus =
            wait_subprocess (child, "csc", false, true, true, false, NULL);
          if (exitstatus != 0)
            csc_present = false;
        }
      csc_tested = true;
    }

  if (csc_present)
    {
      /* Here, we assume that 'csc' is a native Windows program, therefore
         we need to use cygpath_w.  */
      char **malloced =
        (char **)
        xmalloca ((1 + libdirs_count + sources_count * 2) * sizeof (char *));
      char **mallocedp = malloced;

      unsigned int argc =
        2 + 1 + 1 + libdirs_count + libraries_count
        + (optimize ? 1 : 0) + (debug ? 1 : 0) + sources_count;
      const char **argv =
        (const char **) xmalloca ((argc + 1) * sizeof (const char *));
      {
        const char **argp = argv;
        *argp++ = "csc";
        *argp++ = "-nologo";
        *argp++ = (output_is_library ? "-target:library" : "-target:exe");
        {
          char *output_file_converted = cygpath_w (output_file);
          *mallocedp++ = output_file_converted;
          char *option = (char *) xmalloca (5 + strlen (output_file_converted) + 1);
          memcpy (option, "-out:", 5);
          strcpy (option + 5, output_file_converted);
          *argp++ = option;
        }
        for (unsigned int i = 0; i < libdirs_count; i++)
          {
            const char *libdir = libdirs[i];
            char *libdir_converted = cygpath_w (libdir);
            *mallocedp++ = libdir_converted;
            char *option = (char *) xmalloca (5 + strlen (libdir_converted) + 1);
            memcpy (option, "-lib:", 5);
            strcpy (option + 5, libdir_converted);
            *argp++ = option;
          }
        for (unsigned int i = 0; i < libraries_count; i++)
          {
            char *option = (char *) xmalloca (11 + strlen (libraries[i]) + 4 + 1);
            memcpy (option, "-reference:", 11);
            memcpy (option + 11, libraries[i], strlen (libraries[i]));
            strcpy (option + 11 + strlen (libraries[i]), ".dll");
            *argp++ = option;
          }
        if (optimize)
          *argp++ = "-optimize+";
        if (debug)
          *argp++ = "-debug+";
        for (unsigned int i = 0; i < sources_count; i++)
          {
            const char *source_file = sources[i];
            char *source_file_converted = cygpath_w (source_file);
            *mallocedp++ = source_file_converted;
            if (strlen (source_file_converted) >= 10
                && memeq ((source_file_converted
                           + strlen (source_file_converted) - 10),
                          ".resources", 10))
              {
                char *option =
                  (char *) xmalloc (10 + strlen (source_file_converted) + 1);
                memcpy (option, "-resource:", 10);
                strcpy (option + 10, source_file_converted);
                *mallocedp++ = option;
                *argp++ = option;
              }
            else
              *argp++ = source_file_converted;
          }
        *argp = NULL;
        /* Ensure argv length was correctly calculated.  */
        if (argp - argv != argc)
          abort ();
      }

      if (verbose)
        {
          char *command = shell_quote_argv (argv);
          printf ("%s\n", command);
          free (command);
        }

      int exitstatus = execute ("csc", "csc", argv, NULL, NULL,
                                false, false, false, false,
                                true, true, NULL);

      for (unsigned int i = 3; i < 4 + libdirs_count + libraries_count; i++)
        freea ((char *) argv[i]);
      while (mallocedp > malloced)
        free (*--mallocedp);
      freea (argv);
      freea (malloced);

      return (exitstatus != 0);
    }
  else
    return -1;
}

bool
compile_csharp_class (const char * const *sources,
                      unsigned int sources_count,
                      const char * const *libdirs,
                      unsigned int libdirs_count,
                      const char * const *libraries,
                      unsigned int libraries_count,
                      const char *output_file,
                      bool optimize, bool debug,
                      bool verbose)
{
  bool output_is_library =
    (strlen (output_file) >= 4
     && memeq (output_file + strlen (output_file) - 4, ".dll", 4));

  int result;

  /* First try the C# implementation specified through --enable-csharp.  */
#if CSHARP_CHOICE_MONO
  result = compile_csharp_using_mono (sources, sources_count,
                                      libdirs, libdirs_count,
                                      libraries, libraries_count,
                                      output_file, output_is_library,
                                      optimize, debug, verbose);
  if (result >= 0)
    return (bool) result;
#endif
#if CSHARP_CHOICE_DOTNET
  result = compile_csharp_using_dotnet (sources, sources_count,
                                        libdirs, libdirs_count,
                                        libraries, libraries_count,
                                        output_file, output_is_library,
                                        optimize, debug, verbose);
  if (result >= 0)
    return (bool) result;
#endif

  /* Then try the remaining C# implementations in our standard order.  */
#if !CSHARP_CHOICE_MONO
  result = compile_csharp_using_mono (sources, sources_count,
                                      libdirs, libdirs_count,
                                      libraries, libraries_count,
                                      output_file, output_is_library,
                                      optimize, debug, verbose);
  if (result >= 0)
    return (bool) result;
#endif

#if !CSHARP_CHOICE_DOTNET
  result = compile_csharp_using_dotnet (sources, sources_count,
                                        libdirs, libdirs_count,
                                        libraries, libraries_count,
                                        output_file, output_is_library,
                                        optimize, debug, verbose);
  if (result >= 0)
    return (bool) result;
#endif

  result = compile_csharp_using_sscli (sources, sources_count,
                                       libdirs, libdirs_count,
                                       libraries, libraries_count,
                                       output_file, output_is_library,
                                       optimize, debug, verbose);
  if (result >= 0)
    return (bool) result;

  error (0, 0, _("C# compiler not found, try installing mono or dotnet"));
  return true;
}
