/* check_mparam - check a mparam.h file

Copyright 2018-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>. */

/* To check some mparam.h tables:
   1) make a symbolic link to the corresponding mparam.h or
      provide -DMPARAM='"..."' with a path to the mparam.h
      file when compiling this program
   2) compile and run this program */

#include <stdio.h>

#ifndef MPARAM
# define MPARAM "mparam.h"
#endif

#include MPARAM

#define numberof_const(x)  (sizeof (x) / sizeof ((x)[0]))

static short mulhigh_ktab[] = {MPFR_MULHIGH_TAB};
#define MPFR_MULHIGH_TAB_SIZE (numberof_const (mulhigh_ktab))

static short sqrhigh_ktab[] = {MPFR_SQRHIGH_TAB};
#define MPFR_SQRHIGH_TAB_SIZE (numberof_const (sqrhigh_ktab))

static short divhigh_ktab[] = {MPFR_DIVHIGH_TAB};
#define MPFR_DIVHIGH_TAB_SIZE (numberof_const (divhigh_ktab))

int main (void)
{
  int err = 0, n;

  for (n = 0; n < MPFR_MULHIGH_TAB_SIZE; n++)
    if (mulhigh_ktab[n] >= n)
      {
        printf ("Error, mulhigh_ktab[%d] = %d\n", n, mulhigh_ktab[n]);
        err = 1;
      }

  for (n = 0; n < MPFR_SQRHIGH_TAB_SIZE; n++)
    if (sqrhigh_ktab[n] >= n)
      {
        printf ("Error, sqrhigh_ktab[%d] = %d\n", n, sqrhigh_ktab[n]);
        err = 1;
      }

  for (n = 2; n < MPFR_DIVHIGH_TAB_SIZE; n++)
    if (divhigh_ktab[n] >= n-1)
      {
        printf ("Error, divhigh_ktab[%d] = %d\n", n, divhigh_ktab[n]);
        err = 1;
      }

  return err;
}
