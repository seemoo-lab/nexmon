/* Test of canonical normalization of UTF-8 strings.
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

#if GNULIB_TEST_UNINORM_U8_NORMALIZE

#include "uninorm.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "unistr.h"
#define NO_MAIN_HERE
#include "macros.h"

static int
check (const uint8_t *input, size_t input_length,
       const uint8_t *expected, size_t expected_length)
{
  size_t length;
  uint8_t *result;

  /* Test return conventions with resultbuf == NULL.  */
  result = u8_normalize (UNINORM_NFC, input, input_length, NULL, &length);
  if (!(result != NULL))
    return 1;
  if (!(length == expected_length))
    return 2;
  if (!(u8_cmp (result, expected, expected_length) == 0))
    return 3;
  free (result);

  /* Test return conventions with resultbuf too small.  */
  if (expected_length > 0)
    {
      uint8_t *preallocated;

      length = expected_length - 1;
      preallocated = (uint8_t *) malloc (length * sizeof (uint8_t));
      result = u8_normalize (UNINORM_NFC, input, input_length, preallocated, &length);
      if (!(result != NULL))
        return 4;
      if (!(result != preallocated))
        return 5;
      if (!(length == expected_length))
        return 6;
      if (!(u8_cmp (result, expected, expected_length) == 0))
        return 7;
      free (result);
      free (preallocated);
    }

  /* Test return conventions with resultbuf large enough.  */
  {
    uint8_t *preallocated;

    length = expected_length;
    preallocated = (uint8_t *) malloc (length * sizeof (uint8_t));
    result = u8_normalize (UNINORM_NFC, input, input_length, preallocated, &length);
    if (!(result != NULL))
      return 8;
    if (!(preallocated == NULL || result == preallocated))
      return 9;
    if (!(length == expected_length))
      return 10;
    if (!(u8_cmp (result, expected, expected_length) == 0))
      return 11;
    free (preallocated);
  }

  return 0;
}

