/* Macros for infinity.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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


/* Infinityf () returns a 'float' +Infinity.  */

/* The Microsoft MSVC 9 compiler chokes on the expression 1.0f / 0.0f.  */
#if defined _MSC_VER
static float
Infinityf ()
{
  static float zero = 0.0f;
  return 1.0f / zero;
}
#else
# define Infinityf() (1.0f / 0.0f)
#endif


/* Infinityd () returns a 'double' +Infinity.  */

/* The Microsoft MSVC 9 compiler chokes on the expression 1.0 / 0.0.  */
#if defined _MSC_VER
static double
Infinityd ()
{
  static double zero = 0.0;
  return 1.0 / zero;
}
#else
# define Infinityd() (1.0 / 0.0)
#endif


/* Infinityl () returns a 'long double' +Infinity.  */

/* The Microsoft MSVC 9 compiler chokes on the expression 1.0L / 0.0L.  */
#if defined _MSC_VER
static double
Infinityl ()
{
  static long double zero = 0.0L;
  return 1.0L / zero;
}
#else
# define Infinityl() (1.0L / 0.0L)
#endif
