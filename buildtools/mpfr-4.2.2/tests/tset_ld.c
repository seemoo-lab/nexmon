/* Test file for mpfr_set_ld and mpfr_get_ld.

Copyright 2002-2025 Free Software Foundation, Inc.
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

#include <float.h>

#include "mpfr-test.h"

static void
check_gcc33_bug (void)
{
  volatile long double x;

  x = (long double) 9007199254740992.0 + 1.0;
  if (x != 0.0)
    return;  /* OK */
  printf
    ("Detected optimization bug of gcc 3.3 on Alpha concerning long double\n"
     "comparisons; set_ld tests might fail (set_ld won't work correctly).\n"
     "See https://gcc.gnu.org/legacy-ml/gcc-bugs/2003-10/msg00853.html for\n"
     "more information.\n");
}

static int
Isnan_ld (long double d)
{
  /* Do not convert d to double as this can give an overflow, which
     may confuse compilers without IEEE 754 support (such as clang
     -fsanitize=undefined), or trigger a trap if enabled.
     The DOUBLE_ISNAN macro should work fine on long double. */
  if (DOUBLE_ISNAN (d))
    return 1;
  LONGDOUBLE_NAN_ACTION (d, goto yes);
  return 0;
 yes:
  return 1;
}

/* Return the minimal number of bits to represent d exactly (0 for zero).
   If flag is non-zero, also print d. */
/* FIXME: This function doesn't work if the rounding precision is reduced. */
static mpfr_prec_t
print_binary (long double d, int flag)
{
  long double e, f, r;
  long exp = 1;
  mpfr_prec_t prec = 0;

  if (Isnan_ld (d))
    {
      if (flag)
        printf ("NaN\n");
      return 0;
    }
  if (d < (long double) 0.0
#if !defined(MPFR_ERRDIVZERO)
      || (d == (long double) 0.0 && (1.0 / (double) d < 0.0))
#endif
      )
    {
      if (flag)
        printf ("-");
      d = -d;
    }
  /* now d >= 0 */
  /* Use 2 different tests for Inf, to avoid potential bugs
     in implementations. */
  if (Isnan_ld (d - d) || (d > 1 && d * 0.5 == d))
    {
      if (flag)
        printf ("Inf\n");
      return 0;
    }
  if (d == (long double) 0.0)
    {
      if (flag)
        printf ("0.0\n");
      return prec;
    }
  MPFR_ASSERTN (d > 0);
  e = (long double) 1.0;
  while (e > d)
    {
      e = e * (long double) 0.5;
      exp --;
    }
  if (flag == 2) printf ("1: e=%.36Le\n", e);
  MPFR_ASSERTN (d >= e);
  /* FIXME: There can be an overflow here, which may not be supported
     on all platforms. */
  while (f = e + e, d >= f)
    {
      e = f;
      exp ++;
    }
  if (flag == 2) printf ("2: e=%.36Le\n", e);
  MPFR_ASSERTN (e <= d && d < f);
  if (flag == 1)
    printf ("0.");
  if (flag == 2) printf ("3: d=%.36Le e=%.36Le prec=%ld\n", d, e,
                         (long) prec);
  /* Note: the method we use here to extract the bits of d is the following,
     to deal with the case where the rounding precision is less than the
     precision of d:
     (1) we accumulate the upper bits of d into f
     (2) when accumulating a new bit into f is not exact, we subtract
         f from d and reset f to 0
     This is guaranteed to work only when the rounding precision is at least
     half the precision of d, since otherwise d-f might not be exact.
     This method does not work with flush-to-zero on underflow. */
  f = 0.0; /* will hold accumulated powers of 2 */
  while (1)
    {
      prec++;
      r = f + e;
      /* r is close to f (in particular in the cases where f+e may
         not be exact), so that r - f should be exact. */
      if (r - f != e) /* f+e is not exact */
        {
          d -= f; /* should be exact */
          f = 0.0;
          r = e;
        }
      if (d >= r)
        {
          if (flag == 1)
            printf ("1");
          if (d == r)
            break;
          f = r;
        }
      else
        {
          if (flag == 1)
            printf ("0");
        }
      e *= (long double) 0.5;
      MPFR_ASSERTN (e != 0); /* may fail with flush-to-zero on underflow */
      if (flag == 2) printf ("4: d=%.36Le e=%.36Le prec=%ld\n", d, e,
                             (long) prec);
    }
  if (flag == 1)
    printf ("e%ld\n", exp);
  return prec;
}

