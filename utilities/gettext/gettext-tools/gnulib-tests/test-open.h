/* Test of opening a file descriptor.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

/* Tell GCC not to warn about the specific edge cases tested here.  */
#if _GL_GNUC_PREREQ (13, 0)
# pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
#endif

/* Make test_open always inline if we're using Fortify, which defines
   __always_inline to do that.  Do nothing otherwise.  This works
   around a glibc bug whereby 'open' cannot be used as a function
   pointer when _FORTIFY_SOURCE is positive.  */

#if __GLIBC__ && defined __always_inline
# define ALWAYS_INLINE __always_inline
#else
# define ALWAYS_INLINE
#endif

/* This file is designed to test open(n,buf[,mode]),
   openat(dfd,n,buf[,mode]), and openat2(dfd,n,how,size).
   FUNC is the function to test; for openat and openat2 it is a wrapper.
   Assumes that BASE and ASSERT are already defined, and that
   appropriate headers are already included.  If PRINT, warn before
   skipping symlink tests with status 77.  */

static ALWAYS_INLINE int
test_open (int (*func) (char const *, int, ...), bool print)
{
#if HAVE_DECL_ALARM
  /* Declare failure if test takes too long, by using default abort
     caused by SIGALRM.  */
  int alarm_value = 5;
  signal (SIGALRM, SIG_DFL);
  alarm (alarm_value);
#endif

  int fd;

  /* Remove anything from prior partial run.  */
  unlink (BASE "fifo");
  unlink (BASE "file");
  unlink (BASE "e.exe");
  unlink (BASE "link");

  /* Cannot create directory.  */
  errno = 0;
  ASSERT (func ("nonexist.ent/", O_CREAT | O_RDONLY, 0600) == -1);
  ASSERT (errno == ENOTDIR || errno == EISDIR || errno == ENOENT
          || errno == EINVAL);

  /* Create a regular file.  */
  fd = func (BASE "file", O_CREAT | O_RDONLY, 0600);
  ASSERT (0 <= fd);
  ASSERT (close (fd) == 0);

  /* Create an executable regular file.  */
  fd = func (BASE "e.exe", O_CREAT | O_RDONLY, 0700);
  ASSERT (0 <= fd);
  ASSERT (close (fd) == 0);

  /* Trailing slash handling.  */
  errno = 0;
  ASSERT (func (BASE "file/", O_RDONLY) == -1);
  ASSERT (errno == ENOTDIR || errno == EISDIR || errno == EINVAL);

  /* Cannot open regular file with O_DIRECTORY.  */
  errno = 0;
  ASSERT (func (BASE "file", O_RDONLY | O_DIRECTORY) == -1);
  ASSERT (errno == ENOTDIR);

  /* Cannot open /dev/null with trailing slash or O_DIRECTORY.  */
  errno = 0;
  ASSERT (func ("/dev/null/", O_RDONLY) == -1);
#if defined _WIN32 && !defined __CYGWIN__
  ASSERT (errno == ENOENT);
#else
  ASSERT (errno == ENOTDIR || errno == EISDIR || errno == EINVAL);
#endif

  errno = 0;
  ASSERT (func ("/dev/null", O_RDONLY | O_DIRECTORY) == -1);
  ASSERT (errno == ENOTDIR);

  /* Cannot open /dev/tty with trailing slash or O_DIRECTORY,
     though errno may differ as there may not be a controlling tty.  */
  ASSERT (func ("/dev/tty/", O_RDONLY) == -1);
  ASSERT (func ("/dev/tty", O_RDONLY | O_DIRECTORY) == -1);

  /* Cannot open fifo with trailing slash or O_DIRECTORY.  */
  if (mkfifo (BASE "fifo", 0666) == 0)
    {
      errno = 0;
      ASSERT (func (BASE "fifo/", O_RDONLY) == -1);
      ASSERT (errno == ENOTDIR || errno == EISDIR || errno == EINVAL);

      errno = 0;
      ASSERT (func (BASE "fifo", O_RDONLY | O_DIRECTORY) == -1);
      ASSERT (errno == ENOTDIR);

      ASSERT (unlink (BASE "fifo") == 0);
    }

  /* Directories cannot be opened for writing.  */
  errno = 0;
  ASSERT (func (".", O_WRONLY) == -1);
  ASSERT (errno == EISDIR || errno == EACCES);

  /* /dev/null must exist, and be writable.  */
  fd = func ("/dev/null", O_RDONLY);
  ASSERT (0 <= fd);
  {
    char c;
    ASSERT (read (fd, &c, 1) == 0);
  }
  ASSERT (close (fd) == 0);
  fd = func ("/dev/null", O_WRONLY);
  ASSERT (0 <= fd);
  ASSERT (write (fd, "c", 1) == 1);
  ASSERT (close (fd) == 0);

  /* Although O_NONBLOCK on regular files can be ignored, it must not
     cause a failure.  */
  fd = func (BASE "file", O_NONBLOCK | O_RDONLY);
  ASSERT (0 <= fd);
  ASSERT (close (fd) == 0);

  /* O_CLOEXEC must be honoured.  */
  if (O_CLOEXEC)
    {
      /* Since the O_CLOEXEC handling goes through a special code path at its
         first invocation, test it twice.  */
      for (int i = 0; i < 2; i++)
        {
          int flags;

          fd = func (BASE "file", O_CLOEXEC | O_RDONLY);
          ASSERT (0 <= fd);
          flags = fcntl (fd, F_GETFD);
          ASSERT (flags >= 0);
          ASSERT ((flags & FD_CLOEXEC) != 0);
          ASSERT (close (fd) == 0);
        }
    }

  /* Symlink handling, where supported.  */
  if (symlink (BASE "file", BASE "link") != 0)
    {
      ASSERT (unlink (BASE "file") == 0);
      if (print)
        fputs ("skipping test: symlinks not supported on this file system\n",
               stderr);
      return 77;
    }
  errno = 0;
  ASSERT (func (BASE "link/", O_RDONLY) == -1);
  ASSERT (errno == ENOTDIR);
  fd = func (BASE "link", O_RDONLY);
  ASSERT (0 <= fd);
  ASSERT (close (fd) == 0);

  /* Cleanup.  */
  ASSERT (unlink (BASE "file") == 0);
  ASSERT (unlink (BASE "e.exe") == 0);
  ASSERT (unlink (BASE "link") == 0);

  return 0;
}
