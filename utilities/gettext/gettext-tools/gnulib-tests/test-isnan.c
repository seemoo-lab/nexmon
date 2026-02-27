/* Test of isnand() substitute.
   Copyright (C) 2007-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff <blp@cs.stanford.edu>, from code by Bruno
   Haible <bruno@clisp.org>.  */

#include <config.h>

#include <math.h>

/* isnan must be a macro.  */
#ifndef isnan
# error missing declaration
#endif

#include <float.h>
#include <limits.h>

#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

static void
test_float (void)
{
  /* Finite values.  */
  ASSERT (!isnan (3.141f));
  ASSERT (!isnan (3.141e30f));
  ASSERT (!isnan (3.141e-30f));
  ASSERT (!isnan (-2.718f));
  ASSERT (!isnan (-2.718e30f));
  ASSERT (!isnan (-2.718e-30f));
  ASSERT (!isnan (0.0f));
  ASSERT (!isnan (minus_zerof));
  /* Infinite values.  */
  ASSERT (!isnan (Infinityf ()));
  ASSERT (!isnan (- Infinityf ()));
  /* Quiet NaN.  */
  ASSERT (isnan (NaNf ()));
#if defined FLT_EXPBIT0_WORD && defined FLT_EXPBIT0_BIT
  /* Signalling NaN.  */
  {
    #define NWORDSF \
      ((sizeof (float) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
    typedef union { float value; unsigned int word[NWORDSF]; } memory_float;
    memory_float m;
    m.value = NaNf ();
# if FLT_EXPBIT0_BIT > 0
    m.word[FLT_EXPBIT0_WORD] ^= (unsigned int) 1 << (FLT_EXPBIT0_BIT - 1);
# else
    m.word[FLT_EXPBIT0_WORD + (FLT_EXPBIT0_WORD < NWORDSF / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    if (FLT_EXPBIT0_WORD < NWORDSF / 2)
      m.word[FLT_EXPBIT0_WORD + 1] |= (unsigned int) 1 << FLT_EXPBIT0_BIT;
    else
      m.word[0] |= (unsigned int) 1;
    ASSERT (isnan (m.value));
  }
#endif
}

static void
test_double (void)
{
  /* Finite values.  */
  ASSERT (!isnan (3.141));
  ASSERT (!isnan (3.141e30));
  ASSERT (!isnan (3.141e-30));
  ASSERT (!isnan (-2.718));
  ASSERT (!isnan (-2.718e30));
  ASSERT (!isnan (-2.718e-30));
  ASSERT (!isnan (0.0));
  ASSERT (!isnan (minus_zerod));
  /* Infinite values.  */
  ASSERT (!isnan (Infinityd ()));
  ASSERT (!isnan (- Infinityd ()));
  /* Quiet NaN.  */
  ASSERT (isnan (NaNd ()));
#if defined DBL_EXPBIT0_WORD && defined DBL_EXPBIT0_BIT
  /* Signalling NaN.  */
  {
    #define NWORDSD \
      ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
    typedef union { double value; unsigned int word[NWORDSD]; } memory_double;
    memory_double m;
    m.value = NaNd ();
# if DBL_EXPBIT0_BIT > 0
    m.word[DBL_EXPBIT0_WORD] ^= (unsigned int) 1 << (DBL_EXPBIT0_BIT - 1);
# else
    m.word[DBL_EXPBIT0_WORD + (DBL_EXPBIT0_WORD < NWORDSD / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    m.word[DBL_EXPBIT0_WORD + (DBL_EXPBIT0_WORD < NWORDSD / 2 ? 1 : - 1)]
      |= (unsigned int) 1 << DBL_EXPBIT0_BIT;
    ASSERT (isnan (m.value));
  }
#endif
}

static void
test_long_double (void)
{
  #define NWORDSL \
    ((sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
  typedef union { unsigned int word[NWORDSL]; long double value; }
          memory_long_double;

  /* Finite values.  */
  ASSERT (!isnan (3.141L));
  ASSERT (!isnan (3.141e30L));
  ASSERT (!isnan (3.141e-30L));
  ASSERT (!isnan (-2.718L));
  ASSERT (!isnan (-2.718e30L));
  ASSERT (!isnan (-2.718e-30L));
  ASSERT (!isnan (0.0L));
  ASSERT (!isnan (minus_zerol));
  /* Infinite values.  */
  ASSERT (!isnan (Infinityl ()));
  ASSERT (!isnan (- Infinityl ()));
  /* Quiet NaN.  */
  ASSERT (isnan (NaNl ()));

#if defined LDBL_EXPBIT0_WORD && defined LDBL_EXPBIT0_BIT
  /* A bit pattern that is different from a Quiet NaN.  With a bit of luck,
     it's a Signalling NaN.  */
  {
#if defined __powerpc__ && LDBL_MANT_DIG == 106
    /* This is PowerPC "double double", a pair of two doubles.  Inf and Nan are
       represented as the corresponding 64-bit IEEE values in the first double;
       the second is ignored.  Manipulate only the first double.  */
    #undef NWORDSL
    #define NWORDSL \
      ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
#endif

    memory_long_double m;
    m.value = NaNl ();
# if LDBL_EXPBIT0_BIT > 0
    m.word[LDBL_EXPBIT0_WORD] ^= (unsigned int) 1 << (LDBL_EXPBIT0_BIT - 1);
# else
    m.word[LDBL_EXPBIT0_WORD + (LDBL_EXPBIT0_WORD < NWORDSL / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    m.word[LDBL_EXPBIT0_WORD + (LDBL_EXPBIT0_WORD < NWORDSL / 2 ? 1 : - 1)]
      |= (unsigned int) 1 << LDBL_EXPBIT0_BIT;
    ASSERT (isnan (m.value));
  }
#endif

#if ((defined __ia64 && LDBL_MANT_DIG == 64) || (defined __x86_64__ || defined __amd64__) || (defined __i386 || defined __i386__ || defined _I386 || defined _M_IX86 || defined _X86_)) && !HAVE_SAME_LONG_DOUBLE_AS_DOUBLE
/* Representation of an 80-bit 'long double' as an initializer for a sequence
   of 'unsigned int' words.  */
# ifdef WORDS_BIGENDIAN
#  define LDBL80_WORDS(exponent,manthi,mantlo) \
     { ((unsigned int) (exponent) << 16) | ((unsigned int) (manthi) >> 16), \
       ((unsigned int) (manthi) << 16) | ((unsigned int) (mantlo) >> 16),   \
       (unsigned int) (mantlo) << 16                                        \
     }
# else
#  define LDBL80_WORDS(exponent,manthi,mantlo) \
     { mantlo, manthi, exponent }
# endif
  { /* Quiet NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0xC3333333, 0x00000000) };
    ASSERT (isnan (x.value));
  }
  {
    /* Signalling NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x83333333, 0x00000000) };
    ASSERT (isnan (x.value));
  }
  /* isnan should return something for noncanonical values.  */
  { /* Pseudo-NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x40000001, 0x00000000) };
    ASSERT (isnan (x.value) || !isnan (x.value));
  }
  { /* Pseudo-Infinity.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x00000000, 0x00000000) };
    ASSERT (isnan (x.value) || !isnan (x.value));
  }
  { /* Pseudo-Zero.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4004, 0x00000000, 0x00000000) };
    ASSERT (isnan (x.value) || !isnan (x.value));
  }
  { /* Unnormalized number.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4000, 0x63333333, 0x00000000) };
    ASSERT (isnan (x.value) || !isnan (x.value));
  }
  { /* Pseudo-Denormal.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x0000, 0x83333333, 0x00000000) };
    ASSERT (isnan (x.value) || !isnan (x.value));
  }
#endif
}

int
main ()
{
  test_float ();
  test_double ();
  test_long_double ();
  return 0;
}