/* Checks that a long double converted exactly to a MPFR number, then
   converted back to a long double gives the initial value, or in other
   words, mpfr_get_ld(mpfr_set_ld(d)) = d.
*/
static void
check_set_get (long double d)
{
  mpfr_exp_t emin, emax;
  mpfr_t x;
  mpfr_prec_t prec;
  int r;
  long double e;
  int inex;
  int red;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  /* Select a precision to ensure that the conversion of d to x be exact. */
  prec = print_binary (d, 0);
  if (prec < MPFR_PREC_MIN)
    prec = MPFR_PREC_MIN;
  mpfr_init2 (x, prec);

  RND_LOOP(r)
    {
      inex = mpfr_set_ld (x, d, (mpfr_rnd_t) r);
      if (inex != 0)
        {
          printf ("Error: mpfr_set_ld should be exact (rnd = %s)\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          /* We use 36 digits here, as the maximum LDBL_MANT_DIG value
             seen in the current implementations is 113 (binary128),
             and ceil(1+113*log(2)/log(10)) = 36. But the current glibc
             implementation of printf with double-double arithmetic
             (e.g. on PowerPC) is not accurate. */
          printf ("  d ~= %.36Le (output may be wrong!)\n", d);
          printf ("  inex = %d\n", inex);
          if (emin >= LONG_MIN)
            printf ("  emin = %ld\n", (long) emin);
          if (emax <= LONG_MAX)
            printf ("  emax = %ld\n", (long) emax);
          ld_trace ("  d", d);
          printf ("  d = ");
          print_binary (d, 1);
          printf ("  x = ");
          mpfr_dump (x);
          printf ("  MPFR_LDBL_MANT_DIG=%u\n", MPFR_LDBL_MANT_DIG);
          printf ("  prec=%ld\n", (long) prec);
          print_binary (d, 2);
          exit (1);
        }
      for (red = 0; red < 2; red++)
        {
          if (red)
            {
              mpfr_exp_t ex;

              if (MPFR_IS_SINGULAR (x))
                break;
              ex = MPFR_GET_EXP (x);
              set_emin (ex);
              set_emax (ex);
            }
          e = mpfr_get_ld (x, (mpfr_rnd_t) r);
          set_emin (emin);
          set_emax (emax);
          if (inex == 0 && ((Isnan_ld(d) && ! Isnan_ld(e)) ||
                            (Isnan_ld(e) && ! Isnan_ld(d)) ||
                            (e != d && !(Isnan_ld(e) && Isnan_ld(d)))))
            {
              printf ("Error: mpfr_get_ld o mpfr_set_ld <> Id%s\n",
                      red ? ", reduced exponent range" : "");
              printf ("  rnd = %s\n", mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              printf ("  d ~= %.36Le (output may be wrong!)\n", d);
              printf ("  e ~= %.36Le (output may be wrong!)\n", e);
              ld_trace ("  d", d);
              printf ("  x = ");
              mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
              printf ("\n");
              ld_trace ("  e", e);
              printf ("  d = ");
              print_binary (d, 1);
              printf ("  x = ");
              mpfr_dump (x);
              printf ("  e = ");
              print_binary (e, 1);
              printf ("  MPFR_LDBL_MANT_DIG=%u\n", MPFR_LDBL_MANT_DIG);
#ifdef MPFR_NANISNAN
              if (Isnan_ld(d) || Isnan_ld(e))
                printf ("The reason is that NAN == NAN. Please look at the "
                        "configure output\nand Section \"In case of problem\""
                        " of the INSTALL file.\n");
#endif
              exit (1);
            }
        }
    }

  mpfr_clear (x);
}

static void
test_small (void)
{
  mpfr_t x, y, z;
  long double d;

  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);
  mpfr_init2 (y, MPFR_LDBL_MANT_DIG);
  mpfr_init2 (z, MPFR_LDBL_MANT_DIG);

  /* x = 11906603631607553907/2^(16381+64) */
  mpfr_set_str (x, "0.1010010100111100110000001110101101000111010110000001111101110011E-16381", 2, MPFR_RNDN);
  d = mpfr_get_ld (x, MPFR_RNDN);  /* infinite loop? */
  mpfr_set_ld (y, d, MPFR_RNDN);
  mpfr_sub (z, x, y, MPFR_RNDN);
  mpfr_abs (z, z, MPFR_RNDN);
  mpfr_clear_erangeflag ();
  /* If long double = double, d should be equal to 0;
     in this case, everything is OK. */
  if (d != 0 && (mpfr_cmp_str (z, "1E-16434", 2, MPFR_RNDN) > 0 ||
                 mpfr_erangeflag_p ()))
    {
      printf ("Error with x = ");
      mpfr_out_str (stdout, 10, 21, x, MPFR_RNDN);
      printf (" = ");
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
      printf ("\n        -> d = %.33Le", d);
      printf ("\n        -> y = ");
      mpfr_out_str (stdout, 10, 21, y, MPFR_RNDN);
      printf (" = ");
      mpfr_out_str (stdout, 16, 0, y, MPFR_RNDN);
      printf ("\n        -> |x-y| = ");
      mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

static void
test_fixed_bugs (void)
{
  mpfr_t x;
  long double l, m;

  /* bug found by Steve Kargl (2009-03-14) */
  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);
  mpfr_set_ui_2exp (x, 1, -16447, MPFR_RNDN);
  mpfr_get_ld (x, MPFR_RNDN);  /* an assertion failed in init2.c:50 */

  /* bug reported by Jakub Jelinek (2010-10-17)
     https://gforge.inria.fr/tracker/?func=detail&aid=11300 */
  mpfr_set_prec (x, MPFR_LDBL_MANT_DIG);
  /* l = 0x1.23456789abcdef0123456789abcdp-914L; */
  l = 8.215640181713713164092636634579e-276;
  mpfr_set_ld (x, l, MPFR_RNDN);
  m = mpfr_get_ld (x, MPFR_RNDN);
  if (m != l)
    {
      printf ("Error in get_ld o set_ld for l=%Le\n", l);
      printf ("Got m=%Le instead of l\n", m);
      exit (1);
    }

  /* another similar test which failed with extended double precision and the
     generic code for mpfr_set_ld */
  /* l = 0x1.23456789abcdef0123456789abcdp-968L; */
  l = 4.560596445887084662336528403703e-292;
  mpfr_set_ld (x, l, MPFR_RNDN);
  m = mpfr_get_ld (x, MPFR_RNDN);
  if (m != l)
    {
      printf ("Error in get_ld o set_ld for l=%Le\n", l);
      printf ("Got m=%Le instead of l\n", m);
      exit (1);
    }

  mpfr_clear (x);
}

static void
check_subnormal (void)
{
  long double d, e;
  mpfr_t x;

  d = 17.0;
  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);
  while (d != 0.0)
    {
      mpfr_set_ld (x, d, MPFR_RNDN);
      e = mpfr_get_ld (x, MPFR_RNDN);
      if (e != d)
        {
          printf ("Error in check_subnormal for mpfr_get_ld o mpfr_set_ld\n");
          printf ("d=%.30Le\n", d);
          printf ("x="); mpfr_dump (x);
          printf ("e=%.30Le\n", e);
          exit (1);
        }
      d *= 0.5;
    }
  mpfr_clear (x);
}

static void
check_overflow (void)
{
  long double d, e;
  mpfr_t x;
  int i;

  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);
  for (i = 0; i < 2; i++)
    {
      d = i == 0 ? LDBL_MAX : -LDBL_MAX;
      mpfr_set_ld (x, d, MPFR_RNDN);
      mpfr_mul_2ui (x, x, 1, MPFR_RNDN);
      e = mpfr_get_ld (x, MPFR_RNDN);
      if (! DOUBLE_ISINF (e) || (i == 0 ? (e < 0) : (e > 0)))
        {
          printf ("Error in check_overflow.\n");
          printf ("d=%Le\n", d);
          printf ("x="); mpfr_dump (x);
          printf ("e=%Le\n", e);
          exit (1);
        }
    }
  mpfr_clear (x);
}

