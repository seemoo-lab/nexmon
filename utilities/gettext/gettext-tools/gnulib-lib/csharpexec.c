/* Execute a C# program.
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
#include "csharpexec.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <error.h>
#include "dirname.h"
#include "concat-filename.h"
#include "canonicalize.h"
#include "cygpath.h"
#include "execute.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "sh-quote.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "copy-file.h"
#include "clean-temp-simple.h"
#include "clean-temp.h"
#include "gettext.h"

/* Handling of MONO_PATH is just like Java CLASSPATH.  */
#define CLASSPATHVAR "MONO_PATH"
#define new_classpath new_monopath
#define set_classpath set_monopath
#define reset_classpath reset_monopath
#include "classpath.h"
#include "classpath.c"
#undef reset_classpath
#undef set_classpath
#undef new_classpath
#undef CLASSPATHVAR

/* Handling of clix' PATH variable is just like Java CLASSPATH.  */
#if defined _WIN32 || defined __CYGWIN__
  /* Native Windows, Cygwin */
  #define CLASSPATHVAR "PATH"
#elif defined __APPLE__ && defined __MACH__
  /* Mac OS X */
  #define CLASSPATHVAR "DYLD_LIBRARY_PATH"
#else
  /* Normal Unix */
  #define CLASSPATHVAR "LD_LIBRARY_PATH"
#endif
#define new_classpath new_clixpath
#define set_classpath set_clixpath
#define reset_classpath reset_clixpath
#include "classpath.h"
#include "classpath.c"
#undef reset_classpath
#undef set_classpath
#undef new_classpath
#undef CLASSPATHVAR

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)


/* Survey of CIL interpreters.

   Program    from

   mono       mono
   dotnet     dotnet or MSVC
   clix       sscli  (non-free)

   With Mono, the MONO_PATH is a colon separated list of pathnames. (On
   Windows: semicolon separated list of pathnames.)

   We try the CIL interpreters in the following order:
     1. "mono", because it is nowadays the "traditional" C# system on Unix.
     2. "dotnet", because it is another (newer) free C# system on Unix.
     3. "clix", although it is not free, because it is a kind of "reference
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
        return 1;
    }
  return 0;
}

static int
execute_csharp_using_mono (const char *assembly_path,
                           const char * const *libdirs,
                           unsigned int libdirs_count,
                           const char * const *args, unsigned int nargs,
                           bool verbose, bool quiet,
                           execute_fn *executer, void *private_data)
{
  static bool mono_tested;
  static bool mono_present;

  if (!mono_tested)
    {
      /* Test for presence of mono:
         "mono --version >/dev/null 2>/dev/null"  */
      const char *argv[3];
      argv[0] = "mono";
      argv[1] = "--version";
      argv[2] = NULL;
      int exitstatus = execute ("mono", "mono", argv, NULL, NULL,
                                false, false, true, true,
                                true, false, NULL);
      mono_present = (exitstatus == 0);
      mono_tested = true;
    }

  if (mono_present)
    {
      /* Set MONO_PATH.  */
      char *old_monopath = set_monopath (libdirs, libdirs_count, false, verbose);

      const char **argv =
        (const char **) xmalloca ((2 + nargs + 1) * sizeof (const char *));
      argv[0] = "mono";
      argv[1] = assembly_path;
      for (unsigned int i = 0; i <= nargs; i++)
        argv[2 + i] = args[i];

      if (verbose)
        {
          char *command = shell_quote_argv (argv);
          printf ("%s\n", command);
          free (command);
        }

      bool err = executer ("mono", "mono", argv, private_data);

      freea (argv);

      /* Reset MONO_PATH.  */
      reset_monopath (old_monopath);

      return err;
    }
  else
    return -1;
}

