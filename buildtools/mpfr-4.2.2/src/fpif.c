/* mpfr_fpif -- Binary export & import of MPFR numbers
   (floating-point interchange format)

Copyright 2012-2025 Free Software Foundation, Inc.
Contributed by Olivier Demengeon.

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

#include "mpfr-impl.h"

#if !defined (HAVE_BIG_ENDIAN) && !defined (HAVE_LITTLE_ENDIAN)
#error "Endianness is unknown. Not supported yet."
#endif

/* The format is described as follows. Any multi-byte number is encoded
   in little endian.

   1. We first store the precision p (this format is able to represent
      any precision from 1 to 2^64 + 248).
      Let B be the first byte (0 <= B <= 255).
        * If B >= 8, the precision p is B-7.
          Here, the condition is equivalent to 1 <= p <= 248.
        * If B <= 7, the next B+1 bytes contain p-249.
          Here, the condition is equivalent to 249 <= p <= 2^64 + 248.
      We will use the following macros:
        * MPFR_MAX_PRECSIZE = 7
        * MPFR_MAX_EMBEDDED_PRECISION = 255 - 7 = 248

   2. Then we store the sign bit and exponent related information
      (possibly a special value). We first have byte A = [seeeeeee],
      where s is the sign bit and E = [eeeeeee] such that:
        * If 0 <= E <= 94, then the exponent e is E-47 (-47 <= e <= 47).
        * If 95 <= E <= 110, the exponent is stored in the next E-94 bytes
          (1 to 16 bytes) in sign + absolute value representation,
          where the absolute value is increased by 47 (e <= -47 or 47 <= e).
        * If 111 <= E <= 118, the exponent size S is stored in the next
          E-110 bytes (1 to 8), then the exponent itself is stored in the
          next S bytes. [Not implemented yet]
        * If 119 <= E <= 127, we have a special value:
          E = 119 (MPFR_KIND_ZERO) for a signed zero;
          E = 120 (MPFR_KIND_INF) for a signed infinity;
          E = 121 (MPFR_KIND_NAN) for NaN.

   3. Then we store the significand (for regular values).

   The sign bit is preserved by the import/export functions, even for NaN.

   Note: When a size is stored, it must be minimal, i.e. a number cannot
   start with a null byte. Otherwise the import may fail.
*/

#define MPFR_MAX_PRECSIZE 7
#define MPFR_MAX_EMBEDDED_PRECISION (255 - MPFR_MAX_PRECSIZE)

#define MPFR_KIND_ZERO 119
#define MPFR_KIND_INF 120
#define MPFR_KIND_NAN 121
#define MPFR_MAX_EMBEDDED_EXPONENT 47
#define MPFR_EXTERNAL_EXPONENT 94

/* Begin: Low level helper functions */

/* storage must have an unsigned type */
#define COUNT_NB_BYTE(storage, size)            \
  do                                            \
    {                                           \
      (storage) >>= 8;                          \
      (size)++;                                 \
    }                                           \
  while ((storage) != 0)

#define ALLOC_RESULT(buffer, buffer_size, wanted_size)                  \
  do                                                                    \
    {                                                                   \
      if ((buffer) == NULL || *(buffer_size) < (wanted_size))           \
        {                                                               \
          (buffer) = (unsigned char *) mpfr_reallocate_func             \
            ((buffer), *(buffer_size), (wanted_size));                  \
          MPFR_ASSERTN((buffer) != 0);                                  \
        }                                                               \
      *(buffer_size) = (wanted_size);                                   \
    }                                                                   \
  while (0)

/*
 * size in byte of a MPFR number in a binary object of a variable size
 */
#define MAX_VARIABLE_STORAGE(exponent_size, precision) \
  ((size_t)(((precision) >> 3) + (exponent_size) +     \
            ((precision) > 248 ? sizeof(mpfr_prec_t) : 0) + 3))

