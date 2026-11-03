/* Uniform Interface to GMP.

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

#ifndef __GMPFR_GMP_H__
#define __GMPFR_GMP_H__

#ifndef __MPFR_IMPL_H__
# error  "mpfr-impl.h not included"
#endif


/******************************************************
 ******************** C++ Compatibility ***************
 ******************************************************/
#if defined (__cplusplus)
extern "C" {
#endif


/******************************************************
 ******************** Identify GMP ********************
 ******************************************************/

/* Macro to detect the GMP version */
#if defined(__GNU_MP_VERSION) && \
    defined(__GNU_MP_VERSION_MINOR) && \
    defined(__GNU_MP_VERSION_PATCHLEVEL)
# define __MPFR_GMP(a,b,c) \
  (MPFR_VERSION_NUM(__GNU_MP_VERSION,__GNU_MP_VERSION_MINOR,__GNU_MP_VERSION_PATCHLEVEL) >= MPFR_VERSION_NUM(a,b,c))
#else
# define __MPFR_GMP(a,b,c) 0
#endif



/******************************************************
 ******************** Check GMP ***********************
 ******************************************************/

#if !__MPFR_GMP(5,0,0) && !defined(MPFR_USE_MINI_GMP)
# error "GMP 5.0.0 or newer is required"
#endif

#if GMP_NAIL_BITS != 0
# error "MPFR doesn't support non-zero values of GMP_NAIL_BITS"
#endif

#if (GMP_NUMB_BITS<8) || (GMP_NUMB_BITS & (GMP_NUMB_BITS - 1))
# error "GMP_NUMB_BITS must be a power of 2, and >= 8"
#endif

#if GMP_NUMB_BITS == 8
# define MPFR_LOG2_GMP_NUMB_BITS 3
#elif GMP_NUMB_BITS == 16
# define MPFR_LOG2_GMP_NUMB_BITS 4
#elif GMP_NUMB_BITS == 32
# define MPFR_LOG2_GMP_NUMB_BITS 5
#elif GMP_NUMB_BITS == 64
# define MPFR_LOG2_GMP_NUMB_BITS 6
#elif GMP_NUMB_BITS == 128
# define MPFR_LOG2_GMP_NUMB_BITS 7
#elif GMP_NUMB_BITS == 256
# define MPFR_LOG2_GMP_NUMB_BITS 8
#else
# error "Can't compute log2(GMP_NUMB_BITS)"
#endif



/******************************************************
 ************* Define GMP Internal Interface  *********
 ******************************************************/

#ifdef MPFR_HAVE_GMP_IMPL  /* with gmp build */

#define mpfr_allocate_func   (*__gmp_allocate_func)
#define mpfr_reallocate_func (*__gmp_reallocate_func)
#define mpfr_free_func       (*__gmp_free_func)

#else  /* without gmp build (gmp-impl.h replacement) */

/* Define some macros */

#define ULONG_HIGHBIT (ULONG_MAX ^ ((unsigned long) ULONG_MAX >> 1))
#define UINT_HIGHBIT  (UINT_MAX ^ ((unsigned) UINT_MAX >> 1))
#define USHRT_HIGHBIT ((unsigned short) (USHRT_MAX ^ ((unsigned short) USHRT_MAX >> 1)))


#if __GMP_MP_SIZE_T_INT
#define MP_SIZE_T_MAX      INT_MAX
#define MP_SIZE_T_MIN      INT_MIN
#else
#define MP_SIZE_T_MAX      LONG_MAX
#define MP_SIZE_T_MIN      LONG_MIN
#endif

/* MP_LIMB macros.
   Note: GMP now also has the MPN_FILL macro, and GMP's MPN_ZERO(dst,n) is
   defined as "if (n) MPN_FILL(dst, n, 0);". */
#define MPN_ZERO(dst, n) memset((dst), 0, (n)*MPFR_BYTES_PER_MP_LIMB)
#define MPN_COPY(dst,src,n) \
  do                                                                  \
    {                                                                 \
      if ((dst) != (src))                                             \
        {                                                             \
          MPFR_ASSERTD ((char *) (dst) >= (char *) (src) +            \
                                     (n) * MPFR_BYTES_PER_MP_LIMB ||  \
                        (char *) (src) >= (char *) (dst) +            \
                                     (n) * MPFR_BYTES_PER_MP_LIMB);   \
          memcpy ((dst), (src), (n) * MPFR_BYTES_PER_MP_LIMB);        \
        }                                                             \
    }                                                                 \
  while (0)

/* MPN macros taken from gmp-impl.h */
#define MPN_NORMALIZE(DST, NLIMBS) \
  do {                                        \
    while (NLIMBS > 0)                        \
      {                                       \
        if ((DST)[(NLIMBS) - 1] != 0)         \
          break;                              \
        NLIMBS--;                             \
      }                                       \
  } while (0)
#define MPN_NORMALIZE_NOT_ZERO(DST, NLIMBS)     \
  do {                                          \
    MPFR_ASSERTD ((NLIMBS) >= 1);               \
    while (1)                                   \
      {                                         \
        if ((DST)[(NLIMBS) - 1] != 0)           \
          break;                                \
        NLIMBS--;                               \
      }                                         \
  } while (0)
#define MPN_OVERLAP_P(xp, xsize, yp, ysize) \
  ((xp) + (xsize) > (yp) && (yp) + (ysize) > (xp))
#define MPN_SAME_OR_INCR2_P(dst, dsize, src, ssize)             \
  ((dst) <= (src) || ! MPN_OVERLAP_P (dst, dsize, src, ssize))
#define MPN_SAME_OR_INCR_P(dst, src, size)      \
  MPN_SAME_OR_INCR2_P(dst, size, src, size)
#define MPN_SAME_OR_DECR2_P(dst, dsize, src, ssize)             \
  ((dst) >= (src) || ! MPN_OVERLAP_P (dst, dsize, src, ssize))
#define MPN_SAME_OR_DECR_P(dst, src, size)      \
  MPN_SAME_OR_DECR2_P(dst, size, src, size)

#ifndef MUL_FFT_THRESHOLD
#define MUL_FFT_THRESHOLD 8448
#endif

/* If mul_basecase or mpn_sqr_basecase are not exported, used mpn_mul instead */
#ifndef mpn_mul_basecase
# define mpn_mul_basecase(dst,s1,n1,s2,n2) mpn_mul((dst),(s1),(n1),(s2),(n2))
#endif
#ifndef mpn_sqr_basecase
# define mpn_sqr_basecase(dst,src,n) mpn_mul((dst),(src),(n),(src),(n))
#endif

/* ASSERT */
__MPFR_DECLSPEC void mpfr_assert_fail (const char *, int,
                                       const char *);

#define ASSERT_FAIL(expr)  mpfr_assert_fail (__FILE__, __LINE__, #expr)
/* ASSERT() is for mpfr-longlong.h only. */
#define ASSERT(expr)       MPFR_ASSERTD(expr)

/* Access fields of GMP struct */
#define SIZ(x) ((x)->_mp_size)
#define ABSIZ(x) ABS (SIZ (x))
#define PTR(x) ((x)->_mp_d)
#define ALLOC(x) ((x)->_mp_alloc)
/* For mpf numbers only. */
#ifdef MPFR_NEED_MPF_INTERNALS
/* Note: the EXP macro name is reserved when <errno.h> is included.
   For compatibility with gmp-impl.h (cf --with-gmp-build), we cannot
   change this macro, but let's define it only when we need it, where
   <errno.h> will not be included. */
#define EXP(x) ((x)->_mp_exp)
#define PREC(x) ((x)->_mp_prec)
#endif

/* For longlong.h */
#ifdef HAVE_ATTRIBUTE_MODE
typedef unsigned int UQItype    __attribute__ ((mode (QI)));
typedef          int SItype     __attribute__ ((mode (SI)));
typedef unsigned int USItype    __attribute__ ((mode (SI)));
typedef          int DItype     __attribute__ ((mode (DI)));
typedef unsigned int UDItype    __attribute__ ((mode (DI)));
#else
typedef unsigned char UQItype;
typedef          long SItype;
typedef unsigned long USItype;
#ifdef HAVE_LONG_LONG
typedef long long int DItype;
typedef unsigned long long int UDItype;
#else /* Assume `long' gives us a wide enough type.  Needed for hppa2.0w.  */
typedef long int DItype;
typedef unsigned long int UDItype;
#endif
#endif
typedef mp_limb_t UWtype;
typedef unsigned int UHWtype;
#define W_TYPE_SIZE GMP_NUMB_BITS

/* Remap names of internal mpn functions (for longlong.h).  */
#undef  __clz_tab
#define __clz_tab               mpfr_clz_tab

/* Use (4.0 * ...) instead of (2.0 * ...) to work around buggy compilers
   that don't convert ulong->double correctly (eg. SunOS 4 native cc).  */
#undef MP_BASE_AS_DOUBLE
#define MP_BASE_AS_DOUBLE (4.0 * (MPFR_LIMB_ONE << (GMP_NUMB_BITS - 2)))

/* Structure for conversion between internal binary format and
   strings in base 2..36.  */
struct bases
{
  /* log(2)/log(conversion_base) */
  double chars_per_bit_exactly;
};
#undef  __mp_bases
#define __mp_bases mpfr_bases
__MPFR_DECLSPEC extern const struct bases mpfr_bases[257];

/* Standard macros */
#undef ABS
#undef MIN
#undef MAX
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MIN(l,o) ((l) < (o) ? (l) : (o))
#define MAX(h,i) ((h) > (i) ? (h) : (i))

__MPFR_DECLSPEC void * mpfr_allocate_func (size_t);
__MPFR_DECLSPEC void * mpfr_reallocate_func (void *, size_t, size_t);
__MPFR_DECLSPEC void   mpfr_free_func (void *, size_t);

#if defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_SBPI1_DIVAPPR_Q)
#ifndef __gmpn_sbpi1_divappr_q
__MPFR_DECLSPEC mp_limb_t __gmpn_sbpi1_divappr_q (mp_limb_t*,
                mp_limb_t*, mp_size_t, mp_limb_t*, mp_size_t, mp_limb_t);
#endif
#endif

#if defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_INVERT_LIMB)
#ifndef __gmpn_invert_limb
__MPFR_DECLSPEC mp_limb_t __gmpn_invert_limb (mp_limb_t);
#endif
#endif

#if defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_RSBLSH1_N)
#ifndef __gmpn_rsblsh1_n
__MPFR_DECLSPEC mp_limb_t __gmpn_rsblsh1_n (mp_limb_t*, mp_limb_t*, mp_limb_t*, mp_size_t);
#endif
#endif

/* Definitions related to temporary memory allocation */

struct tmp_marker
{
  void *ptr;
  size_t size;
  struct tmp_marker *next;
};

__MPFR_DECLSPEC void *mpfr_tmp_allocate (struct tmp_marker **,
                                         size_t);
__MPFR_DECLSPEC void mpfr_tmp_free (struct tmp_marker *);

/* Default MPFR_ALLOCA_MAX value. It can be overridden at configure time;
   with some tools, by giving a low value such as 0, this is useful for
   checking buffer overflow, which may not be possible with alloca.
   If HAVE_ALLOCA is not defined, then alloca() is not available, so that
   MPFR_ALLOCA_MAX needs to be 0 (see the definition of TMP_ALLOC below);
   if the user has explicitly given a non-zero value, this will probably
   yield an error at link time or at run time. */
#ifndef MPFR_ALLOCA_MAX
# ifdef HAVE_ALLOCA
#  define MPFR_ALLOCA_MAX 16384
# else
#  define MPFR_ALLOCA_MAX 0
# endif
#endif

/* Do not define TMP_SALLOC (see the test in mpfr-impl.h)! */

#if MPFR_ALLOCA_MAX != 0

/* The following tries to get a good version of alloca.
   See gmp-impl.h for implementation details and original version */
/* FIXME: the autoconf manual gives a different piece of code under the
   documentation of the AC_FUNC_ALLOCA macro. Should we switch to it?
   But note that the HAVE_ALLOCA test in it seems wrong.
   https://lists.gnu.org/archive/html/bug-autoconf/2019-01/msg00009.html */
#ifndef alloca
# if defined ( __GNUC__ )
#  define alloca __builtin_alloca
# elif defined (__DECC)
#  define alloca(x) __ALLOCA(x)
# elif defined (_MSC_VER)
#  include <malloc.h>
#  define alloca _alloca
# elif defined (HAVE_ALLOCA_H)
#  include <alloca.h>
# elif defined (_AIX) || defined (_IBMR2)
#  pragma alloca
# else
void *alloca (size_t);
# endif
#endif

#define TMP_ALLOC(n) (MPFR_ASSERTD ((n) > 0),                      \
                      MPFR_LIKELY ((n) <= MPFR_ALLOCA_MAX) ?       \
                      alloca (n) : mpfr_tmp_allocate (&tmp_marker, (n)))

