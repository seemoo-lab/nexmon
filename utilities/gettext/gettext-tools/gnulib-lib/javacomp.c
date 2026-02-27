/* Compile a Java program.
   Copyright (C) 2001-2003, 2006-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <alloca.h>

/* Specification.  */
#include "javacomp.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "javaversion.h"
#include "execute.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "classpath.h"
#include "xsetenv.h"
#include "sh-quote.h"
#include "binary-io.h"
#include "safe-read.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "concat-filename.h"
#include "fwriteerror.h"
#include "clean-temp.h"
#include "error.h"
#include "xvasprintf.h"
#include "c-strstr.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Survey of Java compilers.

   A = does it work without CLASSPATH being set
   C = option to set CLASSPATH, other than setting it in the environment
   O = option for optimizing
   g = option for debugging
   T = test for presence

   Program  from        A  C               O  g  T

   $JAVAC   unknown     N  n/a            -O -g  true
   gcj -C   GCC 3.2     Y  --classpath=P  -O -g  gcj --version | sed -e 's,^[^0-9]*,,' -e 1q | sed -e '/^3\.[01]/d' | grep '^[3-9]' >/dev/null
   javac    JDK 1.1.8   Y  -classpath P   -O -g  javac 2>/dev/null; test $? = 1
   javac    JDK 1.3.0   Y  -classpath P   -O -g  javac 2>/dev/null; test $? -le 2
   jikes    Jikes 1.14  N  -classpath P   -O -g  jikes 2>/dev/null; test $? = 1

   All compilers support the option "-d DIRECTORY" for the base directory
   of the classes to be written.

   The CLASSPATH is a colon separated list of pathnames. (On Windows: a
   semicolon separated list of pathnames.)

   We try the Java compilers in the following order:
     1. getenv ("JAVAC"), because the user must be able to override our
        preferences,
     2. "gcj -C", because it is a completely free compiler,
     3. "javac", because it is a standard compiler,
     4. "jikes", comes last because it has some deviating interpretation
        of the Java Language Specification and because it requires a
        CLASSPATH environment variable.

   We unset the JAVA_HOME environment variable, because a wrong setting of
   this variable can confuse the JDK's javac.
 */

/* Return the default target_version.  */
static const char *
default_target_version (void)
{
  /* Use a cache.  Assumes that the PATH environment variable doesn't change
     during the lifetime of the program.  */
  static const char *java_version_cache;
  if (java_version_cache == NULL)
    {
      /* Determine the version from the found JVM.  */
      java_version_cache = javaexec_version ();
      if (java_version_cache == NULL
          || !(java_version_cache[0] == '1' && java_version_cache[1] == '.'
               && (java_version_cache[2] >= '1' && java_version_cache[2] <= '6')
               && java_version_cache[3] == '\0'))
        java_version_cache = "1.1";
    }
  return java_version_cache;
}

/* ======================= Source version dependent ======================= */

/* Convert a source version to an index.  */
#define SOURCE_VERSION_BOUND 3 /* exclusive upper bound */
static unsigned int
source_version_index (const char *source_version)
{
  if (source_version[0] == '1' && source_version[1] == '.'
      && (source_version[2] >= '3' && source_version[2] <= '5')
      && source_version[3] == '\0')
    return source_version[2] - '3';
  error (EXIT_FAILURE, 0, _("invalid source_version argument to compile_java_class"));
  return 0;
}

/* Return a snippet of code that should compile in the given source version.  */
static const char *
get_goodcode_snippet (const char *source_version)
{
  if (strcmp (source_version, "1.3") == 0)
    return "class conftest {}\n";
  if (strcmp (source_version, "1.4") == 0)
    return "class conftest { static { assert(true); } }\n";
  if (strcmp (source_version, "1.5") == 0)
    return "class conftest<T> { T foo() { return null; } }\n";
  error (EXIT_FAILURE, 0, _("invalid source_version argument to compile_java_class"));
  return NULL;
}

/* Return a snippet of code that should fail to compile in the given source
   version, or NULL (standing for a snippet that would fail to compile with
   any compiler).  */
static const char *
get_failcode_snippet (const char *source_version)
{
  if (strcmp (source_version, "1.3") == 0)
    return "class conftestfail { static { assert(true); } }\n";
  if (strcmp (source_version, "1.4") == 0)
    return "class conftestfail<T> { T foo() { return null; } }\n";
  if (strcmp (source_version, "1.5") == 0)
    return NULL;
  error (EXIT_FAILURE, 0, _("invalid source_version argument to compile_java_class"));
  return NULL;
}

/* ======================= Target version dependent ======================= */

/* Convert a target version to an index.  */
#define TARGET_VERSION_BOUND 6 /* exclusive upper bound */
static unsigned int
target_version_index (const char *target_version)
{
  if (target_version[0] == '1' && target_version[1] == '.'
      && (target_version[2] >= '1' && target_version[2] <= '6')
      && target_version[3] == '\0')
    return target_version[2] - '1';
  error (EXIT_FAILURE, 0, _("invalid target_version argument to compile_java_class"));
  return 0;
}

/* Return the class file version number corresponding to a given target
   version.  */
static int
corresponding_classfile_version (const char *target_version)
{
  if (strcmp (target_version, "1.1") == 0)
    return 45;
  if (strcmp (target_version, "1.2") == 0)
    return 46;
  if (strcmp (target_version, "1.3") == 0)
    return 47;
  if (strcmp (target_version, "1.4") == 0)
    return 48;
  if (strcmp (target_version, "1.5") == 0)
    return 49;
  if (strcmp (target_version, "1.6") == 0)
    return 50;
  error (EXIT_FAILURE, 0, _("invalid target_version argument to compile_java_class"));
  return 0;
}

/* ======================== Compilation subroutines ======================== */

/* Try to compile a set of Java sources with $JAVAC.
   Return a failure indicator (true upon error).  */
static bool
compile_using_envjavac (const char *javac,
                        const char * const *java_sources,
                        unsigned int java_sources_count,
                        const char *directory,
                        bool optimize, bool debug,
                        bool verbose, bool null_stderr)
{
  /* Because $JAVAC may consist of a command and options, we use the
     shell.  Because $JAVAC has been set by the user, we leave all
     environment variables in place, including JAVA_HOME, and we don't
     erase the user's CLASSPATH.  */
  bool err;
  unsigned int command_length;
  char *command;
  char *argv[4];
  int exitstatus;
  unsigned int i;
  char *p;

  command_length = strlen (javac);
  if (optimize)
    command_length += 3;
  if (debug)
    command_length += 3;
  if (directory != NULL)
    command_length += 4 + shell_quote_length (directory);
  for (i = 0; i < java_sources_count; i++)
    command_length += 1 + shell_quote_length (java_sources[i]);
  command_length += 1;

  command = (char *) xmalloca (command_length);
  p = command;
  /* Don't shell_quote $JAVAC, because it may consist of a command
     and options.  */
  memcpy (p, javac, strlen (javac));
  p += strlen (javac);
  if (optimize)
    {
      memcpy (p, " -O", 3);
      p += 3;
    }
  if (debug)
    {
      memcpy (p, " -g", 3);
      p += 3;
    }
  if (directory != NULL)
    {
      memcpy (p, " -d ", 4);
      p += 4;
      p = shell_quote_copy (p, directory);
    }
  for (i = 0; i < java_sources_count; i++)
    {
      *p++ = ' ';
      p = shell_quote_copy (p, java_sources[i]);
    }
  *p++ = '\0';
  /* Ensure command_length was correctly calculated.  */
  if (p - command > command_length)
    abort ();

  if (verbose)
    printf ("%s\n", command);

  argv[0] = "/bin/sh";
  argv[1] = "-c";
  argv[2] = command;
  argv[3] = NULL;
  exitstatus = execute (javac, "/bin/sh", argv, false, false, false,
                        null_stderr, true, true, NULL);
  err = (exitstatus != 0);

  freea (command);

  return err;
}

