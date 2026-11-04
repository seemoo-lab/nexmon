/* Test file for mpfr_sinu.

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
  inexact = mpfr_sinu (y, x, 0, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = NaN */
  mpfr_set_nan (x);
  inexact = mpfr_sinu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = +Inf */
  mpfr_set_inf (x, 1);
  inexact = mpfr_sinu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = -Inf */
  mpfr_set_inf (x, -1);
  inexact = mpfr_sinu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  /* check x = +0 */
  mpfr_set_zero (x, 1);
  inexact = mpfr_sinu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check x = -0 */
  mpfr_set_zero (x, -1);
  inexact = mpfr_sinu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) != 0);
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
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0 && inexact == 0);

  /* check 2*pi*x/u = pi thus x/u = 1/2, for example x=2 and u=4 */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = 3*pi/2 thus x/u = 3/4, for example x=3 and u=4 */
  mpfr_set_ui (x, 3, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si (y, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 2*pi thus x/u = 1, for example x=4 and u=4 */
  mpfr_set_ui (x, 4, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check x/u = 2^16, for example x=3*2^16 and u=3 */
  mpfr_set_ui_2exp (x, 3, 16, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 3, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = -pi/2 thus x/u = -1/4, for example x=-1 and u=4 */
  mpfr_set_si (x, -1, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si (y, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = -pi thus x/u = -1/2, for example x=-2 and u=4 */
  mpfr_set_si (x, -2, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) != 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = -3*pi/2 thus x/u = -3/4, for example x=-3 and u=4 */
  mpfr_set_si (x, -3, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0 && inexact == 0);

  /* check 2*pi*x/u = -2*pi thus x/u = -1, for example x=-4 and u=4 */
  mpfr_set_si (x, -4, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) != 0);
  MPFR_ASSERTN(inexact == 0);

  /* check x/u = -2^16, for example x=-3*2^16 and u=3 */
  mpfr_set_si_2exp (x, -3, 16, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 3, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) != 0);
  MPFR_ASSERTN(inexact == 0);

  /* check 2*pi*x/u = pi/6, for example x=1 and u=12 */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 12, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 1, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 5*pi/6, for example x=5 and u=12 */
  mpfr_set_ui (x, 5, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 12, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 1, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 7*pi/6, for example x=5 and u=12 */
  mpfr_set_ui (x, 7, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 12, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si_2exp (y, -1, -1) == 0 && inexact == 0);

  /* check 2*pi*x/u = 11*pi/6, for example x=5 and u=12 */
  mpfr_set_ui (x, 11, MPFR_RNDN);
  inexact = mpfr_sinu (y, x, 12, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si_2exp (y, -1, -1) == 0 && inexact == 0);

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
  inexact = mpfr_sinu (y, x, 42, MPFR_RNDN);
  /* y should be sin(2*17*pi/42) rounded to nearest */
  mpfr_set_str (z, "0x9.035be4a906768p-4", 16, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (y, z));
  MPFR_ASSERTN(inexact > 0);

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

/* Check argument reduction with large hard-coded inputs. The following
   values were generated with gen_random(sin,10,53,100,20), where the
   Sage code for gen_random is given in the tcosu.c file. */
static void
test_large (void)
{
  static struct {
    const char *x;
    unsigned long u;
    const char *y;
  } t[] = {
    { "-0x4.338bcaf01cb4p+16", 37, "0xa.a85c8758c1228p-4" },
    { "-0x6.01aa844d11acp+16", 12, "0x4.46e2cea96ee2cp-4" },
    { "-0x7.997759de3b3dp+16", 35, "0xf.144b13bc0b008p-4" },
    { "-0xc.74f72253bc7ep+16", 74, "-0x9.1c24e8ac6fb48p-4" },
    { "-0x4.ea97564d3ecp+12", 93, "0x3.2d49033cc4556p-4" },
    { "0xc.1cd85e0766ffp+16", 48, "-0xd.74b642b1c7368p-4" },
    { "0xe.add2f0441bap+16", 82, "-0xf.fc35c5a5e1448p-4" },
    { "0xc.8feb943a0fe5p+16", 43, "-0x5.8af3e0e11b794p-4" },
    { "-0xd.d61b06ace6d9p+16", 55, "0xa.175c9f7e4ede8p-4" },
    { "0xb.cf036b4e5fap+12", 46, "0x1.b7128e1975d9ap-4" },
    /* The following case does not come from the random values.
       Here, x mod u may need more precision than x. */
    { "0x3.p+16", 99, "-0xb.e4a8a986c9fd03ap-5" }
  };
  int i;
  mpfr_t x, y, z;

  mpfr_inits2 (53, x, y, z, (mpfr_ptr) 0);
  for (i = 0; i < numberof (t); i++)
    {
      mpfr_set_str (x, t[i].x, 0, MPFR_RNDN);
      mpfr_set_str (y, t[i].y, 0, MPFR_RNDN);
      mpfr_sinu (z, x, t[i].u, MPFR_RNDN);
      MPFR_ASSERTN (mpfr_equal_p (y, z));
      /* Same test after reducing the precision of x to its minimum one. */
      mpfr_prec_round (x, mpfr_min_prec (x), MPFR_RNDN);
      mpfr_sinu (z, x, t[i].u, MPFR_RNDN);
      MPFR_ASSERTN (mpfr_equal_p (y, z));
      mpfr_prec_round (x, 53, MPFR_RNDN);
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

#define TEST_FUNCTION mpfr_sinu
#define ULONG_ARG2
#include "tgeneric.c"

static int
mpfr_sin2pi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t r)
{
  return mpfr_sinu (y, x, 1, r);
}

int
main (void)
{
  tests_start_mpfr ();

  test_singular ();
  test_exact ();
  test_regular ();
  test_large ();

  test_generic (MPFR_PREC_MIN, 100, 1000);

  data_check ("data/sin2pi", mpfr_sin2pi, "mpfr_sin2pi");

  tests_end_mpfr ();
  return 0;
}
