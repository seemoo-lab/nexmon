/* Machine mode definitions for GCC; included by rtl.h and tree.h.
   Copyright (C) 1991-2015 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef HAVE_MACHINE_MODES
#define HAVE_MACHINE_MODES

/* Make an enum class that gives all the machine modes.  */
#include "insn-modes.h"

/* Get the name of mode MODE as a string.  */

extern const char * const mode_name[NUM_MACHINE_MODES];
#define GET_MODE_NAME(MODE)  mode_name[MODE]

/* Mode classes.  */

#include "mode-classes.def"
#define DEF_MODE_CLASS(M) M
enum mode_class { MODE_CLASSES, MAX_MODE_CLASS };
#undef DEF_MODE_CLASS
#undef MODE_CLASSES

/* Get the general kind of object that mode MODE represents
   (integer, floating, complex, etc.)  */

extern const unsigned char mode_class[NUM_MACHINE_MODES];
#define GET_MODE_CLASS(MODE)  ((enum mode_class) mode_class[MODE])

/* Nonzero if MODE is an integral mode.  */
#define INTEGRAL_MODE_P(MODE)			\
  (GET_MODE_CLASS (MODE) == MODE_INT		\
   || GET_MODE_CLASS (MODE) == MODE_PARTIAL_INT \
   || GET_MODE_CLASS (MODE) == MODE_COMPLEX_INT \
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_INT)

/* Nonzero if MODE is a floating-point mode.  */
#define FLOAT_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_FLOAT	\
   || GET_MODE_CLASS (MODE) == MODE_DECIMAL_FLOAT \
   || GET_MODE_CLASS (MODE) == MODE_COMPLEX_FLOAT \
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_FLOAT)

/* Nonzero if MODE is a complex mode.  */
#define COMPLEX_MODE_P(MODE)			\
  (GET_MODE_CLASS (MODE) == MODE_COMPLEX_INT	\
   || GET_MODE_CLASS (MODE) == MODE_COMPLEX_FLOAT)

/* Nonzero if MODE is a vector mode.  */
#define VECTOR_MODE_P(MODE)			\
  (GET_MODE_CLASS (MODE) == MODE_VECTOR_INT	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_FLOAT	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_FRACT	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_UFRACT	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_ACCUM	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_UACCUM)

/* Nonzero if MODE is a scalar integral mode.  */
#define SCALAR_INT_MODE_P(MODE)			\
  (GET_MODE_CLASS (MODE) == MODE_INT		\
   || GET_MODE_CLASS (MODE) == MODE_PARTIAL_INT)

/* Nonzero if MODE is a scalar floating point mode.  */
#define SCALAR_FLOAT_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_FLOAT		\
   || GET_MODE_CLASS (MODE) == MODE_DECIMAL_FLOAT)

/* Nonzero if MODE is a decimal floating point mode.  */
#define DECIMAL_FLOAT_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_DECIMAL_FLOAT)

/* Nonzero if MODE is a scalar fract mode.  */
#define SCALAR_FRACT_MODE_P(MODE)	\
  (GET_MODE_CLASS (MODE) == MODE_FRACT)

/* Nonzero if MODE is a scalar ufract mode.  */
#define SCALAR_UFRACT_MODE_P(MODE)	\
  (GET_MODE_CLASS (MODE) == MODE_UFRACT)

/* Nonzero if MODE is a scalar fract or ufract mode.  */
#define ALL_SCALAR_FRACT_MODE_P(MODE)	\
  (SCALAR_FRACT_MODE_P (MODE) || SCALAR_UFRACT_MODE_P (MODE))

/* Nonzero if MODE is a scalar accum mode.  */
#define SCALAR_ACCUM_MODE_P(MODE)	\
  (GET_MODE_CLASS (MODE) == MODE_ACCUM)

/* Nonzero if MODE is a scalar uaccum mode.  */
#define SCALAR_UACCUM_MODE_P(MODE)	\
  (GET_MODE_CLASS (MODE) == MODE_UACCUM)