/* Try to compile a set of Java sources with gcj.
   Return a failure indicator (true upon error).  */
static bool
compile_using_gcj (const char * const *java_sources,
                   unsigned int java_sources_count,
                   bool no_assert_option,
                   bool fsource_option, const char *source_version,
                   bool ftarget_option, const char *target_version,
                   const char *directory,
                   bool optimize, bool debug,
                   bool verbose, bool null_stderr)
{
  bool err;
  unsigned int argc;
  char **argv;
  char **argp;
  char *fsource_arg;
  char *ftarget_arg;
  int exitstatus;
  unsigned int i;

  argc =
    2 + (no_assert_option ? 1 : 0) + (fsource_option ? 1 : 0)
    + (ftarget_option ? 1 : 0) + (optimize ? 1 : 0) + (debug ? 1 : 0)
    + (directory != NULL ? 2 : 0) + java_sources_count;
  argv = (char **) xmalloca ((argc + 1) * sizeof (char *));

  argp = argv;
  *argp++ = "gcj";
  *argp++ = "-C";
  if (no_assert_option)
    *argp++ = "-fno-assert";
  if (fsource_option)
    {
      fsource_arg = (char *) xmalloca (9 + strlen (source_version) + 1);
      memcpy (fsource_arg, "-fsource=", 9);
      strcpy (fsource_arg + 9, source_version);
      *argp++ = fsource_arg;
    }
  else
    fsource_arg = NULL;
  if (ftarget_option)
    {
      ftarget_arg = (char *) xmalloca (9 + strlen (target_version) + 1);
      memcpy (ftarget_arg, "-ftarget=", 9);
      strcpy (ftarget_arg + 9, target_version);
      *argp++ = ftarget_arg;
    }
  else
    ftarget_arg = NULL;
  if (optimize)
    *argp++ = "-O";
  if (debug)
    *argp++ = "-g";
  if (directory != NULL)
    {
      *argp++ = "-d";
      *argp++ = (char *) directory;
    }
  for (i = 0; i < java_sources_count; i++)
    *argp++ = (char *) java_sources[i];
  *argp = NULL;
  /* Ensure argv length was correctly calculated.  */
  if (argp - argv != argc)
    abort ();

  if (verbose)
    {
      char *command = shell_quote_argv (argv);
      printf ("%s\n", command);
      free (command);
    }

  exitstatus = execute ("gcj", "gcj", argv, false, false, false, null_stderr,
                        true, true, NULL);
  err = (exitstatus != 0);

  if (ftarget_arg != NULL)
    freea (ftarget_arg);
  if (fsource_arg != NULL)
    freea (fsource_arg);
  freea (argv);

  return err;
}

/* Try to compile a set of Java sources with javac.
   Return a failure indicator (true upon error).  */
static bool
compile_using_javac (const char * const *java_sources,
                     unsigned int java_sources_count,
                     bool source_option, const char *source_version,
                     bool target_option, const char *target_version,
                     const char *directory,
                     bool optimize, bool debug,
                     bool verbose, bool null_stderr)
{
  bool err;
  unsigned int argc;
  char **argv;
  char **argp;
  int exitstatus;
  unsigned int i;

  argc =
    1 + (source_option ? 2 : 0) + (target_option ? 2 : 0) + (optimize ? 1 : 0)
    + (debug ? 1 : 0) + (directory != NULL ? 2 : 0) + java_sources_count;
  argv = (char **) xmalloca ((argc + 1) * sizeof (char *));

  argp = argv;
  *argp++ = "javac";
  if (source_option)
    {
      *argp++ = "-source";
      *argp++ = (char *) source_version;
    }
  if (target_option)
    {
      *argp++ = "-target";
      *argp++ = (char *) target_version;
    }
  if (optimize)
    *argp++ = "-O";
  if (debug)
    *argp++ = "-g";
  if (directory != NULL)
    {
      *argp++ = "-d";
      *argp++ = (char *) directory;
    }
  for (i = 0; i < java_sources_count; i++)
    *argp++ = (char *) java_sources[i];
  *argp = NULL;
  /* Ensure argv length was correctly calculated.  */
  if (argp - argv != argc)
    abort ();

  if (verbose)
    {
      char *command = shell_quote_argv (argv);
      printf ("%s\n", command);
      free (command);
    }

  exitstatus = execute ("javac", "javac", argv, false, false, false,
                        null_stderr, true, true, NULL);
  err = (exitstatus != 0);

  freea (argv);

  return err;
}

/* Try to compile a set of Java sources with jikes.
   Return a failure indicator (true upon error).  */
static bool
compile_using_jikes (const char * const *java_sources,
                     unsigned int java_sources_count,
                     const char *directory,
                     bool optimize, bool debug,
                     bool verbose, bool null_stderr)
{
  bool err;
  unsigned int argc;
  char **argv;
  char **argp;
  int exitstatus;
  unsigned int i;

  argc =
    1 + (optimize ? 1 : 0) + (debug ? 1 : 0) + (directory != NULL ? 2 : 0)
    + java_sources_count;
  argv = (char **) xmalloca ((argc + 1) * sizeof (char *));

  argp = argv;
  *argp++ = "jikes";
  if (optimize)
    *argp++ = "-O";
  if (debug)
    *argp++ = "-g";
  if (directory != NULL)
    {
      *argp++ = "-d";
      *argp++ = (char *) directory;
    }
  for (i = 0; i < java_sources_count; i++)
    *argp++ = (char *) java_sources[i];
  *argp = NULL;
  /* Ensure argv length was correctly calculated.  */
  if (argp - argv != argc)
    abort ();

  if (verbose)
    {
      char *command = shell_quote_argv (argv);
      printf ("%s\n", command);
      free (command);
    }

  exitstatus = execute ("jikes", "jikes", argv, false, false, false,
                        null_stderr, true, true, NULL);
  err = (exitstatus != 0);

  freea (argv);

  return err;
}

/* ====================== Usability test subroutines ====================== */

/* Write a given contents to a temporary file.
   FILE_NAME is the name of a file inside TMPDIR that is known not to exist
   yet.
   Return a failure indicator (true upon error).  */
static bool
write_temp_file (struct temp_dir *tmpdir, const char *file_name,
                 const char *contents)
{
  FILE *fp;

  register_temp_file (tmpdir, file_name);
  fp = fopen_temp (file_name, "w");
  if (fp == NULL)
    {
      error (0, errno, _("failed to create \"%s\""), file_name);
      unregister_temp_file (tmpdir, file_name);
      return true;
    }
  fputs (contents, fp);
  if (fwriteerror_temp (fp))
    {
      error (0, errno, _("error while writing \"%s\" file"), file_name);
      return true;
    }
  return false;
}