#else  /* MPFR_ALLOCA_MAX == 0, alloca() not needed */

#define TMP_ALLOC(n) (mpfr_tmp_allocate (&tmp_marker, (n)))

#endif

#define TMP_DECL(m) struct tmp_marker *tmp_marker

#define TMP_MARK(m) (tmp_marker = 0)

/* Note about TMP_FREE: For small precisions, tmp_marker is null as
   the allocation is done on the stack (see TMP_ALLOC above). */
#define TMP_FREE(m) \
  (MPFR_LIKELY (tmp_marker == NULL) ? (void) 0 : mpfr_tmp_free (tmp_marker))

#endif  /* gmp-impl.h replacement */



/******************************************************
 ****** GMP Interface which changes with versions *****
 ****** to other versions of GMP. Add missing     *****
 ****** interfaces.                               *****
 ******************************************************/

#ifndef MPFR_LONG_WITHIN_LIMB

/* the following routines assume that an unsigned long has at least twice the
   size of an mp_limb_t */

#define umul_ppmm(ph, pl, m0, m1)                                       \
  do {                                                                  \
    unsigned long _p = (unsigned long) (m0) * (unsigned long) (m1);     \
    (ph) = (mp_limb_t) (_p >> GMP_NUMB_BITS);                           \
    (pl) = (mp_limb_t) (_p & MPFR_LIMB_MAX);                            \
  } while (0)