/* Nonzero if MODE is a scalar accum or uaccum mode.  */
#define ALL_SCALAR_ACCUM_MODE_P(MODE)	\
  (SCALAR_ACCUM_MODE_P (MODE) || SCALAR_UACCUM_MODE_P (MODE))

/* Nonzero if MODE is a scalar fract or accum mode.  */
#define SIGNED_SCALAR_FIXED_POINT_MODE_P(MODE)	\
  (SCALAR_FRACT_MODE_P (MODE) || SCALAR_ACCUM_MODE_P (MODE))

/* Nonzero if MODE is a scalar ufract or uaccum mode.  */
#define UNSIGNED_SCALAR_FIXED_POINT_MODE_P(MODE)	\
  (SCALAR_UFRACT_MODE_P (MODE) || SCALAR_UACCUM_MODE_P (MODE))

/* Nonzero if MODE is a scalar fract, ufract, accum or uaccum mode.  */
#define ALL_SCALAR_FIXED_POINT_MODE_P(MODE)	\
  (SIGNED_SCALAR_FIXED_POINT_MODE_P (MODE)	\
   || UNSIGNED_SCALAR_FIXED_POINT_MODE_P (MODE))

/* Nonzero if MODE is a scalar/vector fract mode.  */
#define FRACT_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_FRACT	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_FRACT)

/* Nonzero if MODE is a scalar/vector ufract mode.  */
#define UFRACT_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_UFRACT	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_UFRACT)

/* Nonzero if MODE is a scalar/vector fract or ufract mode.  */
#define ALL_FRACT_MODE_P(MODE)		\
  (FRACT_MODE_P (MODE) || UFRACT_MODE_P (MODE))

/* Nonzero if MODE is a scalar/vector accum mode.  */
#define ACCUM_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_ACCUM	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_ACCUM)

/* Nonzero if MODE is a scalar/vector uaccum mode.  */
#define UACCUM_MODE_P(MODE)		\
  (GET_MODE_CLASS (MODE) == MODE_UACCUM	\
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_UACCUM)

/* Nonzero if MODE is a scalar/vector accum or uaccum mode.  */
#define ALL_ACCUM_MODE_P(MODE)		\
  (ACCUM_MODE_P (MODE) || UACCUM_MODE_P (MODE))

/* Nonzero if MODE is a scalar/vector fract or accum mode.  */
#define SIGNED_FIXED_POINT_MODE_P(MODE)		\
  (FRACT_MODE_P (MODE) || ACCUM_MODE_P (MODE))

/* Nonzero if MODE is a scalar/vector ufract or uaccum mode.  */
#define UNSIGNED_FIXED_POINT_MODE_P(MODE)	\
  (UFRACT_MODE_P (MODE) || UACCUM_MODE_P (MODE))

/* Nonzero if MODE is a scalar/vector fract, ufract, accum or uaccum mode.  */
#define ALL_FIXED_POINT_MODE_P(MODE)		\
  (SIGNED_FIXED_POINT_MODE_P (MODE)		\
   || UNSIGNED_FIXED_POINT_MODE_P (MODE))

/* Nonzero if CLASS modes can be widened.  */
#define CLASS_HAS_WIDER_MODES_P(CLASS)         \
  (CLASS == MODE_INT                           \
   || CLASS == MODE_PARTIAL_INT                \
   || CLASS == MODE_FLOAT                      \
   || CLASS == MODE_DECIMAL_FLOAT              \
   || CLASS == MODE_COMPLEX_FLOAT              \
   || CLASS == MODE_FRACT                      \
   || CLASS == MODE_UFRACT                     \
   || CLASS == MODE_ACCUM                      \
   || CLASS == MODE_UACCUM)

#define POINTER_BOUNDS_MODE_P(MODE)      \
  (GET_MODE_CLASS (MODE) == MODE_POINTER_BOUNDS)

/* Get the size in bytes and bits of an object of mode MODE.  */