/* Return the class file version number of a class file on disk.  */
static int
get_classfile_version (const char *compiled_file_name)
{
  unsigned char header[8];
  int fd;

  /* Open the class file.  */
  fd = open (compiled_file_name, O_RDONLY | O_BINARY, 0);
  if (fd >= 0)
    {
      /* Read its first 8 bytes.  */
      if (safe_read (fd, header, 8) == 8)
        {
          /* Verify the class file signature.  */
          if (header[0] == 0xCA && header[1] == 0xFE
              && header[2] == 0xBA && header[3] == 0xBE)
            return header[7];
        }
      close (fd);
    }

  /* Could not get the class file version.  Return a very large one.  */
  return INT_MAX;
}

/* Return true if $JAVAC is a version of gcj.  */
static bool
is_envjavac_gcj (const char *javac)
{
  static bool envjavac_tested;
  static bool envjavac_gcj;

  if (!envjavac_tested)
    {
      /* Test whether $JAVAC is gcj:
         "$JAVAC --version 2>/dev/null | sed -e 1q | grep gcj > /dev/null"  */
      unsigned int command_length;
      char *command;
      char *argv[4];
      pid_t child;
      int fd[1];
      FILE *fp;
      char *line;
      size_t linesize;
      size_t linelen;
      int exitstatus;
      char *p;

      /* Setup the command "$JAVAC --version".  */
      command_length = strlen (javac) + 1 + 9 + 1;
      command = (char *) xmalloca (command_length);
      p = command;
      /* Don't shell_quote $JAVAC, because it may consist of a command
         and options.  */
      memcpy (p, javac, strlen (javac));
      p += strlen (javac);
      memcpy (p, " --version", 1 + 9 + 1);
      p += 1 + 9 + 1;
      /* Ensure command_length was correctly calculated.  */
      if (p - command > command_length)
        abort ();

      /* Call $JAVAC --version 2>/dev/null.  */
      argv[0] = "/bin/sh";
      argv[1] = "-c";
      argv[2] = command;
      argv[3] = NULL;
      child = create_pipe_in (javac, "/bin/sh", argv, DEV_NULL, true, true,
                              false, fd);
      if (child == -1)
        goto failed;

      /* Retrieve its result.  */
      fp = fdopen (fd[0], "r");
      if (fp == NULL)
        goto failed;

      line = NULL; linesize = 0;
      linelen = getline (&line, &linesize, fp);
      if (linelen == (size_t)(-1))
        {
          fclose (fp);
          goto failed;
        }
      /* It is safe to call c_strstr() instead of strstr() here; see the
         comments in c-strstr.h.  */
      envjavac_gcj = (c_strstr (line, "gcj") != NULL);

      fclose (fp);

      /* Remove zombie process from process list, and retrieve exit status.  */
      exitstatus =
        wait_subprocess (child, javac, true, true, true, false, NULL);
      if (exitstatus != 0)
        envjavac_gcj = false;

     failed:
      freea (command);

      envjavac_tested = true;
    }

  return envjavac_gcj;
}

/* Return true if $JAVAC, known to be a version of gcj, is a version >= 4.3
   of gcj.  */
static bool
is_envjavac_gcj43 (const char *javac)
{
  static bool envjavac_tested;
  static bool envjavac_gcj43;

  if (!envjavac_tested)
    {
      /* Test whether $JAVAC is gcj:
         "$JAVAC --version 2>/dev/null | sed -e 's,^[^0-9]*,,' -e 1q \
          | sed -e '/^4\.[012]/d' | grep '^[4-9]' >/dev/null"  */
      unsigned int command_length;
      char *command;
      char *argv[4];
      pid_t child;
      int fd[1];
      FILE *fp;
      char *line;
      size_t linesize;
      size_t linelen;
      int exitstatus;
      char *p;

      /* Setup the command "$JAVAC --version".  */
      command_length = strlen (javac) + 1 + 9 + 1;
      command = (char *) xmalloca (command_length);
      p = command;
      /* Don't shell_quote $JAVAC, because it may consist of a command
         and options.  */
      memcpy (p, javac, strlen (javac));
      p += strlen (javac);
      memcpy (p, " --version", 1 + 9 + 1);
      p += 1 + 9 + 1;
      /* Ensure command_length was correctly calculated.  */
      if (p - command > command_length)
        abort ();

      /* Call $JAVAC --version 2>/dev/null.  */
      argv[0] = "/bin/sh";
      argv[1] = "-c";
      argv[2] = command;
      argv[3] = NULL;
      child = create_pipe_in (javac, "/bin/sh", argv, DEV_NULL, true, true,
                              false, fd);
      if (child == -1)
        goto failed;

      /* Retrieve its result.  */
      fp = fdopen (fd[0], "r");
      if (fp == NULL)
        goto failed;

      line = NULL; linesize = 0;
      linelen = getline (&line, &linesize, fp);
      if (linelen == (size_t)(-1))
        {
          fclose (fp);
          goto failed;
        }
      p = line;
      while (*p != '\0' && !(*p >= '0' && *p <= '9'))
        p++;
      envjavac_gcj43 =
        !(*p == '4' && p[1] == '.' && p[2] >= '0' && p[2] <= '2')
        && (*p >= '4' && *p <= '9');

      fclose (fp);

      /* Remove zombie process from process list, and retrieve exit status.  */
      exitstatus =
        wait_subprocess (child, javac, true, true, true, false, NULL);
      if (exitstatus != 0)
        envjavac_gcj43 = false;

     failed:
      freea (command);

      envjavac_tested = true;
    }

  return envjavac_gcj43;
}

/* Test whether $JAVAC, known to be a version of gcj >= 4.3, can be used, and
   whether it needs a -fsource and/or -ftarget option.
   Return a failure indicator (true upon error).  */
static bool
is_envjavac_gcj43_usable (const char *javac,
                          const char *source_version,
                          const char *target_version,
                          bool *usablep,
                          bool *fsource_option_p, bool *ftarget_option_p)
{
  /* The cache depends on the source_version and target_version.  */
  struct result_t
  {
    bool tested;
    bool usable;
    bool fsource_option;
    bool ftarget_option;
  };
  static struct result_t result_cache[SOURCE_VERSION_BOUND][TARGET_VERSION_BOUND];
  struct result_t *resultp;

  resultp = &result_cache[source_version_index (source_version)]
                         [target_version_index (target_version)];
  if (!resultp->tested)
    {
      /* Try $JAVAC.  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet (source_version)))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_envjavac (javac,
                                   java_sources, 1, tmpdir->dir_name,
                                   false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0
          && get_classfile_version (compiled_file_name)
             <= corresponding_classfile_version (target_version))
        {
          /* $JAVAC compiled conftest.java successfully.  */
          /* Try adding -fsource option if it is useful.  */
          char *javac_source =
            xasprintf ("%s -fsource=%s", javac, source_version);

          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_envjavac (javac_source,
                                       java_sources, 1, tmpdir->dir_name,
                                       false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              const char *failcode = get_failcode_snippet (source_version);

              if (failcode != NULL)
                {
                  free (compiled_file_name);
                  free (conftest_file_name);

                  conftest_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.java",
                                            NULL);
                  if (write_temp_file (tmpdir, conftest_file_name, failcode))
                    {
                      free (conftest_file_name);
                      free (javac_source);
                      cleanup_temp_dir (tmpdir);
                      return true;
                    }

                  compiled_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.class",
                                            NULL);
                  register_temp_file (tmpdir, compiled_file_name);

                  java_sources[0] = conftest_file_name;
                  if (!compile_using_envjavac (javac,
                                               java_sources, 1,
                                               tmpdir->dir_name,
                                               false, false, false, true)
                      && stat (compiled_file_name, &statbuf) >= 0)
                    {
                      unlink (compiled_file_name);

                      java_sources[0] = conftest_file_name;
                      if (compile_using_envjavac (javac_source,
                                                  java_sources, 1,
                                                  tmpdir->dir_name,
                                                  false, false, false, true))
                        /* $JAVAC compiled conftestfail.java successfully, and
                           "$JAVAC -fsource=$source_version" rejects it.  So
                           the -fsource option is useful.  */
                        resultp->fsource_option = true;
                    }
                }
            }

          free (javac_source);

          resultp->usable = true;
        }
      else
        {
          /* Try with -fsource and -ftarget options.  */
          char *javac_target =
            xasprintf ("%s -fsource=%s -ftarget=%s",
                       javac, source_version, target_version);

          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_envjavac (javac_target,
                                       java_sources, 1, tmpdir->dir_name,
                                       false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              /* "$JAVAC -fsource $source_version -ftarget $target_version"
                 compiled conftest.java successfully.  */
              resultp->fsource_option = true;
              resultp->ftarget_option = true;
              resultp->usable = true;
            }

          free (javac_target);
        }

      free (compiled_file_name);
      free (conftest_file_name);

      resultp->tested = true;
    }

  *usablep = resultp->usable;
  *fsource_option_p = resultp->fsource_option;
  *ftarget_option_p = resultp->ftarget_option;
  return false;
}

