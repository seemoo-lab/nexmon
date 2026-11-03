/* mpfr_sub1sp1 -- internal function to perform a "real" subtraction on one limb
   This code was extracted by Kremlin from a formal proof in F*
   done by Félix Breton in June-July 2019: do not modify it!

Source: https://github.com/project-everest/hacl-star/tree/dev_mpfr/code/mpfr

Copyright 2004-2025 Free Software Foundation, Inc.
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

#include <stdint.h>
#include <inttypes.h> /* for __builtin_clzll */

/* beginning of manually added declarations */

#define MPFR_mpfr_sub1sp1 mpfr_sub1sp1
#define MPFR_RoundingMode_mpfr_rnd_t mpfr_rnd_t
#define MPFR_Lib_mpfr_struct __mpfr_struct
#define MPFR_Lib_gmp_NUMB_BITS GMP_NUMB_BITS
#define __eq__MPFR_RoundingMode_mpfr_rnd_t(x,y) ((x)==(y))
#define MPFR_RoundingMode_MPFR_RNDD MPFR_RNDD
#define MPFR_RoundingMode_MPFR_RNDN MPFR_RNDN
#define MPFR_RoundingMode_MPFR_RNDZ MPFR_RNDZ
#define MPFR_RoundingMode_MPFR_RNDU MPFR_RNDU
#define MPFR_Lib_mpfr_SET_EXP MPFR_SET_EXP
#define MPFR_Lib_mpfr_RET MPFR_RET
#define MPFR_Lib_mpfr_NEG_SIGN(x) (-(x))
#define MPFR_Exceptions_mpfr_underflow mpfr_underflow
#define MPFR_Exceptions_mpfr_overflow  mpfr_overflow
#define true 1
#define false 0

/* the original code had mpfr_exp instead of _mpfr_exp */
#define mpfr_exp _mpfr_exp
/* the original code had mpfr_d instead of _mpfr_d */
#define mpfr_d _mpfr_d
/* the original code had mpfr_prec instead of _mpfr_prec */
#define mpfr_prec _mpfr_prec
/* the original code had mpfr_sign instead of _mpfr_sign */
#define mpfr_sign _mpfr_sign
#define bool int /* to avoid conflict with C++ keyword */

/* end of manually added declarations */

static uint32_t MPFR_Lib_Clz_count_leading_zeros (uint64_t a) {
  return __builtin_clzll(a);
}

typedef struct MPFR_Add1sp1_state_s
{
  int64_t sh;
  int64_t bx;
  uint64_t rb;
  uint64_t sb;
}
MPFR_Add1sp1_state;

typedef struct
K___MPFR_Lib_mpfr_struct__MPFR_Lib_mpfr_struct__int64_t_int64_t_uint64_t__uint64_t__s
{
  const MPFR_Lib_mpfr_struct *fst; /* added const */
  const MPFR_Lib_mpfr_struct *snd; /* added const */
  int64_t thd;
  int64_t f3;
  uint64_t *f4;
  uint64_t *f5;
}
K___MPFR_Lib_mpfr_struct__MPFR_Lib_mpfr_struct__int64_t_int64_t_uint64_t__uint64_t_;

static MPFR_Add1sp1_state
MPFR_Add1sp1_mk_state(int64_t sh, int64_t bx, uint64_t rb, uint64_t sb)
{
  MPFR_Add1sp1_state lit;
  lit.sh = sh;
  lit.bx = bx;
  lit.rb = rb;
  lit.sb = sb;
  return lit;
}

