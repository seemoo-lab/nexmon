/* Test program, used by the format-c-5 test.
   Copyright (C) 2004, 2006, 2010, 2015-2016 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xsetenv.h"

/* For %Id to work, we need the real setlocale(), not the fake one. */
#if !(__GLIBC__ >= 2 && !defined __UCLIBC__)
# include "setlocale.c"
#endif

/* Make sure we use the included libintl, not the system's one. */
#undef _LIBINTL_H
#include "libgnuintl.h"

#define _(string) gettext (string)

int
main (int argc, char *argv[])
{
  int n = 5;
  const char *en;
  const char *s;
  const char *expected_translation;
  const char *expected_result;
  char buf[100];

  xsetenv ("LC_ALL", argv[1], 1);
  if (setlocale (LC_ALL, "") == NULL)
    /* Couldn't set locale.  */
    exit (77);

  textdomain ("fc5");
  bindtextdomain ("fc5", ".");

  s = gettext ("father of %d children");
  en = "father of %d children";
#if (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)) && !defined __UCLIBC__
  expected_translation = "Vater von %Id Kindern";
  expected_result = "Vater von \xdb\xb5 Kindern";
#else
  expected_translation = "Vater von %d Kindern";
  expected_result = "Vater von 5 Kindern";
#endif

  if (strcmp (s, en) == 0)
    {
      fprintf (stderr, "String untranslated.\n");
      exit (1);
    }
  if (strcmp (s, expected_translation) != 0)
    {
      fprintf (stderr, "String incorrectly translated.\n");
      exit (1);
    }
  sprintf (buf, s, n);
  if (strcmp (buf, expected_result) != 0)
    {
      fprintf (stderr, "printf of translation wrong.\n");
      exit (1);
    }
  return 0;
}