/* issue reported by Sisyphus on powerpc */
static void
test_20140212 (void)
{
  mpfr_t fr1, fr2;
  long double ld, h, l, ld2;
  int i, c1, c2;

  mpfr_init2 (fr1, 106);
  mpfr_init2 (fr2, 2098);

  for (h = 1.0L, i = 0; i < 1023; i++)
    h *= 2.0L;
  for (l = 1.0L, i = 0; i < 1074; i++)
    l *= 0.5L;
  ld = h + l; /* rounding of 2^1023 + 2^(-1074) */

  mpfr_set_ld (fr1, ld, MPFR_RNDN);
  mpfr_set_ld (fr2, ld, MPFR_RNDN);

  c1 = mpfr_cmp_ld (fr1, ld);
  c2 = mpfr_cmp_ld (fr2, ld);

  /* If long double is binary64, then ld = fr1 = fr2 = 2^1023.
     If long double is double-double, then ld = 2^1023 + 2^(-1074),
     fr1 = 2^1023 and fr2 = 2^1023 + 2^(-1074) */
  MPFR_ASSERTN(ld == h ? (c1 == 0) : (c1 < 0));

  MPFR_ASSERTN(c2 == 0);

  ld2 = mpfr_get_ld (fr2, MPFR_RNDN);
  MPFR_ASSERTN(ld2 == ld);

  mpfr_clear (fr1);
  mpfr_clear (fr2);
}