static MPFR_Add1sp1_state
MPFR_Sub1sp1_mpfr_sub1sp1_gt_branch_1(
  MPFR_Lib_mpfr_struct *a,
  const MPFR_Lib_mpfr_struct *b, /* added const */
  const MPFR_Lib_mpfr_struct *c, /* added const */
  uint64_t *ap,
  uint64_t *bp,
  uint64_t *cp,
  int64_t bx,
  int64_t cx,
  int64_t p,
  int64_t sh,
  uint64_t mask,
  uint64_t sb_1,
  uint64_t a0_base
)
{
  uint32_t cnt = MPFR_Lib_Clz_count_leading_zeros(a0_base);
  uint64_t a0;
  if (cnt == (uint32_t)0U)
  {
    a0 = a0_base;
  }
  else
  {
    a0 = a0_base << cnt | sb_1 >> ((uint32_t)64U - cnt);
  }
  {
    uint64_t sb_2 = sb_1 << cnt;
    uint64_t rb = a0 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
    uint64_t sb = sb_2 | ((a0 & mask) ^ rb);
    ap[0U] = a0 & ~mask;
    return MPFR_Add1sp1_mk_state(sh, bx - (int64_t)cnt, rb, sb);
  }
}

static bool MPFR_RoundingMode_uu___is_MPFR_RNDN(MPFR_RoundingMode_mpfr_rnd_t projectee)
{
  switch (projectee)
  {
    case MPFR_RoundingMode_MPFR_RNDN:
      {
        return true;
      }
    default:
      {
        return false;
      }
  }
}

static bool MPFR_RoundingMode_uu___is_MPFR_RNDZ(MPFR_RoundingMode_mpfr_rnd_t projectee)
{
  switch (projectee)
  {
    case MPFR_RoundingMode_MPFR_RNDZ:
      {
        return true;
      }
    default:
      {
        return false;
      }
  }
}

static bool MPFR_RoundingMode_uu___is_MPFR_RNDU(MPFR_RoundingMode_mpfr_rnd_t projectee)
{
  switch (projectee)
  {
    case MPFR_RoundingMode_MPFR_RNDU:
      {
        return true;
      }
    default:
      {
        return false;
      }
  }
}

static bool MPFR_RoundingMode_uu___is_MPFR_RNDD(MPFR_RoundingMode_mpfr_rnd_t projectee)
{
  switch (projectee)
  {
    case MPFR_RoundingMode_MPFR_RNDD:
      {
        return true;
      }
    default:
      {
        return false;
      }
  }
}