static int
execute_csharp_using_dotnet (const char *assembly_path,
                             const char * const *libdirs,
                             unsigned int libdirs_count,
                             const char * const *args, unsigned int nargs,
                             bool verbose, bool quiet,
                             execute_fn *executer, void *private_data)
{
  static bool dotnet_tested;
  static bool dotnet_present;

  if (!dotnet_tested)
    {
      /* Test for presence of dotnet:
         "dotnet --list-runtimes >/dev/null 2>/dev/null"  */
      const char *argv[3];
      argv[0] = "dotnet";
      argv[1] = "--list-runtimes";
      argv[2] = NULL;
      int exitstatus = execute ("dotnet", "dotnet", argv, NULL, NULL,
                                false, false, true, true,
                                true, false, NULL);
      dotnet_present = (exitstatus == 0);
      dotnet_tested = true;
    }

  if (dotnet_present)
    {
      /* Handle the -L options.
         The way this works is that we have to copy (or symlink) the DLLs into
         the directory where FOO.exe resides.
         Maybe there is another way to do this, but I haven't found it, trying
           - the --additionalprobingpath command-line option,
           - the additionalProbingPaths property in runtimeconfig.json,
           - adding a --deps deps.json option,
         cf. <https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet#options-for-running-an-application>
         and <https://github.com/dotnet/runtime/blob/main/docs/design/features/host-probing.md>.  */
      char **tmpfiles = NULL;
      size_t tmpfiles_alloc = 0;
      size_t tmpfiles_count = 0;
      if (libdirs_count > 0)
        {
          /* Copy the DLLs.  */
          char *assembly_dir = dir_name (assembly_path);

          for (unsigned int l = 0; l < libdirs_count; l++)
            {
              const char *libdir = libdirs[l];

              /* Get a list of all *.dll files in libdir.  */
              struct dirent **dlls;
              int num_dlls = scandir (libdir, &dlls, name_is_dll, alphasort);
              if (num_dlls < 0 && errno == ENOMEM)
                xalloc_die ();
              if (num_dlls <= 0)
                {
                  dlls = NULL;
                  num_dlls = 0;
                }

              for (int i = 0; i < num_dlls; i++)
                {
                  char *target_dll =
                    xconcatenated_filename (assembly_dir, dlls[i]->d_name,
                                            NULL);
                  struct stat statbuf;
                  if (stat (target_dll, &statbuf) == 0 || errno == EOVERFLOW)
                    {
                      /* A DLL of this name is already at the expected
                         location.  */
                    }
                  else
                    {
                      char *absolute_target_dll =
                        canonicalize_filename_mode (target_dll, CAN_ALL_BUT_LAST);
                      if (absolute_target_dll == NULL)
                        {
                          if (errno == ENOMEM)
                            xalloc_die ();
                        }
                      else
                        {
                          char *libdir_dll =
                            xconcatenated_filename (libdir, dlls[i]->d_name,
                                                    NULL);

                          /* Create absolute_target_dll as a temporary file.  */
                          if (register_temporary_file (absolute_target_dll) < 0)
                            xalloc_die ();

                          if (tmpfiles_count >= tmpfiles_alloc)
                            {
                              tmpfiles_alloc = 2 * tmpfiles_alloc + 1;
                              tmpfiles =
                                (char **)
                                xrealloc (tmpfiles, tmpfiles_alloc * sizeof (char *));
                            }
                          tmpfiles[tmpfiles_count++] = absolute_target_dll;

                          if (copy_file_to (libdir_dll, absolute_target_dll) != 0)
                            {
                              remove (absolute_target_dll);
                              unregister_temporary_file (absolute_target_dll);
                              error (0, 0, _("failed to copy '%s' to '%s'"),
                                     libdir_dll, absolute_target_dll);
                              free (libdir_dll);
                              free (target_dll);
                              for (int ii = 0; ii < num_dlls; ii++)
                                free (dlls[ii]);
                              free (dlls);
                              free (assembly_dir);
                              while (tmpfiles_count > 0)
                                {
                                  char *tmpfile = tmpfiles[--tmpfiles_count];
                                  remove (tmpfile);
                                  unregister_temporary_file (tmpfile);
                                  free (tmpfile);
                                }
                              free (tmpfiles);
                              return 1;
                            }
                          free (libdir_dll);
                        }
                    }
                  free (target_dll);
                }

              for (int i = 0; i < num_dlls; i++)
                free (dlls[i]);
              free (dlls);
            }

          free (assembly_dir);
        }

      bool err = false;

      char *assembly_path_converted = cygpath_w (assembly_path);

      /* Test whether alongside FOO.exe, a file FOO.runtimeconfig.json already
         exists.  */
      char *runtimeconfig_filename =
        (char *) xmalloca (strlen (assembly_path) + 19 + 1);
      {
        size_t assembly_path_len = strlen (assembly_path);
        if (assembly_path_len > 4
            && memeq (assembly_path + (assembly_path_len - 4), ".exe", 4))
          assembly_path_len -= 4;
        char *p = runtimeconfig_filename;
        memcpy (p, assembly_path, assembly_path_len);
        p += assembly_path_len;
        strcpy (p, ".runtimeconfig.json");
      }
      struct stat runtimeconfig_statbuf;
      if (stat (runtimeconfig_filename, &runtimeconfig_statbuf) == 0
          || errno == EOVERFLOW)
        {
          const char **argv =
            (const char **) xmalloca ((3 + nargs + 1) * sizeof (const char *));
          argv[0] = "dotnet";
          argv[1] = "exec";
          argv[2] = assembly_path_converted;
          for (unsigned int i = 0; i <= nargs; i++)
            argv[3 + i] = args[i];

          if (verbose)
            {
              char *command = shell_quote_argv (argv);
              printf ("%s\n", command);
              free (command);
            }

          err = executer ("dotnet", "dotnet", argv, private_data);

          freea (argv);
        }
      else
        {
          /* dotnet needs a FOO.runtimeconfig.json alongside FOO.exe in order to
             execute FOO.exe.  Create a dummy one in a temporary directory
             (because the directory where FOO.exe sits is not necessarily
             writable).
             Documentation of this file format:
             <https://learn.microsoft.com/en-us/dotnet/core/runtime-config/>  */

          char *netcore_version;

          /* Invoke 'dotnet --list-runtimes' and extract the .NET Core version
             from its output.  */
          {
            netcore_version = NULL;

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
                        if (version_end != NULL)
                          {
                            *version_end = '\0';
                            netcore_version = xstrdup (version);
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
                  wait_subprocess (child, "dotnet", true, false, true, false,
                                   NULL);
                if (exitstatus != 0)
                  {
                    error (0, 0, _("%s subprocess failed"), "dotnet");
                    err = true;
                  }
              }
          }

          if (!err)
            {
              if (netcore_version == NULL)
                {
                  error (0, 0, _("could not determine %s version"), "dotnet");
                  err = true;
                }
            }

          if (!err)
            {
              /* Create the runtimeconfig.json file.  */
              struct temp_dir *tmpdir = create_temp_dir ("csharp", NULL, false);
              if (tmpdir == NULL)
                err = true;
              else
                {
                  char *runtimeconfig =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "runtimeconfig.json", NULL);
                  register_temp_file (tmpdir, runtimeconfig);
                  FILE *fp = fopen_temp (runtimeconfig, "w", false);
                  if (fp == NULL)
                    {
                      error (0, errno, _("failed to create \"%s\""), runtimeconfig);
                      unregister_temp_file (tmpdir, runtimeconfig);
                      err = true;
                    }
                  else
                    {
                      fprintf (fp,
                               "{ \"runtimeOptions\":\n"
                               "  { \"framework\":\n"
                               "    { \"name\": \"Microsoft.NETCore.App\", \"version\": \"%s\" }\n"
                               "  }\n"
                               "}",
                               netcore_version);
                      if (fwriteerror_temp (fp))
                        {
                          error (0, errno, _("error while writing \"%s\" file"),
                                 runtimeconfig);
                          err = true;
                        }
                      else
                        {
                          char *runtimeconfig_converted =
                            cygpath_w (runtimeconfig);

                          const char **argv =
                            (const char **)
                            xmalloca ((5 + nargs + 1) * sizeof (const char *));
                          argv[0] = "dotnet";
                          argv[1] = "exec";
                          argv[2] = "--runtimeconfig";
                          argv[3] = runtimeconfig_converted;
                          argv[4] = assembly_path_converted;
                          for (unsigned int i = 0; i <= nargs; i++)
                            argv[5 + i] = args[i];

                          if (verbose)
                            {
                              char *command = shell_quote_argv (argv);
                              printf ("%s\n", command);
                              free (command);
                            }

                          err = executer ("dotnet", "dotnet", argv,
                                          private_data);

                          freea (argv);
                          free (runtimeconfig_converted);
                        }
                    }
                  free (runtimeconfig);
                  cleanup_temp_dir (tmpdir);
                }
            }
        }

      freea (runtimeconfig_filename);
      while (tmpfiles_count > 0)
        {
          char *tmpfile = tmpfiles[--tmpfiles_count];
          remove (tmpfile);
          unregister_temporary_file (tmpfile);
          free (tmpfile);
        }
      free (tmpfiles);
      free (assembly_path_converted);
      return err;
    }
  else
    return -1;
}