/* Test whether $JAVAC, known to be a version of gcj < 4.3, can be used for
   compiling with target_version = 1.4 and source_version = 1.4.
   Return a failure indicator (true upon error).  */
static bool
is_envjavac_oldgcj_14_14_usable (const char *javac, bool *usablep)
{
  static bool envjavac_tested;
  static bool envjavac_usable;

  if (!envjavac_tested)
    {
      /* Try $JAVAC.  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet ("1.4")))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_envjavac (javac, java_sources, 1, tmpdir->dir_name,
                                   false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0)
        /* Compilation succeeded.  */
        envjavac_usable = true;

      free (compiled_file_name);
      free (conftest_file_name);

      cleanup_temp_dir (tmpdir);

      envjavac_tested = true;
    }

  *usablep = envjavac_usable;
  return false;
}

/* Test whether $JAVAC, known to be a version of gcj < 4.3, can be used for
   compiling with target_version = 1.4 and source_version = 1.3.
   Return a failure indicator (true upon error).  */
static bool
is_envjavac_oldgcj_14_13_usable (const char *javac,
                                 bool *usablep, bool *need_no_assert_option_p)
{
  static bool envjavac_tested;
  static bool envjavac_usable;
  static bool envjavac_need_no_assert_option;

  if (!envjavac_tested)
    {
      /* Try $JAVAC and "$JAVAC -fno-assert".  But add -fno-assert only if
         it makes a difference.  (It could already be part of $JAVAC.)  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;
      bool javac_works;
      char *javac_noassert;
      bool javac_noassert_works;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet ("1.3")))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_envjavac (javac,
                                   java_sources, 1, tmpdir->dir_name,
                                   false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0)
        /* Compilation succeeded.  */
        javac_works = true;
      else
        javac_works = false;

      unlink (compiled_file_name);

      javac_noassert = xasprintf ("%s -fno-assert", javac);

      java_sources[0] = conftest_file_name;
      if (!compile_using_envjavac (javac_noassert,
                                   java_sources, 1, tmpdir->dir_name,
                                   false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0)
        /* Compilation succeeded.  */
        javac_noassert_works = true;
      else
        javac_noassert_works = false;

      free (compiled_file_name);
      free (conftest_file_name);

      if (javac_works && javac_noassert_works)
        {
          conftest_file_name =
            xconcatenated_filename (tmpdir->dir_name, "conftestfail.java",
                                    NULL);
          if (write_temp_file (tmpdir, conftest_file_name,
                               get_failcode_snippet ("1.3")))
            {
              free (conftest_file_name);
              free (javac_noassert);
              cleanup_temp_dir (tmpdir);
              return true;
            }

          compiled_file_name =
            xconcatenated_filename (tmpdir->dir_name, "conftestfail.class",
                                    NULL);
          register_temp_file (tmpdir, compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_envjavac (javac,
                                       java_sources, 1, tmpdir->dir_name,
                                       false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0)
            {
              /* Compilation succeeded.  */
              unlink (compiled_file_name);

              java_sources[0] = conftest_file_name;
              if (!(!compile_using_envjavac (javac_noassert,
                                             java_sources, 1, tmpdir->dir_name,
                                             false, false, false, true)
                    && stat (compiled_file_name, &statbuf) >= 0))
                /* Compilation failed.  */
                /* "$JAVAC -fno-assert" works better than $JAVAC.  */
                javac_works = true;
            }

          free (compiled_file_name);
          free (conftest_file_name);
        }

      cleanup_temp_dir (tmpdir);

      if (javac_works)
        {
          envjavac_usable = true;
          envjavac_need_no_assert_option = false;
        }
      else if (javac_noassert_works)
        {
          envjavac_usable = true;
          envjavac_need_no_assert_option = true;
        }

      envjavac_tested = true;
    }

  *usablep = envjavac_usable;
  *need_no_assert_option_p = envjavac_need_no_assert_option;
  return false;
}

/* Test whether $JAVAC, known to be not a version of gcj, can be used, and
   whether it needs a -source and/or -target option.
   Return a failure indicator (true upon error).  */
static bool
is_envjavac_nongcj_usable (const char *javac,
                           const char *source_version,
                           const char *target_version,
                           bool *usablep,
                           bool *source_option_p, bool *target_option_p)
{
  /* The cache depends on the source_version and target_version.  */
  struct result_t
  {
    bool tested;
    bool usable;
    bool source_option;
    bool target_option;
  };
  static struct result_t result_cache[SOURCE_VERSION_BOUND][TARGET_VERSION_BOUND];
  struct result_t *resultp;

  resultp = &result_cache[source_version_index (source_version)]
                         [target_version_index (target_version)];
  if (!resultp->tested)
    {
      /* Try $JAVAC.  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet (source_version)))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_envjavac (javac,
                                   java_sources, 1, tmpdir->dir_name,
                                   false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0
          && get_classfile_version (compiled_file_name)
             <= corresponding_classfile_version (target_version))
        {
          /* $JAVAC compiled conftest.java successfully.  */
          /* Try adding -source option if it is useful.  */
          char *javac_source =
            xasprintf ("%s -source %s", javac, source_version);

          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_envjavac (javac_source,
                                       java_sources, 1, tmpdir->dir_name,
                                       false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              const char *failcode = get_failcode_snippet (source_version);

              if (failcode != NULL)
                {
                  free (compiled_file_name);
                  free (conftest_file_name);

                  conftest_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.java",
                                            NULL);
                  if (write_temp_file (tmpdir, conftest_file_name, failcode))
                    {
                      free (conftest_file_name);
                      free (javac_source);
                      cleanup_temp_dir (tmpdir);
                      return true;
                    }

                  compiled_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.class",
                                            NULL);
                  register_temp_file (tmpdir, compiled_file_name);

                  java_sources[0] = conftest_file_name;
                  if (!compile_using_envjavac (javac,
                                               java_sources, 1,
                                               tmpdir->dir_name,
                                               false, false, false, true)
                      && stat (compiled_file_name, &statbuf) >= 0)
                    {
                      unlink (compiled_file_name);

                      java_sources[0] = conftest_file_name;
                      if (compile_using_envjavac (javac_source,
                                                  java_sources, 1,
                                                  tmpdir->dir_name,
                                                  false, false, false, true))
                        /* $JAVAC compiled conftestfail.java successfully, and
                           "$JAVAC -source $source_version" rejects it.  So the
                           -source option is useful.  */
                        resultp->source_option = true;
                    }
                }
            }

          free (javac_source);

          resultp->usable = true;
        }
      else
        {
          /* Try with -target option alone. (Sun javac 1.3.1 has the -target
             option but no -source option.)  */
          char *javac_target =
            xasprintf ("%s -target %s", javac, target_version);

          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_envjavac (javac_target,
                                       java_sources, 1, tmpdir->dir_name,
                                       false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              /* "$JAVAC -target $target_version" compiled conftest.java
                 successfully.  */
              /* Try adding -source option if it is useful.  */
              char *javac_target_source =
                xasprintf ("%s -source %s", javac_target, source_version);

              unlink (compiled_file_name);

              java_sources[0] = conftest_file_name;
              if (!compile_using_envjavac (javac_target_source,
                                           java_sources, 1, tmpdir->dir_name,
                                           false, false, false, true)
                  && stat (compiled_file_name, &statbuf) >= 0
                  && get_classfile_version (compiled_file_name)
                     <= corresponding_classfile_version (target_version))
                {
                  const char *failcode = get_failcode_snippet (source_version);

                  if (failcode != NULL)
                    {
                      free (compiled_file_name);
                      free (conftest_file_name);

                      conftest_file_name =
                        xconcatenated_filename (tmpdir->dir_name,
                                                "conftestfail.java",
                                                NULL);
                      if (write_temp_file (tmpdir, conftest_file_name,
                                           failcode))
                        {
                          free (conftest_file_name);
                          free (javac_target_source);
                          free (javac_target);
                          cleanup_temp_dir (tmpdir);
                          return true;
                        }

                      compiled_file_name =
                        xconcatenated_filename (tmpdir->dir_name,
                                                "conftestfail.class",
                                                NULL);
                      register_temp_file (tmpdir, compiled_file_name);

                      java_sources[0] = conftest_file_name;
                      if (!compile_using_envjavac (javac_target,
                                                   java_sources, 1,
                                                   tmpdir->dir_name,
                                                   false, false, false, true)
                          && stat (compiled_file_name, &statbuf) >= 0)
                        {
                          unlink (compiled_file_name);

                          java_sources[0] = conftest_file_name;
                          if (compile_using_envjavac (javac_target_source,
                                                      java_sources, 1,
                                                      tmpdir->dir_name,
                                                      false, false, false,
                                                      true))
                            /* "$JAVAC -target $target_version" compiled
                               conftestfail.java successfully, and
                               "$JAVAC -target $target_version -source $source_version"
                               rejects it.  So the -source option is useful.  */
                            resultp->source_option = true;
                        }
                    }
                }

              free (javac_target_source);

              resultp->target_option = true;
              resultp->usable = true;
            }
          else
            {
              /* Maybe this -target option requires a -source option? Try with
                 -target and -source options. (Supported by Sun javac 1.4 and
                 higher.)  */
              char *javac_target_source =
                xasprintf ("%s -source %s", javac_target, source_version);

              unlink (compiled_file_name);

              java_sources[0] = conftest_file_name;
              if (!compile_using_envjavac (javac_target_source,
                                           java_sources, 1, tmpdir->dir_name,
                                           false, false, false, true)
                  && stat (compiled_file_name, &statbuf) >= 0
                  && get_classfile_version (compiled_file_name)
                     <= corresponding_classfile_version (target_version))
                {
                  /* "$JAVAC -target $target_version -source $source_version"
                     compiled conftest.java successfully.  */
                  resultp->source_option = true;
                  resultp->target_option = true;
                  resultp->usable = true;
                }

              free (javac_target_source);
            }

          free (javac_target);
        }

      free (compiled_file_name);
      free (conftest_file_name);

      resultp->tested = true;
    }

  *usablep = resultp->usable;
  *source_option_p = resultp->source_option;
  *target_option_p = resultp->target_option;
  return false;
}

