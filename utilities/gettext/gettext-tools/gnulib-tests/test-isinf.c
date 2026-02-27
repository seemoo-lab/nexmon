/* Test of isinf() substitute.
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

/* Written by Ben Pfaff, 2008, using Bruno Haible's code as a
   template. */

#include <config.h>

#include <math.h>

/* isinf must be a macro.  */
#ifndef isinf
# error missing declaration
#endif

#include <float.h>
#include <limits.h>

#include "infinity.h"
#include "macros.h"

float zerof = 0.0f;
double zerod = 0.0;
long double zerol = 0.0L;

static void
test_isinff ()
{
  /* Zero. */
  ASSERT (!isinf (0.0f));
  /* Subnormal values. */
  ASSERT (!isinf (FLT_MIN / 2));
  ASSERT (!isinf (-FLT_MIN / 2));
  /* Finite values.  */
  ASSERT (!isinf (3.141f));
  ASSERT (!isinf (3.141e30f));
  ASSERT (!isinf (3.141e-30f));
  ASSERT (!isinf (-2.718f));
  ASSERT (!isinf (-2.718e30f));
  ASSERT (!isinf (-2.718e-30f));
  ASSERT (!isinf (FLT_MAX));
  ASSERT (!isinf (-FLT_MAX));
  /* Infinite values.  */
  ASSERT (isinf (Infinityf ()));
  ASSERT (isinf (- Infinityf ()));
  /* Quiet NaN.  */
  ASSERT (!isinf (zerof / zerof));
#if defined FLT_EXPBIT0_WORD && defined FLT_EXPBIT0_BIT
  /* Signalling NaN.  */
  {
    #define NWORDS \
      ((sizeof (float) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
    typedef union { float value; unsigned int word[NWORDS]; } memory_float;
    memory_float m;
    m.value = zerof / zerof;
# if FLT_EXPBIT0_BIT > 0
    m.word[FLT_EXPBIT0_WORD] ^= (unsigned int) 1 << (FLT_EXPBIT0_BIT - 1);
# else
    m.word[FLT_EXPBIT0_WORD + (FLT_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    if (FLT_EXPBIT0_WORD < NWORDS / 2)
      m.word[FLT_EXPBIT0_WORD + 1] |= (unsigned int) 1 << FLT_EXPBIT0_BIT;
    else
      m.word[0] |= (unsigned int) 1;
    ASSERT (!isinf (m.value));
    #undef NWORDS
  }
#endif
}

static void
test_isinfd ()
{
  /* Zero. */
  ASSERT (!isinf (0.0));
  /* Subnormal values. */
  ASSERT (!isinf (DBL_MIN / 2));
  ASSERT (!isinf (-DBL_MIN / 2));
  /* Finite values. */
  ASSERT (!isinf (3.141));
  ASSERT (!isinf (3.141e30));
  ASSERT (!isinf (3.141e-30));
  ASSERT (!isinf (-2.718));
  ASSERT (!isinf (-2.718e30));
  ASSERT (!isinf (-2.718e-30));
  ASSERT (!isinf (DBL_MAX));
  ASSERT (!isinf (-DBL_MAX));
  /* Infinite values.  */
  ASSERT (isinf (Infinityd ()));
  ASSERT (isinf (- Infinityd ()));
  /* Quiet NaN.  */
  ASSERT (!isinf (zerod / zerod));
#if defined DBL_EXPBIT0_WORD && defined DBL_EXPBIT0_BIT
  /* Signalling NaN.  */
  {
    #define NWORDS \
      ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
    typedef union { double value; unsigned int word[NWORDS]; } memory_double;
    memory_double m;
    m.value = zerod / zerod;
# if DBL_EXPBIT0_BIT > 0
    m.word[DBL_EXPBIT0_WORD] ^= (unsigned int) 1 << (DBL_EXPBIT0_BIT - 1);
# else
    m.word[DBL_EXPBIT0_WORD + (DBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    m.word[DBL_EXPBIT0_WORD + (DBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      |= (unsigned int) 1 << DBL_EXPBIT0_BIT;
    ASSERT (!isinf (m.value));
    #undef NWORDS
  }
#endif
}

static void
test_isinfl ()
{
  #define NWORDS \
    ((sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
  typedef union { unsigned int word[NWORDS]; long double value; }
          memory_long_double;

  /* Zero. */
  ASSERT (!isinf (0.0L));
  /* Subnormal values. */
  ASSERT (!isinf (LDBL_MIN / 2));
  ASSERT (!isinf (-LDBL_MIN / 2));
  /* Finite values. */
  ASSERT (!isinf (3.141L));
  ASSERT (!isinf (3.141e30L));
  ASSERT (!isinf (3.141e-30L));
  ASSERT (!isinf (-2.718L));
  ASSERT (!isinf (-2.718e30L));
  ASSERT (!isinf (-2.718e-30L));
  ASSERT (!isinf (LDBL_MAX));
  ASSERT (!isinf (-LDBL_MAX));
  /* Infinite values.  */
  ASSERT (isinf (Infinityl ()));
  ASSERT (isinf (- Infinityl ()));
  /* Quiet NaN.  */
  ASSERT (!isinf (zerol / zerol));

#if defined LDBL_EXPBIT0_WORD && defined LDBL_EXPBIT0_BIT
  /* A bit pattern that is different from a Quiet NaN.  With a bit of luck,
     it's a Signalling NaN.  */
  {
#if defined __powerpc__ && LDBL_MANT_DIG == 106
    /* This is PowerPC "double double", a pair of two doubles.  Inf and Nan are
       represented as the corresponding 64-bit IEEE values in the first double;
       the second is ignored.  Manipulate only the first double.  */
    #undef NWORDS
    #define NWORDS \
      ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
#endif

    memory_long_double m;
    m.value = zerol / zerol;
# if LDBL_EXPBIT0_BIT > 0
    m.word[LDBL_EXPBIT0_WORD] ^= (unsigned int) 1 << (LDBL_EXPBIT0_BIT - 1);
# else
    m.word[LDBL_EXPBIT0_WORD + (LDBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    m.word[LDBL_EXPBIT0_WORD + (LDBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      |= (unsigned int) 1 << LDBL_EXPBIT0_BIT;
    ASSERT (!isinf (m.value));
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
    ASSERT (!isinf (x.value));
  }
  {
    /* Signalling NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x83333333, 0x00000000) };
    ASSERT (!isinf (x.value));
  }
  /* isinf should return something for noncanonical values.  */
  { /* Pseudo-NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x40000001, 0x00000000) };
    ASSERT (isinf (x.value) || !isinf (x.value));
  }
  { /* Pseudo-Infinity.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x00000000, 0x00000000) };
    ASSERT (isinf (x.value) || !isinf (x.value));
  }
  { /* Pseudo-Zero.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4004, 0x00000000, 0x00000000) };
    ASSERT (isinf (x.value) || !isinf (x.value));
  }
  { /* Unnormalized number.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4000, 0x63333333, 0x00000000) };
    ASSERT (isinf (x.value) || !isinf (x.value));
  }
  { /* Pseudo-Denormal.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x0000, 0x83333333, 0x00000000) };
    ASSERT (isinf (x.value) || !isinf (x.value));
  }
#endif

  #undef NWORDS
}

int
main ()
{
  test_isinff ();
  test_isinfd ();
  test_isinfl ();
  return 0;
}
