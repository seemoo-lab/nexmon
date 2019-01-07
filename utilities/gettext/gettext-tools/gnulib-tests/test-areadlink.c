/* Tests of areadlink.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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

/* Written by Eric Blake <ebb9@byu.net>, 2009.  */

#include <config.h>

#include "areadlink.h"

#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ignore-value.h"
#include "macros.h"

#define BASE "test-areadlink.t"

#include "test-areadlink.h"

/* Wrapper for testing areadlink.  */
static char *
do_areadlink (char const *name, size_t ignored _GL_UNUSED)
{
  return areadlink (name);
}

int
main (void)
{
  /* Remove any leftovers from a previous partial run.  */
  ignore_value (system ("rm -rf " BASE "*"));

  return test_areadlink (do_areadlink, true);
}