/* copy in result[] the values in data[] with a different endianness,
   where data_size might be smaller than data_max_size, so that we only
   copy data_size bytes from the end of data[]. */
static void
#if defined (HAVE_BIG_ENDIAN)
putLittleEndianData (unsigned char *result, unsigned char *data,
                     size_t data_max_size, size_t data_size)
#elif defined (HAVE_LITTLE_ENDIAN)
putBigEndianData (unsigned char *result, unsigned char *data,
                  size_t data_max_size, size_t data_size)
#endif
{
  size_t j;

  MPFR_ASSERTD (data_size <= data_max_size);
  for (j = 0; j < data_size; j++)
    result[j] = data[data_max_size - j - 1];
}

/* copy in result[] the values in data[] with the same endianness */
static void
#if defined (HAVE_BIG_ENDIAN)
putBigEndianData (unsigned char *result, unsigned char *data,
                  size_t data_max_size, size_t data_size)
#elif defined (HAVE_LITTLE_ENDIAN)
putLittleEndianData (unsigned char *result, unsigned char *data,
                     size_t data_max_size, size_t data_size)
#endif
{
  MPFR_ASSERTD (data_size <= data_max_size);
  memcpy (result, data, data_size);
}

/* copy in result[] the values in data[] with a different endianness;
   the data are written at the end of the result[] buffer (if
   data_size < data_max_size, the first bytes of result[] are
   left untouched). */
static void
#if defined (HAVE_BIG_ENDIAN)
getLittleEndianData (unsigned char *result, unsigned char *data,
                     size_t data_max_size, size_t data_size)
#elif defined (HAVE_LITTLE_ENDIAN)
getBigEndianData (unsigned char *result, unsigned char *data,
                  size_t data_max_size, size_t data_size)
#endif
{
  size_t j;

  MPFR_ASSERTD (data_size <= data_max_size);
  for (j = 0; j < data_size; j++)
    result[data_max_size - j - 1] = data[j];
}

/* copy in result[] the values in data[] with the same endianness */
static void
#if defined (HAVE_BIG_ENDIAN)
getBigEndianData (unsigned char *result, unsigned char *data,
                  size_t data_max_size, size_t data_size)
#elif defined (HAVE_LITTLE_ENDIAN)
getLittleEndianData (unsigned char *result, unsigned char *data,
                     size_t data_max_size, size_t data_size)
#endif
{
  MPFR_ASSERTD (data_size <= data_max_size);
  memcpy (result, data, data_size);
}

/* End: Low level helper functions */

/* Internal Function */
/*
 * buffer : OUT : store the precision in binary format, can be null
 *               (may be reallocated if too small)
 * buffer_size : IN/OUT : size of the buffer => size used in the buffer
 * precision : IN : precision to store
 * return pointer to a buffer storing the precision in binary format
 */
static unsigned char *
mpfr_fpif_store_precision (unsigned char *buffer, size_t *buffer_size,
                           mpfr_prec_t precision)
{
  unsigned char *result;
  size_t size_precision;

  MPFR_ASSERTD (precision >= 1);
  size_precision = 0;

  if (precision > MPFR_MAX_EMBEDDED_PRECISION)
    {
      mpfr_uprec_t copy_precision;

      copy_precision = precision - (MPFR_MAX_EMBEDDED_PRECISION + 1);
      COUNT_NB_BYTE(copy_precision, size_precision);
    }

  result = buffer;
  ALLOC_RESULT(result, buffer_size, size_precision + 1);

  if (precision > MPFR_MAX_EMBEDDED_PRECISION)
    {
      result[0] = size_precision - 1;
      precision -= (MPFR_MAX_EMBEDDED_PRECISION + 1);
      putLittleEndianData (result + 1, (unsigned char *) &precision,
                           sizeof(mpfr_prec_t), size_precision);
    }
  else
    result[0] = precision + MPFR_MAX_PRECSIZE;

  return result;
}

#define BUFFER_SIZE 8

/*
 * fh : IN : file handler
 * return the precision stored in the binary buffer, 0 in case of error
 */