/* bug reported by Walter Mascarenhas
   https://sympa.inria.fr/sympa/arc/mpfr/2016-09/msg00005.html */
static void
bug_20160907 (void)
{
#if HAVE_LDOUBLE_IEEE_EXT_LITTLE
  long double dn, ld;
  mpfr_t mp;
  long e;
  mpfr_long_double_t x;

  /* the following is the encoding of the smallest subnormal number
     for HAVE_LDOUBLE_IEEE_EXT_LITTLE */
  x.s.manl = 1;
  x.s.manh = 0;
  x.s.expl = 0;
  x.s.exph = 0;
  x.s.sign= 0;
  dn = x.ld;
  e = -16445;
  /* dn=2^e is now the smallest subnormal. */

  mpfr_init2 (mp, 64);
  mpfr_set_ui_2exp (mp, 1, e - 1, MPFR_RNDN);
  ld = mpfr_get_ld (mp, MPFR_RNDU);
  /* since mp = 2^(e-1) and ld is rounded upwards, we should have
     ld = 2^e */
  if (ld != dn)
    {
      printf ("Error, ld = %Le <> dn = %Le\n", ld, dn);
      printf ("mp=");
      mpfr_out_str (stdout, 10, 0, mp, MPFR_RNDN);
      printf ("\n");
      printf ("mp="); mpfr_dump (mp);
      exit (1);
    }

  /* check a few more numbers */
  for (e = -16446; e <= -16381; e++)
    {
      mpfr_set_ui_2exp (mp, 1, e, MPFR_RNDN);
      ld = mpfr_get_ld (mp, MPFR_RNDU);
      mpfr_set_ld (mp, ld, MPFR_RNDU);
      /* mp is 2^e rounded up, thus should be >= 2^e */
      if (mpfr_cmp_ui_2exp (mp, 1, e) < 0)
        {
          if (tests_run_within_valgrind () && MPFR_IS_ZERO (mp))
            {
              /* Since this is not a bug in MPFR and it is just caused by
                 Valgrind, let's output a message and skip the remaining
                 part of the test without an error. Note that the message
                 will be not be visible via "make check".
                 Note that the other tests do not fail probably because
                 long double has the same behavior as double (which is
                 allowed by the C standard), but here this is a test that
                 is specific to x86 extended precision. */
              printf
                ("Error in bug_20160907 due to a bug in Valgrind.\n"
                 "https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=890215\n"
                 "https://bugs.kde.org/show_bug.cgi?id=421262\n");
              break;
            }
          printf ("Error, expected value >= 2^(%ld)\n", e);
          printf ("got "); mpfr_dump (mp);
          exit (1);
        }

      mpfr_set_ui_2exp (mp, 1, e, MPFR_RNDN);
      ld = mpfr_get_ld (mp, MPFR_RNDD);
      mpfr_set_ld (mp, ld, MPFR_RNDD);
      /* mp is 2^e rounded down, thus should be <= 2^e */
      if (mpfr_cmp_ui_2exp (mp, 1, e) > 0)
        {
          printf ("Error, expected value <= 2^(%ld)\n", e);
          printf ("got "); mpfr_dump (mp);
          exit (1);
        }
    }

  mpfr_clear (mp);
#endif
}

