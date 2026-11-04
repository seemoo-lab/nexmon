/* mpfr_add1sp1 -- internal function to perform a "real" addition on one limb
   This code was extracted by Kremlin from a formal proof in F*
   done by Jianyang Pan in April-August 2018: do not modify it!

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

#define int32_t int
#define int64_t long
#define uint32_t unsigned int
#define uint64_t mp_limb_t

typedef struct MPFR_Add1sp1_state_s
{
  int64_t sh;
  int64_t bx;
  uint64_t rb;
  uint64_t sb;
}
MPFR_Add1sp1_state;

static MPFR_Add1sp1_state
MPFR_Add1sp1_mk_state(int64_t sh, int64_t bx, uint64_t rb, uint64_t sb)
{
  return ((MPFR_Add1sp1_state){ .sh = sh, .bx = bx, .rb = rb, .sb = sb });
}

typedef struct K___uint64_t_int64_t_s
{
  uint64_t fst;
  int64_t snd;
}
K___uint64_t_int64_t;

typedef struct K___uint64_t_uint64_t_int64_t_s
{
  uint64_t fst;
  uint64_t snd;
  int64_t thd;
}
K___uint64_t_uint64_t_int64_t;

#define MPFR_Lib_mpfr_struct __mpfr_struct
#define MPFR_Lib_mpfr_RET(I) ((I) != 0 ? ((__gmpfr_flags |= MPFR_FLAGS_INEXACT), (I)) : 0)
#define MPFR_Exceptions_mpfr_overflow mpfr_overflow
#define mpfr_prec _mpfr_prec
#define mpfr_exp _mpfr_exp
#define mpfr_d _mpfr_d
#define mpfr_sign _mpfr_sign
#define MPFR_Lib_gmp_NUMB_BITS GMP_NUMB_BITS
#define MPFR_Lib_mpfr_EMAX __gmpfr_emax

#define MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode) (rnd_mode == MPFR_RNDN)
#define MPFR_RoundingMode_mpfr_IS_LIKE_RNDZ MPFR_IS_LIKE_RNDZ

/* same as mpfr_add1sp, but for p < GMP_NUMB_BITS */
static int
mpfr_add1sp1 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  MPFR_Lib_mpfr_struct a0 = a[0U];
  MPFR_Lib_mpfr_struct b0 = b[0U];
  MPFR_Lib_mpfr_struct c0 = c[0U];
  int64_t bx = b0.mpfr_exp;
  int64_t cx = c0.mpfr_exp;
  int64_t sh = MPFR_Lib_gmp_NUMB_BITS - p;
  MPFR_Add1sp1_state st;
  if (bx == cx)
  {
    uint64_t *ap = a0.mpfr_d;
    uint64_t *bp = b0.mpfr_d;
    uint64_t *cp = c0.mpfr_d;
    uint64_t a01 = (bp[0U] >> (uint32_t)1U) + (cp[0U] >> (uint32_t)1U);
    int64_t bx1 = b0.mpfr_exp + (int64_t)1;
    uint64_t rb = a01 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
    ap[0U] = a01 ^ rb;
    uint64_t sb = (uint64_t)0U;
    st = MPFR_Add1sp1_mk_state(sh, bx1, rb, sb);
  }
  else
  {
    MPFR_Add1sp1_state ite0;
    if (bx > cx)
    {
      int64_t bx1 = b0.mpfr_exp;
      int64_t cx1 = c0.mpfr_exp;
      int64_t d = bx1 - cx1;
      uint64_t mask = ((uint64_t)1U << (uint32_t)sh) - (uint64_t)1U;
      MPFR_Add1sp1_state ite1;
      if (d < sh)
      {
        uint64_t *ap = a0.mpfr_d;
        uint64_t *bp = b0.mpfr_d;
        uint64_t *cp = c0.mpfr_d;
        int64_t bx2 = b0.mpfr_exp;
        uint64_t a01 = bp[0U] + (cp[0U] >> (uint32_t)d);
        K___uint64_t_int64_t scrut;
        if (a01 < bp[0U])
          scrut =
            (
              (K___uint64_t_int64_t){
                .fst = (uint64_t)0x8000000000000000U | a01 >> (uint32_t)1U,
                .snd = bx2 + (int64_t)1
              }
            );
        else
          scrut = ((K___uint64_t_int64_t){ .fst = a01, .snd = bx2 });
        uint64_t a02 = scrut.fst;
        int64_t bx3 = scrut.snd;
        uint64_t rb = a02 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
        uint64_t sb = (a02 & mask) ^ rb;
        ap[0U] = a02 & ~mask;
        ite1 = MPFR_Add1sp1_mk_state(sh, bx3, rb, sb);
      }
      else
      {
        MPFR_Add1sp1_state ite;
        if (d < MPFR_Lib_gmp_NUMB_BITS)
        {
          uint64_t *ap = a0.mpfr_d;
          uint64_t *bp = b0.mpfr_d;
          uint64_t *cp = c0.mpfr_d;
          int64_t bx2 = b0.mpfr_exp;
          uint64_t sb = cp[0U] << (uint32_t)(MPFR_Lib_gmp_NUMB_BITS - d);
          uint64_t a01 = bp[0U] + (cp[0U] >> (uint32_t)d);
          K___uint64_t_uint64_t_int64_t scrut;
          if (a01 < bp[0U])
            scrut =
              (
                (K___uint64_t_uint64_t_int64_t){
                  .fst = sb | (a01 & (uint64_t)1U),
                  .snd = (uint64_t)0x8000000000000000U | a01 >> (uint32_t)1U,
                  .thd = bx2 + (int64_t)1
                }
              );
          else
            scrut = ((K___uint64_t_uint64_t_int64_t){ .fst = sb, .snd = a01, .thd = bx2 });
          uint64_t sb1 = scrut.fst;
          uint64_t a02 = scrut.snd;
          int64_t bx3 = scrut.thd;
          uint64_t rb = a02 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
          uint64_t sb2 = sb1 | ((a02 & mask) ^ rb);
          ap[0U] = a02 & ~mask;
          ite = MPFR_Add1sp1_mk_state(sh, bx3, rb, sb2);
        }
        else
        {
          uint64_t *ap = a0.mpfr_d;
          uint64_t *bp = b0.mpfr_d;
          int64_t bx2 = b0.mpfr_exp;
          ap[0U] = bp[0U];
          uint64_t rb = (uint64_t)0U;
          uint64_t sb = (uint64_t)1U;
          ite = MPFR_Add1sp1_mk_state(sh, bx2, rb, sb);
        }
        ite1 = ite;
      }
      ite0 = ite1;
    }
    else
    {
      int64_t bx1 = c0.mpfr_exp;
      int64_t cx1 = b0.mpfr_exp;
      int64_t d = bx1 - cx1;
      uint64_t mask = ((uint64_t)1U << (uint32_t)sh) - (uint64_t)1U;
      MPFR_Add1sp1_state ite1;
      if (d < sh)
      {
        uint64_t *ap = a0.mpfr_d;
        uint64_t *bp = c0.mpfr_d;
        uint64_t *cp = b0.mpfr_d;
        int64_t bx2 = c0.mpfr_exp;
        uint64_t a01 = bp[0U] + (cp[0U] >> (uint32_t)d);
        K___uint64_t_int64_t scrut;
        if (a01 < bp[0U])
          scrut =
            (
              (K___uint64_t_int64_t){
                .fst = (uint64_t)0x8000000000000000U | a01 >> (uint32_t)1U,
                .snd = bx2 + (int64_t)1
              }
            );
        else
          scrut = ((K___uint64_t_int64_t){ .fst = a01, .snd = bx2 });
        uint64_t a02 = scrut.fst;
        int64_t bx3 = scrut.snd;
        uint64_t rb = a02 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
        uint64_t sb = (a02 & mask) ^ rb;
        ap[0U] = a02 & ~mask;
        ite1 = MPFR_Add1sp1_mk_state(sh, bx3, rb, sb);
      }
      else
      {
        MPFR_Add1sp1_state ite;
        if (d < MPFR_Lib_gmp_NUMB_BITS)
        {
          uint64_t *ap = a0.mpfr_d;
          uint64_t *bp = c0.mpfr_d;
          uint64_t *cp = b0.mpfr_d;
          int64_t bx2 = c0.mpfr_exp;
          uint64_t sb = cp[0U] << (uint32_t)(MPFR_Lib_gmp_NUMB_BITS - d);
          uint64_t a01 = bp[0U] + (cp[0U] >> (uint32_t)d);
          K___uint64_t_uint64_t_int64_t scrut;
          if (a01 < bp[0U])
            scrut =
              (
                (K___uint64_t_uint64_t_int64_t){
                  .fst = sb | (a01 & (uint64_t)1U),
                  .snd = (uint64_t)0x8000000000000000U | a01 >> (uint32_t)1U,
                  .thd = bx2 + (int64_t)1
                }
              );
          else
            scrut = ((K___uint64_t_uint64_t_int64_t){ .fst = sb, .snd = a01, .thd = bx2 });
          uint64_t sb1 = scrut.fst;
          uint64_t a02 = scrut.snd;
          int64_t bx3 = scrut.thd;
          uint64_t rb = a02 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
          uint64_t sb2 = sb1 | ((a02 & mask) ^ rb);
          ap[0U] = a02 & ~mask;
          ite = MPFR_Add1sp1_mk_state(sh, bx3, rb, sb2);
        }
        else
        {
          uint64_t *ap = a0.mpfr_d;
          uint64_t *bp = c0.mpfr_d;
          int64_t bx2 = c0.mpfr_exp;
          ap[0U] = bp[0U];
          uint64_t rb = (uint64_t)0U;
          uint64_t sb = (uint64_t)1U;
          ite = MPFR_Add1sp1_mk_state(sh, bx2, rb, sb);
        }
        ite1 = ite;
      }
      ite0 = ite1;
    }
    st = ite0;
  }
  if (st.bx > MPFR_Lib_mpfr_EMAX)
  {
    int32_t t = MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
    return t;
  }
  else
  {
    uint64_t *ap = a->mpfr_d;
    uint64_t a01 = ap[0U];
    MPFR_Lib_mpfr_struct uu___72_3478 = a[0U];
    a[0U] =
      (
        (MPFR_Lib_mpfr_struct){
          .mpfr_prec = uu___72_3478.mpfr_prec,
          .mpfr_sign = uu___72_3478.mpfr_sign,
          .mpfr_exp = st.bx,
          .mpfr_d = uu___72_3478.mpfr_d
        }
      );
    if (st.rb == (uint64_t)0U && st.sb == (uint64_t)0U)
      return MPFR_Lib_mpfr_RET((int32_t)0);
    else if (MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode))
      if
      (
        st.rb
        == (uint64_t)0U
        || (st.sb == (uint64_t)0U && (a01 & (uint64_t)1U << (uint32_t)st.sh) == (uint64_t)0U)
      )
      {
        int32_t ite;
        if (a->mpfr_sign == (int32_t)1)
          ite = (int32_t)-1;
        else
          ite = (int32_t)1;
        return MPFR_Lib_mpfr_RET(ite);
      }
      else
      {
        uint64_t *ap1 = a->mpfr_d;
        ap1[0U] = ap1[0U] + ((uint64_t)1U << (uint32_t)st.sh);
        if (ap1[0U] == (uint64_t)0U)
        {
          ap1[0U] = (uint64_t)0x8000000000000000U;
          if (st.bx + (int64_t)1 <= MPFR_Lib_mpfr_EMAX)
          {
            MPFR_Lib_mpfr_struct uu___72_3574 = a[0U];
            a[0U] =
              (
                (MPFR_Lib_mpfr_struct){
                  .mpfr_prec = uu___72_3574.mpfr_prec,
                  .mpfr_sign = uu___72_3574.mpfr_sign,
                  .mpfr_exp = st.bx + (int64_t)1,
                  .mpfr_d = uu___72_3574.mpfr_d
                }
              );
            return MPFR_Lib_mpfr_RET(a->mpfr_sign);
          }
          else
          {
            int32_t t = MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
            return MPFR_Lib_mpfr_RET(t);
          }
        }
        else
          return MPFR_Lib_mpfr_RET(a->mpfr_sign);
      }
    else if (MPFR_RoundingMode_mpfr_IS_LIKE_RNDZ(rnd_mode, a->mpfr_sign < (int32_t)0))
    {
      int32_t ite;
      if (a->mpfr_sign == (int32_t)1)
        ite = (int32_t)-1;
      else
        ite = (int32_t)1;
      return MPFR_Lib_mpfr_RET(ite);
    }
    else
    {
      uint64_t *ap1 = a->mpfr_d;
      ap1[0U] = ap1[0U] + ((uint64_t)1U << (uint32_t)st.sh);
      if (ap1[0U] == (uint64_t)0U)
      {
        ap1[0U] = (uint64_t)0x8000000000000000U;
        if (st.bx + (int64_t)1 <= MPFR_Lib_mpfr_EMAX)
        {
          MPFR_Lib_mpfr_struct uu___72_3781 = a[0U];
          a[0U] =
            (
              (MPFR_Lib_mpfr_struct){
                .mpfr_prec = uu___72_3781.mpfr_prec,
                .mpfr_sign = uu___72_3781.mpfr_sign,
                .mpfr_exp = st.bx + (int64_t)1,
                .mpfr_d = uu___72_3781.mpfr_d
              }
            );
          return MPFR_Lib_mpfr_RET(a->mpfr_sign);
        }
        else
        {
          int32_t t = MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
          return MPFR_Lib_mpfr_RET(t);
        }
      }
      else
        return MPFR_Lib_mpfr_RET(a->mpfr_sign);
    }
  }
}