static mpfr_prec_t
mpfr_fpif_read_precision_from_file (FILE *fh)
{
  mpfr_prec_t precision;
  size_t precision_size;
  unsigned char buffer[BUFFER_SIZE];

  if (fh == NULL)
    return 0;

  if (fread (buffer, 1, 1, fh) != 1)
    return 0;

  precision_size = buffer[0];
  if (precision_size > MPFR_MAX_PRECSIZE)
    return precision_size - MPFR_MAX_PRECSIZE;

  precision_size++;
  MPFR_ASSERTD (precision_size <= BUFFER_SIZE);

  /* Read the precision in little-endian format. */
  if (fread (buffer, precision_size, 1, fh) != 1)
    return 0;

  /* Justification of the #if below. */
  MPFR_ASSERTD (precision_size <= MPFR_MAX_PRECSIZE + 1);

#if (MPFR_MAX_PRECSIZE + 1) * CHAR_BIT > MPFR_PREC_BITS
  while (precision_size > sizeof(mpfr_prec_t))
    {
      if (buffer[precision_size-1] != 0)
        return 0;  /* the read precision doesn't fit in a mpfr_prec_t */
      precision_size--;
    }
#endif

  /* To detect bugs affecting particular platforms (thus MPFR_ASSERTN)... */
  MPFR_ASSERTN (precision_size <= sizeof(mpfr_prec_t));

  /* Since mpfr_prec_t is signed, one also needs to check that the
     most significant bit of the corresponding unsigned value is 0. */
  if (precision_size == sizeof(mpfr_prec_t) &&
      buffer[precision_size-1] >= 0x80)
    return 0;  /* the read precision doesn't fit in a mpfr_prec_t */

  precision = 0;  /* to pad with 0's if data_size < data_max_size */

  /* On big-endian machines, the data must be copied at the end of the
     precision object in the memory; thus data_max_size (3rd argument)
     must be sizeof(mpfr_prec_t). */
  getLittleEndianData ((unsigned char *) &precision, buffer,
                       sizeof(mpfr_prec_t), precision_size);

  return precision + (MPFR_MAX_EMBEDDED_PRECISION + 1);
}

/*
 * buffer : OUT : store the kind of the MPFR number x, its sign, the size of
 *                its exponent and its exponent value in a binary format,
 *                can be null (may be reallocated if too small)
 * buffer_size : IN/OUT : size of the buffer => size used in the buffer
 * x : IN : MPFR number
 * return pointer to a buffer storing the kind of the MPFR number x, its sign,
 *        the size of its exponent and its exponent value in a binary format,
 */
/* TODO
 *   Exponents that use more than 16 bytes are not managed (not an issue
 *   until one has integer types larger than 128 bits).
 */
