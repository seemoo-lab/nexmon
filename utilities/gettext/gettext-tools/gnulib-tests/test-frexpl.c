/* Test of splitting a 'long double' into fraction and mantissa.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (frexpl, long double, (long double, int *));

#include <float.h>

#include "fpucw.h"
#include "isnanl-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

/* Avoid some warnings from "gcc -Wshadow".
   This file doesn't use the exp() function.  */
#undef exp
#define exp exponent

#undef INFINITY
#undef NAN

#define DOUBLE long double
#define VOLATILE
#define ISNAN isnanl
#define INFINITY Infinityl ()
#define NAN NaNl ()
#define L_(literal) literal##L
#define MINUS_ZERO minus_zerol
#define MAX_EXP LDBL_MAX_EXP
#define MIN_EXP LDBL_MIN_EXP
/* On PowerPC machines, LDBL_MIN_EXP is -1021, but the smallest reliable
   exponent for 'long double' is -968.  For exponents below that, the precision
   may be truncated to the precision used for 'double'.  */
#if defined _ARCH_PPC
# define MIN_NORMAL_EXP (LDBL_MIN_EXP + 53)
#else
# define MIN_NORMAL_EXP LDBL_MIN_EXP
#endif
#define FREXP frexpl
#define RANDOM randoml
#include "test-frexp.h"

int
main ()
{
  DECL_LONG_DOUBLE_ROUNDING

  BEGIN_LONG_DOUBLE_ROUNDING ();

  test_function ();

  return test_exit_status;
}