static bool
is_gcj_present (void)
{
  static bool gcj_tested;
  static bool gcj_present;

  if (!gcj_tested)
    {
      /* Test for presence of gcj:
         "gcj --version 2> /dev/null | \
          sed -e 's,^[^0-9]*,,' -e 1q | \
          sed -e '/^3\.[01]/d' | grep '^[3-9]' > /dev/null"  */
      char *argv[3];
      pid_t child;
      int fd[1];
      int exitstatus;

      argv[0] = "gcj";
      argv[1] = "--version";
      argv[2] = NULL;
      child = create_pipe_in ("gcj", "gcj", argv, DEV_NULL, true, true,
                              false, fd);
      gcj_present = false;
      if (child != -1)
        {
          /* Read the subprocess output, drop all lines except the first,
             drop all characters before the first digit, and test whether
             the remaining string starts with a digit >= 3, but not with
             "3.0" or "3.1".  */
          char c[3];
          size_t count = 0;

          while (safe_read (fd[0], &c[count], 1) > 0)
            {
              if (c[count] == '\n')
                break;
              if (count == 0)
                {
                  if (!(c[0] >= '0' && c[0] <= '9'))
                    continue;
                  gcj_present = (c[0] >= '3');
                }
              count++;
              if (count == 3)
                {
                  if (c[0] == '3' && c[1] == '.'
                      && (c[2] == '0' || c[2] == '1'))
                    gcj_present = false;
                  break;
                }
            }
          while (safe_read (fd[0], &c[0], 1) > 0)
            ;

          close (fd[0]);

          /* Remove zombie process from process list, and retrieve exit
             status.  */
          exitstatus =
            wait_subprocess (child, "gcj", false, true, true, false, NULL);
          if (exitstatus != 0)
            gcj_present = false;
        }

      if (gcj_present)
        {
          /* See if libgcj.jar is well installed.  */
          struct temp_dir *tmpdir;

          tmpdir = create_temp_dir ("java", NULL, false);
          if (tmpdir == NULL)
            gcj_present = false;
          else
            {
              char *conftest_file_name;

              conftest_file_name =
                xconcatenated_filename (tmpdir->dir_name, "conftestlib.java",
                                        NULL);
              if (write_temp_file (tmpdir, conftest_file_name,
"public class conftestlib {\n"
"  public static void main (String[] args) {\n"
"  }\n"
"}\n"))
                gcj_present = false;
              else
                {
                  char *compiled_file_name;
                  const char *java_sources[1];

                  compiled_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestlib.class",
                                            NULL);
                  register_temp_file (tmpdir, compiled_file_name);

                  java_sources[0] = conftest_file_name;
                  if (compile_using_gcj (java_sources, 1, false,
                                         false, NULL, false, NULL,
                                         tmpdir->dir_name,
                                         false, false, false, true))
                    gcj_present = false;

                  free (compiled_file_name);
                }
              free (conftest_file_name);
            }
          cleanup_temp_dir (tmpdir);
        }

      gcj_tested = true;
    }

  return gcj_present;
}

