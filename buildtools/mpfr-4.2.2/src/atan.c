/* mpfr_atan -- arc-tangent of a floating-point number

Copyright 2001-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

#if GMP_NUMB_BITS == 64
/* for each pair (r,p), we store a 192-bit approximation of atan(x)/x for
   x=p/2^r, with lowest limb first.
   Sage code:
   for p in range(1,2^ceil(r/2)):
      x=p/2^r
      l=floor(2^192*n(atan(x)/x, 300)).digits(2^64)
      print ("{0x%x, 0x%x, 0x%x}, /"+"* (%d,%d) *"+"/") % (l[0],l[1],l[2],r,p)
*/
static const mp_limb_t atan_table[][3] = {
    {0x6e141587261cdf00, 0x6fe445ecbc3a8d03, 0xed63382b0dda7b45}, /* (1,1) */
    {0xaa7fa90388b3836b, 0x6dc79ef5f7a217e5, 0xfadbafc96406eb15}, /* (2,1) */
    {0x319c12cf59d4b2dc, 0xcb2792dc0e2e0d51, 0xffaaddb967ef4e36}, /* (4,1) */
    {0x8b3957d95d9ad922, 0xc897989f3e888ef7, 0xfeadd4d5617b6e32}, /* (4,2) */
    {0xc4e6abc8af62e439, 0x4eb9bf602625f0b4, 0xfd0fcdd343cac19b}, /* (4,3) */
    {0x7c18baeb9bc95789, 0xb12afb6b6d4f7e16, 0xffffaaaaddddb94b}, /* (8,1) */
    {0x6856a0171a2f001a, 0x62351fbbe60af47,  0xfffeaaadddd4b968}, /* (8,2) */
    {0x69164c094f49da06, 0xd517294f7373d07a, 0xfffd001032cb1179}, /* (8,3) */
    {0x20ef65c10deef460, 0xe78c564015f76048, 0xfffaaadddb94d5bb}, /* (8,4) */
    {0x3ce233aa002f0344, 0x9dd8ea342a65d4cc, 0xfff7ab27a1f32f95}, /* (8,5) */
    {0xa37f403c7279c5cb, 0x13ab53a1c8db8497, 0xfff40103192ce74d}, /* (8,6) */
    {0xe5a85657103c1aa8, 0xb8409e6c914191d3, 0xffefac8a9c40a26b}, /* (8,7) */
    {0x806d0294c0db8816, 0x779d776dda8c6213, 0xffeaaddd4bb12542}, /* (8,8) */
    {0x5545d1914ef21478, 0x3aea58d6660f5a12, 0xffe5051f0aebf73a}, /* (8,9) */
    {0x6e47a91d015f4133, 0xc085ab6b490b7f02, 0xffdeb2787d4adac1}, /* (8,10) */
    {0x4efc1f931f7ec9b3, 0xb7f43cd16195ef4b, 0xffd7b61702b09aad}, /* (8,11) */
    {0xd27d1dbf55fed60d, 0xd812c11d7d473e5e, 0xffd0102cb3c1bfbe}, /* (8,12) */
    {0xca629e927383fe97, 0x8c61aedf58e42206, 0xffc7c0f05db9d1b6}, /* (8,13) */
    {0x4eff0b53d4e905b7, 0x28ac1e800ca31e9d, 0xffbec89d7dddd7e9}, /* (8,14) */
    {0xb0a7931deec6fe60, 0xb46feea78588554b, 0xffb527743c8cdd8f}  /* (8,15) */
  };

static void
set_table (mpfr_ptr y, const mp_limb_t x[3])
{
  mpfr_prec_t p = MPFR_PREC(y);
  mp_size_t n = MPFR_PREC2LIMBS(p);
  mpfr_prec_t sh;
  mp_limb_t *yp = MPFR_MANT(y);

  MPFR_UNSIGNED_MINUS_MODULO (sh, p);
  MPFR_ASSERTD (n >= 1 && n <= 3);
  mpn_copyi (yp, x + 3 - n, n);
  yp[0] &= ~MPFR_LIMB_MASK(sh);
  MPFR_SET_EXP(y, 0);
}
#endif

