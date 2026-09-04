/* Multithread-safety test for localeconv()
   (in the absence of setlocale() or uselocale() calls).
   Copyright (C) 2019-2026 Free Software Foundation, Inc.

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

/* Work around GCC bug 44511.  */
#if _GL_GNUC_PREREQ (4, 3)
# pragma GCC diagnostic ignored "-Wreturn-type"
#endif

#if USE_ISOC_THREADS || USE_POSIX_THREADS || USE_ISOC_AND_POSIX_THREADS || USE_WINDOWS_THREADS

/* Specification.  */
#include <locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "glthread/thread.h"


/* Some common locale names.  */

#if defined _WIN32 && !defined __CYGWIN__
# define ENGLISH "English_United States"
# define FRENCH  "French_France"
# define GERMAN  "German_Germany"
# define ENCODING ".1252"
#else
# define ENGLISH "en_US"
# define FRENCH  "fr_FR"
# define GERMAN  "de_DE"
# if defined __hpux
#  define ENCODING ".utf8"
# else
#  define ENCODING ".UTF-8"
# endif
#endif

static const char LOCALE1[] = ENGLISH ENCODING;
static const char LOCALE2[] = FRENCH ENCODING;
static const char LOCALE3[] = GERMAN ENCODING;

static char *expected1;

static void *
thread1_func (void *arg)
{
  for (;;)
    {
      const char *value = localeconv () -> decimal_point;
      if (strcmp (expected1, value) != 0)
        {
          fprintf (stderr, "thread1 disturbed by threadN!\n"); fflush (stderr);
          abort ();
        }
    }

  /*NOTREACHED*/
}

static char *expected2;

static void *
thread2_func (void *arg)
{
  for (;;)
    {
      const char *value = localeconv () -> thousands_sep;
      if (strcmp (expected2, value) != 0)
        {
          fprintf (stderr, "thread2 disturbed by threadN!\n"); fflush (stderr);
          abort ();
        }
    }

  /*NOTREACHED*/
}

static char *expected3;

static void *
thread3_func (void *arg)
{
  for (;;)
    {
      const char *value = localeconv () -> mon_decimal_point;
      if (strcmp (expected3, value) != 0)
        {
          fprintf (stderr, "thread3 disturbed by threadN!\n"); fflush (stderr);
          abort ();
        }
    }

  /*NOTREACHED*/
}

static char *expected4;

static void *
thread4_func (void *arg)
{
  for (;;)
    {
      const char *value = localeconv () -> mon_thousands_sep;
      if (strcmp (expected4, value) != 0)
        {
          fprintf (stderr, "thread4 disturbed by threadN!\n"); fflush (stderr);
          abort ();
        }
    }

  /*NOTREACHED*/
}

static char *expected5;

static void *
thread5_func (void *arg)
{
  for (;;)
    {
      const char *value = localeconv () -> currency_symbol;
      if (strcmp (expected5, value) != 0)
        {
          fprintf (stderr, "thread5 disturbed by threadN!\n"); fflush (stderr);
          abort ();
        }
    }

  /*NOTREACHED*/
}

static char *expected6;

static void *
thread6_func (void *arg)
{
  for (;;)
    {
      const char *value = localeconv () -> int_curr_symbol;
      if (strcmp (expected6, value) != 0)
        {
          fprintf (stderr, "thread6 disturbed by threadN!\n"); fflush (stderr);
          abort ();
        }
    }

  /*NOTREACHED*/
}

static void *
threadN_func (void *arg)
{
  for (;;)
    {
      (void) localeconv () -> decimal_point;     /* LC_NUMERIC */  /* locale -k decimal_point */
      (void) localeconv () -> thousands_sep;     /* LC_NUMERIC */  /* locale -k thousands_sep */
      (void) localeconv () -> mon_decimal_point; /* LC_MONETARY */ /* locale -k mon_decimal_point */
      (void) localeconv () -> mon_thousands_sep; /* LC_MONETARY */ /* locale -k mon_thousands_sep */
      (void) localeconv () -> currency_symbol;   /* LC_MONETARY */ /* locale -k currency_symbol */
      (void) localeconv () -> int_curr_symbol;   /* LC_MONETARY */ /* locale -k int_curr_symbol */
    }

  /*NOTREACHED*/
}

int
main (int argc, char *argv[])
{
  if (setlocale (LC_ALL, LOCALE1) == NULL)
    {
      fprintf (stderr, "Skipping test: LOCALE1 not recognized\n");
      return 77;
    }
  if (setlocale (LC_MONETARY, LOCALE2) == NULL)
    {
      fprintf (stderr, "Skipping test: LOCALE2 not recognized\n");
      return 77;
    }
  if (setlocale (LC_NUMERIC, LOCALE3) == NULL)
    {
      fprintf (stderr, "Skipping test: LOCALE3 not recognized\n");
      return 77;
    }

  expected1 = strdup (localeconv () -> decimal_point);
  expected2 = strdup (localeconv () -> thousands_sep);
  expected3 = strdup (localeconv () -> mon_decimal_point);
  expected4 = strdup (localeconv () -> mon_thousands_sep);
  expected5 = strdup (localeconv () -> currency_symbol);
  expected6 = strdup (localeconv () -> int_curr_symbol);

  /* Create the checker threads.  */
  gl_thread_create (thread1_func, NULL);
  gl_thread_create (thread2_func, NULL);
  gl_thread_create (thread3_func, NULL);
  gl_thread_create (thread4_func, NULL);
  gl_thread_create (thread5_func, NULL);
  gl_thread_create (thread6_func, NULL);
  /* Create the disturber thread.  */
  gl_thread_create (threadN_func, NULL);

  /* Let them run for 2 seconds.  */
  {
    struct timespec duration;
    duration.tv_sec = (argc > 1 ? atoi (argv[1]) : 2);
    duration.tv_nsec = 0;

    nanosleep (&duration, NULL);
  }

  return 0;
}

#else

/* No multithreading available.  */

#include <stdio.h>

int
main ()
{
  fputs ("Skipping test: multithreading not enabled\n", stderr);
  return 77;
}

#endif
