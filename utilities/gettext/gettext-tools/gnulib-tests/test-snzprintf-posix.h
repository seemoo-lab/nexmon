/* Test of POSIX compatible vsnzprintf() and snzprintf() functions.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

/* This test exercises only a few POSIX compliance problems that are still
   visible on platforms relevant in 2024.  For a much more complete test suite,
   see test-snprintf-posix.h.  */

#include "infinity.h"
#include "nan.h"

/* Test whether string[start_index..end_index-1] is a valid textual
   representation of NaN.  */
static int
strisnan (const char *string, size_t start_index, size_t end_index, int uppercase)
{
  if (start_index < end_index)
    {
      if (string[start_index] == '-')
        start_index++;
      if (start_index + 3 <= end_index
          && memcmp (string + start_index, uppercase ? "NAN" : "nan", 3) == 0)
        {
          start_index += 3;
          if (start_index == end_index
              || (string[start_index] == '(' && string[end_index - 1] == ')'))
            return 1;
        }
    }
  return 0;
}

static void
test_function (ptrdiff_t (*my_snzprintf) (char *, size_t, const char *, ...))
{
  char result[5000];

  /* Test the support of the 'a' and 'A' conversion specifier for hexadecimal
     output of floating-point numbers.  */

  { /* This test would fail on Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%a %d", 3.1416015625, 33, 44, 55);
    ASSERT (strcmp (result, "0x1.922p+1 33") == 0
            || strcmp (result, "0x3.244p+0 33") == 0
            || strcmp (result, "0x6.488p-1 33") == 0
            || strcmp (result, "0xc.91p-2 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%A %d", -3.1416015625, 33, 44, 55);
    ASSERT (strcmp (result, "-0X1.922P+1 33") == 0
            || strcmp (result, "-0X3.244P+0 33") == 0
            || strcmp (result, "-0X6.488P-1 33") == 0
            || strcmp (result, "-0XC.91P-2 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on FreeBSD 6.1, NetBSD 10.0. */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%.2a %d", 1.51, 33, 44, 55);
    ASSERT (strcmp (result, "0x1.83p+0 33") == 0
            || strcmp (result, "0x3.05p-1 33") == 0
            || strcmp (result, "0x6.0ap-2 33") == 0
            || strcmp (result, "0xc.14p-3 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on macOS 14, FreeBSD 14.0, OpenBSD 7.5, AIX 7.3,
       Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%.0a %d", 1.51, 33, 44, 55);
    ASSERT (strcmp (result, "0x2p+0 33") == 0
            || strcmp (result, "0x3p-1 33") == 0
            || strcmp (result, "0x6p-2 33") == 0
            || strcmp (result, "0xcp-3 33") == 0);
    ASSERT (retval == strlen (result));
  }

  /* Test the support of the %f format directive.  */

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%f %d", Infinityd (), 33, 44, 55);
    ASSERT (strcmp (result, "inf 33") == 0
            || strcmp (result, "infinity 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%f %d", NaNd (), 33, 44, 55);
    ASSERT (strlen (result) >= 3 + 3
            && strisnan (result, 0, strlen (result) - 3, 0)
            && strcmp (result + strlen (result) - 3, " 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%010f %d", Infinityd (), 33, 44, 55);
    ASSERT (strcmp (result, "       inf 33") == 0
            || strcmp (result, "  infinity 33") == 0);
    ASSERT (retval == strlen (result));
  }

  /* Test the support of the %e format directive.  */

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%e %d", Infinityd (), 33, 44, 55);
    ASSERT (strcmp (result, "inf 33") == 0
            || strcmp (result, "infinity 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%e %d", NaNd (), 33, 44, 55);
    ASSERT (strlen (result) >= 3 + 3
            && strisnan (result, 0, strlen (result) - 3, 0)
            && strcmp (result + strlen (result) - 3, " 33") == 0);
    ASSERT (retval == strlen (result));
  }

  /* Test the support of the %g format directive.  */

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%g %d", Infinityd (), 33, 44, 55);
    ASSERT (strcmp (result, "inf 33") == 0
            || strcmp (result, "infinity 33") == 0);
    ASSERT (retval == strlen (result));
  }

  { /* This test would fail on AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%g %d", NaNd (), 33, 44, 55);
    ASSERT (strlen (result) >= 3 + 3
            && strisnan (result, 0, strlen (result) - 3, 0)
            && strcmp (result + strlen (result) - 3, " 33") == 0);
    ASSERT (retval == strlen (result));
  }

  /* Test the support of large precision.  */

  { /* This test would fail on AIX 7.1.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%.4000d %d", 1234567, 99);
    for (size_t i = 0; i < 4000 - 7; i++)
      ASSERT (result[i] == '0');
    ASSERT (strcmp (result + 4000 - 7, "1234567 99") == 0);
    ASSERT (retval == strlen (result));
  }

  /* Test the support of the 'b' conversion specifier for binary output of
     integers.  */

  { /* This test would fail on glibc 2.34, musl libc, macOS 14,
       FreeBSD 13.2, NetBSD 10.0, OpenBSD 7.5, AIX 7.3, Solaris 11.4.  */
    ptrdiff_t retval =
      my_snzprintf (result, sizeof (result), "%b %d", 12345, 33, 44, 55);
    ASSERT (strcmp (result, "11000000111001 33") == 0);
    ASSERT (retval == strlen (result));
  }
}
