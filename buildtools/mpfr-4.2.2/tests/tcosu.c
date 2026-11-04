/* Test file for mpfr_cosu.

Copyright 2020-2025 Free Software Foundation, Inc.
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

#include "mpfr-test.h"

static void
test_singular (void)
{
  mpfr_t x, y;
  int inexact;

  mpfr_init (x);
  mpfr_init (y);

  /* check u = 0 */
  mpfr_set_ui (x, 17, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 0, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = NaN */
  mpfr_set_nan (x);
  inexact = mpfr_cosu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = +Inf */
  mpfr_set_inf (x, 1);
  inexact = mpfr_cosu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = -Inf */
  mpfr_set_inf (x, -1);
  inexact = mpfr_cosu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = +0 */
  mpfr_set_zero (x, 1);
  inexact = mpfr_cosu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check x = -0 */
  mpfr_set_zero (x, -1);
  inexact = mpfr_cosu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check x/u = 2^16, for example x=3*2^16 and u=3 */
  mpfr_set_ui_2exp (x, 3, 16, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 3, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check x/u = -2^16, for example x=-3*2^16 and u=3 */
  mpfr_set_si_2exp (x, -3, 16, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 3, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0);
  MPFR_ASSERTN(inexact == 0);

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
test_exact (void)
{
  mpfr_t x, y;
  int inexact;

  mpfr_init (x);
  mpfr_init (y);

  /* check 2*pi*x/u = pi/2 thus x/u = 1/4, for example x=1 and u=4 */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = pi thus x/u = 1/2, for example x=2 and u=4 */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si (y, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 3*pi/2 thus x/u = 3/4, for example x=3 and u=4 */
  mpfr_set_ui (x, 3, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = 2*pi thus x/u = 1, for example x=4 and u=4 */
  mpfr_set_ui (x, 4, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0 && inexact == 0);

  /* check 2*pi*x/u = -pi/2 thus x/u = -1/4, for example x=-1 and u=4 */
  mpfr_set_si (x, -1, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = -pi thus x/u = -1/2, for example x=-2 and u=4 */
  mpfr_set_si (x, -2, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si (y, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = -3*pi/2 thus x/u = -3/4, for example x=-3 and u=4 */
  mpfr_set_si (x, -3, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = -2*pi thus x/u = -1, for example x=-4 and u=4 */
  mpfr_set_si (x, -4, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0 && inexact == 0);

  /* check 2*pi*x/u = pi/3, for example x=1 and u=6 */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 6, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 1, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 2*pi/3, for example x=2 and u=6 */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 6, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si_2exp (y, -1, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 4*pi/3, for example x=4 and u=6 */
  mpfr_set_ui (x, 4, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 6, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si_2exp (y, -1, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 5*pi/3, for example x=5 and u=6 */
  mpfr_set_ui (x, 5, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 6, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 1, -1) == 0 && inexact == 0);

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
test_regular (void)
{
  mpfr_t x, y, z;
  int inexact;

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);
  mpfr_init2 (z, 53);

  mpfr_set_ui (x, 17, MPFR_RNDN);
  inexact = mpfr_cosu (y, x, 42, MPFR_RNDN);
  /* y should be cos(2*17*pi/42) rounded to nearest */
  mpfr_set_str (z, "-0xd.38462625fd3ap-4", 16, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (y, z));
  MPFR_ASSERTN(inexact < 0);

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

/* Check argument reduction with large hard-coded inputs. The following
   values were generated with the following Sage code:
# generate N random tests for f, with precision p, u < U, and |x| < 2^K
# f might be cos (for cosu), sin (for sinu) or tan (for tanu)
# gen_random(cos,10,53,100,20)
def gen_random(f,N,p,U,K):
   R = RealField(p)
   for n in range(N):
      u = ZZ.random_element(U)
      x = R.random_element()*2^K
      q = p
      while true:
         q += 10
         RI = RealIntervalField(q)
         y = RI(f(2*pi*x.exact_rational()/u))
         if R(y.lower().exact_rational()) == R(y.upper().exact_rational()):
            break
      y = R(y.lower().exact_rational())
      print (x.hex(), u, y.hex()) */
static void
test_large (void)
{
  static struct {
    const char *x;
    unsigned long u;
    const char *y;
  } t[] = {
    { "0xd.ddfeb0f4a01fp+16", 72, "0x4.8e54ce9b84d78p-4" },
    { "-0xb.ccb63f74f9abp+16", 36, "-0xb.cce98d64941bp-4" },
    { "0x9.8451e45ed4bap+16", 26, "-0xb.b205cfe8a13cp-4" },
    { "-0x7.6b4c16c45445p+16", 60, "-0x7.dee04000f4934p-4" },
    { "0x1.bb80916be884p+16", 43, "-0xc.059d9c8f1b7fp-4" },
    { "-0x5.4d3623b69226p+16", 1, "0xa.3cb353892757p-4" },
    { "0xd.1c59eab5a14bp+16", 58, "0x1.02978f1c99614p-4" },
    { "-0xf.bb1f858b9949p+16", 33, "-0x3.b53e5214db138p-4" },
    { "-0x2.9bcda761bb7p+16", 55, "-0x6.e6c08e7d92898p-4" },
    { "-0x9.f8f40e2c50f9p+16", 73, "0x7.0e0ff5e4dccbp-4" }
  };
  int i;
  mpfr_t x, y, z;

  mpfr_inits2 (53, x, y, z, (mpfr_ptr) 0);
  for (i = 0; i < numberof (t); i++)
    {
      mpfr_set_str (x, t[i].x, 0, MPFR_RNDN);
      mpfr_set_str (y, t[i].y, 0, MPFR_RNDN);
      mpfr_cosu (z, x, t[i].u, MPFR_RNDN);
      MPFR_ASSERTN (mpfr_equal_p (y, z));
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

#define TEST_FUNCTION mpfr_cosu
#define ULONG_ARG2
#include "tgeneric.c"

static int
mpfr_cos2pi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t r)
{
  return mpfr_cosu (y, x, 1, r);
}

int
main (void)
{
  tests_start_mpfr ();

  test_singular ();
  test_exact ();
  test_regular ();
  test_large ();

  /* Note: since the value of u can be large (up to 2^64 - 1 on 64-bit
     machines), the cos argument can be very small, yielding a special
     case in small precision. Thus it is better to use a maximum
     precision (second test_generic argument) that is large enough. */
  test_generic (MPFR_PREC_MIN, 200, 1000);

  data_check ("data/cos2pi", mpfr_cos2pi, "mpfr_cos2pi");

  tests_end_mpfr ();
  return 0;
}