/* If x = p/2^r, put in y an approximation to atan(x)/x using 2^m terms
   for the series expansion, with an error of at most 1 ulp.
   Assumes 0 < x < 1, thus 1 <= p < 2^r.
   More precisely, p consists of the floor(r/2) bits of the binary expansion
   of a number 0 < s < 1:
   * the bit of weight 2^-1 is for r=1, thus p <= 1
   * the bit of weight 2^-2 is for r=2, thus p <= 1
   * the two bits of weight 2^-3 and 2^-4 are for r=4, thus p <= 3
   * more generally p < 2^(r/2).

   If X=x^2, we want 1 - X/3 + X^2/5 - ... + (-1)^k*X^k/(2k+1) + ...

   When we sum terms up to x^k/(2k+1), the denominator Q[0] is
   3*5*7*...*(2k+1) ~ (2k/e)^k.

   The tab[] array should have at least 3*(m+1) entries.
*/
static void
mpfr_atan_aux (mpfr_ptr y, mpz_ptr p, unsigned long r, int m, mpz_t *tab)
{
  mpz_t *S, *Q, *ptoj;
  mp_bitcnt_t n, h, j;  /* unsigned type, which is >= unsigned long */
  mpfr_exp_t diff, expo;
  int im, i, k, l, done;
  mpfr_prec_t mult;
  mpfr_prec_t accu[MPFR_PREC_BITS], log2_nb_terms[MPFR_PREC_BITS];
  mpfr_prec_t precy = MPFR_PREC(y);

  MPFR_ASSERTD (mpz_sgn (p) > 0);
  MPFR_ASSERTD (m > 0);
  MPFR_ASSERTD (m <= MPFR_PREC_BITS - 1);

#if GMP_NUMB_BITS == 64
  /* tabulate values for small precision and small value of r (which are the
     most expensive to compute) */
  if (precy <= 192)
    {
      unsigned long u;

      switch (r)
        {
        case 1:
          /* p has 1 bit: necessarily p=1 */
          MPFR_ASSERTD(mpz_cmp_ui (p, 1) == 0);
          set_table (y, atan_table[0]);
          return;
        case 2:
          /* p has 1 bit: necessarily p=1 too */
          MPFR_ASSERTD(mpz_cmp_ui (p, 1) == 0);
          set_table (y, atan_table[1]);
          return;
        case 4:
          /* p has at most 2 bits: 1 <= p <= 3 */
          u = mpz_get_ui (p);
          MPFR_ASSERTD(1 <= u && u <= 3);
          set_table (y, atan_table[1 + u]);
          return;
        case 8:
          /* p has at most 4 bits: 1 <= p <= 15 */
          u = mpz_get_ui (p);
          MPFR_ASSERTD(1 <= u && u <= 15);
          set_table (y, atan_table[4 + u]);
          return;
        }
    }
#endif

  /* Set Tables */
  S    = tab;           /* S */
  ptoj = S + 1*(m+1);   /* p^2^j Precomputed table */
  Q    = S + 2*(m+1);   /* Product of Odd integer  table  */

  /* From p to p^2, and r to 2r */
  mpz_mul (p, p, p);
  MPFR_ASSERTD (2 * r > r);
  r = 2 * r;

  /* Normalize p */
  n = mpz_scan1 (p, 0);
  if (n > 0)
    {
      mpz_tdiv_q_2exp (p, p, n); /* exact */
      MPFR_ASSERTD (r > n);
      r -= n;
    }

  /* Since |p/2^r| < 1, and p is a non-zero integer, necessarily r > 0. */
  MPFR_ASSERTD (mpz_sgn (p) > 0);
  MPFR_ASSERTD (m > 0);
  MPFR_ASSERTD (r > 0);

  /* check if p=1 (special case) */
  l = 0;
  /*
    We compute by binary splitting, with X = x^2 = p/2^r:
    P(a,b) = p if a+1=b, P(a,c)*P(c,b) otherwise
    Q(a,b) = (2a+1)*2^r if a+1=b [except Q(0,1)=1], Q(a,c)*Q(c,b) otherwise
    S(a,b) = p*(2a+1) if a+1=b, Q(c,b)*S(a,c)+Q(a,c)*P(a,c)*S(c,b) otherwise
    Then atan(x)/x ~ S(0,i)/Q(0,i) for i so that (p/2^r)^i/i is small enough.
    The factor 2^(r*(b-a)) in Q(a,b) is implicit, thus we have to take it
    into account when we compute with Q.
  */
  accu[0] = 0; /* accu[k] = Mult[0] + ... + Mult[k], where Mult[j] is the
                  number of bits of the corresponding term S[j]/Q[j] */
  if (mpz_cmp_ui (p, 1) != 0)
    {
      /* p <> 1: precompute ptoj table */
      mpz_set (ptoj[0], p);
      for (im = 1 ; im <= m ; im ++)
        mpz_mul (ptoj[im], ptoj[im - 1], ptoj[im - 1]);
      /* main loop */
      n = 1UL << m;
      MPFR_ASSERTN (n != 0);  /* no overflow */
      /* the i-th term being X^i/(2i+1) with X=p/2^r, we can stop when
         p^i/2^(r*i) < 2^(-precy), i.e. r*i > precy + log2(p^i) */
      for (i = k = done = 0; (i < n) && (done == 0); i += 2, k ++)
        {
          /* initialize both S[k],Q[k] and S[k+1],Q[k+1] */
          mpz_set_ui (Q[k+1], 2 * i + 3); /* Q(i+1,i+2) */
          mpz_mul_ui (S[k+1], p, 2 * i + 1); /* S(i+1,i+2) */
          mpz_mul_2exp (S[k], Q[k+1], r);
          mpz_sub (S[k], S[k], S[k+1]); /* S(i,i+2) */
          mpz_mul_ui (Q[k], Q[k+1], 2 * i + 1); /* Q(i,i+2) */
          log2_nb_terms[k] = 1; /* S[k]/Q[k] corresponds to 2 terms */
          for (j = (i + 2) >> 1, l = 1; (j & 1) == 0; l ++, j >>= 1, k --)
            {
              /* invariant: S[k-1]/Q[k-1] and S[k]/Q[k] correspond
                 to 2^l terms each. We combine them into S[k-1]/Q[k-1] */
              MPFR_ASSERTD (k > 0);
              mpz_mul (S[k], S[k], Q[k-1]);
              mpz_mul (S[k], S[k], ptoj[l]);
              mpz_mul (S[k-1], S[k-1], Q[k]);
              mpz_mul_2exp (S[k-1], S[k-1], r << l);
              mpz_add (S[k-1], S[k-1], S[k]);
              mpz_mul (Q[k-1], Q[k-1], Q[k]);
              log2_nb_terms[k-1] = l + 1;
              /* now S[k-1]/Q[k-1] corresponds to 2^(l+1) terms */
              MPFR_MPZ_SIZEINBASE2(mult, ptoj[l+1]);
              mult = (r << (l + 1)) - mult - 1;
              accu[k-1] = (k == 1) ? mult : accu[k-2] + mult;
              if (accu[k-1] > precy)
                done = 1;
            }
        }
    }
  else /* special case p=1: the i-th term being X^i/(2i+1) with X=1/2^r,
          we can stop when r*i > precy i.e. i > precy/r */
    {
      n = 1UL << m;
      if (precy / r <= n)
        n = (precy / r) + 1;
      MPFR_ASSERTN (n != 0);  /* no overflow */
      for (i = k = 0; i < n; i += 2, k ++)
        {
          mpz_set_ui (Q[k + 1], 2 * i + 3);
          mpz_mul_2exp (S[k], Q[k+1], r);
          mpz_sub_ui (S[k], S[k], 1 + 2 * i);
          mpz_mul_ui (Q[k], Q[k + 1], 1 + 2 * i);
          log2_nb_terms[k] = 1; /* S[k]/Q[k] corresponds to 2 terms */
          for (j = (i + 2) >> 1, l = 1; (j & 1) == 0; l++, j >>= 1, k --)
            {
              MPFR_ASSERTD (k > 0);
              mpz_mul (S[k], S[k], Q[k-1]);
              mpz_mul (S[k-1], S[k-1], Q[k]);
              mpz_mul_2exp (S[k-1], S[k-1], r << l);
              mpz_add (S[k-1], S[k-1], S[k]);
              mpz_mul (Q[k-1], Q[k-1], Q[k]);
              log2_nb_terms[k-1] = l + 1;
            }
        }
    }

  /* we need to combine S[0]/Q[0]...S[k-1]/Q[k-1] */
  h = 0; /* number of terms accumulated in S[k]/Q[k] */
  while (k > 1)
    {
      k --;
      /* combine S[k-1]/Q[k-1] and S[k]/Q[k] */
      mpz_mul (S[k], S[k], Q[k-1]);
      if (mpz_cmp_ui (p, 1) != 0)
        mpz_mul (S[k], S[k], ptoj[log2_nb_terms[k-1]]);
      mpz_mul (S[k-1], S[k-1], Q[k]);
      h += (mp_bitcnt_t) 1 << log2_nb_terms[k];
      mpz_mul_2exp (S[k-1], S[k-1], r * h);
      mpz_add (S[k-1], S[k-1], S[k]);
      mpz_mul (Q[k-1], Q[k-1], Q[k]);
    }

  MPFR_MPZ_SIZEINBASE2 (diff, S[0]);
  diff -= 2 * precy;
  expo = diff;
  if (diff >= 0)
    mpz_tdiv_q_2exp (S[0], S[0], diff);
  else
    mpz_mul_2exp (S[0], S[0], -diff);

  MPFR_MPZ_SIZEINBASE2 (diff, Q[0]);
  diff -= precy;
  expo -= diff;
  if (diff >= 0)
    mpz_tdiv_q_2exp (Q[0], Q[0], diff);
  else
    mpz_mul_2exp (Q[0], Q[0], -diff);

  mpz_tdiv_q (S[0], S[0], Q[0]);
  mpfr_set_z (y, S[0], MPFR_RNDD);
  /* TODO: Check/prove that the following expression doesn't overflow. */
  expo = MPFR_GET_EXP (y) + expo - r * (i - 1);
  MPFR_SET_EXP (y, expo);
}

