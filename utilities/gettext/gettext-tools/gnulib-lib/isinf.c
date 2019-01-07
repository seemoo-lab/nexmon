/* Test for positive or negative infinity.
   Copyright (C) 2007-2016 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff <blp@gnu.org>, 2008. */

#include <config.h>

#include <float.h>

int
gl_isinff (float x)
{
  return x < -FLT_MAX || x > FLT_MAX;
}

int
gl_isinfd (double x)
{
  return x < -DBL_MAX || x > DBL_MAX;
}

int
gl_isinfl (long double x)
{
  return x < -LDBL_MAX || x > LDBL_MAX;
}