#define add_ssaaaa(sh, sl, ah, al, bh, bl)                              \
  do {                                                                  \
    unsigned long _a = ((unsigned long) (ah) << GMP_NUMB_BITS) + (al);  \
    unsigned long _b = ((unsigned long) (bh) << GMP_NUMB_BITS) + (bl);  \
    unsigned long _s = _a + _b;                                         \
    (sh) = (mp_limb_t) (_s >> GMP_NUMB_BITS);                           \
    (sl) = (mp_limb_t) (_s & MPFR_LIMB_MAX);                            \
  } while (0)

#define sub_ddmmss(sh, sl, ah, al, bh, bl)                              \
  do {                                                                  \
    unsigned long _a = ((unsigned long) (ah) << GMP_NUMB_BITS) + (al);  \
    unsigned long _b = ((unsigned long) (bh) << GMP_NUMB_BITS) + (bl);  \
    unsigned long _s = _a - _b;                                         \
    (sh) = (mp_limb_t) (_s >> GMP_NUMB_BITS);                           \
    (sl) = (mp_limb_t) (_s & MPFR_LIMB_MAX);                            \
  } while (0)

#define count_leading_zeros(count,x)                                    \
  do {                                                                  \
    int _c = 0;                                                         \
    mp_limb_t _x = (mp_limb_t) (x);                                     \
    while (GMP_NUMB_BITS > 16 && (_x >> (GMP_NUMB_BITS - 16)) == 0)     \
      {                                                                 \
        _c += 16;                                                       \
        _x = (mp_limb_t) (_x << 16);                                    \
      }                                                                 \
    if (GMP_NUMB_BITS > 8 && (_x >> (GMP_NUMB_BITS - 8)) == 0)          \
      {                                                                 \
        _c += 8;                                                        \
        _x = (mp_limb_t) (_x << 8);                                     \
      }                                                                 \
    if (GMP_NUMB_BITS > 4 && (_x >> (GMP_NUMB_BITS - 4)) == 0)          \
      {                                                                 \
        _c += 4;                                                        \
        _x = (mp_limb_t) (_x << 4);                                     \
      }                                                                 \
    if (GMP_NUMB_BITS > 2 && (_x >> (GMP_NUMB_BITS - 2)) == 0)          \
      {                                                                 \
        _c += 2;                                                        \
        _x = (mp_limb_t) (_x << 2);                                     \
      }                                                                 \
    if ((_x & MPFR_LIMB_HIGHBIT) == 0)                                  \
      _c ++;                                                            \
    (count) = _c;                                                       \
  } while (0)

