/* mpfr_mul_1 -- internal function to perform a one-limb multiplication
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
#define bool int

typedef struct K___int64_t_uint64_t_uint64_t_s
{
  int64_t fst;
  uint64_t snd;
  uint64_t thd;
}
K___int64_t_uint64_t_uint64_t;

#define MPFR_Lib_mpfr_struct __mpfr_struct
#define MPFR_Lib_mpfr_RET(I) (I) != 0 ? ((__gmpfr_flags |= MPFR_FLAGS_INEXACT), (I)) : 0
#define MPFR_Exceptions_mpfr_overflow mpfr_overflow
#define MPFR_Exceptions_mpfr_underflow mpfr_underflow
#define mpfr_prec _mpfr_prec
#define mpfr_exp _mpfr_exp
#define mpfr_d _mpfr_d
#define mpfr_sign _mpfr_sign
#define MPFR_Lib_gmp_NUMB_BITS GMP_NUMB_BITS
#define MPFR_Lib_mpfr_EMAX __gmpfr_emax
#define MPFR_Lib_mpfr_EMIN __gmpfr_emin
#define MPFR_RoundingMode_MPFR_RNDZ MPFR_RNDZ

#define MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode) (rnd_mode == MPFR_RNDN)
#define MPFR_RoundingMode_mpfr_IS_LIKE_RNDZ MPFR_IS_LIKE_RNDZ
#define MPFR_RoundingMode_mpfr_IS_LIKE_RNDA MPFR_IS_LIKE_RNDA

/* Special code for prec(a) < GMP_NUMB_BITS and
   prec(b), prec(c) <= GMP_NUMB_BITS.
   Note: this code was copied in sqr.c, function mpfr_sqr_1 (this saves a few cycles
   with respect to have this function exported). As a consequence, any change here
   should be reported in mpfr_sqr_1. */