extern CONST_MODE_SIZE unsigned char mode_size[NUM_MACHINE_MODES];
#if GCC_VERSION >= 4001
#define GET_MODE_SIZE(MODE) \
  ((unsigned short) (__builtin_constant_p (MODE) \
		     ? mode_size_inline (MODE) : mode_size[MODE]))
#else
#define GET_MODE_SIZE(MODE)    ((unsigned short) mode_size[MODE])
#endif
#define GET_MODE_BITSIZE(MODE) \
  ((unsigned short) (GET_MODE_SIZE (MODE) * BITS_PER_UNIT))

/* Get the number of value bits of an object of mode MODE.  */
extern const unsigned short mode_precision[NUM_MACHINE_MODES];
#define GET_MODE_PRECISION(MODE)  mode_precision[MODE]

/* Get the number of integral bits of an object of mode MODE.  */
extern CONST_MODE_IBIT unsigned char mode_ibit[NUM_MACHINE_MODES];
#define GET_MODE_IBIT(MODE) mode_ibit[MODE]

/* Get the number of fractional bits of an object of mode MODE.  */
extern CONST_MODE_FBIT unsigned char mode_fbit[NUM_MACHINE_MODES];
#define GET_MODE_FBIT(MODE) mode_fbit[MODE]

/* Get a bitmask containing 1 for all bits in a word
   that fit within mode MODE.  */

extern const unsigned HOST_WIDE_INT mode_mask_array[NUM_MACHINE_MODES];

#define GET_MODE_MASK(MODE) mode_mask_array[MODE]

/* Return the mode of the inner elements in a vector.  */

extern const unsigned char mode_inner[NUM_MACHINE_MODES];
#if GCC_VERSION >= 4001
#define GET_MODE_INNER(MODE) \
  ((machine_mode) (__builtin_constant_p (MODE) \
			? mode_inner_inline (MODE) : mode_inner[MODE]))
#else
#define GET_MODE_INNER(MODE) ((machine_mode) mode_inner[MODE])
#endif

/* Get the size in bytes or bites of the basic parts of an
   object of mode MODE.  */

#define GET_MODE_UNIT_SIZE(MODE)		\
  (GET_MODE_INNER (MODE) == VOIDmode		\
   ? GET_MODE_SIZE (MODE)			\
   : GET_MODE_SIZE (GET_MODE_INNER (MODE)))

#define GET_MODE_UNIT_BITSIZE(MODE) \
  ((unsigned short) (GET_MODE_UNIT_SIZE (MODE) * BITS_PER_UNIT))

#define GET_MODE_UNIT_PRECISION(MODE)		\
  (GET_MODE_INNER (MODE) == VOIDmode		\
   ? GET_MODE_PRECISION (MODE)			\
   : GET_MODE_PRECISION (GET_MODE_INNER (MODE)))

/* Get the number of units in the object.  */

extern const unsigned char mode_nunits[NUM_MACHINE_MODES];
#if GCC_VERSION >= 4001
#define GET_MODE_NUNITS(MODE) \
  ((unsigned char) (__builtin_constant_p (MODE) \
		    ? mode_nunits_inline (MODE) : mode_nunits[MODE]))
#else
#define GET_MODE_NUNITS(MODE)  mode_nunits[MODE]
#endif

/* Get the next wider natural mode (eg, QI -> HI -> SI -> DI -> TI).  */

extern const unsigned char mode_wider[NUM_MACHINE_MODES];
#define GET_MODE_WIDER_MODE(MODE) ((machine_mode) mode_wider[MODE])

/* For scalars, this is a mode with twice the precision.  For vectors,
   this is a mode with the same inner mode but with twice the elements.  */
extern const unsigned char mode_2xwider[NUM_MACHINE_MODES];
#define GET_MODE_2XWIDER_MODE(MODE) ((machine_mode) mode_2xwider[MODE])