static bool
is_gcj_43 (void)
{
  static bool gcj_tested;
  static bool gcj_43;

  if (!gcj_tested)
    {
      /* Test for presence of gcj:
         "gcj --version 2> /dev/null | \
          sed -e 's,^[^0-9]*,,' -e 1q | \
          sed -e '/^4\.[012]/d' | grep '^[4-9]'"  */
      char *argv[3];
      pid_t child;
      int fd[1];
      int exitstatus;

      argv[0] = "gcj";
      argv[1] = "--version";
      argv[2] = NULL;
      child = create_pipe_in ("gcj", "gcj", argv, DEV_NULL, true, true,
                              false, fd);
      gcj_43 = false;
      if (child != -1)
        {
          /* Read the subprocess output, drop all lines except the first,
             drop all characters before the first digit, and test whether
             the remaining string starts with a digit >= 4, but not with
             "4.0" or "4.1" or "4.2".  */
          char c[3];
          size_t count = 0;

          while (safe_read (fd[0], &c[count], 1) > 0)
            {
              if (c[count] == '\n')
                break;
              if (count == 0)
                {
                  if (!(c[0] >= '0' && c[0] <= '9'))
                    continue;
                  gcj_43 = (c[0] >= '4');
                }
              count++;
              if (count == 3)
                {
                  if (c[0] == '4' && c[1] == '.' && c[2] >= '0' && c[2] <= '2')
                    gcj_43 = false;
                  break;
                }
            }
          while (safe_read (fd[0], &c[0], 1) > 0)
            ;

          close (fd[0]);

          /* Remove zombie process from process list, and retrieve exit
             status.  */
          exitstatus =
            wait_subprocess (child, "gcj", false, true, true, false, NULL);
          if (exitstatus != 0)
            gcj_43 = false;
        }

      gcj_tested = true;
    }

  return gcj_43;
}

/* Test whether gcj >= 4.3 can be used, and whether it needs a -fsource and/or
   -ftarget option.
   Return a failure indicator (true upon error).  */
static bool
is_gcj43_usable (const char *source_version,
                 const char *target_version,
                 bool *usablep,
                 bool *fsource_option_p, bool *ftarget_option_p)
{
  /* The cache depends on the source_version and target_version.  */
  struct result_t
  {
    bool tested;
    bool usable;
    bool fsource_option;
    bool ftarget_option;
  };
  static struct result_t result_cache[SOURCE_VERSION_BOUND][TARGET_VERSION_BOUND];
  struct result_t *resultp;

  resultp = &result_cache[source_version_index (source_version)]
                         [target_version_index (target_version)];
  if (!resultp->tested)
    {
      /* Try gcj.  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet (source_version)))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_gcj (java_sources, 1, false, false, NULL, false, NULL,
                              tmpdir->dir_name, false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0
          && get_classfile_version (compiled_file_name)
             <= corresponding_classfile_version (target_version))
        {
          /* gcj compiled conftest.java successfully.  */
          /* Try adding -fsource option if it is useful.  */
          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_gcj (java_sources, 1,
                                  false, true, source_version, false, NULL,
                                  tmpdir->dir_name, false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              const char *failcode = get_failcode_snippet (source_version);

              if (failcode != NULL)
                {
                  free (compiled_file_name);
                  free (conftest_file_name);

                  conftest_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.java",
                                            NULL);
                  if (write_temp_file (tmpdir, conftest_file_name, failcode))
                    {
                      free (conftest_file_name);
                      cleanup_temp_dir (tmpdir);
                      return true;
                    }

                  compiled_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.class",
                                            NULL);
                  register_temp_file (tmpdir, compiled_file_name);

                  java_sources[0] = conftest_file_name;
                  if (!compile_using_gcj (java_sources, 1,
                                          false, false, NULL, false, NULL,
                                          tmpdir->dir_name,
                                          false, false, false, true)
                      && stat (compiled_file_name, &statbuf) >= 0)
                    {
                      unlink (compiled_file_name);

                      java_sources[0] = conftest_file_name;
                      if (compile_using_gcj (java_sources, 1,
                                             false, true, source_version,
                                             false, NULL,
                                             tmpdir->dir_name,
                                             false, false, false, true))
                        /* gcj compiled conftestfail.java successfully, and
                           "gcj -fsource=$source_version" rejects it.  So
                           the -fsource option is useful.  */
                        resultp->fsource_option = true;
                    }
                }
            }

          resultp->usable = true;
        }
      else
        {
          /* Try with -fsource and -ftarget options.  */
          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_gcj (java_sources, 1,
                                  false, true, source_version,
                                  true, target_version,
                                  tmpdir->dir_name,
                                  false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              /* "gcj -fsource $source_version -ftarget $target_version"
                 compiled conftest.java successfully.  */
              resultp->fsource_option = true;
              resultp->ftarget_option = true;
              resultp->usable = true;
            }
        }

      free (compiled_file_name);
      free (conftest_file_name);

      resultp->tested = true;
    }

  *usablep = resultp->usable;
  *fsource_option_p = resultp->fsource_option;
  *ftarget_option_p = resultp->ftarget_option;
  return false;
}

/* Test whether gcj < 4.3 can be used for compiling with target_version = 1.4
   and source_version = 1.4.
   Return a failure indicator (true upon error).  */
static bool
is_oldgcj_14_14_usable (bool *usablep)
{
  static bool gcj_tested;
  static bool gcj_usable;

  if (!gcj_tested)
    {
      /* Try gcj.  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet ("1.4")))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_gcj (java_sources, 1, false, false, NULL, false, NULL,
                              tmpdir->dir_name, false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0)
        /* Compilation succeeded.  */
        gcj_usable = true;

      free (compiled_file_name);
      free (conftest_file_name);

      cleanup_temp_dir (tmpdir);

      gcj_tested = true;
    }

  *usablep = gcj_usable;
  return false;
}

/* Test whether gcj < 4.3 can be used for compiling with target_version = 1.4
   and source_version = 1.3.
   Return a failure indicator (true upon error).  */
static bool
is_oldgcj_14_13_usable (bool *usablep, bool *need_no_assert_option_p)
{
  static bool gcj_tested;
  static bool gcj_usable;
  static bool gcj_need_no_assert_option;

  if (!gcj_tested)
    {
      /* Try gcj and "gcj -fno-assert".  But add -fno-assert only if
         it works (not gcj < 3.3).  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet ("1.3")))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_gcj (java_sources, 1, true, false, NULL, false, NULL,
                              tmpdir->dir_name, false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0)
        /* Compilation succeeded.  */
        {
          gcj_usable = true;
          gcj_need_no_assert_option = true;
        }
      else
        {
          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_gcj (java_sources, 1, false,
                                  false, NULL, false, NULL,
                                  tmpdir->dir_name, false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0)
            /* Compilation succeeded.  */
            {
              gcj_usable = true;
              gcj_need_no_assert_option = false;
            }
        }

      free (compiled_file_name);
      free (conftest_file_name);

      cleanup_temp_dir (tmpdir);

      gcj_tested = true;
    }

  *usablep = gcj_usable;
  *need_no_assert_option_p = gcj_need_no_assert_option;
  return false;
}

static bool
is_javac_present (void)
{
  static bool javac_tested;
  static bool javac_present;

  if (!javac_tested)
    {
      /* Test for presence of javac: "javac 2> /dev/null ; test $? -le 2"  */
      char *argv[2];
      int exitstatus;

      argv[0] = "javac";
      argv[1] = NULL;
      exitstatus = execute ("javac", "javac", argv, false, false, true, true,
                            true, false, NULL);
      javac_present = (exitstatus == 0 || exitstatus == 1 || exitstatus == 2);
      javac_tested = true;
    }

  return javac_present;
}

/* Test whether javac can be used and whether it needs a -source and/or
   -target option.
   Return a failure indicator (true upon error).  */