static int
mpfr_mul_1 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
            mpfr_prec_t p)
{
  uint64_t *ap = a->mpfr_d;
  uint64_t *bp = b->mpfr_d;
  uint64_t *cp = c->mpfr_d;
  uint64_t b0 = bp[0U];
  uint64_t c0 = cp[0U];
  int64_t sh = MPFR_Lib_gmp_NUMB_BITS - p;
  uint64_t mask = ((uint64_t)1U << (uint32_t)sh) - (uint64_t)1U;
  int64_t ax = b->mpfr_exp + c->mpfr_exp;
  //K___uint64_t_uint64_t scrut0 = MPFR_Umul_ppmm_umul_ppmm(b0, c0);
  //uint64_t a0 = scrut0.fst;
  //uint64_t sb = scrut0.snd;
  uint64_t a0, sb;
  umul_ppmm (a0, sb, b0, c0);
  K___int64_t_uint64_t_uint64_t scrut;
  if (a0 < (uint64_t)0x8000000000000000U)
    scrut =
      (
        (K___int64_t_uint64_t_uint64_t){
          .fst = ax - (int64_t)1,
          .snd = a0 << (uint32_t)1U | sb >> (uint32_t)(MPFR_Lib_gmp_NUMB_BITS - (int64_t)1),
          .thd = sb << (uint32_t)1U
        }
      );
  else
    scrut = ((K___int64_t_uint64_t_uint64_t){ .fst = ax, .snd = a0, .thd = sb });
  int64_t ax1 = scrut.fst;
  uint64_t a01 = scrut.snd;
  uint64_t sb1 = scrut.thd;
  uint64_t rb = a01 & (uint64_t)1U << (uint32_t)(sh - (int64_t)1);
  uint64_t sb2 = sb1 | ((a01 & mask) ^ rb);
  ap[0U] = a01 & ~mask;
  MPFR_Lib_mpfr_struct uu___73_2756 = a[0U];
  a[0U] =
    (
      (MPFR_Lib_mpfr_struct){
        .mpfr_prec = uu___73_2756.mpfr_prec,
        .mpfr_sign = b->mpfr_sign * c->mpfr_sign,
        .mpfr_exp = uu___73_2756.mpfr_exp,
        .mpfr_d = uu___73_2756.mpfr_d
      }
    );
  uint64_t *ap1 = a->mpfr_d;
  uint64_t a02 = ap1[0U];
  if (ax1 > MPFR_Lib_mpfr_EMAX)
    return MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
  else if (ax1 < MPFR_Lib_mpfr_EMIN)
  {
    bool aneg = a->mpfr_sign < (int32_t)0;
    if
    (
      ax1
      == MPFR_Lib_mpfr_EMIN - (int64_t)1
      && a02 == ~mask
      &&
        ((MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode) && rb > (uint64_t)0U)
        || ((rb | sb2) > (uint64_t)0U && MPFR_RoundingMode_mpfr_IS_LIKE_RNDA(rnd_mode, aneg)))
    )
    {
      uint64_t *ap2 = a->mpfr_d;
      uint64_t a03 = ap2[0U];
      MPFR_Lib_mpfr_struct uu___72_2907 = a[0U];
      a[0U] =
        (
          (MPFR_Lib_mpfr_struct){
            .mpfr_prec = uu___72_2907.mpfr_prec,
            .mpfr_sign = uu___72_2907.mpfr_sign,
            .mpfr_exp = ax1,
            .mpfr_d = uu___72_2907.mpfr_d
          }
        );
      if (rb == (uint64_t)0U && sb2 == (uint64_t)0U)
        return MPFR_Lib_mpfr_RET((int32_t)0);
      else if (MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode))
        if
        (
          rb
          == (uint64_t)0U
          || (sb2 == (uint64_t)0U && (a03 & (uint64_t)1U << (uint32_t)sh) == (uint64_t)0U)
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
          uint64_t *ap3 = a->mpfr_d;
          ap3[0U] = ap3[0U] + ((uint64_t)1U << (uint32_t)sh);
          if (ap3[0U] == (uint64_t)0U)
          {
            ap3[0U] = (uint64_t)0x8000000000000000U;
            if (ax1 + (int64_t)1 > MPFR_Lib_mpfr_EMAX)
              return MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
            else
            {
              MPFR_Lib_mpfr_struct uu___72_3047 = a[0U];
              a[0U] =
                (
                  (MPFR_Lib_mpfr_struct){
                    .mpfr_prec = uu___72_3047.mpfr_prec,
                    .mpfr_sign = uu___72_3047.mpfr_sign,
                    .mpfr_exp = ax1 + (int64_t)1,
                    .mpfr_d = uu___72_3047.mpfr_d
                  }
                );
              return MPFR_Lib_mpfr_RET(a->mpfr_sign);
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
        uint64_t *ap3 = a->mpfr_d;
        ap3[0U] = ap3[0U] + ((uint64_t)1U << (uint32_t)sh);
        if (ap3[0U] == (uint64_t)0U)
        {
          ap3[0U] = (uint64_t)0x8000000000000000U;
          if (ax1 + (int64_t)1 > MPFR_Lib_mpfr_EMAX)
            return MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
          else
          {
            MPFR_Lib_mpfr_struct uu___72_3237 = a[0U];
            a[0U] =
              (
                (MPFR_Lib_mpfr_struct){
                  .mpfr_prec = uu___72_3237.mpfr_prec,
                  .mpfr_sign = uu___72_3237.mpfr_sign,
                  .mpfr_exp = ax1 + (int64_t)1,
                  .mpfr_d = uu___72_3237.mpfr_d
                }
              );
            return MPFR_Lib_mpfr_RET(a->mpfr_sign);
          }
        }
        else
          return MPFR_Lib_mpfr_RET(a->mpfr_sign);
      }
    }
    else if
    (
      MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode)
      &&
        (ax1
        < MPFR_Lib_mpfr_EMIN - (int64_t)1
        || (a02 == (uint64_t)0x8000000000000000U && (rb | sb2) == (uint64_t)0U))
    )
      return MPFR_Exceptions_mpfr_underflow(a, MPFR_RoundingMode_MPFR_RNDZ, a->mpfr_sign);
    else
      return MPFR_Exceptions_mpfr_underflow(a, rnd_mode, a->mpfr_sign);
  }
  else
  {
    uint64_t *ap2 = a->mpfr_d;
    uint64_t a03 = ap2[0U];
    MPFR_Lib_mpfr_struct uu___72_3422 = a[0U];
    a[0U] =
      (
        (MPFR_Lib_mpfr_struct){
          .mpfr_prec = uu___72_3422.mpfr_prec,
          .mpfr_sign = uu___72_3422.mpfr_sign,
          .mpfr_exp = ax1,
          .mpfr_d = uu___72_3422.mpfr_d
        }
      );
    if (rb == (uint64_t)0U && sb2 == (uint64_t)0U)
      return MPFR_Lib_mpfr_RET((int32_t)0);
    else if (MPFR_RoundingMode_uu___is_MPFR_RNDN(rnd_mode))
      if
      (
        rb
        == (uint64_t)0U
        || (sb2 == (uint64_t)0U && (a03 & (uint64_t)1U << (uint32_t)sh) == (uint64_t)0U)
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
        uint64_t *ap3 = a->mpfr_d;
        ap3[0U] = ap3[0U] + ((uint64_t)1U << (uint32_t)sh);
        if (ap3[0U] == (uint64_t)0U)
        {
          ap3[0U] = (uint64_t)0x8000000000000000U;
          if (ax1 + (int64_t)1 > MPFR_Lib_mpfr_EMAX)
            return MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
          else
          {
            MPFR_Lib_mpfr_struct uu___72_3562 = a[0U];
            a[0U] =
              (
                (MPFR_Lib_mpfr_struct){
                  .mpfr_prec = uu___72_3562.mpfr_prec,
                  .mpfr_sign = uu___72_3562.mpfr_sign,
                  .mpfr_exp = ax1 + (int64_t)1,
                  .mpfr_d = uu___72_3562.mpfr_d
                }
              );
            return MPFR_Lib_mpfr_RET(a->mpfr_sign);
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
      uint64_t *ap3 = a->mpfr_d;
      ap3[0U] = ap3[0U] + ((uint64_t)1U << (uint32_t)sh);
      if (ap3[0U] == (uint64_t)0U)
      {
        ap3[0U] = (uint64_t)0x8000000000000000U;
        if (ax1 + (int64_t)1 > MPFR_Lib_mpfr_EMAX)
          return MPFR_Exceptions_mpfr_overflow(a, rnd_mode, a->mpfr_sign);
        else
        {
          MPFR_Lib_mpfr_struct uu___72_3752 = a[0U];
          a[0U] =
            (
              (MPFR_Lib_mpfr_struct){
                .mpfr_prec = uu___72_3752.mpfr_prec,
                .mpfr_sign = uu___72_3752.mpfr_sign,
                .mpfr_exp = ax1 + (int64_t)1,
                .mpfr_d = uu___72_3752.mpfr_d
              }
            );
          return MPFR_Lib_mpfr_RET(a->mpfr_sign);
        }
      }
      else
        return MPFR_Lib_mpfr_RET(a->mpfr_sign);
    }
  }
}

