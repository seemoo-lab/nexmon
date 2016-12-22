/* Test program, used by the gettext-4 test.
   Copyright (C) 2001, 2005-2006, 2015-2016 Free Software Foundation, Inc.

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

/* Contributed to the GNU C Library by
   Bruno Haible <haible@clisp.cons.org>, 2001.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "xsetenv.h"

/* Make sure we use the included libintl, not the system's one. */
#undef _LIBINTL_H
#include "libgnuintl.h"

int
main (int argc, char *argv[])
{
  char *s;
  int result = 0;

  /* Clean up environment.  */
  unsetenv ("LANGUAGE");
  unsetenv ("LC_ALL");
  unsetenv ("LC_MESSAGES");
  unsetenv ("LC_CTYPE");
  unsetenv ("LANG");
  unsetenv ("OUTPUT_CHARSET");

  xsetenv ("LC_ALL", argv[1], 1);
  setlocale (LC_ALL, "");
  textdomain ("codeset");
  bindtextdomain ("codeset", "gt-4");

  /* Here we expect output in ISO-8859-1.
     Except on Darwin 7 or newer and on BeOS, for which locale_charset ()
     always returns "UTF-8" (see config.charset).  */
#if !((defined __APPLE__ && defined __MACH__) || defined __BEOS__)
  s = gettext ("cheese");
  if (strcmp (s, "K\344se"))
    {
      fprintf (stderr, "call 1 returned: %s\n", s);
      result = 1;
    }
#endif

  bind_textdomain_codeset ("codeset", "UTF-8");

  /* Here we expect output in UTF-8.  */
  s = gettext ("cheese");
  if (strcmp (s, "K\303\244se"))
    {
      fprintf (stderr, "call 2 returned: %s\n", s);
      result = 1;
    }

  bind_textdomain_codeset ("codeset", "ISO-8859-1");

  /* Here we expect output in ISO-8859-1.  */
  s = gettext ("cheese");
  if (strcmp (s, "K\344se"))
    {
      fprintf (stderr, "call 3 returned: %s\n", s);
      result = 1;
    }

  return result;
}