int
mpfr_atan (mpfr_ptr atan, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  mpfr_t xp, arctgt, sk, tmp, tmp2;
  mpz_t  ukz;
  mpz_t tabz[3*(MPFR_PREC_BITS+1)];
  mpfr_exp_t exptol;
  mpfr_prec_t prec, realprec, est_lost, lost;
  unsigned long twopoweri, log2p, red;
  int comparison, inexact;
  int i, n0, oldn0;
  MPFR_GROUP_DECL (group);
  MPFR_SAVE_EXPO_DECL (expo);
  MPFR_ZIV_DECL (loop);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd_mode),
     ("atan[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (atan), mpfr_log_prec, atan, inexact));

  /* Singular cases */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (atan);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (x))
        {
          MPFR_SAVE_EXPO_MARK (expo);
          if (MPFR_IS_POS (x))  /* arctan(+inf) = Pi/2 */
            inexact = mpfr_const_pi (atan, rnd_mode);
          else /* arctan(-inf) = -Pi/2 */
            {
              inexact = -mpfr_const_pi (atan,
                                        MPFR_INVERT_RND (rnd_mode));
              MPFR_CHANGE_SIGN (atan);
            }
          mpfr_div_2ui (atan, atan, 1, rnd_mode);  /* exact (no exceptions) */
          MPFR_SAVE_EXPO_FREE (expo);
          return mpfr_check_range (atan, inexact, rnd_mode);
        }
      else /* x is necessarily 0 */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          MPFR_SET_ZERO (atan);
          MPFR_SET_SAME_SIGN (atan, x);
          MPFR_RET (0);
        }
    }

  /* atan(x) = x - x^3/3 + x^5/5...
     so the error is < 2^(3*EXP(x)-1)
     so `EXP(x)-(3*EXP(x)-1)` = -2*EXP(x)+1 */
  MPFR_FAST_COMPUTE_IF_SMALL_INPUT (atan, x, -2 * MPFR_GET_EXP (x), 1, 0,
                                    rnd_mode, {});

  /* Set x_p=|x| */
  MPFR_TMP_INIT_ABS (xp, x);

  MPFR_SAVE_EXPO_MARK (expo);

  /* Other simple case arctan(-+1)=-+pi/4 */
  comparison = mpfr_cmp_ui (xp, 1);
  if (MPFR_UNLIKELY (comparison == 0))
    {
      int neg = MPFR_IS_NEG (x);
      inexact = mpfr_const_pi (atan, MPFR_IS_POS (x) ? rnd_mode
                               : MPFR_INVERT_RND (rnd_mode));
      if (neg)
        {
          inexact = -inexact;
          MPFR_CHANGE_SIGN (atan);
        }
      mpfr_div_2ui (atan, atan, 2, rnd_mode);  /* exact (no exceptions) */
      MPFR_SAVE_EXPO_FREE (expo);
      return mpfr_check_range (atan, inexact, rnd_mode);
    }

  realprec = MPFR_PREC (atan) + MPFR_INT_CEIL_LOG2 (MPFR_PREC (atan)) + 4;
  prec = realprec + GMP_NUMB_BITS;

  /* Initialisation */
  mpz_init2 (ukz, prec); /* ukz will need 'prec' bits below */
  MPFR_GROUP_INIT_4 (group, prec, sk, tmp, tmp2, arctgt);
  oldn0 = 0;

  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      /* First, if |x| < 1, we need to have more prec to be able to round (sup)
         n0 = ceil(log(prec_requested + 2 + 1+ln(2.4)/ln(2))/log(2)) */
      mpfr_prec_t sup;
      sup = MPFR_GET_EXP (xp) < 0 ? 2 - MPFR_GET_EXP (xp) : 1; /* sup >= 1 */

      n0 = MPFR_INT_CEIL_LOG2 ((realprec + sup) + 3);
      /* since realprec >= 4, n0 >= ceil(log2(8)) >= 3, thus 3*n0 > 2 */
      prec = (realprec + sup) + 1 + MPFR_INT_CEIL_LOG2 (3*n0-2);

      /* the number of lost bits due to argument reduction is
         9 - 2 * EXP(sk), which we estimate by 9 + 2*ceil(log2(p))
         since we manage that sk < 1/p */
      if (MPFR_PREC (atan) > 100)
        {
          log2p = MPFR_INT_CEIL_LOG2(prec) / 2 - 3;
          est_lost = 9 + 2 * log2p;
          prec += est_lost;
        }
      else
        log2p = est_lost = 0; /* don't reduce the argument */

      /* Initialisation */
      MPFR_GROUP_REPREC_4 (group, prec, sk, tmp, tmp2, arctgt);
      MPFR_ASSERTD (n0 <= MPFR_PREC_BITS);
      /* Note: the tabz[] entries are used to get a rational approximation
         of atan(x) to precision 'prec', thus allocating them to 'prec' bits
         should be good enough. */
      for (i = oldn0; i < 3 * (n0 + 1); i++)
        mpz_init2 (tabz[i], prec);
      oldn0 = 3 * (n0 + 1);

      /* The mpfr_ui_div below mustn't underflow. This is guaranteed by
         MPFR_SAVE_EXPO_MARK, but let's check that for maintainability. */
      MPFR_ASSERTD (__gmpfr_emax <= 1 - __gmpfr_emin);

      if (comparison > 0) /* use atan(xp) = Pi/2 - atan(1/xp) */
        mpfr_ui_div (sk, 1, xp, MPFR_RNDN);
      else
        mpfr_set (sk, xp, MPFR_RNDN);

      /* now 0 < sk <= 1 */

      /* Argument reduction: atan(x) = 2 atan((sqrt(1+x^2)-1)/x).
         We want |sk| < k/sqrt(p) where p is the target precision. */
      lost = 0;
      for (red = 0; MPFR_GET_EXP(sk) > - (mpfr_exp_t) log2p; red ++)
        {
          lost = 9 - 2 * MPFR_EXP(sk);
          mpfr_sqr (tmp, sk, MPFR_RNDN);
          mpfr_add_ui (tmp, tmp, 1, MPFR_RNDN);
          mpfr_sqrt (tmp, tmp, MPFR_RNDN);
          mpfr_sub_ui (tmp, tmp, 1, MPFR_RNDN);
          if (red == 0 && comparison > 0)
            /* use xp = 1/sk */
            mpfr_mul (sk, tmp, xp, MPFR_RNDN);
          else
            mpfr_div (sk, tmp, sk, MPFR_RNDN);
        }

      /* We started from x0 = 1/|x| if |x| > 1, and |x| otherwise, thus
         we had x0 = min(|x|, 1/|x|) <= 1, and applied 'red' times the
         argument reduction x -> (sqrt(1+x^2)-1)/x, which keeps 0 < x <= 1 */

      /* We first show that if the for-loop is executed at least once, then
         sk < 1 after the loop. Indeed for 1/2 <= x <= 1, interval
         arithmetic with precision 5 shows that (sqrt(1+x^2)-1)/x,
         when evaluated with rounding to nearest, gives a value <= 0.875.
         Now assume 2^(-k-1) <= x <= 2^(-k) for k >= 1.
         Then o(x^2) <= 2^(-2k), o(1+x^2) <= 1+2^(-2k),
         o(sqrt(1+x^2)) <= 1+2^(-2k-1), o(sqrt(1+x^2)-1) <= 2^(-2k-1),
         and o((sqrt(1+x^2)-1)/x) <= 2^(-k) <= 1/2.

         Now if sk=1 before the loop, then EXP(sk)=1 and since log2p >= 0,
         the loop is performed at least once, thus the case sk=1 cannot
         happen below.
      */

      MPFR_ASSERTD(mpfr_cmp_ui (sk, 1) < 0);

      /* Assignation  */
      MPFR_SET_ZERO (arctgt);
      twopoweri = 1 << 0;
      MPFR_ASSERTD (n0 >= 4);
      for (i = 0 ; i < n0; i++)
        {
          if (MPFR_UNLIKELY (MPFR_IS_ZERO (sk)))
            break;
          /* Calculation of trunc(tmp) --> mpz */
          mpfr_mul_2ui (tmp, sk, twopoweri, MPFR_RNDN);
          mpfr_trunc (tmp, tmp);
          if (!MPFR_IS_ZERO (tmp))
            {
              /* tmp = ukz*2^exptol */
              exptol = mpfr_get_z_2exp (ukz, tmp);
              /* since the s_k are decreasing (see algorithms.tex),
                 and s_0 = min(|x|, 1/|x|) < 1, we have sk < 1,
                 thus exptol < 0 */
              MPFR_ASSERTD (exptol < 0);
              mpz_tdiv_q_2exp (ukz, ukz, (unsigned long int) (-exptol));
              /* since tmp is a non-zero integer, and tmp = ukzold*2^exptol,
                 we now have ukz = tmp, thus ukz is non-zero */
              /* Calculation of arctan(Ak) */
              mpfr_set_z (tmp, ukz, MPFR_RNDN);
              mpfr_div_2ui (tmp, tmp, twopoweri, MPFR_RNDN);
              mpfr_atan_aux (tmp2, ukz, twopoweri, n0 - i, tabz);
              mpfr_mul (tmp2, tmp2, tmp, MPFR_RNDN);
              /* Addition */
              mpfr_add (arctgt, arctgt, tmp2, MPFR_RNDN);
              /* Next iteration */
              mpfr_sub (tmp2, sk, tmp, MPFR_RNDN);
              mpfr_mul (sk, sk, tmp, MPFR_RNDN);
              mpfr_add_ui (sk, sk, 1, MPFR_RNDN);
              mpfr_div (sk, tmp2, sk, MPFR_RNDN);
            }
          twopoweri <<= 1;
        }
      /* Add last step (Arctan(sk) ~= sk */
      mpfr_add (arctgt, arctgt, sk, MPFR_RNDN);

      /* argument reduction */
      mpfr_mul_2ui (arctgt, arctgt, red, MPFR_RNDN);

      if (comparison > 0)
        { /* atan(x) = Pi/2-atan(1/x) for x > 0 */
          mpfr_const_pi (tmp, MPFR_RNDN);
          mpfr_div_2ui (tmp, tmp, 1, MPFR_RNDN);
          mpfr_sub (arctgt, tmp, arctgt, MPFR_RNDN);
        }
      MPFR_SET_POS (arctgt);

      if (MPFR_LIKELY (MPFR_CAN_ROUND (arctgt, realprec + est_lost - lost,
                                       MPFR_PREC (atan), rnd_mode)))
        break;
      MPFR_ZIV_NEXT (loop, realprec);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set4 (atan, arctgt, rnd_mode, MPFR_SIGN (x));

  for (i = 0 ; i < oldn0 ; i++)
    mpz_clear (tabz[i]);
  mpz_clear (ukz);
  MPFR_GROUP_CLEAR (group);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (atan, inexact, rnd_mode);
}
