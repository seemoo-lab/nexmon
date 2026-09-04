/* Test of canonical normalization of UTF-16 strings.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#if GNULIB_TEST_UNINORM_U16_NORMALIZE

#include "uninorm.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "unistr.h"
#define NO_MAIN_HERE
#include "macros.h"

static int
check (const uint16_t *input, size_t input_length,
       const uint16_t *expected, size_t expected_length)
{
  size_t length;
  uint16_t *result;

  /* Test return conventions with resultbuf == NULL.  */
  result = u16_normalize (UNINORM_NFC, input, input_length, NULL, &length);
  if (!(result != NULL))
    return 1;
  if (!(length == expected_length))
    return 2;
  if (!(u16_cmp (result, expected, expected_length) == 0))
    return 3;
  free (result);

  /* Test return conventions with resultbuf too small.  */
  if (expected_length > 0)
    {
      uint16_t *preallocated;

      length = expected_length - 1;
      preallocated = (uint16_t *) malloc (length * sizeof (uint16_t));
      result = u16_normalize (UNINORM_NFC, input, input_length, preallocated, &length);
      if (!(result != NULL))
        return 4;
      if (!(result != preallocated))
        return 5;
      if (!(length == expected_length))
        return 6;
      if (!(u16_cmp (result, expected, expected_length) == 0))
        return 7;
      free (result);
      free (preallocated);
    }

  /* Test return conventions with resultbuf large enough.  */
  {
    uint16_t *preallocated;

    length = expected_length;
    preallocated = (uint16_t *) malloc (length * sizeof (uint16_t));
    result = u16_normalize (UNINORM_NFC, input, input_length, preallocated, &length);
    if (!(result != NULL))
      return 8;
    if (!(preallocated == NULL || result == preallocated))
      return 9;
    if (!(length == expected_length))
      return 10;
    if (!(u16_cmp (result, expected, expected_length) == 0))
      return 11;
    free (preallocated);
  }

  return 0;
}

void
test_u16_nfc (void)
{
  { /* Empty string.  */
    ASSERT (check (NULL, 0, NULL, 0) == 0);
  }
  { /* SPACE */
    static const uint16_t input[]    = { 0x0020 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    static const uint16_t input[]      = { 0x00C4 };
    static const uint16_t decomposed[] = { 0x0041, 0x0308 };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    static const uint16_t input[]      = { 0x01DE };
    static const uint16_t decomposed[] = { 0x0041, 0x0308, 0x0304 };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* ANGSTROM SIGN */
    static const uint16_t input[]      = { 0x212B };
    static const uint16_t decomposed[] = { 0x0041, 0x030A };
    static const uint16_t expected[]   = { 0x00C5 };
    ASSERT (check (input, SIZEOF (input),           expected, SIZEOF (expected)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), expected, SIZEOF (expected)) == 0);
    ASSERT (check (expected, SIZEOF (expected),     expected, SIZEOF (expected)) == 0);
  }

  { /* GREEK DIALYTIKA AND PERISPOMENI */
    static const uint16_t input[]      = { 0x1FC1 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* SCRIPT SMALL L */
    static const uint16_t input[]      = { 0x2113 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* NO-BREAK SPACE */
    static const uint16_t input[]      = { 0x00A0 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH INITIAL FORM */
    static const uint16_t input[]      = { 0xFB6C };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH MEDIAL FORM */
    static const uint16_t input[]      = { 0xFB6D };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH FINAL FORM */
    static const uint16_t input[]      = { 0xFB6B };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH ISOLATED FORM */
    static const uint16_t input[]      = { 0xFB6A };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* CIRCLED NUMBER FIFTEEN */
    static const uint16_t input[]      = { 0x246E };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* TRADE MARK SIGN */
    static const uint16_t input[]      = { 0x2122 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* LATIN SUBSCRIPT SMALL LETTER I */
    static const uint16_t input[]      = { 0x1D62 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS */
    static const uint16_t input[]      = { 0xFE35 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* FULLWIDTH LATIN CAPITAL LETTER A */
    static const uint16_t input[]      = { 0xFF21 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* HALFWIDTH IDEOGRAPHIC COMMA */
    static const uint16_t input[]      = { 0xFF64 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* SMALL IDEOGRAPHIC COMMA */
    static const uint16_t input[]      = { 0xFE51 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* SQUARE MHZ */
    static const uint16_t input[]      = { 0x3392 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* VULGAR FRACTION THREE EIGHTHS */
    static const uint16_t input[]      = { 0x215C };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* MICRO SIGN */
    static const uint16_t input[]      = { 0x00B5 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LIGATURE SALLALLAHOU ALAYHE WASALLAM */
    static const uint16_t input[]      = { 0xFDFA };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* HANGUL SYLLABLE GEUL */
    static const uint16_t input[]      = { 0xAE00 };
    static const uint16_t decomposed[] = { 0x1100, 0x1173, 0x11AF };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* HANGUL SYLLABLE GEU */
    static const uint16_t input[]      = { 0xADF8 };
    static const uint16_t decomposed[] = { 0x1100, 0x1173 };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint16_t input[] =
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    static const uint16_t decomposed[] =
      { 'G', 'r', 0x0075, 0x0308, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0438, 0x0306, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',',
        0x1112, 0x1161, 0x11AB, 0x1100, 0x1173, 0x11AF, '\n'
      };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

#if HAVE_DECL_ALARM
  /* Declare failure if test takes too long, by using default abort
     caused by SIGALRM.  */
  signal (SIGALRM, SIG_DFL);
  alarm (50);
#endif

  /* Check that the sorting is not O(n²) but O(n log n).  */
  for (int pass = 0; pass < 3; pass++)
    {
      size_t repeat = 1;
      size_t m = 100000;
      uint16_t *input = (uint16_t *) malloc (2 * m * sizeof (uint16_t));
      if (input != NULL)
        {
          uint16_t *expected = input + m;
          size_t m1 = m / 2;
          size_t m2 = (m - 1) / 2;
          /* NB: m1 + m2 == m - 1.  */
          uint16_t *p;

          input[0] = 0x0041;
          p = input + 1;
          switch (pass)
            {
            case 0:
              for (size_t i = 0; i < m1; i++)
                *p++ = 0x0319;
              for (size_t i = 0; i < m2; i++)
                *p++ = 0x0300;
              break;

            case 1:
              for (size_t i = 0; i < m2; i++)
                *p++ = 0x0300;
              for (size_t i = 0; i < m1; i++)
                *p++ = 0x0319;
              break;

            case 2:
              for (size_t i = 0; i < m2; i++)
                {
                  *p++ = 0x0319;
                  *p++ = 0x0300;
                }
              for (size_t i = m2; i < m1; i++)
                *p++ = 0x0319;
              break;

            default:
              abort ();
            }

          expected[0] = 0x00C0;
          p = expected + 1;
          for (size_t i = 0; i < m1; i++)
            *p++ = 0x0319;
          for (size_t i = 0; i < m2 - 1; i++)
            *p++ = 0x0300;

          for (; repeat > 0; repeat--)
            {
              ASSERT (check (input, m,        expected, m - 1) == 0);
              ASSERT (check (expected, m - 1, expected, m - 1) == 0);
            }

          free (input);
        }
    }
}

#else

void
test_u16_nfc (void)
{
}

#endif
