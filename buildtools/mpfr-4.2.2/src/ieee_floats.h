/* auxiliary data to generate special IEEE floats (NaN, +Inf, -Inf)

Copyright 1999-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>. */

/* "double" NaN and infinities are written as explicit bytes to be sure of
   getting what we want, and to be sure of not depending on libm.

   Could use 4-byte "float" values and let the code convert them, but it
   seems more direct to give exactly what we want.  Certainly for gcc 3.0.2
   on alphaev56-unknown-freebsd4.3 the NaN must be 8-bytes, since that
   compiler+system was seen incorrectly converting from a "float" NaN.  */

#if _MPFR_IEEE_FLOATS

/* The "d" field guarantees alignment to a suitable boundary for a double.
   Could use a union instead, if we checked the compiler supports union
   initializers.  */
union dbl_bytes {
  unsigned char b[8];
  double d;
};

#define MPFR_DBL_INFP  (dbl_infp.d)
#define MPFR_DBL_INFM  (dbl_infm.d)
#define MPFR_DBL_NAN   DBL_NAN

/* For NaN, we use DBL_NAN since the memory representation of a NaN depends
   on the processor: a fixed memory representation could yield either a
   quiet NaN (qNaN) or a signaling NaN (sNaN). For instance, HP PA-RISC
   is known to do the opposite way of the usual choice recommended in
   IEEE 754-2008; see:
     https://grouper.ieee.org/groups/1788/email/msg03272.html

   Moreover, the right choice is to generate a qNaN in particular because
   signaling NaNs are not supported by all compilers (note that the support
   must be in the compiler used to build the user-end application because
   this is where the sNaN will be obtained). */

#ifdef HAVE_DOUBLE_IEEE_LITTLE_ENDIAN
static const union dbl_bytes dbl_infp =
  { { 0, 0, 0, 0, 0, 0, 0xF0, 0x7F } };
static const union dbl_bytes dbl_infm =
  { { 0, 0, 0, 0, 0, 0, 0xF0, 0xFF } };
#endif

#ifdef HAVE_DOUBLE_IEEE_BIG_ENDIAN
static const union dbl_bytes dbl_infp =
  { { 0x7F, 0xF0, 0, 0, 0, 0, 0, 0 } };
static const union dbl_bytes dbl_infm =
  { { 0xFF, 0xF0, 0, 0, 0, 0, 0, 0 } };
#endif

#else /* _MPFR_IEEE_FLOATS */

#define MPFR_DBL_INFP DBL_POS_INF
#define MPFR_DBL_INFM DBL_NEG_INF
#define MPFR_DBL_NAN DBL_NAN

#endif /* _MPFR_IEEE_FLOATS */