#define invert_limb(invxl,xl)                                           \
  do {                                                                  \
    unsigned long _num;                                                 \
    MPFR_ASSERTD ((xl) != 0);                                           \
    _num = (unsigned long) (mp_limb_t) ~(xl);                           \
    _num = (_num << GMP_NUMB_BITS) | MPFR_LIMB_MAX;                     \
    (invxl) = _num / (xl);                                              \
  } while (0)

#define udiv_qrnnd(q, r, n1, n0, d)                                     \
  do {                                                                  \
    unsigned long _num;                                                 \
    _num = ((unsigned long) (n1) << GMP_NUMB_BITS) | (n0);              \
    (q) = _num / (d);                                                   \
    (r) = _num % (d);                                                   \
  } while (0)

#endif

/* If mpn_sqr is not defined, use mpn_mul_n instead
   (mpn_sqr was called mpn_sqr_n (internal) in older versions of GMP). */
#ifndef mpn_sqr
# define mpn_sqr(dst,src,n) mpn_mul_n((dst),(src),(src),(n))
#endif

/* invert_limb macro, copied from GMP 5.0.2, file gmp-impl.h.
   It returns invxl = floor((B^2-1)/xl)-B, where B=2^BITS_PER_LIMB,
   assuming the most significant bit of xl is set. */