void
test_u8_nfc (void)
{
  { /* Empty string.  */
    ASSERT (check (NULL, 0, NULL, 0) == 0);
  }
  { /* SPACE */
    static const uint8_t input[]    = { 0x20 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    static const uint8_t input[]      = { 0xC3, 0x84 };
    static const uint8_t decomposed[] = { 0x41, 0xCC, 0x88 };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    static const uint8_t input[]      = { 0xC7, 0x9E };
    static const uint8_t decomposed[] = { 0x41, 0xCC, 0x88, 0xCC, 0x84 };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* ANGSTROM SIGN */
    static const uint8_t input[]      = { 0xE2, 0x84, 0xAB };
    static const uint8_t decomposed[] = { 0x41, 0xCC, 0x8A };
    static const uint8_t expected[]   = { 0xC3, 0x85 };
    ASSERT (check (input, SIZEOF (input),           expected, SIZEOF (expected)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), expected, SIZEOF (expected)) == 0);
    ASSERT (check (expected, SIZEOF (expected),     expected, SIZEOF (expected)) == 0);
  }

  { /* GREEK DIALYTIKA AND PERISPOMENI */
    static const uint8_t input[]      = { 0xE1, 0xBF, 0x81 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* SCRIPT SMALL L */
    static const uint8_t input[]      = { 0xE2, 0x84, 0x93 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* NO-BREAK SPACE */
    static const uint8_t input[]      = { 0xC2, 0xA0 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH INITIAL FORM */
    static const uint8_t input[]      = { 0xEF, 0xAD, 0xAC };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH MEDIAL FORM */
    static const uint8_t input[]      = { 0xEF, 0xAD, 0xAD };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH FINAL FORM */
    static const uint8_t input[]      = { 0xEF, 0xAD, 0xAB };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LETTER VEH ISOLATED FORM */
    static const uint8_t input[]      = { 0xEF, 0xAD, 0xAA };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* CIRCLED NUMBER FIFTEEN */
    static const uint8_t input[]      = { 0xE2, 0x91, 0xAE };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* TRADE MARK SIGN */
    static const uint8_t input[]      = { 0xE2, 0x84, 0xA2 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* LATIN SUBSCRIPT SMALL LETTER I */
    static const uint8_t input[]      = { 0xE1, 0xB5, 0xA2 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS */
    static const uint8_t input[]      = { 0xEF, 0xB8, 0xB5 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* FULLWIDTH LATIN CAPITAL LETTER A */
    static const uint8_t input[]      = { 0xEF, 0xBC, 0xA1 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* HALFWIDTH IDEOGRAPHIC COMMA */
    static const uint8_t input[]      = { 0xEF, 0xBD, 0xA4 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* SMALL IDEOGRAPHIC COMMA */
    static const uint8_t input[]      = { 0xEF, 0xB9, 0x91 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* SQUARE MHZ */
    static const uint8_t input[]      = { 0xE3, 0x8E, 0x92 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* VULGAR FRACTION THREE EIGHTHS */
    static const uint8_t input[]      = { 0xE2, 0x85, 0x9C };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* MICRO SIGN */
    static const uint8_t input[]      = { 0xC2, 0xB5 };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* ARABIC LIGATURE SALLALLAHOU ALAYHE WASALLAM */
    static const uint8_t input[]      = { 0xEF, 0xB7, 0xBA };
    ASSERT (check (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  { /* HANGUL SYLLABLE GEUL */
    static const uint8_t input[]      = { 0xEA, 0xB8, 0x80 };
    static const uint8_t decomposed[] =
      { 0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3, 0xE1, 0x86, 0xAF };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* HANGUL SYLLABLE GEU */
    static const uint8_t input[]      = { 0xEA, 0xB7, 0xB8 };
    static const uint8_t decomposed[] = { 0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3 };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint8_t input[] =
      { 'G', 'r', 0xC3, 0xBC, 0xC3, 0x9F, ' ', 'G', 'o', 't', 't', '.',
        ' ', 0xD0, 0x97, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1,
        0x81, 0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB9,
        0xD1, 0x82, 0xD0, 0xB5, '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1,
        's', 'q', 'r', 't', '(', 'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')',
        '/', '(', '2', 'a', ')', ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC,
        0xE8, 0xAA, 0x9E, ',', 0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C,
        0xEA, 0xB8, 0x80, '\n'
      };
    static const uint8_t decomposed[] =
      { 'G', 'r', 0x75, 0xCC, 0x88, 0xC3, 0x9F, ' ', 'G', 'o', 't', 't', '.',
        ' ', 0xD0, 0x97, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1,
        0x81, 0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB8, 0xCC, 0x86,
        0xD1, 0x82, 0xD0, 0xB5, '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1,
        's', 'q', 'r', 't', '(', 'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')',
        '/', '(', '2', 'a', ')', ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC,
        0xE8, 0xAA, 0x9E, ',', 0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xE1, 0x84, 0x92, 0xE1, 0x85, 0xA1, 0xE1, 0x86, 0xAB,
        0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3, 0xE1, 0x86, 0xAF, '\n'
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
      uint8_t *input = (uint8_t *) malloc (2 * (2 * m - 1) * sizeof (uint8_t));
      if (input != NULL)
        {
          uint8_t *expected = input + (2 * m - 1);
          size_t m1 = m / 2;
          size_t m2 = (m - 1) / 2;
          /* NB: m1 + m2 == m - 1.  */
          uint8_t *p;

          input[0] = 0x41;
          p = input + 1;
          switch (pass)
            {
            case 0:
              for (size_t i = 0; i < m1; i++)
                {
                  *p++ = 0xCC;
                  *p++ = 0x99;
                }
              for (size_t i = 0; i < m2; i++)
                {
                  *p++ = 0xCC;
                  *p++ = 0x80;
                }
              break;

            case 1:
              for (size_t i = 0; i < m2; i++)
                {
                  *p++ = 0xCC;
                  *p++ = 0x80;
                }
              for (size_t i = 0; i < m1; i++)
                {
                  *p++ = 0xCC;
                  *p++ = 0x99;
                }
              break;

            case 2:
              for (size_t i = 0; i < m2; i++)
                {
                  *p++ = 0xCC;
                  *p++ = 0x99;
                  *p++ = 0xCC;
                  *p++ = 0x80;
                }
              for (size_t i = m2; i < m1; i++)
                {
                  *p++ = 0xCC;
                  *p++ = 0x99;
                }
              break;

            default:
              abort ();
            }

          expected[0] = 0xC3;
          expected[1] = 0x80;
          p = expected + 2;
          for (size_t i = 0; i < m1; i++)
            {
              *p++ = 0xCC;
              *p++ = 0x99;
            }
          for (size_t i = 0; i < m2 - 1; i++)
            {
              *p++ = 0xCC;
              *p++ = 0x80;
            }

          for (; repeat > 0; repeat--)
            {
              ASSERT (check (input, 2 * m - 1,    expected, 2 * m - 2) == 0);
              ASSERT (check (expected, 2 * m - 2, expected, 2 * m - 2) == 0);
            }

          free (input);
        }
    }
}

#else

void
test_u8_nfc (void)
{
}

#endif
