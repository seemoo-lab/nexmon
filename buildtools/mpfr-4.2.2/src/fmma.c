/* mpfr_fmma, mpfr_fmms -- Compute a*b +/- c*d

Copyright 2014-2025 Free Software Foundation, Inc.
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

#include "mpfr-impl.h"

/* compute a*b+c*d if neg=0 (fmma), a*b-c*d otherwise (fmms) */
static int
mpfr_fmma_aux (mpfr_ptr z, mpfr_srcptr a, mpfr_srcptr b, mpfr_srcptr c,
               mpfr_srcptr d, mpfr_rnd_t rnd, int neg)
{
  mpfr_ubf_t u, v;
  mpfr_t zz;
  mpfr_prec_t prec_z = MPFR_PREC(z);
  mp_size_t un, vn;
  mpfr_limb_ptr up, vp, zp;
  int inex;
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC
    (("a[%Pd]=%.*Rg b[%Pd]=%.*Rg c[%Pd]=%.*Rg d[%Pd]=%.*Rg rnd=%d neg=%d",
      mpfr_get_prec (a), mpfr_log_prec, a,
      mpfr_get_prec (b), mpfr_log_prec, b,
      mpfr_get_prec (c), mpfr_log_prec, c,
      mpfr_get_prec (d), mpfr_log_prec, d, rnd, neg),
     ("z[%Pd]=%.*Rg inex=%d",
      mpfr_get_prec (z), mpfr_log_prec, z, inex));

  MPFR_TMP_MARK (marker);

  un = MPFR_LIMB_SIZE (a) + MPFR_LIMB_SIZE (b);
  vn = MPFR_LIMB_SIZE (c) + MPFR_LIMB_SIZE (d);
  MPFR_TMP_INIT (up, u, (mpfr_prec_t) un * GMP_NUMB_BITS, un);
  MPFR_TMP_INIT (vp, v, (mpfr_prec_t) vn * GMP_NUMB_BITS, vn);

  mpfr_ubf_mul_exact (u, a, b);
  mpfr_ubf_mul_exact (v, c, d);
  if (prec_z == MPFR_PREC(a) && prec_z == MPFR_PREC(b) &&
      prec_z == MPFR_PREC(c) && prec_z == MPFR_PREC(d) &&
      un == MPFR_PREC2LIMBS(2 * prec_z))
    {
      MPFR_TMP_INIT (zp, zz, 2 * prec_z, un);
      MPFR_PREC(u) = MPFR_PREC(v) = 2 * prec_z;
      inex = (neg == 0) ? mpfr_add (zz, (mpfr_srcptr) u, (mpfr_srcptr) v, rnd)
        : mpfr_sub (zz, (mpfr_srcptr) u, (mpfr_srcptr) v, rnd);
      inex = mpfr_set_1_2 (z, zz, rnd, inex);
    }
  else
    inex = (neg == 0) ? mpfr_add (z, (mpfr_srcptr) u, (mpfr_srcptr) v, rnd)
      : mpfr_sub (z, (mpfr_srcptr) u, (mpfr_srcptr) v, rnd);

  MPFR_UBF_CLEAR_EXP (u);
  MPFR_UBF_CLEAR_EXP (v);

  MPFR_TMP_FREE (marker);

  return inex;
}

/* z <- a*b + c*d */
int
mpfr_fmma (mpfr_ptr z, mpfr_srcptr a, mpfr_srcptr b, mpfr_srcptr c,
           mpfr_srcptr d, mpfr_rnd_t rnd)
{
  return mpfr_fmma_aux (z, a, b, c, d, rnd, 0);
}

/* z <- a*b - c*d */
int
mpfr_fmms (mpfr_ptr z, mpfr_srcptr a, mpfr_srcptr b, mpfr_srcptr c,
           mpfr_srcptr d, mpfr_rnd_t rnd)
{
  return mpfr_fmma_aux (z, a, b, c, d, rnd, 1);
}
