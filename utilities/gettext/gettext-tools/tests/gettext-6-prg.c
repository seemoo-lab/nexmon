/* Test program, used by the gettext-6 test.
   Copyright (C) 2005-2007, 2009-2010, 2015-2016 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <haible@clisp.cons.org>, 2005.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if USE_POSIX_THREADS && ((__GLIBC__ >= 2 && !defined __UCLIBC__) || (defined __APPLE__ && defined __MACH__)) && HAVE_USELOCALE

#include <pthread.h>

/* Make sure we use the included libintl, not the system's one. */
#undef _LIBINTL_H
#include "libgnuintl.h"

/* Name of German and French locale in ISO-8859-1 or ISO-8859-15 encoding.  */
#if __GLIBC__ >= 2
# define LOCALE_DE_ISO8859 "de_DE.ISO-8859-1"
# define LOCALE_FR_ISO8859 "fr_FR.ISO-8859-1"
#elif defined __APPLE__ && defined __MACH__ /* MacOS X */
# define LOCALE_DE_ISO8859 "de_DE.ISO8859-1"
# define LOCALE_FR_ISO8859 "fr_FR.ISO8859-1"
#else
# define LOCALE_DE_ISO8859 "de_DE"
# define LOCALE_FR_ISO8859 "fr_FR"
#endif

/* Set to 1 if the program is not behaving correctly.  */
int result;

/* Denotes which thread should run next.  */
int flipflop;
/* Lock and wait queue used to switch between the threads.  */
pthread_mutex_t lock;
pthread_cond_t waitqueue;

/* Waits until the flipflop has a given value.
   Before the call, the lock is unlocked.  After the call, it is locked.  */
static void
waitfor (int value)
{
  if (pthread_mutex_lock (&lock))
    exit (10);
  while (flipflop != value)
    if (pthread_cond_wait (&waitqueue, &lock))
      exit (11);
}

/* Sets the flipflop to a given value.
   Before the call, the lock is locked.  After the call, it is unlocked.  */
static void
setto (int value)
{
  flipflop = value;
  if (pthread_cond_signal (&waitqueue))
    exit (20);
  if (pthread_mutex_unlock (&lock))
    exit (21);
}

void *
thread1_execution (void *arg)
{
  char *s;

  waitfor (1);
  uselocale (newlocale (LC_ALL_MASK, LOCALE_DE_ISO8859, NULL));
  setto (2);

  waitfor (1);
  s = gettext ("beauty");
  puts (s);
  if (strcmp (s, "Sch\366nheit"))
    {
      fprintf (stderr, "thread 1 call 1 returned: %s\n", s);
      result = 1;
    }
  setto (2);

  waitfor (1);
  s = gettext ("beauty");
  puts (s);
  if (strcmp (s, "Sch\366nheit"))
    {
      fprintf (stderr, "thread 1 call 2 returned: %s\n", s);
      result = 1;
    }
  setto (2);

  return NULL;
}

void *
thread2_execution (void *arg)
{
  char *s;

  waitfor (2);
  uselocale (newlocale (LC_ALL_MASK, LOCALE_FR_ISO8859, NULL));
  setto (1);

  waitfor (2);
  s = gettext ("beauty");
  puts (s);
  if (strcmp (s, "beaut\351"))
    {
      fprintf (stderr, "thread 2 call 1 returned: %s\n", s);
      result = 1;
    }
  setto (1);

  waitfor (2);
  s = gettext ("beauty");
  puts (s);
  if (strcmp (s, "beaut\351"))
    {
      fprintf (stderr, "thread 2 call 2 returned: %s\n", s);
      result = 1;
    }
  setto (1);

  return NULL;
}

static void
check_locale_exists (const char *name)
{
  if (newlocale (LC_ALL_MASK, name, NULL) == NULL)
    {
      printf ("%s\n", name);
      exit (1);
    }
}

int
main (int argc, char *argv[])
{
  int arg;
  pthread_t thread1;
  pthread_t thread2;

  arg = (argc > 1 ? atoi (argv[1]) : 0);
  switch (arg)
    {
    case 1:
      /* Check for the existence of the first locale.  */
      check_locale_exists (LOCALE_DE_ISO8859);
      /* Check for the existence of the second locale.  */
      check_locale_exists (LOCALE_FR_ISO8859);
      return 0;
    default:
      break;
    }

  unsetenv ("LANGUAGE");
  unsetenv ("OUTPUT_CHARSET");
  textdomain ("tstthread");
  bindtextdomain ("tstthread", "gt-6");
  result = 0;

  flipflop = 1;
  if (pthread_mutex_init (&lock, NULL))
    exit (2);
  if (pthread_cond_init (&waitqueue, NULL))
    exit (2);
  if (pthread_create (&thread1, NULL, &thread1_execution, NULL))
    exit (2);
  if (pthread_create (&thread2, NULL, &thread2_execution, NULL))
    exit (2);
  if (pthread_join (thread2, NULL))
    exit (3);

  return result;
}

#else

/* This test is not executed.  */

int
main (void)
{
  return 77;
}

#endif