static bool
is_javac_usable (const char *source_version, const char *target_version,
                 bool *usablep, bool *source_option_p, bool *target_option_p)
{
  /* The cache depends on the source_version and target_version.  */
  struct result_t
  {
    bool tested;
    bool usable;
    bool source_option;
    bool target_option;
  };
  static struct result_t result_cache[SOURCE_VERSION_BOUND][TARGET_VERSION_BOUND];
  struct result_t *resultp;

  resultp = &result_cache[source_version_index (source_version)]
                         [target_version_index (target_version)];
  if (!resultp->tested)
    {
      /* Try javac.  */
      struct temp_dir *tmpdir;
      char *conftest_file_name;
      char *compiled_file_name;
      const char *java_sources[1];
      struct stat statbuf;

      tmpdir = create_temp_dir ("java", NULL, false);
      if (tmpdir == NULL)
        return true;

      conftest_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.java", NULL);
      if (write_temp_file (tmpdir, conftest_file_name,
                           get_goodcode_snippet (source_version)))
        {
          free (conftest_file_name);
          cleanup_temp_dir (tmpdir);
          return true;
        }

      compiled_file_name =
        xconcatenated_filename (tmpdir->dir_name, "conftest.class", NULL);
      register_temp_file (tmpdir, compiled_file_name);

      java_sources[0] = conftest_file_name;
      if (!compile_using_javac (java_sources, 1,
                                false, source_version,
                                false, target_version,
                                tmpdir->dir_name, false, false, false, true)
          && stat (compiled_file_name, &statbuf) >= 0
          && get_classfile_version (compiled_file_name)
             <= corresponding_classfile_version (target_version))
        {
          /* javac compiled conftest.java successfully.  */
          /* Try adding -source option if it is useful.  */
          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_javac (java_sources, 1,
                                    true, source_version,
                                    false, target_version,
                                    tmpdir->dir_name, false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              const char *failcode = get_failcode_snippet (source_version);

              if (failcode != NULL)
                {
                  free (compiled_file_name);
                  free (conftest_file_name);

                  conftest_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.java",
                                            NULL);
                  if (write_temp_file (tmpdir, conftest_file_name, failcode))
                    {
                      free (conftest_file_name);
                      cleanup_temp_dir (tmpdir);
                      return true;
                    }

                  compiled_file_name =
                    xconcatenated_filename (tmpdir->dir_name,
                                            "conftestfail.class",
                                            NULL);
                  register_temp_file (tmpdir, compiled_file_name);

                  java_sources[0] = conftest_file_name;
                  if (!compile_using_javac (java_sources, 1,
                                            false, source_version,
                                            false, target_version,
                                            tmpdir->dir_name,
                                            false, false, false, true)
                      && stat (compiled_file_name, &statbuf) >= 0)
                    {
                      unlink (compiled_file_name);

                      java_sources[0] = conftest_file_name;
                      if (compile_using_javac (java_sources, 1,
                                               true, source_version,
                                               false, target_version,
                                               tmpdir->dir_name,
                                               false, false, false, true))
                        /* javac compiled conftestfail.java successfully, and
                           "javac -source $source_version" rejects it.  So the
                           -source option is useful.  */
                        resultp->source_option = true;
                    }
                }
            }

          resultp->usable = true;
        }
      else
        {
          /* Try with -target option alone. (Sun javac 1.3.1 has the -target
             option but no -source option.)  */
          unlink (compiled_file_name);

          java_sources[0] = conftest_file_name;
          if (!compile_using_javac (java_sources, 1,
                                    false, source_version,
                                    true, target_version,
                                    tmpdir->dir_name,
                                    false, false, false, true)
              && stat (compiled_file_name, &statbuf) >= 0
              && get_classfile_version (compiled_file_name)
                 <= corresponding_classfile_version (target_version))
            {
              /* "javac -target $target_version" compiled conftest.java
                 successfully.  */
              /* Try adding -source option if it is useful.  */
              unlink (compiled_file_name);

              java_sources[0] = conftest_file_name;
              if (!compile_using_javac (java_sources, 1,
                                        true, source_version,
                                        true, target_version,
                                        tmpdir->dir_name,
                                        false, false, false, true)
                  && stat (compiled_file_name, &statbuf) >= 0
                  && get_classfile_version (compiled_file_name)
                     <= corresponding_classfile_version (target_version))
                {
                  const char *failcode = get_failcode_snippet (source_version);

                  if (failcode != NULL)
                    {
                      free (compiled_file_name);
                      free (conftest_file_name);

                      conftest_file_name =
                        xconcatenated_filename (tmpdir->dir_name,
                                                "conftestfail.java",
                                                NULL);
                      if (write_temp_file (tmpdir, conftest_file_name,
                                           failcode))
                        {
                          free (conftest_file_name);
                          cleanup_temp_dir (tmpdir);
                          return true;
                        }

                      compiled_file_name =
                        xconcatenated_filename (tmpdir->dir_name,
                                                "conftestfail.class",
                                                NULL);
                      register_temp_file (tmpdir, compiled_file_name);

                      java_sources[0] = conftest_file_name;
                      if (!compile_using_javac (java_sources, 1,
                                                false, source_version,
                                                true, target_version,
                                                tmpdir->dir_name,
                                                false, false, false, true)
                          && stat (compiled_file_name, &statbuf) >= 0)
                        {
                          unlink (compiled_file_name);

                          java_sources[0] = conftest_file_name;
                          if (compile_using_javac (java_sources, 1,
                                                   true, source_version,
                                                   true, target_version,
                                                   tmpdir->dir_name,
                                                   false, false, false, true))
                            /* "javac -target $target_version" compiled
                               conftestfail.java successfully, and
                               "javac -target $target_version -source $source_version"
                               rejects it.  So the -source option is useful.  */
                            resultp->source_option = true;
                        }
                    }
                }

              resultp->target_option = true;
              resultp->usable = true;
            }
          else
            {
              /* Maybe this -target option requires a -source option? Try with
                 -target and -source options. (Supported by Sun javac 1.4 and
                 higher.)  */
              unlink (compiled_file_name);

              java_sources[0] = conftest_file_name;
              if (!compile_using_javac (java_sources, 1,
                                        true, source_version,
                                        true, target_version,
                                        tmpdir->dir_name,
                                        false, false, false, true)
                  && stat (compiled_file_name, &statbuf) >= 0
                  && get_classfile_version (compiled_file_name)
                     <= corresponding_classfile_version (target_version))
                {
                  /* "javac -target $target_version -source $source_version"
                     compiled conftest.java successfully.  */
                  resultp->source_option = true;
                  resultp->target_option = true;
                  resultp->usable = true;
                }
            }
        }

      free (compiled_file_name);
      free (conftest_file_name);

      resultp->tested = true;
    }

  *usablep = resultp->usable;
  *source_option_p = resultp->source_option;
  *target_option_p = resultp->target_option;
  return false;
}

static bool
is_jikes_present (void)
{
  static bool jikes_tested;
  static bool jikes_present;

  if (!jikes_tested)
    {
      /* Test for presence of jikes: "jikes 2> /dev/null ; test $? = 1"  */
      char *argv[2];
      int exitstatus;

      argv[0] = "jikes";
      argv[1] = NULL;
      exitstatus = execute ("jikes", "jikes", argv, false, false, true, true,
                            true, false, NULL);
      jikes_present = (exitstatus == 0 || exitstatus == 1);
      jikes_tested = true;
    }

  return jikes_present;
}

/* ============================= Main function ============================= */