static unsigned char*
mpfr_fpif_store_exponent (unsigned char *buffer, size_t *buffer_size,
                          mpfr_ptr x)
{
  unsigned char *result;
  mpfr_uexp_t uexp;
  size_t exponent_size;

  exponent_size = 0;

  if (MPFR_IS_PURE_FP (x))
    {
      mpfr_exp_t exponent = MPFR_GET_EXP (x);

      if (exponent > MPFR_MAX_EMBEDDED_EXPONENT ||
          exponent < -MPFR_MAX_EMBEDDED_EXPONENT)
        {
          mpfr_uexp_t copy_exponent, exp_sign_bit;

          uexp = SAFE_ABS (mpfr_uexp_t, exponent)
            - MPFR_MAX_EMBEDDED_EXPONENT;

          /* Shift uexp to take the sign bit of the exponent into account.
             Because of constraints on the valid exponents, this cannot
             overflow (check with an MPFR_ASSERTD). */
          copy_exponent = uexp << 1;
          MPFR_ASSERTD (copy_exponent > uexp);
          COUNT_NB_BYTE(copy_exponent, exponent_size);
          MPFR_ASSERTN (exponent_size <= 16);  /* see TODO */

          /* Sign bit of the exponent. */
          exp_sign_bit = (mpfr_uexp_t) 1 << (8 * exponent_size - 1);
          MPFR_ASSERTD (uexp < exp_sign_bit);
          if (exponent < 0)
            uexp |= exp_sign_bit;
        }
      else
        uexp = exponent + MPFR_MAX_EMBEDDED_EXPONENT;
    }

  result = buffer;
  ALLOC_RESULT(result, buffer_size, exponent_size + 1);

  if (MPFR_IS_PURE_FP (x))
    {
      if (exponent_size == 0)
        result[0] = uexp;
      else
        {
          result[0] = MPFR_EXTERNAL_EXPONENT + exponent_size;

          putLittleEndianData (result + 1, (unsigned char *) &uexp,
                               sizeof(mpfr_exp_t), exponent_size);
        }
    }
  else if (MPFR_IS_ZERO (x))
    result[0] = MPFR_KIND_ZERO;
  else if (MPFR_IS_INF (x))
    result[0] = MPFR_KIND_INF;
  else
    {
      MPFR_ASSERTD (MPFR_IS_NAN (x));
      result[0] = MPFR_KIND_NAN;
    }

  /* Set the sign, even for NaN. */
  if (MPFR_IS_NEG (x))
    result[0] |= 0x80;

  return result;
}

/*
 * x : OUT : MPFR number extracted from the binary buffer
 * fh : IN : file handler (should not be NULL)
 * return 0 if successful
 */
/* TODO
 *   Exponents that use more than 16 bytes are not managed (this is not
 *   an issue if the data were written by MPFR with mpfr_exp_t not larger
 *   than 128 bits).
 */
static int
mpfr_fpif_read_exponent_from_file (mpfr_ptr x, FILE * fh)
{
  mpfr_exp_t exponent;
  mpfr_uexp_t uexp;
  size_t exponent_size;
  int sign;
  unsigned char buffer[sizeof(mpfr_exp_t)];

  MPFR_ASSERTD(fh != NULL);

  if (fread (buffer, 1, 1, fh) != 1)
    return 1;

  /* sign value that can be used with MPFR_SET_SIGN,
     mpfr_set_zero and mpfr_set_inf */
  sign = (buffer[0] & 0x80) ? MPFR_SIGN_NEG : MPFR_SIGN_POS;
  /* Set the sign, even for NaN. */
  MPFR_SET_SIGN (x, sign);

  exponent = buffer[0] & 0x7F;
  exponent_size = 1;

  if (exponent > MPFR_EXTERNAL_EXPONENT && exponent < MPFR_KIND_ZERO)
    {
      mpfr_uexp_t exp_sign_bit;

      exponent_size = exponent - MPFR_EXTERNAL_EXPONENT;

      /* A failure is acceptable when the exponent starts with leading zeros,
         even if it would fit in mpfr_exp_t (see format description). */
      if (MPFR_UNLIKELY (exponent_size > 16 /* see TODO */ ||
                         exponent_size > sizeof(mpfr_exp_t)))
        return 1;

      if (MPFR_UNLIKELY (fread (buffer, exponent_size, 1, fh) != 1))
        return 1;

      uexp = 0;
      getLittleEndianData ((unsigned char *) &uexp, buffer,
                           sizeof(mpfr_exp_t), exponent_size);

      /* Sign bit of the exponent. */
      exp_sign_bit = uexp & ((mpfr_uexp_t) 1 << (8 * exponent_size - 1));

      uexp &= ~exp_sign_bit;
      uexp += MPFR_MAX_EMBEDDED_EXPONENT;
      if (MPFR_UNLIKELY (uexp > MPFR_EMAX_MAX && uexp > -MPFR_EMIN_MIN))
        return 1;

      exponent = exp_sign_bit ? - (mpfr_exp_t) uexp : (mpfr_exp_t) uexp;
      if (MPFR_UNLIKELY (! MPFR_EXP_IN_RANGE (exponent)))
        return 1;
      MPFR_SET_EXP (x, exponent);

      exponent_size++;
    }
  else if (exponent == MPFR_KIND_ZERO)
    MPFR_SET_ZERO (x);
  else if (exponent == MPFR_KIND_INF)
    MPFR_SET_INF (x);
  else if (exponent == MPFR_KIND_NAN)
    MPFR_SET_NAN (x);
  else if (exponent <= MPFR_EXTERNAL_EXPONENT)
    {
      exponent -= MPFR_MAX_EMBEDDED_EXPONENT;
      if (MPFR_UNLIKELY (! MPFR_EXP_IN_RANGE (exponent)))
        return 1;
      MPFR_SET_EXP (x, exponent);
    }
  else
    return 1;

  return 0;
}

