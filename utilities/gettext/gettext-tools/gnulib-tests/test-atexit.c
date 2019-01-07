/* Test of execution of program termination handlers.
   Copyright (C) 2007, 2009-2016 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <stdlib.h>

#include "signature.h"
SIGNATURE_CHECK (atexit, int, (void (*) (void)));

#include <unistd.h>

#define TEMPFILE "t-atexit.tmp"

static void
clear_temp_file (void)
{
  unlink (TEMPFILE);
}

int
main (int argc, char *argv[])
{
  atexit (clear_temp_file);

  if (argc > 1)
    exit (atoi (argv[1]));
  else
    return 0;
}