#ifndef invert_limb
#if defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_INVERT_LIMB)
#define invert_limb(invxl,xl)                             \
  do {                                                    \
    invxl = __gmpn_invert_limb (xl);                      \
  } while (0)
#else
#define invert_limb(invxl,xl)                             \
  do {                                                    \
    mp_limb_t dummy MPFR_MAYBE_UNUSED;                    \
    MPFR_ASSERTD ((xl) != 0);                             \
    udiv_qrnnd (invxl, dummy, ~(xl), MPFR_LIMB_MAX, xl);  \
  } while (0)
#endif /* defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_INVERT_LIMB) */
#endif /* ifndef invert_limb */

/* udiv_qr_3by2 macro, adapted from GMP 5.0.2, file gmp-impl.h.
   Compute quotient the quotient and remainder for n / d. Requires d
   >= B^2 / 2 and n < d B. dinv is the inverse

     floor ((B^3 - 1) / (d0 + d1 B)) - B.

   NOTE: Output variables are updated multiple times. Only some inputs
   and outputs may overlap.
*/
#ifndef udiv_qr_3by2
# ifdef MPFR_USE_MINI_GMP
/* Avoid integer overflow on int in case of integer promotion
   (when mp_limb_t is shorter than int). Note that unsigned long
   may be longer than necessary, but GCC seems to optimize. */