/*
 * buffer : OUT : store the limb of the MPFR number x in a binary format,
 *                can be null (may be reallocated if too small)
 * buffer_size : IN/OUT : size of the buffer => size used in the buffer
 * x : IN : MPFR number
 * return pointer to a buffer storing the limb of the MPFR number x in a binary
 *        format
 */
static unsigned char*
mpfr_fpif_store_limbs (unsigned char *buffer, size_t *buffer_size, mpfr_ptr x)
{
  unsigned char *result;
  mpfr_prec_t precision;
  size_t nb_byte;
  size_t nb_limb, mp_bytes_per_limb;
  size_t nb_partial_byte;
  size_t i, j;

  precision = mpfr_get_prec (x);
  nb_byte = (precision + 7) >> 3;
  mp_bytes_per_limb = mp_bits_per_limb >> 3;
  nb_partial_byte = nb_byte % mp_bytes_per_limb;
  nb_limb = (nb_byte + mp_bytes_per_limb - 1) / mp_bytes_per_limb;

  result = buffer;
  ALLOC_RESULT(result, buffer_size, nb_byte);

  putBigEndianData (result, (unsigned char*) MPFR_MANT(x),
                    sizeof(mp_limb_t), nb_partial_byte);
  for (i = nb_partial_byte, j = (nb_partial_byte == 0) ? 0 : 1; j < nb_limb;
       i += mp_bytes_per_limb, j++)
    putLittleEndianData (result + i, (unsigned char*) (MPFR_MANT(x) + j),
                         sizeof(mp_limb_t), sizeof(mp_limb_t));

  return result;
}

/*
 * x : OUT : MPFR number extracted from the binary buffer, should have the same
 *           precision than the number in the binary format
 * buffer : IN : limb of the MPFR number x in a binary format
 * nb_byte : IN : size of the buffer (in bytes)
 * Assume buffer is not NULL.
 */
static void
mpfr_fpif_read_limbs (mpfr_ptr x, unsigned char *buffer, size_t nb_byte)
{
  size_t mp_bytes_per_limb;
  size_t nb_partial_byte;
  size_t i, j;

  MPFR_ASSERTD (buffer != NULL);

  mp_bytes_per_limb = mp_bits_per_limb >> 3;
  nb_partial_byte = nb_byte % mp_bytes_per_limb;

  if (nb_partial_byte > 0)
    {
      memset (MPFR_MANT(x), 0, sizeof(mp_limb_t));
      getBigEndianData ((unsigned char*) MPFR_MANT(x), buffer,
                        sizeof(mp_limb_t), nb_partial_byte);
    }
  for (i = nb_partial_byte, j = (nb_partial_byte == 0) ? 0 : 1; i < nb_byte;
       i += mp_bytes_per_limb, j++)
    getLittleEndianData ((unsigned char*) (MPFR_MANT(x) + j), buffer + i,
                         sizeof(mp_limb_t), sizeof(mp_limb_t));
}

/* External Function */
/*
 * fh : IN : file handler
 * x : IN : MPFR number to put in the file
 * return 0 if successful
 */