static int
execute_csharp_using_sscli (const char *assembly_path,
                            const char * const *libdirs,
                            unsigned int libdirs_count,
                            const char * const *args, unsigned int nargs,
                            bool verbose, bool quiet,
                            execute_fn *executer, void *private_data)
{
  static bool clix_tested;
  static bool clix_present;

  if (!clix_tested)
    {
      /* Test for presence of clix:
         "clix >/dev/null 2>/dev/null ; test $? = 1"  */
      const char *argv[2];
      argv[0] = "clix";
      argv[1] = NULL;
      int exitstatus = execute ("clix", "clix", argv, NULL, NULL,
                                false, false, true, true,
                                true, false, NULL);
      clix_present = (exitstatus == 0 || exitstatus == 1);
      clix_tested = true;
    }

  if (clix_present)
    {
      /* Here, we assume that 'clix' is a native Windows program, therefore
         we need to use cygpath_w.  */
      char *assembly_path_converted = cygpath_w (assembly_path);

      /* Set clix' PATH variable.  */
      char *old_clixpath = set_clixpath (libdirs, libdirs_count, false, verbose);

      const char **argv =
        (const char **) xmalloca ((2 + nargs + 1) * sizeof (const char *));
      argv[0] = "clix";
      argv[1] = assembly_path_converted;
      for (unsigned int i = 0; i <= nargs; i++)
        argv[2 + i] = args[i];

      if (verbose)
        {
          char *command = shell_quote_argv (argv);
          printf ("%s\n", command);
          free (command);
        }

      bool err = executer ("clix", "clix", argv, private_data);

      freea (argv);

      /* Reset clix' PATH variable.  */
      reset_clixpath (old_clixpath);

      free (assembly_path_converted);

      return err;
    }
  else
    return -1;
}

