/* Test parts of the API.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

#include <libintl.h>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32 && !defined __CYGWIN__

# define ENGLISH  "English_United States"
# define ENCODING ".1252"

# define LOCALE1 ENGLISH ENCODING

#else

# define LOCALE1 "en_US.UTF-8"

#endif

int
main ()
{
  /* Clean up environment.  */
#if defined _WIN32 && !defined __CYGWIN__
  _putenv ("LANGUAGE=");
  _putenv ("OUTPUT_CHARSET=");
#else
  unsetenv ("LANGUAGE");
  unsetenv ("OUTPUT_CHARSET");
#endif

  textdomain ("itest");

#if defined _WIN32 && !defined __CYGWIN__
  _putenv ("LC_ALL=" LOCALE1);
#else
  setenv ("LC_ALL", LOCALE1, 1);
#endif
  if (setlocale (LC_ALL, "") == NULL)
    {
      fprintf (stderr, "Skipping test: Locale %s is not installed.\n", LOCALE1);
      return 0;
    }

  bindtextdomain ("itest", SRCDIR "locale");

  bind_textdomain_codeset ("itest", "UTF-8");

  const char *s = gettext ("She is the doppelganger of my fiancee.");
  const char *expected = "She is the doppelgänger of my fiancée.";
  if (strcmp (s, expected) != 0)
    {
      fprintf (stderr, "gettext() => %s\n", s);
      fprintf (stderr, "Expected:    %s\n", expected);
      return 1;
    }

  return 0;
}