int
mpfr_fpif_export (FILE *fh, mpfr_ptr x)
{
  int status;
  unsigned char *buf;
  unsigned char *bufResult;
  size_t used_size, buf_size;

  if (fh == NULL)
    return -1;

  buf_size = MAX_VARIABLE_STORAGE(sizeof(mpfr_exp_t), mpfr_get_prec (x));
  buf = (unsigned char*) mpfr_allocate_func (buf_size);
  MPFR_ASSERTN(buf != NULL);

  used_size = buf_size;
  buf = mpfr_fpif_store_precision (buf, &used_size, mpfr_get_prec (x));
  used_size > buf_size ? buf_size = used_size : 0;
  status = fwrite (buf, used_size, 1, fh);
  if (status != 1)
    {
      mpfr_free_func (buf, buf_size);
      return -1;
    }
  used_size = buf_size;
  bufResult = mpfr_fpif_store_exponent (buf, &used_size, x);
  /* bufResult cannot be NULL: if reallocation failed in
     mpfr_fpif_store_exponent, an assertion failed */
  buf = bufResult;
  used_size > buf_size ? buf_size = used_size : 0;
  status = fwrite (buf, used_size, 1, fh);
  if (status != 1)
    {
      mpfr_free_func (buf, buf_size);
      return -1;
    }

  if (mpfr_regular_p (x))
    {
      used_size = buf_size;
      buf = mpfr_fpif_store_limbs (buf, &used_size, x);
      used_size > buf_size ? buf_size = used_size : 0;
      status = fwrite (buf, used_size, 1, fh);
      if (status != 1)
        {
          mpfr_free_func (buf, buf_size);
          return -1;
        }
    }

  mpfr_free_func (buf, buf_size);
  return 0;
}

/*
 * x : IN/OUT : MPFR number extracted from the file, its precision is reset to
 *              be able to hold the number
 * fh : IN : file handler
 * Return 0 if the import was successful.
 */
int
mpfr_fpif_import (mpfr_ptr x, FILE *fh)
{
  int status;
  mpfr_prec_t precision;
  unsigned char *buffer;
  size_t used_size;

  precision = mpfr_fpif_read_precision_from_file (fh);
  if (precision == 0) /* precision = 0 means an error */
    return -1;
  MPFR_ASSERTD(fh != NULL); /* checked by mpfr_fpif_read_precision_from_file */
  if (precision > MPFR_PREC_MAX)
    return -1;
  MPFR_STAT_STATIC_ASSERT (MPFR_PREC_MIN == 1);  /* as specified */
  mpfr_set_prec (x, precision);

  status = mpfr_fpif_read_exponent_from_file (x, fh);
  if (status != 0)
    {
      mpfr_set_nan (x);
      return -1;
    }

  /* Warning! The significand of x is not set yet. Thus use MPFR_IS_SINGULAR
     for the test. */
  if (!MPFR_IS_SINGULAR (x))
    {
      /* For portability, we need to consider bytes with only 8 significant
         bits in the interchange format. That's OK because CHAR_BIT >= 8.
         But the implementation is currently not clear when CHAR_BIT > 8.
         This may have never been tested. For safety, require CHAR_BIT == 8,
         and test/adapt the code if this ever fails. */
      MPFR_STAT_STATIC_ASSERT (CHAR_BIT == 8);
      MPFR_STAT_STATIC_ASSERT ((MPFR_PREC_MAX + 7) >> 3 <= (size_t) -1);
      used_size = (precision + 7) >> 3; /* ceil(precision/8) */
      buffer = (unsigned char*) mpfr_allocate_func (used_size);
      MPFR_ASSERTN(buffer != NULL);
      status = fread (buffer, used_size, 1, fh);
      if (status != 1)
        {
          mpfr_free_func (buffer, used_size);
          mpfr_set_nan (x);
          return -1;
        }
      mpfr_fpif_read_limbs (x, buffer, used_size);
      mpfr_free_func (buffer, used_size);
    }

  return 0;
}