#  define OP_CAST (unsigned long)
# else
#  define OP_CAST
# endif
# define udiv_qr_3by2(q, r1, r0, n2, n1, n0, d1, d0, dinv)              \
  do {                                                                  \
    mp_limb_t _q0, _t1, _t0, _mask;                                     \
    umul_ppmm ((q), _q0, (n2), (dinv));                                 \
    add_ssaaaa ((q), _q0, (q), _q0, (n2), (n1));                        \
                                                                        \
    /* Compute the two most significant limbs of n - q'd */             \
    (r1) = (n1) - OP_CAST (d1) * (q);                                   \
    (r0) = (n0);                                                        \
    sub_ddmmss ((r1), (r0), (r1), (r0), (d1), (d0));                    \
    umul_ppmm (_t1, _t0, (d0), (q));                                    \
    sub_ddmmss ((r1), (r0), (r1), (r0), _t1, _t0);                      \
    (q)++;                                                              \
                                                                        \
    /* Conditionally adjust q and the remainders */                     \
    _mask = - (mp_limb_t) ((r1) >= _q0);                                \
    (q) += _mask;                                                       \
    add_ssaaaa ((r1), (r0), (r1), (r0), _mask & (d1), _mask & (d0));    \
    if (MPFR_UNLIKELY ((r1) >= (d1)))                                   \
      {                                                                 \
        if ((r1) > (d1) || (r0) >= (d0))                                \
          {                                                             \
            (q)++;                                                      \
            sub_ddmmss ((r1), (r0), (r1), (r0), (d1), (d0));            \
          }                                                             \
      }                                                                 \
  } while (0)
#endif

/* invert_pi1 macro adapted from GMP 5, this computes in (dinv).inv32
   the value of floor((beta^3 - 1)/(d1*beta+d0)) - beta,
   cf "Improved Division by Invariant Integers" by Niels Möller and
   Torbjörn Granlund */
typedef struct {mp_limb_t inv32;} mpfr_pi1_t;
#ifndef invert_pi1
#define invert_pi1(dinv, d1, d0)                                        \
  do {                                                                  \
    mp_limb_t _v, _p, _t1, _t0, _mask;                                  \
    invert_limb (_v, d1);                                               \
    _p = (d1) * _v;                                                     \
    _p += (d0);                                                         \
    if (_p < (d0))                                                      \
      {                                                                 \
        _v--;                                                           \
        _mask = -(mp_limb_t) (_p >= (d1));                              \
        _p -= (d1);                                                     \
        _v += _mask;                                                    \
        _p -= _mask & (d1);                                             \
      }                                                                 \
    umul_ppmm (_t1, _t0, d0, _v);                                       \
    _p += _t1;                                                          \
    if (_p < _t1)                                                       \
      {                                                                 \
        _v--;                                                           \
        if (MPFR_UNLIKELY (_p >= (d1)))                                 \
          {                                                             \
            if (_p > (d1) || _t0 >= (d0))                               \
              _v--;                                                     \
          }                                                             \
      }                                                                 \
    (dinv).inv32 = _v;                                                  \
  } while (0)
#endif

/* The following macro is copied from GMP-6.1.1, file gmp-impl.h,
   macro udiv_qrnnd_preinv.
   It computes q and r such that nh*2^GMP_NUMB_BITS + nl = q*d + r,
   with 0 <= r < d, assuming di = __gmpn_invert_limb (d). */
#define __udiv_qrnnd_preinv(q, r, nh, nl, d, di)                        \
  do {                                                                  \
    mp_limb_t _qh, _ql, _r, _mask;                                      \
    umul_ppmm (_qh, _ql, (nh), (di));                                   \
    if (__builtin_constant_p (nl) && (nl) == 0)                         \
      {                                                                 \
        _qh += (nh) + 1;                                                \
        _r = - _qh * (d);                                               \
        _mask = -(mp_limb_t) (_r > _ql); /* both > and >= are OK */     \
        _qh += _mask;                                                   \
        _r += _mask & (d);                                              \
      }                                                                 \
    else                                                                \
      {                                                                 \
        add_ssaaaa (_qh, _ql, _qh, _ql, (nh) + 1, (nl));                \
        _r = (nl) - _qh * (d);                                          \
        _mask = -(mp_limb_t) (_r > _ql); /* both > and >= are OK */     \
        _qh += _mask;                                                   \
        _r += _mask & (d);                                              \
        if (MPFR_UNLIKELY (_r >= (d)))                                  \
          {                                                             \
            _r -= (d);                                                  \
            _qh++;                                                      \
          }                                                             \
      }                                                                 \
    (r) = _r;                                                           \
    (q) = _qh;                                                          \
  } while (0)