int
main (int argc, char *argv[])
{
  volatile long double d, e, maxp2;
  mpfr_t x;
  int i;
  mpfr_exp_t emax;

  tests_start_mpfr ();
  mpfr_test_init ();

  check_gcc33_bug ();
  test_fixed_bugs ();

  mpfr_init2 (x, MPFR_LDBL_MANT_DIG + 64);

#if !defined(MPFR_ERRDIVZERO)
  /* check NaN */
  mpfr_set_nan (x);
  d = mpfr_get_ld (x, MPFR_RNDN);
  check_set_get (d);
#endif

  /* check +0.0 and -0.0 */
  d = 0.0;
  check_set_get (d);
  d = DBL_NEG_ZERO;
  check_set_get (d);

  /* check that the sign of -0.0 is set */
  mpfr_set_ld (x, DBL_NEG_ZERO, MPFR_RNDN);
  if (MPFR_IS_POS (x))
    {
#if defined(HAVE_SIGNEDZ)
      printf ("Error: sign of -0.0 is not set correctly\n");
      exit (1);
#else
      /* Non IEEE doesn't support negative zero yet */
      printf ("Warning: sign of -0.0 is not set correctly\n");
#endif
    }

#if !defined(MPFR_ERRDIVZERO)
  /* check +Inf */
  mpfr_set_inf (x, 1);
  d = mpfr_get_ld (x, MPFR_RNDN);
  check_set_get (d);

  /* check -Inf */
  mpfr_set_inf (x, -1);
  d = mpfr_get_ld (x, MPFR_RNDN);
  check_set_get (d);
#endif

  /* check the largest power of two */
  maxp2 = 1.0;
  while (maxp2 < LDBL_MAX / 2.0)
    maxp2 *= 2.0;
  check_set_get (maxp2);
  check_set_get (-maxp2);

  d = LDBL_MAX;
  e = d / 2.0;
  if (e != maxp2)  /* false under NetBSD/x86 */
    {
      /* d = LDBL_MAX does not have excess precision. */
      check_set_get (d);
      check_set_get (-d);
    }

  /* check the smallest power of two */
  d = 1.0;
  while ((e = d / 2.0) != (long double) 0.0 && e != d)
    d = e;
  check_set_get (d);
  check_set_get (-d);

  /* check that 2^i, 2^i+1, 2^i-1 and 2^i-2^(i-2)-1 are correctly converted */
  d = 1.0;
  for (i = 1; i < MPFR_LDBL_MANT_DIG + 8; i++)
    {
      d = 2.0 * d; /* d = 2^i */
      check_set_get (d);
      if (d + 1.0 != d)
        check_set_get (d + 1.0);
      else
        {
          mpfr_set_ui_2exp (x, 1, i, MPFR_RNDN);
          mpfr_add_ui (x, x, 1, MPFR_RNDN);
          e = mpfr_get_ld (x, MPFR_RNDN);
          check_set_get (e);
        }
      if (d - 1.0 != d)
        check_set_get (d - 1.0);
      else
        {
          mpfr_set_ui_2exp (x, 1, i, MPFR_RNDN);
          mpfr_sub_ui (x, x, 1, MPFR_RNDN);
          e = mpfr_get_ld (x, MPFR_RNDN);
          check_set_get (e);
        }
      if (i < 3)
        continue;
      /* The following test triggers a failure in r10844 for i = 56,
         with gcc -mpc64 on x86 (64-bit ABI). */
      mpfr_set_ui_2exp (x, 3, i-2, MPFR_RNDN);
      mpfr_sub_ui (x, x, 1, MPFR_RNDN);
      e = mpfr_get_ld (x, MPFR_RNDN);
      check_set_get (e);
    }

  for (i = 0; i < 10000; i++)
    {
      mpfr_urandomb (x, RANDS);
      d = mpfr_get_ld (x, MPFR_RNDN);
      check_set_get (d);
    }

  /* check with reduced emax to exercise overflow */
  emax = mpfr_get_emax ();
  mpfr_set_prec (x, 2);
  set_emax (1);
  mpfr_set_ld (x, (long double) 2.0, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);
  for (d = (long double) 2.0, i = 0; i < 13; i++, d *= d);
  /* now d = 2^8192, or an infinity (e.g. with double or double-double) */
  mpfr_set_ld (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);
  set_emax (emax);

  mpfr_clear (x);

  test_small ();

  check_subnormal ();
#if !defined(MPFR_ERRDIVZERO)
  check_overflow ();
#endif

  test_20140212 ();
  bug_20160907 ();

  tests_end_mpfr ();

  return 0;
}