static int32_t
MPFR_Sub1sp1_mpfr_sub1sp1(
  MPFR_Lib_mpfr_struct *a,
  const MPFR_Lib_mpfr_struct *b, /* added const */
  const MPFR_Lib_mpfr_struct *c, /* added const */
  MPFR_RoundingMode_mpfr_rnd_t rnd_mode,
  int64_t p
)
{
  int64_t bx = b->mpfr_exp;
  int64_t cx = c->mpfr_exp;
  uint64_t *ap = a->mpfr_d;
  uint64_t *bp = b->mpfr_d;
  uint64_t *cp = c->mpfr_d;
  int64_t sh = MPFR_Lib_gmp_NUMB_BITS - p;
  uint64_t bp0ul = bp[0U];
  uint64_t cp0ul = cp[0U];
  if (bx == cx && bp0ul == cp0ul)
  {
    if (__eq__MPFR_RoundingMode_mpfr_rnd_t(rnd_mode, MPFR_RoundingMode_MPFR_RNDD))
    {
      MPFR_Lib_mpfr_struct uu____0 = a[0U];
      MPFR_Lib_mpfr_struct lit;
      lit.mpfr_prec = uu____0.mpfr_prec;
      lit.mpfr_sign = (int32_t)-1;
      lit.mpfr_exp = uu____0.mpfr_exp;
      lit.mpfr_d = uu____0.mpfr_d;
      a[0U] = lit;
    }
    else
    {
      MPFR_Lib_mpfr_struct uu____1 = a[0U];
      MPFR_Lib_mpfr_struct lit;
      lit.mpfr_prec = uu____1.mpfr_prec;
      lit.mpfr_sign = (int32_t)1;
      lit.mpfr_exp = uu____1.mpfr_exp;
      lit.mpfr_d = uu____1.mpfr_d;
      a[0U] = lit;
    }
    MPFR_Lib_mpfr_SET_EXP(a, (int64_t)-0x7fffffffffffffff);
    /* the following return was commented out from the extracted code */
    /*return*/ MPFR_Lib_mpfr_RET((int32_t)0);
  }
  else
  {
    MPFR_Add1sp1_state st;
    if (bx == cx)
    {
      /* Prims_int vb = FStar_UInt64_v(bp[0U]); */ /* unused */
      /* Prims_int vc = FStar_UInt64_v(cp[0U]); */ /* unused */
      /* Prims_int vsh = FStar_Int64_v(sh); */     /* unused */
      if (cp[0U] > bp[0U])
      {
        uint64_t a0 = cp[0U] - bp[0U];
        uint32_t cnt = MPFR_Lib_Clz_count_leading_zeros(a0);
        int32_t uu____2 = MPFR_Lib_mpfr_NEG_SIGN(b->mpfr_sign);
        MPFR_Lib_mpfr_struct uu____3 = a[0U];
        MPFR_Lib_mpfr_struct lit;
        lit.mpfr_prec = uu____3.mpfr_prec;
        lit.mpfr_sign = uu____2;
        lit.mpfr_exp = uu____3.mpfr_exp;
        lit.mpfr_d = uu____3.mpfr_d;
        a[0U] = lit;
        ap[0U] = a0 << cnt;
        {
          int64_t bx1 = bx - (int64_t)cnt;
          st = MPFR_Add1sp1_mk_state(sh, bx1, (uint64_t)0U, (uint64_t)0U);
        }
      }
      else
      {
        uint64_t a0 = bp[0U] - cp[0U];
        uint32_t cnt = MPFR_Lib_Clz_count_leading_zeros(a0);
        MPFR_Lib_mpfr_struct uu____4 = a[0U];
        MPFR_Lib_mpfr_struct lit;
        lit.mpfr_prec = uu____4.mpfr_prec;
        lit.mpfr_sign = b->mpfr_sign;
        lit.mpfr_exp = uu____4.mpfr_exp;
        lit.mpfr_d = uu____4.mpfr_d;
        a[0U] = lit;
        ap[0U] = a0 << cnt;
        {
          int64_t bx1 = bx - (int64_t)cnt;
          st = MPFR_Add1sp1_mk_state(sh, bx1, (uint64_t)0U, (uint64_t)0U);
        }
      }
    }
    else
    {
      K___MPFR_Lib_mpfr_struct__MPFR_Lib_mpfr_struct__int64_t_int64_t_uint64_t__uint64_t_ scrut;
      if (bx >= cx)
      {
        MPFR_Lib_mpfr_struct uu____5 = a[0U];
        MPFR_Lib_mpfr_struct lit;
        lit.mpfr_prec = uu____5.mpfr_prec;
        lit.mpfr_sign = b->mpfr_sign;
        lit.mpfr_exp = uu____5.mpfr_exp;
        lit.mpfr_d = uu____5.mpfr_d;
        a[0U] = lit;
        {
          K___MPFR_Lib_mpfr_struct__MPFR_Lib_mpfr_struct__int64_t_int64_t_uint64_t__uint64_t_ lit0;
          lit0.fst = b;
          lit0.snd = c;
          lit0.thd = bx;
          lit0.f3 = cx;
          lit0.f4 = bp;
          lit0.f5 = cp;
          scrut = lit0;
        }
      }
      else
      {
        int32_t uu____6 = MPFR_Lib_mpfr_NEG_SIGN(b->mpfr_sign);
        MPFR_Lib_mpfr_struct uu____7 = a[0U];
        MPFR_Lib_mpfr_struct lit;
        lit.mpfr_prec = uu____7.mpfr_prec;
        lit.mpfr_sign = uu____6;
        lit.mpfr_exp = uu____7.mpfr_exp;
        lit.mpfr_d = uu____7.mpfr_d;
        a[0U] = lit;
        {
          K___MPFR_Lib_mpfr_struct__MPFR_Lib_mpfr_struct__int64_t_int64_t_uint64_t__uint64_t_ lit0;
          lit0.fst = c;
          lit0.snd = b;
          lit0.thd = cx;
          lit0.f3 = bx;
          lit0.f4 = cp;
          lit0.f5 = bp;
          scrut = lit0;
        }
      }
      {
        const MPFR_Lib_mpfr_struct *b1 = scrut.fst; /* added const */
        const MPFR_Lib_mpfr_struct *c1 = scrut.snd; /* added const */
        int64_t bx1 = scrut.thd;
        int64_t cx1 = scrut.f3;
        uint64_t *bp1 = scrut.f4;
        uint64_t *cp1 = scrut.f5;
        int64_t d = bx1 - cx1;
        uint64_t bp0ul1 = bp1[0U];
        uint64_t cp0ul1 = cp1[0U];
        uint64_t mask = ((uint64_t)1U << (uint32_t)sh) - (uint64_t)1U;
        if (d < (int64_t)64)
        {
          uint64_t sb_1 = ~(cp0ul1 << (uint32_t)(MPFR_Lib_gmp_NUMB_BITS - d)) + (uint64_t)1U;
          uint64_t ite;
          if (sb_1 == (uint64_t)0U)
          {
            ite = (uint64_t)0U;
          }
          else
          {
            ite = (uint64_t)1U;
          }
          st =
            MPFR_Sub1sp1_mpfr_sub1sp1_gt_branch_1(a,
              b1,
              c1,
              ap,
              bp1,
              cp1,
              bx1,
              cx1,
              p,
              sh,
              mask,
              sb_1,
              bp0ul1 - ite - (cp0ul1 >> (uint32_t)d));
        }
        else if (bp0ul1 > (uint64_t)0x8000000000000000U)
        {
          ap[0U] = bp0ul1 - ((uint64_t)1U << (uint32_t)sh);
          st = MPFR_Add1sp1_mk_state(sh, bx1, (uint64_t)1U, (uint64_t)1U);
        }
        else
        {
          bool
          rb =
            sh
            > (int64_t)1
            || d > MPFR_Lib_gmp_NUMB_BITS
            || cp0ul1 == (uint64_t)0x8000000000000000U;
          bool
          sb =
            sh
            > (int64_t)1
            || d > MPFR_Lib_gmp_NUMB_BITS
            || cp0ul1 != (uint64_t)0x8000000000000000U;
          ap[0U] = ~mask;
          {
            uint64_t ite0;
            if (rb)
            {
              ite0 = (uint64_t)1U;
            }
            else
            {
              ite0 = (uint64_t)0U;
            }
            {
              uint64_t ite;
              if (sb)
              {
                ite = (uint64_t)1U;
              }
              else
              {
                ite = (uint64_t)0U;
              }
              st = MPFR_Add1sp1_mk_state(sh, bx1 - (int64_t)1, ite0, ite);
            }
          }
        }
      }
    }
    /* the constant (int64_t)-0x000000003fffffff from the original extracted
       code was manually replaced by __gmpfr_emin */
    if (st.bx < __gmpfr_emin)
    {
      int32_t s = a->mpfr_sign;
      uint64_t ap0ul = ap[0U];
      if
      (
        __eq__MPFR_RoundingMode_mpfr_rnd_t(rnd_mode,
          MPFR_RoundingMode_MPFR_RNDN)
        /* the constant (int64_t)-1073741824 from the original extracted
           code was manually replaced by __gmpfr_emin-1 */
        && (st.bx < __gmpfr_emin - 1 || ap0ul == (uint64_t)0x8000000000000000U)
      )
      {
        MPFR_Lib_mpfr_SET_EXP(a, (int64_t)-0x7fffffffffffffff);
        return MPFR_Lib_mpfr_NEG_SIGN(s);
      }
      else
      {
        int32_t t = MPFR_Exceptions_mpfr_underflow(a, rnd_mode, s);
        return t;
      }
    }
    else
    {
      uint64_t a0 = ap[0U];
      MPFR_Lib_mpfr_SET_EXP(a, st.bx);
      if (st.rb == (uint64_t)0U && st.sb == (uint64_t)0U)
      {
        /* the following return was commented out from the extracted code */
        /*return*/ MPFR_Lib_mpfr_RET((int32_t)0);
      }
      else if (MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode))
      {
        if
        (
          st.rb
          == (uint64_t)0U
          || (st.sb == (uint64_t)0U && (a0 & (uint64_t)1U << (uint32_t)st.sh) == (uint64_t)0U)
        )
        {
          /* the following return was commented out from the extracted code */
          /*return*/ MPFR_Lib_mpfr_RET(MPFR_Lib_mpfr_NEG_SIGN(a->mpfr_sign));
        }
        else if (ap[0U] + ((uint64_t)1U << (uint32_t)st.sh) == (uint64_t)0U)
        {
          ap[0U] = ap[0U] + ((uint64_t)1U << (uint32_t)st.sh);
          ap[0U] = (uint64_t)0x8000000000000000U;
          /* the constant (int64_t)0x000000003fffffff from the original
             extracted code was replaced by __gmpfr_emax */
          if (st.bx + (int64_t)1 <= __gmpfr_emax)
          {
            MPFR_Lib_mpfr_SET_EXP(a, st.bx + (int64_t)1);
            /* the following return was commented out from the extracted code */
            /*return*/ MPFR_Lib_mpfr_RET(a->mpfr_sign);
          }
          else
          {
            int32_t t = MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
            /* the following return was commented out from the extracted code */
            /*return*/ MPFR_Lib_mpfr_RET(t);
          }
        }
        else
        {
          ap[0U] = ap[0U] + ((uint64_t)1U << (uint32_t)st.sh);
          /* the following return was commented out from the extracted code */
          /*return*/ MPFR_Lib_mpfr_RET(a->mpfr_sign);
        }
      }
      else
      {
        bool uu____8 = a->mpfr_sign < (int32_t)0;
        if
        (
          MPFR_RoundingMode_uu___is_MPFR_RNDZ(rnd_mode)
          || (MPFR_RoundingMode_uu___is_MPFR_RNDU(rnd_mode) && uu____8)
          || (MPFR_RoundingMode_uu___is_MPFR_RNDD(rnd_mode) && !uu____8)
        )
        {
          /* the following return was commented out from the extracted code */
          /*return*/ MPFR_Lib_mpfr_RET(MPFR_Lib_mpfr_NEG_SIGN(a->mpfr_sign));
        }
        else if (ap[0U] + ((uint64_t)1U << (uint32_t)st.sh) == (uint64_t)0U)
        {
          ap[0U] = ap[0U] + ((uint64_t)1U << (uint32_t)st.sh);
          ap[0U] = (uint64_t)0x8000000000000000U;
          /* the constant (int64_t)0x000000003fffffff from the original
             extracted code was replaced by __gmpfr_emax */
          if (st.bx + (int64_t)1 <= __gmpfr_emax)
          {
            MPFR_Lib_mpfr_SET_EXP(a, st.bx + (int64_t)1);
            /* the following return was commented out from the extracted code */
            /*return*/ MPFR_Lib_mpfr_RET(a->mpfr_sign);
          }
          else
          {
            int32_t t = MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
            /* the following return was commented out from the extracted code */
            /*return*/ MPFR_Lib_mpfr_RET(t);
          }
        }
        else
        {
          ap[0U] = ap[0U] + ((uint64_t)1U << (uint32_t)st.sh);
          /* the following return was commented out from the extracted code */
          /*return*/ MPFR_Lib_mpfr_RET(a->mpfr_sign);
        }
      }
    }
  }
}

int32_t
(*MPFR_mpfr_sub1sp1)(
  MPFR_Lib_mpfr_struct *x0,
  const MPFR_Lib_mpfr_struct *x1, /* added const */
  const MPFR_Lib_mpfr_struct *x2, /* added const */
  MPFR_RoundingMode_mpfr_rnd_t x3,
  int64_t x4
) = MPFR_Sub1sp1_mpfr_sub1sp1;