#if GMP_NUMB_BITS == 64
/* specialized version for nl = 0, with di computed inside */
#define __udiv_qrnd_preinv(q, r, nh, d)                                 \
  do {                                                                  \
    mp_limb_t _di;                                                      \
                                                                        \
    MPFR_ASSERTD ((d) != 0);                                            \
    MPFR_ASSERTD ((nh) < (d));                                          \
    MPFR_ASSERTD ((d) & MPFR_LIMB_HIGHBIT);                             \
                                                                        \
    __gmpfr_invert_limb (_di, d);                                       \
    __udiv_qrnnd_preinv (q, r, nh, 0, d, _di);                          \
  } while (0)
#elif defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_INVERT_LIMB)
/* specialized version for nl = 0, with di computed inside */
#define __udiv_qrnd_preinv(q, r, nh, d)                                 \
  do {                                                                  \
    mp_limb_t _di;                                                      \
                                                                        \
    MPFR_ASSERTD ((d) != 0);                                            \
    MPFR_ASSERTD ((nh) < (d));                                          \
    MPFR_ASSERTD ((d) & MPFR_LIMB_HIGHBIT);                             \
                                                                        \
    _di = __gmpn_invert_limb (d);                                       \
    __udiv_qrnnd_preinv (q, r, nh, 0, d, _di);                          \
  } while (0)
#else
/* Same as __udiv_qrnnd_c from longlong.h, but using a single UWType/UWtype
   division instead of two, and with n0=0. The analysis below assumes a limb
   has 64 bits for simplicity. */
#define __udiv_qrnd_preinv(q, r, n1, d)                                 \
  do {                                                                  \
    UWtype __d1, __d0, __q1, __q0, __r1, __r0, __i;                     \
                                                                        \
    MPFR_ASSERTD ((d) != 0);                                            \
    MPFR_ASSERTD ((n1) < (d));                                          \
    MPFR_ASSERTD ((d) & MPFR_LIMB_HIGHBIT);                             \
                                                                        \
    __d1 = __ll_highpart (d);                                           \
    /* 2^31 <= d1 < 2^32 */                                             \
    __d0 = __ll_lowpart (d);                                            \
    /* 0 <= d0 < 2^32 */                                                \
    __i = ~(UWtype) 0 / __d1;                                           \
    /* 2^32 < i < 2^33 with i < 2^64/d1 */                              \
                                                                        \
    __q1 = (((n1) / __ll_B) * __i) / __ll_B;                            \
    /* Since n1 < d, high(n1) <= d1 = high(d), thus */                  \
    /* q1 <= high(n1) * (2^64/d1) / 2^32 <= 2^32 */                     \
    /* and also q1 <= n1/d1 thus r1 >= 0 below */                       \
    __r1 = (n1) - __q1 * __d1;                                          \
    while (__r1 >= __d1)                                                \
      __q1 ++, __r1 -= __d1;                                            \
    /* by construction, we have n1 = q1*d1+r1, and 0 <= r1 < d1 */      \
    /* thus q1 <= n1/d1 < 2^32+2 */                                     \
    /* q1*d0 <= (2^32+1)*(2^32-1) <= 2^64-1 thus it fits in a limb. */  \
    __r0 = __r1 * __ll_B;                                               \
    __r1 = __r0 - __q1 * __d0;                                          \
    /* At most two corrections are needed like in __udiv_qrnnd_c. */    \
    if (__r1 > __r0) /* borrow when subtracting q1*d0 */                \
      {                                                                 \
        __q1--, __r1 += (d);                                            \
        if (__r1 > (d)) /* no carry when adding d */                    \
          __q1--, __r1 += (d);                                          \
      }                                                                 \
    /* We can have r1 < m here, but in this case the true remainder */  \
    /* is 2^64+r1, which is > m (same analysis as below for r0). */     \
    /* Now we have n1*2^32 = q1*d + r1, with 0 <= r1 < d. */            \
    MPFR_ASSERTD(__r1 < (d));                                           \
                                                                        \
    /* The same analysis as above applies, with n1 replaced by r1, */   \
    /* q1 by q0, r1 by r0. */                                           \
    __q0 = ((__r1 / __ll_B) * __i) / __ll_B;                            \
    __r0 = __r1  - __q0 * __d1;                                         \
    while (__r0 >= __d1)                                                \
      __q0 ++, __r0 -= __d1;                                            \
    __r1 = __r0 * __ll_B;                                               \
    __r0 = __r1 - __q0 * __d0;                                          \
    /* We know the quotient floor(r1*2^64/d) is q0, q0-1 or q0-2.*/     \
    if (__r0 > __r1) /* borrow when subtracting q0*d0 */                \
      {                                                                 \
        /* The quotient is either q0-1 or q0-2. */                      \
        __q0--, __r0 += (d);                                            \
        if (__r0 > (d)) /* no carry when adding d */                    \
          __q0--, __r0 += (d);                                          \
      }                                                                 \
    /* Now we have n1*2^64 = (q1*2^32+q0)*d + r0, with 0 <= r0 < d. */  \
    MPFR_ASSERTD(__r0 < (d));                                           \
                                                                        \
    (q) = __q1 * __ll_B | __q0;                                         \
    (r) = __r0;                                                         \
  } while (0)
