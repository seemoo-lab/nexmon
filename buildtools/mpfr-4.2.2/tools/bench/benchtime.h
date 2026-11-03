/* benchtime.h -- header file for MPFRbench

Copyright 1999, 2001-2025 Free Software Foundation, Inc.
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


/* compute the time to run accurately niter calls of the function for any
   number of inputs */
#define DECLARE_ACCURATE_TIME_NOP(func, funccall)      \
unsigned long int ACCURATE_TIME_NOP##func( unsigned long int niter, int n, mpfr_t* z, mpfr_t* x, mpfr_t* y, int nop);\
unsigned long int ACCURATE_TIME_NOP##func( unsigned long int niter, int n, mpfr_t* z, mpfr_t* x, mpfr_t* y, int nop)\
{                                             \
  unsigned long int ti, i;  int kn;           \
  unsigned long int t0 = get_cputime ();      \
  for (i = niter, kn=0; i > 0; i--)           \
    {                                         \
      funccall;                               \
      kn++; if (kn==n) kn = 0;                \
    }                                         \
  ti = get_cputime () - t0;                   \
  /* following lines are appended to avoid warnings but minimize the number \
     of macros */                             \
  if (nop==2) y = NULL;                       \
  return ti;                                  \
}

/* address of the function to time accurately niter calls of func */
#define ADDR_ACCURATE_TIME_NOP(func) ACCURATE_TIME_NOP##func

/* address of the function to time one call of func */
#define ADDR_TIME_NOP(func) TIME_NOP##func


/* compute the time to run one only call of the function with two inputs  */
#define DECLARE_TIME_NOP(func, funcall, nop)                  \
 DECLARE_ACCURATE_TIME_NOP(func, funcall)                      \
 double TIME_NOP##func(int n, mpfr_t* z, mpfr_t* x, mpfr_t* y); \
 double TIME_NOP##func(int n, mpfr_t* z, mpfr_t* x, mpfr_t* y)  \
 {                                                             \
  double t; unsigned long int nbcall, mytime;                  \
    for (nbcall = 1, mytime=0; mytime<250000; nbcall<<=1)      \
      {                                                        \
       mytime = ACCURATE_TIME_NOP##func(nbcall, n, z, x, y, nop);  \
      }                                                        \
      t = (double) mytime/ nbcall ;                            \
   return t;                                                   \
 }

/* compute the time to run accurately niter calls of the function */
/* functions with 2 operands */
#define DECLARE_TIME_2OP(func)   DECLARE_TIME_NOP(func, func(z[kn],x[kn],y[kn], MPFR_RNDN), 2 )
/* functions with 1 operand */
#define DECLARE_TIME_1OP(func)   DECLARE_TIME_NOP(func, func(z[kn],x[kn], MPFR_RNDN), 1 )