/* Return the mode for data of a given size SIZE and mode class CLASS.
   If LIMIT is nonzero, then don't use modes bigger than MAX_FIXED_MODE_SIZE.
   The value is BLKmode if no other mode is found.  */

extern machine_mode mode_for_size (unsigned int, enum mode_class, int);

/* Similar, but find the smallest mode for a given width.  */

extern machine_mode smallest_mode_for_size (unsigned int,
						 enum mode_class);


/* Return an integer mode of the exact same size as the input mode,
   or BLKmode on failure.  */

extern machine_mode int_mode_for_mode (machine_mode);

extern machine_mode bitwise_mode_for_mode (machine_mode);

/* Return a mode that is suitable for representing a vector,
   or BLKmode on failure.  */

extern machine_mode mode_for_vector (machine_mode, unsigned);

/* A class for iterating through possible bitfield modes.  */
class bit_field_mode_iterator
{
public:
  bit_field_mode_iterator (HOST_WIDE_INT, HOST_WIDE_INT,
			   HOST_WIDE_INT, HOST_WIDE_INT,
			   unsigned int, bool);
  bool next_mode (machine_mode *);
  bool prefer_smaller_modes ();

private:
  machine_mode m_mode;
  /* We use signed values here because the bit position can be negative
     for invalid input such as gcc.dg/pr48335-8.c.  */
  HOST_WIDE_INT m_bitsize;
  HOST_WIDE_INT m_bitpos;
  HOST_WIDE_INT m_bitregion_start;
  HOST_WIDE_INT m_bitregion_end;
  unsigned int m_align;
  bool m_volatilep;
  int m_count;
};

/* Find the best mode to use to access a bit field.  */

extern machine_mode get_best_mode (int, int,
					unsigned HOST_WIDE_INT,
					unsigned HOST_WIDE_INT,
					unsigned int,
					machine_mode, bool);

/* Determine alignment, 1<=result<=BIGGEST_ALIGNMENT.  */

extern CONST_MODE_BASE_ALIGN unsigned char mode_base_align[NUM_MACHINE_MODES];

extern unsigned get_mode_alignment (machine_mode);

#define GET_MODE_ALIGNMENT(MODE) get_mode_alignment (MODE)

/* Get the precision of the mode or its inner mode if it has one.  */

extern unsigned int element_precision (machine_mode);

/* For each class, get the narrowest mode in that class.  */

extern const unsigned char class_narrowest_mode[MAX_MODE_CLASS];
#define GET_CLASS_NARROWEST_MODE(CLASS) \
  ((machine_mode) class_narrowest_mode[CLASS])

/* Define the integer modes whose sizes are BITS_PER_UNIT and BITS_PER_WORD
   and the mode whose class is Pmode and whose size is POINTER_SIZE.  */

extern machine_mode byte_mode;
extern machine_mode word_mode;
extern machine_mode ptr_mode;

/* Target-dependent machine mode initialization - in insn-modes.c.  */
extern void init_adjust_machine_modes (void);

#define TRULY_NOOP_TRUNCATION_MODES_P(MODE1, MODE2) \
  TRULY_NOOP_TRUNCATION (GET_MODE_PRECISION (MODE1), \
			 GET_MODE_PRECISION (MODE2))

#define HWI_COMPUTABLE_MODE_P(MODE) \
  (SCALAR_INT_MODE_P (MODE) \
   && GET_MODE_PRECISION (MODE) <= HOST_BITS_PER_WIDE_INT)

typedef struct {
  /* These parts are initailized by genmodes output */
  unsigned int bitsize;
  machine_mode m;
  /* RID_* is RID_INTN_BASE + index into this array */
} int_n_data_t;

/* This is also in tree.h.  genmodes.c guarantees the're sorted from
   smallest bitsize to largest bitsize. */
extern bool int_n_enabled_p[NUM_INT_N_ENTS];
extern const int_n_data_t int_n_data[NUM_INT_N_ENTS];

#endif /* not HAVE_MACHINE_MODES */
