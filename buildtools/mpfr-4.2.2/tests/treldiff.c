/* Test file for mpfr_reldiff.

Copyright 2000-2025 Free Software Foundation, Inc.
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

#define SPECIAL_MAX 12

/* copied from reuse.c */
static void
set_special (mpfr_ptr x, unsigned int select)
{
  MPFR_ASSERTN (select < SPECIAL_MAX);
  switch (select)
    {
    case 0:
      MPFR_SET_NAN (x);
      break;
    case 1:
      MPFR_SET_INF (x);
      MPFR_SET_POS (x);
      break;
    case 2:
      MPFR_SET_INF (x);
      MPFR_SET_NEG (x);
      break;
    case 3:
      MPFR_SET_ZERO (x);
      MPFR_SET_POS  (x);
      break;
    case 4:
      MPFR_SET_ZERO (x);
      MPFR_SET_NEG  (x);
      break;
    case 5:
      mpfr_set_str_binary (x, "1");
      break;
    case 6:
      mpfr_set_str_binary (x, "-1");
      break;
    case 7:
      mpfr_set_str_binary (x, "1e-1");
      break;
    case 8:
      mpfr_set_str_binary (x, "1e+1");
      break;
    case 9:
      mpfr_const_pi (x, MPFR_RNDN);
      break;
    case 10:
      mpfr_const_pi (x, MPFR_RNDN);
      MPFR_SET_EXP (x, MPFR_GET_EXP (x)-1);
      break;
    case 11:
      mpfr_urandomb (x, RANDS);
      if (RAND_BOOL ())
        mpfr_neg (x, x, MPFR_RNDN);
      break;
    default:
      MPFR_ASSERTN (0);
    }
}

int
main (void)
{
  mpfr_t a1, a2, b, c;
  int pa, pb, pc, i, j, r;
  int prec[] = { MPFR_PREC_MIN, 3, 16, 32, 53 };

  tests_start_mpfr ();

  /* Just check that mpfr_reldiff(a,b,c,rnd) computes |b-c|/b using the
     precision of a, without any optimization. */

  for (pa = 0; pa < numberof(prec); pa++)
    {
      mpfr_inits2 (prec[pa], a1, a2, (mpfr_ptr) 0);
      for (pb = 0; pb < numberof(prec); pb++)
        {
          mpfr_init2 (b, prec[pb]);
          for (pc = 0; pc < numberof(prec); pc++)
            {
              mpfr_init2 (c, prec[pc]);
              for (i = 0; i < SPECIAL_MAX; i++)
                {
                  set_special (b, i);
                  for (j = 0; j < SPECIAL_MAX; j++)
                    {
                      set_special (c, j);
                      RND_LOOP (r)
                        {
                          mpfr_rnd_t rnd = (mpfr_rnd_t) r;

                          mpfr_sub (a1, b, c, rnd);
                          mpfr_abs (a1, a1, rnd);
                          mpfr_div (a1, a1, b, rnd);
                          mpfr_reldiff (a2, b, c, rnd);
                          if (! SAME_VAL (a1, a2))
                            {
                              printf ("Error for pa=%d pb=%d pc=%d "
                                      "i=%d j=%d rnd=%s\n", pa, pb, pc,
                                      i, j, mpfr_print_rnd_mode (rnd));
                              printf ("expected ");
                              mpfr_dump (a1);
                              printf ("got      ");
                              mpfr_dump (a2);
                              exit (1);
                            }
                        }
                    }
                }
              mpfr_clear (c);
            }
          mpfr_clear (b);
        }
      mpfr_clears (a1, a2, (mpfr_ptr) 0);
    }

  tests_end_mpfr ();
  return 0;
}