bool
execute_csharp_program (const char *assembly_path,
                        const char * const *libdirs,
                        unsigned int libdirs_count,
                        const char * const *args,
                        bool verbose, bool quiet,
                        execute_fn *executer, void *private_data)
{
  /* Count args.  */
  unsigned int nargs;
  {
    const char * const *arg;

    for (nargs = 0, arg = args; *arg != NULL; nargs++, arg++)
     ;
  }

  int result;

  /* First try the C# implementation specified through --enable-csharp.  */
#if CSHARP_CHOICE_MONO
  result = execute_csharp_using_mono (assembly_path, libdirs, libdirs_count,
                                      args, nargs, verbose, quiet,
                                      executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif
#if CSHARP_CHOICE_DOTNET
  result = execute_csharp_using_dotnet (assembly_path, libdirs, libdirs_count,
                                        args, nargs, verbose, quiet,
                                        executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

  /* Then try the remaining C# implementations in our standard order.  */
#if !CSHARP_CHOICE_MONO
  result = execute_csharp_using_mono (assembly_path, libdirs, libdirs_count,
                                      args, nargs, verbose, quiet,
                                      executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

#if !CSHARP_CHOICE_DOTNET
  result = execute_csharp_using_dotnet (assembly_path, libdirs, libdirs_count,
                                        args, nargs, verbose, quiet,
                                        executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

  result = execute_csharp_using_sscli (assembly_path, libdirs, libdirs_count,
                                       args, nargs, verbose, quiet,
                                       executer, private_data);
  if (result >= 0)
    return (bool) result;

  if (!quiet)
    error (0, 0, _("C# virtual machine not found, try installing mono or dotnet"));
  return true;
}