#endif

/******************************************************
 ************* GMP Basic Pointer Types ****************
 ******************************************************/

typedef mp_limb_t *mpfr_limb_ptr;
typedef const mp_limb_t *mpfr_limb_srcptr;

/* mpfr_ieee_double_extract structure (copied from GMP 6.1.0, gmp-impl.h, with
   ieee_double_extract changed into mpfr_ieee_double_extract, and
   _GMP_IEEE_FLOATS changed into _MPFR_IEEE_FLOATS). */

/* Define mpfr_ieee_double_extract and _MPFR_IEEE_FLOATS.

   Bit field packing is "implementation defined" according to C99, which
   leaves us at the compiler's mercy here.  For some systems packing is
   defined in the ABI (eg. x86).  In any case so far it seems universal that
   little endian systems pack from low to high, and big endian from high to
   low within the given type.

   Within the fields we rely on the integer endianness being the same as the
   float endianness, this is true everywhere we know of and it'd be a fairly
   strange system that did anything else.  */

/* Note for MPFR: building with "gcc -std=c89 -pedantic -pedantic-errors"
   fails if the bit-field type is unsigned long:

     error: type of bit-field '...' is a GCC extension [-Wpedantic]

   Though with -std=c99 no errors are obtained, this is still an extension
   in C99, which says:

     A bit-field shall have a type that is a qualified or unqualified version
     of _Bool, signed int, unsigned int, or some other implementation-defined
     type.

   So, unsigned int should be better. This will fail with implementations
   having 16-bit int's, but such implementations are not required to
   support bit-fields of size > 16 anyway; if ever an implementation with
   16-bit int's is found, the appropriate minimal changes could still be
   done in the future. See WG14/N2921 (5.16).
*/

#ifndef _MPFR_IEEE_FLOATS

#ifdef HAVE_DOUBLE_IEEE_LITTLE_ENDIAN
#define _MPFR_IEEE_FLOATS 1
union mpfr_ieee_double_extract
{
  struct
    {
      unsigned int manl:32;
      unsigned int manh:20;
      unsigned int exp:11;
      unsigned int sig:1;
    } s;
  double d;
};
#endif

#ifdef HAVE_DOUBLE_IEEE_BIG_ENDIAN
#define _MPFR_IEEE_FLOATS 1
union mpfr_ieee_double_extract
{
  struct
    {
      unsigned int sig:1;
      unsigned int exp:11;
      unsigned int manh:20;
      unsigned int manl:32;
    } s;
  double d;
};
#endif

#endif /* _MPFR_IEEE_FLOATS */

/******************************************************
 ******************** C++ Compatibility ***************
 ******************************************************/
#if defined (__cplusplus)
}
#endif

#endif /* Gmp internal emulator */