bool
compile_java_class (const char * const *java_sources,
                    unsigned int java_sources_count,
                    const char * const *classpaths,
                    unsigned int classpaths_count,
                    const char *source_version,
                    const char *target_version,
                    const char *directory,
                    bool optimize, bool debug,
                    bool use_minimal_classpath,
                    bool verbose)
{
  bool err = false;
  char *old_JAVA_HOME;

  {
    const char *javac = getenv ("JAVAC");
    if (javac != NULL && javac[0] != '\0')
      {
        bool usable = false;
        bool no_assert_option = false;
        bool source_option = false;
        bool target_option = false;
        bool fsource_option = false;
        bool ftarget_option = false;

        if (target_version == NULL)
          target_version = default_target_version ();

        if (is_envjavac_gcj (javac))
          {
            /* It's a version of gcj.  */
            if (is_envjavac_gcj43 (javac))
              {
                /* It's a version of gcj >= 4.3.  Assume the classfile versions
                   are correct.  */
                if (is_envjavac_gcj43_usable (javac,
                                              source_version, target_version,
                                              &usable,
                                              &fsource_option, &ftarget_option))
                  {
                    err = true;
                    goto done1;
                  }
              }
            else
              {
                /* It's a version of gcj < 4.3.  Ignore the version of the
                   class files that it creates.  */
                if (strcmp (target_version, "1.4") == 0
                    && strcmp (source_version, "1.4") == 0)
                  {
                    if (is_envjavac_oldgcj_14_14_usable (javac, &usable))
                      {
                        err = true;
                        goto done1;
                      }
                  }
                else if (strcmp (target_version, "1.4") == 0
                         && strcmp (source_version, "1.3") == 0)
                  {
                    if (is_envjavac_oldgcj_14_13_usable (javac,
                                                         &usable,
                                                         &no_assert_option))
                      {
                        err = true;
                        goto done1;
                      }
                  }
              }
          }
        else
          {
            /* It's not gcj.  Assume the classfile versions are correct.  */
            if (is_envjavac_nongcj_usable (javac,
                                           source_version, target_version,
                                           &usable,
                                           &source_option, &target_option))
              {
                err = true;
                goto done1;
              }
          }

        if (usable)
          {
            char *old_classpath;
            char *javac_with_options;

            /* Set CLASSPATH.  */
            old_classpath =
              set_classpath (classpaths, classpaths_count, false, verbose);

            javac_with_options =
              (no_assert_option
               ? xasprintf ("%s -fno-assert", javac)
               : xasprintf ("%s%s%s%s%s%s%s%s%s",
                            javac,
                            source_option ? " -source " : "",
                            source_option ? source_version : "",
                            target_option ? " -target " : "",
                            target_option ? target_version : "",
                            fsource_option ? " -fsource=" : "",
                            fsource_option ? source_version : "",
                            ftarget_option ? " -ftarget=" : "",
                            ftarget_option ? target_version : ""));

            err = compile_using_envjavac (javac_with_options,
                                          java_sources, java_sources_count,
                                          directory, optimize, debug, verbose,
                                          false);

            free (javac_with_options);

            /* Reset CLASSPATH.  */
            reset_classpath (old_classpath);

            goto done1;
          }
      }
  }

  /* Unset the JAVA_HOME environment variable.  */
  old_JAVA_HOME = getenv ("JAVA_HOME");
  if (old_JAVA_HOME != NULL)
    {
      old_JAVA_HOME = xstrdup (old_JAVA_HOME);
      unsetenv ("JAVA_HOME");
    }

  if (is_gcj_present ())
    {
      /* It's a version of gcj.  */
      bool usable = false;
      bool no_assert_option = false;
      bool fsource_option = false;
      bool ftarget_option = false;

      if (target_version == NULL)
        target_version = default_target_version ();

      if (is_gcj_43 ())
        {
          /* It's a version of gcj >= 4.3.  Assume the classfile versions
             are correct.  */
          if (is_gcj43_usable (source_version, target_version,
                               &usable, &fsource_option, &ftarget_option))
            {
              err = true;
              goto done1;
            }
        }
      else
        {
          /* It's a version of gcj < 4.3.  Ignore the version of the class
             files that it creates.
             Test whether it supports the desired target-version and
             source-version.  */
          if (strcmp (target_version, "1.4") == 0
              && strcmp (source_version, "1.4") == 0)
            {
              if (is_oldgcj_14_14_usable (&usable))
                {
                  err = true;
                  goto done1;
                }
            }
          else if (strcmp (target_version, "1.4") == 0
                   && strcmp (source_version, "1.3") == 0)
            {
              if (is_oldgcj_14_13_usable (&usable, &no_assert_option))
                {
                  err = true;
                  goto done1;
                }
            }
        }

      if (usable)
        {
          char *old_classpath;

          /* Set CLASSPATH.  We could also use the --CLASSPATH=... option
             of gcj.  Note that --classpath=... option is different: its
             argument should also contain gcj's libgcj.jar, but we don't
             know its location.  */
          old_classpath =
            set_classpath (classpaths, classpaths_count, use_minimal_classpath,
                           verbose);

          err = compile_using_gcj (java_sources, java_sources_count,
                                   no_assert_option,
                                   fsource_option, source_version,
                                   ftarget_option, target_version,
                                   directory, optimize, debug, verbose, false);

          /* Reset CLASSPATH.  */
          reset_classpath (old_classpath);

          goto done2;
        }
    }

  if (is_javac_present ())
    {
      bool usable = false;
      bool source_option = false;
      bool target_option = false;

      if (target_version == NULL)
        target_version = default_target_version ();

      if (is_javac_usable (source_version, target_version,
                           &usable, &source_option, &target_option))
        {
          err = true;
          goto done1;
        }

      if (usable)
        {
          char *old_classpath;

          /* Set CLASSPATH.  We don't use the "-classpath ..." option because
             in JDK 1.1.x its argument should also contain the JDK's
             classes.zip, but we don't know its location.  (In JDK 1.3.0 it
             would work.)  */
          old_classpath =
            set_classpath (classpaths, classpaths_count, use_minimal_classpath,
                           verbose);

          err = compile_using_javac (java_sources, java_sources_count,
                                     source_option, source_version,
                                     target_option, target_version,
                                     directory, optimize, debug, verbose,
                                     false);

          /* Reset CLASSPATH.  */
          reset_classpath (old_classpath);

          goto done2;
        }
    }

  if (is_jikes_present ())
    {
      /* Test whether it supports the desired target-version and
         source-version.  */
      bool usable = (strcmp (source_version, "1.3") == 0);

      if (usable)
        {
          char *old_classpath;

          /* Set CLASSPATH.  We could also use the "-classpath ..." option.
             Since jikes doesn't come with its own standard library, it
             needs a classes.zip or rt.jar or libgcj.jar in the CLASSPATH.
             To increase the chance of success, we reuse the current CLASSPATH
             if the user has set it.  */
          old_classpath =
            set_classpath (classpaths, classpaths_count, false, verbose);

          err = compile_using_jikes (java_sources, java_sources_count,
                                     directory, optimize, debug, verbose,
                                     false);

          /* Reset CLASSPATH.  */
          reset_classpath (old_classpath);

          goto done2;
        }
    }

  error (0, 0, _("Java compiler not found, try installing gcj or set $JAVAC"));
  err = true;

 done2:
  if (old_JAVA_HOME != NULL)
    {
      xsetenv ("JAVA_HOME", old_JAVA_HOME, 1);
      free (old_JAVA_HOME);
    }

 done1:
  return err;
}
