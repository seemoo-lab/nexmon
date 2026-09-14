#include <windows.h>
#include <io.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef __CYGWIN__
/* For read() and write() */
#include <unistd.h>
/* Cygwin does not prototype __argc and __argv in stdlib.h */
extern int __argc;
extern char** __argv;
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	struct HINSTANCE__ *hPrevInstance,
	char *lpszCmdLine,
	int   nCmdShow)
{
  if (__argc >= 2 && strcmp (__argv[1], "print_argv0") == 0)
    {
      printf ("%s", __argv[0]);
    }
  else if (__argc <= 2)
    {
      printf ("# This is stdout\n");
      fflush (stdout);
      
      fprintf (stderr, "This is stderr\n");
      fflush (stderr);
    }
  else if (__argc == 4 && strcmp (__argv[1], "pipes") == 0)
    {
      int infd = atoi (__argv[2]);
      int outfd = atoi (__argv[3]);
      SSIZE_T k;
      size_t n;
      char buf[100] = {0};

      if (infd < 0 || outfd < 0)
	{
          fprintf (stderr, "spawn-test-win32-gui: illegal fds on command line %s\n",
		      lpszCmdLine);
	  exit (1);
	}

      n = strlen ("Hello there");
      if (write (outfd, &n, sizeof (n)) == -1 ||
	  write (outfd, "Hello there\n", n) == -1)
	{
	  int errsv = errno;
          fprintf (stderr, "spawn-test-win32-gui: Write error: %s\n", strerror (errsv));
	  exit (1);
	}

      k = read (infd, &n, sizeof (n));
      if (k < 0 || (size_t) k != sizeof (n))
	{
	  int errsv = errno;
	  if (k < 0)
            fprintf (stderr, "spawn-test-win32-gui: Read error: %s\n", strerror (errsv));
	  else
            fprintf (stderr, "spawn-test-win32-gui: Got only %zu bytes, wanted %zu\n",
                     (size_t) k, sizeof (n));
	  exit (1);
	}

      fprintf (stderr, "spawn-test-win32-gui: Parent says %zu bytes to read\n", n);

      k = read (infd, buf, n);
      if (k < 0 || (size_t) k != n)
	{
	  int errsv = errno;
	  if (k < 0)
            fprintf (stderr, "spawn-test-win32-gui: Read error: %s\n", strerror (errsv));
	  else
            fprintf (stderr, "spawn-test-win32-gui: Got only %zu bytes\n", (size_t) k);
	  exit (1);
	}

      n = strlen ("See ya");
      if (write (outfd, &n, sizeof (n)) == -1 ||
	  write (outfd, "See ya", n) == -1)
	{
	  int errsv = errno;
          fprintf (stderr, "spawn-test-win32-gui: Write error: %s\n", strerror (errsv));
	  exit (1);
	}
    }

  return 0;
}
