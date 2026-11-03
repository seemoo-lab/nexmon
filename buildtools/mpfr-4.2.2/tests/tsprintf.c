/* tsprintf.c -- test file for mpfr_sprintf, mpfr_vsprintf, mpfr_snprintf,
   and mpfr_vsnprintf

Copyright 2007-2025 Free Software Foundation, Inc.
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

/* Needed due to the tests on HAVE_STDARG and MPFR_USE_MINI_GMP */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(HAVE_STDARG) && !defined(MPFR_USE_MINI_GMP)
#include <stdarg.h>

#include <float.h>
#include <errno.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#define MPFR_NEED_INTMAX_H
#include "mpfr-test.h"

const int prec_max_printf = 5000; /* limit for random precision in
                                     random_double() */
#define BUF_SIZE 65536

int randsize;

/* 1. compare expected string with the string BUFFER returned by
   mpfr_sprintf(buffer, fmt, x)
   2. then test mpfr_snprintf (buffer, p, fmt, x) with a random p. */
static void
check_sprintf (const char *expected, const char *fmt, mpfr_srcptr x)
{
  int n0, n1;
  char buffer[BUF_SIZE];

  /* test mpfr_sprintf */
  n0 = mpfr_sprintf (buffer, fmt, x);
  if (strcmp (buffer, expected) != 0)
    {
      printf ("Error in mpfr_sprintf (s, \"%s\", x);\n", fmt);
      printf ("expected: \"%s\"\ngot:      \"%s\"\n", expected, buffer);

      exit (1);
    }

  /* test mpfr_snprintf */
  randsize = (int) (randlimb () % (n0 + 3)) - 3;  /* between -3 and n0 - 1 */
  if (randsize < 0)
    {
      n1 = mpfr_snprintf (NULL, 0, fmt, x);
    }
  else
    {
      buffer[randsize] = 17;
      n1 = mpfr_snprintf (buffer, randsize, fmt, x);
      if (buffer[randsize] != 17)
        {
          printf ("Buffer overflow in mpfr_snprintf for randsize = %d!\n",
                  randsize);
          exit (1);
        }
    }
  if (n0 != n1)
    {
      printf ("Error in mpfr_snprintf (s, %d, \"%s\", x) return value\n",
              randsize, fmt);
      printf ("expected: %d\ngot:      %d\nx='", n0, n1);
      mpfr_printf (fmt, x);
      printf ("'\n");
      exit (1);
    }
  if ((randsize > 1 && strncmp (expected, buffer, randsize - 1) != 0)
      || (randsize == 1 && buffer[0] != '\0'))
    {
      char part_expected[BUF_SIZE];
      strncpy (part_expected, expected, randsize);
      part_expected[randsize - 1] = '\0';
      printf ("Error in mpfr_vsnprintf (s, %d, \"%s\", ...);\n",
              randsize, fmt);
      printf ("expected: \"%s\"\ngot:      \"%s\"\n", part_expected, buffer);
      exit (1);
    }
}

/* 1. compare expected string with the string BUFFER returned by
   mpfr_vsprintf(buffer, fmt, ...)
   2. then, test mpfr_vsnprintf. */
static int
check_vsprintf (const char *expected, const char *fmt, ...)
{
  int n0, n1;
  char buffer[BUF_SIZE];
  va_list ap0, ap1;

  va_start (ap0, fmt);
  n0 = mpfr_vsprintf (buffer, fmt, ap0);
  va_end (ap0);

  if (strcmp (buffer, expected) != 0)
    {
      printf ("Error in mpfr_vsprintf (s, \"%s\", ...);\n", fmt);
      printf ("expected: \"%s\"\ngot:      \"%s\"\n", expected, buffer);
      exit (1);
    }

  va_start (ap1, fmt);

  /* test mpfr_snprintf */
  randsize = (int) (randlimb () % (n0 + 3)) - 3;  /* between -3 and n0 - 1 */
  if (randsize < 0)
    {
      n1 = mpfr_vsnprintf (NULL, 0, fmt, ap1);
    }
  else
    {
      buffer[randsize] = 17;
      n1 = mpfr_vsnprintf (buffer, randsize, fmt, ap1);
      if (buffer[randsize] != 17)
        {
          printf ("Buffer overflow in mpfr_vsnprintf for randsize = %d!\n",
                  randsize);
          exit (1);
        }
    }

  va_end (ap1);

  if (n0 != n1)
    {
      printf ("Error in mpfr_vsnprintf (s, %d, \"%s\", ...) return value\n",
              randsize, fmt);
      printf ("expected: %d\ngot:      %d\n", n0, n1);
      exit (1);
    }
  if ((randsize > 1 && strncmp (expected, buffer, randsize - 1) != 0)
      || (randsize == 1 && buffer[0] != '\0'))
    {
      char part_expected[BUF_SIZE];

      strncpy (part_expected, expected, randsize);
      part_expected[randsize - 1] = '\0';
      printf ("Error in mpfr_vsnprintf (s, %d, \"%s\", ...);\n",
              randsize, fmt);
      printf ("expected: \"%s\"\ngot:      \"%s\"\n", part_expected, buffer);
      exit (1);
    }

  return n0;
}

static void
native_types (void)
{
  int c = 'a';
  int i = -1;
  unsigned int ui = 1;
  double d[] = { -1.25, 7.62939453125e-6 /* 2^(-17) */ };
  char s[] = "test";
  char buf[255];
  int k;

  sprintf (buf, "%c", c);
  check_vsprintf (buf, "%c", c);

  sprintf (buf, "%d", i);
  check_vsprintf (buf, "%d", i);

  check_vsprintf ("0", "%d", 0);
  check_vsprintf ("", "%.d", 0);
  check_vsprintf ("", "%.0d", 0);

  sprintf (buf, "%i", i);
  check_vsprintf (buf, "%i", i);

  check_vsprintf ("0", "%i", 0);
  check_vsprintf ("", "%.i", 0);
  check_vsprintf ("", "%.0i", 0);

  for (k = 0; k < numberof(d); k++)
    {
      sprintf (buf, "%e", d[k]);
      check_vsprintf (buf, "%e", d[k]);

      sprintf (buf, "%E", d[k]);
      check_vsprintf (buf, "%E", d[k]);

      sprintf (buf, "%f", d[k]);
      check_vsprintf (buf, "%f", d[k]);

      sprintf (buf, "%g", d[k]);
      check_vsprintf (buf, "%g", d[k]);

      sprintf (buf, "%G", d[k]);
      check_vsprintf (buf, "%G", d[k]);

#if __MPFR_STDC (199901L)

/* If NPRINTF_A is defined, this means that GMP (or MPIR) has been compiled
 * without HAVE_VSNPRINTF defined and that GMP's vsnprintf replacement
 * repl-vsnprintf.c may abort due to an assertion failure like
 *
 *   repl-vsnprintf.c:389: GNU MP assertion failed: len < total_width
 *
 * This affects gmp_snprintf, gmp_vsnprintf and gmp_vasprintf. The latter
 * is used by MPFR. So we disable the corresponding tests.
 *
 * The reason is that __gmp_replacement_vsnprintf does not support %a/%A,
 * even though the C library supports it. This has been fixed in the GMP
 * development branch on 2025-01-31.
 *
 * References:
 *   https://sympa.inria.fr/sympa/arc/mpfr/2022-10/msg00001.html
 *   https://sympa.inria.fr/sympa/arc/mpfr/2022-10/msg00027.html
 *   https://gmplib.org/list-archives/gmp-bugs/2022-October/005200.html
 *   https://gmplib.org/list-archives/gmp-bugs/2025-January/005557.html
 */

# ifndef NPRINTF_A

      gmp_sprintf (buf, "%a", d[k]);
      check_vsprintf (buf, "%a", d[k]);

      gmp_sprintf (buf, "%A", d[k]);
      check_vsprintf (buf, "%A", d[k]);

      gmp_sprintf (buf, "%la", d[k]);
      check_vsprintf (buf, "%la", d[k]);

      gmp_sprintf (buf, "%lA", d[k]);
      check_vsprintf (buf, "%lA", d[k]);

# endif

      sprintf (buf, "%le", d[k]);
      check_vsprintf (buf, "%le", d[k]);

      sprintf (buf, "%lE", d[k]);
      check_vsprintf (buf, "%lE", d[k]);

      sprintf (buf, "%lf", d[k]);
      check_vsprintf (buf, "%lf", d[k]);

      sprintf (buf, "%lg", d[k]);
      check_vsprintf (buf, "%lg", d[k]);

      sprintf (buf, "%lG", d[k]);
      check_vsprintf (buf, "%lG", d[k]);

#endif
    }

  sprintf (buf, "%o", i);
  check_vsprintf (buf, "%o", i);

  sprintf (buf, "%s", s);
  check_vsprintf (buf, "%s", s);

  sprintf (buf, "--%s++", "");
  check_vsprintf (buf, "--%s++", "");

  sprintf (buf, "%u", ui);
  check_vsprintf (buf, "%u", ui);

  sprintf (buf, "%x", ui);
  check_vsprintf (buf, "%x", ui);
}

static void
special (void)
{
  mpfr_t x;
  const char *ns[] = { "nan", "NAN", "inf", "INF" };
  const char *ps[] = { "", " ", "+", "+ ", " +" };
  const char *rs[] = { "", "U", "D", "Y", "Z", "N" };
  const char *fs[] = { "efgab", "EFGA" };
  int ntests = 0, neg, psflag, r, f;
  unsigned int i;

  mpfr_init2 (x, 128);  /* initialized to NaN */

  for (i = 0; i < numberof (ns); i++)
    {
      if (i == 2)
        MPFR_SET_INF (x);

      for (neg = 0; neg < 2; neg++)
        {
          MPFR_SET_SIGN (x, neg ? MPFR_SIGN_NEG : MPFR_SIGN_POS);
          for (psflag = 0; psflag < 3; psflag++)
            for (r = 0; r < numberof (rs); r++)
              for (f = 0; fs[i & 1][f] != '\0'; f++)
                {
                  int fmtpsflags = psflag;
                  int zeroflag, width;
                  char ws[4], fmt[20], expected[20], sign, *p;
                  int sp;

                  /* In case of '+' flag, let's randomly test an additional
                     space flag, which should be ignored. */
                  if (psflag == 2)
                    fmtpsflags += randlimb () % 3;

                  /* '0' flag ignored for NaN and Inf; use it randomly. */
                  zeroflag = RAND_BOOL ();

                  width = randlimb () % 12;
                  if (width != 0)
                    sprintf (ws, "%d", width);
                  else
                    ws[0] = '\0';

                  /* The following is a common prefix to better identify
                     error messages produced in this function. */
                  strcpy (expected, "special ");

                  sprintf (fmt, "%s%%%s%s%sR%s%c", expected, ps[fmtpsflags],
                           zeroflag ? "0" : "", ws, rs[r], fs[i & 1][f]);
                  /* printf ("Format string: \"%s\"\n", fmt); */

                  p = expected + 8;
                  sign = neg ? '-' : ps[psflag][0];
                  for (sp = width - (sign != '\0') - 3; sp > 0; sp--)
                    *p++ = ' ';
                  if (sign != '\0')
                    *p++ = sign;
                  strcpy (p, ns[i]);

                  check_sprintf (expected, fmt, x);
                  ntests++;
                }
        }
    }

  /* 2 base values (NaN and Inf), 2 signs (positive and negative),
     3 possible '+'/space flag cases, 6 cases for the rounding mode,
     9 format specifiers (efgab / EFGA). */
  MPFR_ASSERTN (ntests == 2 * 2 * 3 * 6 * 9);

  mpfr_clear (x);
}

static void
decimal (void)
{
  mpfr_prec_t p = 128;
  mpfr_t x, y, z;

  /* specifier 'P' for precision */
  check_vsprintf ("128", "%Pu", p);
  check_vsprintf ("00128", "%.5Pu", p);
  check_vsprintf ("  128", "%5Pu", p);
  check_vsprintf ("000128", "%06Pu", p);
  check_vsprintf ("128    :", "%-7Pu:", p);
  check_vsprintf ("000128:", "%-2.6Pd:", p);
  check_vsprintf ("  000128:", "%8.6Pd:", p);
  check_vsprintf ("000128  :", "%-8.6Pd:", p);
  check_vsprintf ("+128:", "%+Pd:", p);
  check_vsprintf (" 128:", "% Pd:", p);
  check_vsprintf ("80:", "% Px:", p);
  check_vsprintf ("0x80:", "% #Px:", p);
  check_vsprintf ("0x80:", "%0#+ -Px:", p);
  check_vsprintf ("0200:", "%0#+ -Po:", p);
  check_vsprintf ("+0000128 :", "%0+ *.*Pd:", -9, 7, p);
  check_vsprintf ("+12345   :", "%0+ -*.*Pd:", -9, -3, (mpfr_prec_t) 12345);
  check_vsprintf ("0", "%Pu", (mpfr_prec_t) 0);
  /* Do not add a test like "%05.1Pd" as MS Windows is buggy: when
     a precision is given, the '0' flag must be ignored. */

  /* specifier 'P' with precision field 0 */
  check_vsprintf ("128", "%.Pu", p);
  check_vsprintf ("128", "%.0Pd", p);
  check_vsprintf ("", "%.Pu", (mpfr_prec_t) 0);
  check_vsprintf ("", "%.0Pd", (mpfr_prec_t) 0);

  mpfr_init (z);
  mpfr_init2 (x, 128);

  /* special numbers: tested in special() */

  /* positive numbers */
  mpfr_set_str (x, "18993474.61279296875", 10, MPFR_RNDN);
  mpfr_init2 (y, 59);
  mpfr_set (y, x, MPFR_RNDN);
  mpfr_set_ui (z, 0, MPFR_RNDD);

  /* simplest case right justified */
  check_sprintf ("1.899347461279296875000000000000000000000e+07", "%30Re", x);
  check_sprintf ("      1.899347461279296875e+07", "%30Re", y);
  check_sprintf ("                         2e+07", "%30.0Re", x);
  check_sprintf ("               18993474.612793", "%30Rf", x);
  check_sprintf ("              18993474.6127930", "%30.7Rf", x);
  check_sprintf ("                   1.89935e+07", "%30Rg", x);
  check_sprintf ("                         2e+07", "%30.0Rg", x);
  check_sprintf ("          18993474.61279296875", "%30.19Rg", x);
  check_sprintf ("        0.0000000000000000e+00", "%30Re", z);
  check_sprintf ("                      0.000000", "%30Rf", z);
  check_sprintf ("                             0", "%30Rg", z);
  check_sprintf ("                       0.00000", "%#30Rg", z);
  check_sprintf ("                         0e+00", "%30.0Re", z);
  check_sprintf ("                             0", "%30.0Rf", z);
  check_sprintf ("                        0.0000", "%30.4Rf", z);
  check_sprintf ("                             0", "%30.0Rg", z);
  check_sprintf ("                             0", "%30.4Rg", z);
  /* sign or space, pad with leading zeros */
  check_sprintf (" 1.899347461279296875000000000000000000000E+07", "% 030RE", x);
  check_sprintf (" 000001.899347461279296875E+07", "% 030RE", y);
  check_sprintf (" 0000000000000000001.89935E+07", "% 030RG", x);
  check_sprintf (" 0000000000000000000000002E+07", "% 030.0RE", x);
  check_sprintf (" 0000000000000000000000000E+00", "% 030.0RE", z);
  check_sprintf (" 00000000000000000000000000000", "% 030.0RF", z);
  /* sign + or -, left justified */
  check_sprintf ("+1.899347461279296875000000000000000000000e+07", "%+-30Re", x);
  check_sprintf ("+1.899347461279296875e+07     ", "%+-30Re", y);
  check_sprintf ("+2e+07                        ", "%+-30.0Re", x);
  check_sprintf ("+0e+00                        ", "%+-30.0Re", z);
  check_sprintf ("+0                            ", "%+-30.0Rf", z);
  /* decimal point, left justified, precision and rounding parameter */
  check_vsprintf ("1.9E+07   ", "%#-10.*R*E", 1, MPFR_RNDN, x);
  check_vsprintf ("2.E+07    ", "%#*.*R*E", -10, 0, MPFR_RNDN, x);
  check_vsprintf ("2.E+07    ", "%#-10.*R*G", 0, MPFR_RNDN, x);
  check_vsprintf ("0.E+00    ", "%#-10.*R*E", 0, MPFR_RNDN, z);
  check_vsprintf ("0.        ", "%#-10.*R*F", 0, MPFR_RNDN, z);
  check_vsprintf ("0.        ", "%#-10.*R*G", 0, MPFR_RNDN, z);
  /* sign or space */
  check_sprintf (" 1.899e+07", "% .3RNe", x);
  check_sprintf (" 2e+07",     "% .0RNe", x);
  /* sign + or -, decimal point, pad with leading zeros */
  check_sprintf ("+0001.8E+07", "%0+#11.1RZE", x);
  check_sprintf ("+00001.E+07", "%0+#11.0RZE", x);
  check_sprintf ("+0000.0E+00", "%0+#11.1RZE", z);
  check_sprintf ("+00000000.0", "%0+#11.1RZF", z);
  /* pad with leading zero */
  check_sprintf ("1.899347461279296875000000000000000000000e+07", "%030RDe", x);
  check_sprintf ("0000001.899347461279296875e+07", "%030RDe", y);
  check_sprintf ("00000000000000000000000001e+07", "%030.0RDe", x);
  /* sign or space, decimal point, left justified */
  check_sprintf (" 1.8E+07   ", "%- #11.1RDE", x);
  check_sprintf (" 1.E+07    ", "%- #11.0RDE", x);
  /* large requested precision */
  check_sprintf ("18993474.61279296875", "%.2147483647Rg", x);

  /* negative numbers */
  mpfr_mul_si (x, x, -1, MPFR_RNDD);
  mpfr_mul_si (z, z, -1, MPFR_RNDD);

  /* sign + or - */
  check_sprintf ("  -1.8e+07", "%+10.1RUe", x);
  check_sprintf ("    -1e+07", "%+10.0RUe", x);
  check_sprintf ("    -0e+00", "%+10.0RUe", z);
  check_sprintf ("        -0", "%+10.0RUf", z);

  /* neighborhood of 1 */
  mpfr_set_str (x, "0.99993896484375", 10, MPFR_RNDN);
  mpfr_set_prec (y, 43);
  mpfr_set (y, x, MPFR_RNDN);
  check_sprintf ("9.999389648437500000000000000000000000000E-01", "%-20RE", x);
  check_sprintf ("9.9993896484375E-01 ", "%-20RE", y);
  check_sprintf ("1E+00               ", "%-20.RE", x);
  check_sprintf ("1E+00               ", "%-20.RE", y);
  check_sprintf ("1E+00               ", "%-20.0RE", x);
  check_sprintf ("1.0E+00             ", "%-20.1RE", x);
  check_sprintf ("1.00E+00            ", "%-20.2RE", x);
  check_sprintf ("9.999E-01           ", "%-20.3RE", x);
  check_sprintf ("9.9994E-01          ", "%-20.4RE", x);
  check_sprintf ("0.999939            ", "%-20RF", x);
  check_sprintf ("1                   ", "%-20.RF", x);
  check_sprintf ("1                   ", "%-20.0RF", x);
  check_sprintf ("1.0                 ", "%-20.1RF", x);
  check_sprintf ("1.00                ", "%-20.2RF", x);
  check_sprintf ("1.000               ", "%-20.3RF", x);
  check_sprintf ("0.9999              ", "%-20.4RF", x);
  check_sprintf ("0.999939            ", "%-#20RF", x);
  check_sprintf ("1.                  ", "%-#20.RF", x);
  check_sprintf ("1.                  ", "%-#20.0RF", x);
  check_sprintf ("1.0                 ", "%-#20.1RF", x);
  check_sprintf ("1.00                ", "%-#20.2RF", x);
  check_sprintf ("1.000               ", "%-#20.3RF", x);
  check_sprintf ("0.9999              ", "%-#20.4RF", x);
  check_sprintf ("0.999939            ", "%-20RG", x);
  check_sprintf ("1                   ", "%-20.RG", x);
  check_sprintf ("1                   ", "%-20.0RG", x);
  check_sprintf ("1                   ", "%-20.1RG", x);
  check_sprintf ("1                   ", "%-20.2RG", x);
  check_sprintf ("1                   ", "%-20.3RG", x);
  check_sprintf ("0.9999              ", "%-20.4RG", x);
  check_sprintf ("0.999939            ", "%-#20RG", x);
  check_sprintf ("1.                  ", "%-#20.RG", x);
  check_sprintf ("1.                  ", "%-#20.0RG", x);
  check_sprintf ("1.                  ", "%-#20.1RG", x);
  check_sprintf ("1.0                 ", "%-#20.2RG", x);
  check_sprintf ("1.00                ", "%-#20.3RG", x);
  check_sprintf ("0.9999              ", "%-#20.4RG", x);

  /* powers of 10 */
  mpfr_set_str (x, "1e17", 10, MPFR_RNDN);
  check_sprintf ("1.000000000000000000000000000000000000000e+17", "%Re", x);
  check_sprintf ("1.000e+17", "%.3Re", x);
  check_sprintf ("100000000000000000", "%.Rf", x);
  check_sprintf ("100000000000000000", "%.0Rf", x);
  check_sprintf ("100000000000000000.0", "%.1Rf", x);
  check_sprintf ("100000000000000000.000000", "%'Rf", x);
  check_sprintf ("100000000000000000.0", "%'.1Rf", x);

  mpfr_ui_div (x, 1, x, MPFR_RNDN); /* x=1e-17 */
  check_sprintf ("1.000000000000000000000000000000000000000e-17", "%Re", x);
  check_sprintf ("0.000000", "%Rf", x);
  check_sprintf ("1e-17", "%Rg", x);
  check_sprintf ("0.0", "%.1RDf", x);
  check_sprintf ("0.0", "%.1RZf", x);
  check_sprintf ("0.1", "%.1RUf", x);
  check_sprintf ("0.1", "%.1RYf", x);
  check_sprintf ("0", "%.0RDf", x);
  check_sprintf ("0", "%.0RZf", x);
  check_sprintf ("1", "%.0RUf", x);
  check_sprintf ("1", "%.0RYf", x);

  /* powers of 10 with 'g' style */
  mpfr_set_str (x, "10", 10, MPFR_RNDN);
  check_sprintf ("10", "%Rg", x);
  check_sprintf ("1e+01", "%.0Rg", x);
  check_sprintf ("1e+01", "%.1Rg", x);
  check_sprintf ("10", "%.2Rg", x);

  mpfr_ui_div (x, 1, x, MPFR_RNDN);
  check_sprintf ("0.1", "%Rg", x);
  check_sprintf ("0.1", "%.0Rg", x);
  check_sprintf ("0.1", "%.1Rg", x);

  mpfr_set_str (x, "1000", 10, MPFR_RNDN);
  check_sprintf ("1000", "%Rg", x);
  check_sprintf ("1e+03", "%.0Rg", x);
  check_sprintf ("1e+03", "%.3Rg", x);
  check_sprintf ("1000", "%.4Rg", x);
  check_sprintf ("1e+03", "%.3Rg", x);
  check_sprintf ("1000", "%.4Rg", x);
  check_sprintf ("    1e+03", "%9.3Rg", x);
  check_sprintf ("     1000", "%9.4Rg", x);
  check_sprintf ("00001e+03", "%09.3Rg", x);
  check_sprintf ("000001000", "%09.4Rg", x);

  mpfr_ui_div (x, 1, x, MPFR_RNDN);
  check_sprintf ("0.001", "%Rg", x);
  check_sprintf ("0.001", "%.0Rg", x);
  check_sprintf ("0.001", "%.1Rg", x);

  mpfr_set_str (x, "100000", 10, MPFR_RNDN);
  check_sprintf ("100000", "%Rg", x);
  check_sprintf ("1e+05", "%.0Rg", x);
  check_sprintf ("1e+05", "%.5Rg", x);
  check_sprintf ("100000", "%.6Rg", x);
  check_sprintf ("            1e+05", "%17.5Rg", x);
  check_sprintf ("           100000", "%17.6Rg", x);
  check_sprintf ("0000000000001e+05", "%017.5Rg", x);
  check_sprintf ("00000000000100000", "%017.6Rg", x);

  mpfr_ui_div (x, 1, x, MPFR_RNDN);
  check_sprintf ("1e-05", "%Rg", x);
  check_sprintf ("1e-05", "%.0Rg", x);
  check_sprintf ("1e-05", "%.1Rg", x);

  /* check rounding mode */
  mpfr_set_str (x, "0.0076", 10, MPFR_RNDN);
  check_sprintf ("0.007", "%.3RDF", x);
  check_sprintf ("0.007", "%.3RZF", x);
  check_sprintf ("0.008", "%.3RF", x);
  check_sprintf ("0.008", "%.3RUF", x);
  check_sprintf ("0.008", "%.3RYF", x);
  check_vsprintf ("0.008", "%.3R*F", MPFR_RNDA, x);

  /* check limit between %f-style and %g-style */
  mpfr_set_str (x, "0.0000999", 10, MPFR_RNDN);
  check_sprintf ("0.0001",   "%.0Rg", x);
  check_sprintf ("9e-05",    "%.0RDg", x);
  check_sprintf ("0.0001",   "%.1Rg", x);
  check_sprintf ("0.0001",   "%.2Rg", x);
  check_sprintf ("9.99e-05", "%.3Rg", x);

  /* trailing zeros */
  mpfr_set_si_2exp (x, -1, -15, MPFR_RNDN); /* x=-2^-15 */
  check_sprintf ("-3.0517578125e-05", "%.30Rg", x);
  check_sprintf ("-3.051757812500000000000000000000e-05", "%.30Re", x);
  check_sprintf ("-3.05175781250000000000000000000e-05", "%#.30Rg", x);
  check_sprintf ("-0.000030517578125000000000000000", "%.30Rf", x);

  /* bug 20081023 */
  check_sprintf ("-3.0517578125e-05", "%.30Rg", x);
  mpfr_set_str (x, "1.9999", 10, MPFR_RNDN);
  check_sprintf ("1.999900  ", "%-#10.7RG", x);
  check_sprintf ("1.9999    ", "%-10.7RG", x);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  check_sprintf ("1.", "%#.1Rg", x);
  check_sprintf ("1.   ", "%-#5.1Rg", x);
  check_sprintf ("  1.0", "%#5.2Rg", x);
  check_sprintf ("1.00000000000000000000000000000", "%#.30Rg", x);
  check_sprintf ("1", "%.30Rg", x);
  mpfr_set_ui (x, 0, MPFR_RNDN);
  check_sprintf ("0.", "%#.1Rg", x);
  check_sprintf ("0.   ", "%-#5.1Rg", x);
  check_sprintf ("  0.0", "%#5.2Rg", x);
  check_sprintf ("0.00000000000000000000000000000", "%#.30Rg", x);
  check_sprintf ("0", "%.30Rg", x);

  /* following tests with precision 53 bits */
  mpfr_set_prec (x, 53);

  /* Exponent zero has a plus sign */
  mpfr_set_str (x, "-9.95645044213728791504536275169812142849e-01", 10,
                MPFR_RNDN);
  check_sprintf ("-1.0e+00", "%- #0.1Re", x);

  /* Decimal point and no figure after it with '#' flag and 'G' style */
  mpfr_set_str (x, "-9.90597761233942053494e-01", 10, MPFR_RNDN);
  check_sprintf ("-1.", "%- #0.1RG", x);

  /* precision zero */
  mpfr_set_d (x, 9.5, MPFR_RNDN);
  check_sprintf ("9",    "%.0RDf", x);
  check_sprintf ("10",    "%.0RUf", x);

  mpfr_set_d (x, 19.5, MPFR_RNDN);
  check_sprintf ("19",    "%.0RDf", x);
  check_sprintf ("20",    "%.0RUf", x);

  mpfr_set_d (x, 99.5, MPFR_RNDN);
  check_sprintf ("99",    "%.0RDf", x);
  check_sprintf ("100",   "%.0RUf", x);

  mpfr_set_d (x, -9.5, MPFR_RNDN);
  check_sprintf ("-10",    "%.0RDf", x);
  check_sprintf ("-10",    "%.0RYf", x);
  check_sprintf ("-10",    "%.0Rf", x);
  check_sprintf ("-1e+01", "%.0Re", x);
  check_sprintf ("-1e+01", "%.0Rg", x);
  mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN);
  check_sprintf ("0",      "%.0Rf", x);
  check_sprintf ("5e-01",  "%.0Re", x);
  check_sprintf ("0.5",    "%.0Rg", x);
  mpfr_set_ui_2exp (x, 3, -1, MPFR_RNDN);
  check_sprintf ("2",      "%.0Rf", x);
  mpfr_set_ui_2exp (x, 5, -1, MPFR_RNDN);
  check_sprintf ("2",      "%.0Rf", x);
  mpfr_set_ui (x, 0x1f, MPFR_RNDN);
  check_sprintf ("0x1p+5", "%.0Ra", x);
  mpfr_set_ui (x, 3, MPFR_RNDN);
  check_sprintf ("1p+2",   "%.0Rb", x);

  /* round to next ten power with %f but not with %g */
  mpfr_set_str (x, "-6.64464380544039223686e-02", 10, MPFR_RNDN);
  check_sprintf ("-0.1",  "%.1Rf", x);
  check_sprintf ("-0.0",  "%.1RZf", x);
  check_sprintf ("-0.07", "%.1Rg", x);
  check_sprintf ("-0.06", "%.1RZg", x);

  /* round to next ten power and do not remove trailing zeros */
  mpfr_set_str (x, "9.98429393291486722006e-02", 10, MPFR_RNDN);
  check_sprintf ("0.1",   "%#.1Rg", x);
  check_sprintf ("0.10",  "%#.2Rg", x);
  check_sprintf ("0.099", "%#.2RZg", x);

  /* Halfway cases */
  mpfr_set_str (x, "1.5", 10, MPFR_RNDN);
  check_sprintf ("2e+00", "%.0Re", x);
  mpfr_set_str (x, "2.5", 10, MPFR_RNDN);
  check_sprintf ("2e+00", "%.0Re", x);
  mpfr_set_str (x, "9.5", 10, MPFR_RNDN);
  check_sprintf ("1e+01", "%.0Re", x);
  mpfr_set_str (x, "1.25", 10, MPFR_RNDN);
  check_sprintf ("1.2e+00", "%.1Re", x);
  mpfr_set_str (x, "1.75", 10, MPFR_RNDN);
  check_sprintf ("1.8e+00", "%.1Re", x);
  mpfr_set_str (x, "-0.5", 10, MPFR_RNDN);
  check_sprintf ("-0", "%.0Rf", x);
  mpfr_set_str (x, "1.25", 10, MPFR_RNDN);
  check_sprintf ("1.2", "%.1Rf", x);
  mpfr_set_str (x, "1.75", 10, MPFR_RNDN);
  check_sprintf ("1.8", "%.1Rf", x);
  mpfr_set_str (x, "1.5", 10, MPFR_RNDN);
  check_sprintf ("2", "%.1Rg", x);
  mpfr_set_str (x, "2.5", 10, MPFR_RNDN);
  check_sprintf ("2", "%.1Rg", x);
  mpfr_set_str (x, "9.25", 10, MPFR_RNDN);
  check_sprintf ("9.2", "%.2Rg", x);
  mpfr_set_str (x, "9.75", 10, MPFR_RNDN);
  check_sprintf ("9.8", "%.2Rg", x);

  /* assertion failure in r6320 */
  mpfr_set_str (x, "-9.996", 10, MPFR_RNDN);
  check_sprintf ("-10.0", "%.1Rf", x);

  /* regression in MPFR 3.1.0 (bug introduced in r7761, fixed in r7931) */
  check_sprintf ("-10", "%.2Rg", x);

  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

static void
hexadecimal (void)
{
  mpfr_t x, z;

  mpfr_inits2 (64, x, z, (mpfr_ptr) 0);

  /* special numbers: tested in special() */

  /* regular numbers */
  mpfr_set_str (x, "FEDCBA9.87654321", 16, MPFR_RNDN);
  mpfr_set_ui (z, 0, MPFR_RNDZ);

  /* simplest case right justified */
  check_sprintf ("   0xf.edcba987654321p+24", "%25Ra", x);
  check_sprintf ("   0xf.edcba987654321p+24", "%25RUa", x);
  check_sprintf ("   0xf.edcba987654321p+24", "%25RDa", x);
  check_sprintf ("   0xf.edcba987654321p+24", "%25RYa", x);
  check_sprintf ("   0xf.edcba987654321p+24", "%25RZa", x);
  check_sprintf ("   0xf.edcba987654321p+24", "%25RNa", x);
  check_sprintf ("                  0x1p+28", "%25.0Ra", x);
  check_sprintf ("                   0x0p+0", "%25.0Ra", z);
  check_sprintf ("                   0x0p+0", "%25Ra", z);
  check_sprintf ("                  0x0.p+0", "%#25Ra", z);
  /* sign or space, pad with leading zeros */
  check_sprintf (" 0X00F.EDCBA987654321P+24", "% 025RA", x);
  check_sprintf (" 0X000000000000000001P+28", "% 025.0RA", x);
  check_sprintf (" 0X0000000000000000000P+0", "% 025.0RA", z);
  /* sign + or -, left justified */
  check_sprintf ("+0xf.edcba987654321p+24  ", "%+-25Ra", x);
  check_sprintf ("+0x1p+28                 ", "%+-25.0Ra", x);
  check_sprintf ("+0x0p+0                  ", "%+-25.0Ra", z);
  /* decimal point, left justified, precision and rounding parameter */
  check_vsprintf ("0XF.FP+24 ", "%#-10.*R*A", 1, MPFR_RNDN, x);
  check_vsprintf ("0X1.P+28  ", "%#-10.*R*A", 0, MPFR_RNDN, x);
  check_vsprintf ("0X0.P+0   ", "%#-10.*R*A", 0, MPFR_RNDN, z);
  /* sign or space */
  check_sprintf (" 0xf.eddp+24", "% .3RNa", x);
  check_sprintf (" 0x1p+28",     "% .0RNa", x);
  /* sign + or -, decimal point, pad with leading zeros */
  check_sprintf ("+0X0F.EP+24", "%0+#11.1RZA", x);
  check_sprintf ("+0X00F.P+24", "%0+#11.0RZA", x);
  check_sprintf ("+0X000.0P+0", "%0+#11.1RZA", z);
  /* pad with leading zero */
  check_sprintf ("0x0000f.edcba987654321p+24", "%026RDa", x);
  check_sprintf ("0x0000000000000000000fp+24", "%026.0RDa", x);
  /* sign or space, decimal point, left justified */
  check_sprintf (" 0XF.EP+24 ", "%- #11.1RDA", x);
  check_sprintf (" 0XF.P+24  ", "%- #11.0RDA", x);

  mpfr_mul_si (x, x, -1, MPFR_RNDD);
  mpfr_mul_si (z, z, -1, MPFR_RNDD);

  /* sign + or - */
  check_sprintf ("-0xf.ep+24", "%+10.1RUa", x);
  check_sprintf ("  -0xfp+24", "%+10.0RUa", x);
  check_sprintf ("   -0x0p+0", "%+10.0RUa", z);

  /* rounding bit is zero */
  mpfr_set_str (x, "0xF.7", 16, MPFR_RNDN);
  check_sprintf ("0XFP+0", "%.0RNA", x);
  /* tie case in round to nearest mode */
  mpfr_set_str (x, "0x0.8800000000000000p+3", 16, MPFR_RNDN);
  check_sprintf ("0x9.p-1", "%#.0RNa", x);
  mpfr_set_str (x, "-0x0.9800000000000000p+3", 16, MPFR_RNDN);
  check_sprintf ("-0xap-1", "%.0RNa", x);
  /* trailing zeros in fractional part */
  check_sprintf ("-0X4.C0000000000000000000P+0", "%.20RNA", x);
  /* rounding bit is one and the first non zero bit is far away */
  mpfr_set_prec (x, 1024);
  mpfr_set_ui_2exp (x, 29, -1, MPFR_RNDN);
  mpfr_nextabove (x);
  check_sprintf ("0XFP+0", "%.0RNA", x);

  /* with more than one limb */
  mpfr_set_prec (x, 300);
  mpfr_set_str (x, "0xf.ffffffffffffffffffffffffffffffffffffffffffffffffffff"
                "fffffffffffffffff", 16, MPFR_RNDN);
  check_sprintf ("0x1p+4 [300]", "%.0RNa [300]", x);
  check_sprintf ("0xfp+0 [300]", "%.0RZa [300]", x);
  check_sprintf ("0x1p+4 [300]", "%.0RYa [300]", x);
  check_sprintf ("0xfp+0 [300]", "%.0RDa [300]", x);
  check_sprintf ("0x1p+4 [300]", "%.0RUa [300]", x);
  check_sprintf ("0x1.0000000000000000000000000000000000000000p+4",
                 "%.40RNa", x);
  check_sprintf ("0xf.ffffffffffffffffffffffffffffffffffffffffp+0",
                 "%.40RZa", x);
  check_sprintf ("0x1.0000000000000000000000000000000000000000p+4",
                 "%.40RYa", x);
  check_sprintf ("0xf.ffffffffffffffffffffffffffffffffffffffffp+0",
                 "%.40RDa", x);
  check_sprintf ("0x1.0000000000000000000000000000000000000000p+4",
                 "%.40RUa", x);

  mpfr_set_str (x, "0xf.7fffffffffffffffffffffffffffffffffffffffffffffffffff"
                "ffffffffffffffffff", 16, MPFR_RNDN);
  check_sprintf ("0XFP+0", "%.0RNA", x);
  check_sprintf ("0XFP+0", "%.0RZA", x);
  check_sprintf ("0X1P+4", "%.0RYA", x);
  check_sprintf ("0XFP+0", "%.0RDA", x);
  check_sprintf ("0X1P+4", "%.0RUA", x);
  check_sprintf ("0XF.8P+0", "%.1RNA", x);
  check_sprintf ("0XF.7P+0", "%.1RZA", x);
  check_sprintf ("0XF.8P+0", "%.1RYA", x);
  check_sprintf ("0XF.7P+0", "%.1RDA", x);
  check_sprintf ("0XF.8P+0", "%.1RUA", x);

  /* do not round up to the next power of the base */
  mpfr_set_str (x, "0xf.fffffffffffffffffffffffffffffffffffffeffffffffffffff"
                "ffffffffffffffffff", 16, MPFR_RNDN);
  check_sprintf ("0xf.ffffffffffffffffffffffffffffffffffffff00p+0",
                 "%.40RNa", x);
  check_sprintf ("0xf.fffffffffffffffffffffffffffffffffffffeffp+0",
                 "%.40RZa", x);
  check_sprintf ("0xf.ffffffffffffffffffffffffffffffffffffff00p+0",
                 "%.40RYa", x);
  check_sprintf ("0xf.fffffffffffffffffffffffffffffffffffffeffp+0",
                 "%.40RDa", x);
  check_sprintf ("0xf.ffffffffffffffffffffffffffffffffffffff00p+0",
                 "%.40RUa", x);

  mpfr_clears (x, z, (mpfr_ptr) 0);
}

static void
binary (void)
{
  mpfr_t x;
  mpfr_t z;

  mpfr_inits2 (64, x, z, (mpfr_ptr) 0);

  /* special numbers: tested in special() */

  /* regular numbers */
  mpfr_set_str (x, "1110010101.1001101", 2, MPFR_RNDN);
  mpfr_set_ui (z, 0, MPFR_RNDN);

  /* simplest case: right justified */
  check_sprintf ("    1.1100101011001101p+9", "%25Rb", x);
  check_sprintf ("                     0p+0", "%25Rb", z);
  check_sprintf ("                    0.p+0", "%#25Rb", z);
  /* sign or space, pad with leading zeros */
  check_sprintf (" 0001.1100101011001101p+9", "% 025Rb", x);
  check_sprintf (" 000000000000000000000p+0", "% 025Rb", z);
  /* sign + or -, left justified */
  check_sprintf ("+1.1100101011001101p+9   ", "%+-25Rb", x);
  check_sprintf ("+0p+0                    ", "%+-25Rb", z);
  /* sign or space */
  check_sprintf (" 1.110p+9",  "% .3RNb", x);
  check_sprintf (" 1.1101p+9", "% .4RNb", x);
  check_sprintf (" 0.0000p+0", "% .4RNb", z);
  /* sign + or -, decimal point, pad with leading zeros */
  check_sprintf ("+00001.1p+9", "%0+#11.1RZb", x);
  check_sprintf ("+0001.0p+10", "%0+#11.1RNb", x);
  check_sprintf ("+000000.p+0", "%0+#11.0RNb", z);
  /* pad with leading zero */
  check_sprintf ("00001.1100101011001101p+9", "%025RDb", x);
  /* sign or space, decimal point (unused), left justified */
  check_sprintf (" 1.1p+9    ", "%- #11.1RDb", x);
  check_sprintf (" 1.p+9     ", "%- #11.0RDb", x);
  check_sprintf (" 1.p+10    ", "%- #11.0RUb", x);
  check_sprintf (" 1.p+9     ", "%- #11.0RZb", x);
  check_sprintf (" 1.p+10    ", "%- #11.0RYb", x);
  check_sprintf (" 1.p+10    ", "%- #11.0RNb", x);

  mpfr_mul_si (x, x, -1, MPFR_RNDD);
  mpfr_mul_si (z, z, -1, MPFR_RNDD);

  /* sign + or - */
  check_sprintf ("   -1.1p+9", "%+10.1RUb", x);
  check_sprintf ("   -0.0p+0", "%+10.1RUb", z);

  /* precision 0 */
  check_sprintf ("-1p+10", "%.0RNb", x);
  check_sprintf ("-1p+10", "%.0RDb", x);
  check_sprintf ("-1p+9",  "%.0RUb", x);
  check_sprintf ("-1p+9",  "%.0RZb", x);
  check_sprintf ("-1p+10", "%.0RYb", x);
  /* round to next base power */
  check_sprintf ("-1.0p+10", "%.1RNb", x);
  check_sprintf ("-1.0p+10", "%.1RDb", x);
  check_sprintf ("-1.0p+10", "%.1RYb", x);
  /* do not round to next base power */
  check_sprintf ("-1.1p+9", "%.1RUb", x);
  check_sprintf ("-1.1p+9", "%.1RZb", x);
  /* rounding bit is zero */
  check_sprintf ("-1.11p+9", "%.2RNb", x);
  /* tie case in round to nearest mode */
  check_sprintf ("-1.1100101011001101p+9", "%.16RNb", x);
  /* trailing zeros in fractional part */
  check_sprintf ("-1.110010101100110100000000000000p+9", "%.30RNb", x);

  mpfr_clears (x, z, (mpfr_ptr) 0);
}

static void
mixed (void)
{
  int n1;
  int n2;
  int i = 121;
#ifdef PRINTF_L
  long double d = 1. / 31.;
#endif
  mpf_t mpf;
  mpq_t mpq;
  mpz_t mpz;
  mpfr_t x;
  mpfr_rnd_t rnd;
  int k;

  mpf_init (mpf);
  mpf_set_ui (mpf, 40);
  mpf_div_ui (mpf, mpf, 31); /* mpf = 40.0 / 31.0 */
  mpq_init (mpq);
  mpq_set_ui (mpq, 123456, 4567890);
  mpz_init (mpz);
  mpz_fib_ui (mpz, 64);
  mpfr_init (x);
  mpfr_set_str (x, "-12345678.875", 10, MPFR_RNDN);
  rnd = MPFR_RNDD;

  check_vsprintf ("121%", "%i%%", i);
  check_vsprintf ("121% -1.2345678875000000E+07", "%i%% %RNE", i, x);
  check_vsprintf ("121, -12345679", "%i, %.0Rf", i, x);
  check_vsprintf ("10610209857723, -1.2345678875000000e+07", "%Zi, %R*e", mpz, rnd,
                  x);
  check_vsprintf ("-12345678.9, 121", "%.1Rf, %i", x, i);
  check_vsprintf ("-12345678, 1e240/45b352", "%.0R*f, %Qx", MPFR_RNDZ, x, mpq);

  /* TODO: Systematically test with and without %n in check_vsprintf? */
  /* Do the test several times due to random parameters in check_vsprintf
     and the use of %n. In r11501, n2 is incorrect (seems random) when
     randsize <= 0, i.e. when the size argument of mpfr_vsnprintf is 0. */
  for (k = 0; k < 30; k++)
    {
      n2 = -17;
      /* If this value is obtained for n2 after the check_vsprintf call below,
         this probably means that n2 has not been written as expected. */
      n1 = check_vsprintf ("121, -12345678.875000000000, 1.290323",
                           "%i, %.*Rf, %Ff%n", i, 12, x, mpf, &n2);
      if (n1 != n2)
        {
          printf ("error in number of characters written by mpfr_vsprintf"
                  " for k = %d, randsize = %d\n", k, randsize);
          printf ("expected: %d\n", n2);
          printf ("     got: %d\n", n1);
          exit (1);
        }
    }

#ifdef PRINTF_L
  /* under MinGW, -D__USE_MINGW_ANSI_STDIO is required to support %Lf
     see https://gcc.gnu.org/legacy-ml/gcc/2013-03/msg00103.html */
  check_vsprintf ("00000010610209857723, -1.2345678875000000e+07, 0.032258",
                  "%.*Zi, %R*e, %Lf", 20, mpz, rnd, x, d);
#endif

  /* check invalid spec.spec */
  check_vsprintf ("%,", "%,");
  check_vsprintf ("%3*Rg", "%3*Rg");

  /* check empty format */
  check_vsprintf ("%", "%");

  mpf_clear (mpf);
  mpq_clear (mpq);
  mpz_clear (mpz);
  mpfr_clear (x);
}

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE) && MPFR_LCONV_DPTS

/* Check with locale "da_DK.utf8" or "da_DK".
   On most platforms, decimal point is ',' and thousands separator is '.';
   if this is not the case or if the locale does not exist, the test is not
   performed (and if the MPFR_CHECK_LOCALES environment variable is set,
   the program fails). */
static void
locale_da_DK (void)
{
  mpfr_prec_t p = 128;
  mpfr_t x, y;

  if ((setlocale (LC_ALL, "da_DK.utf8") == 0 &&
       setlocale (LC_ALL, "da_DK") == 0) ||
      localeconv()->decimal_point[0] != ',' ||
      localeconv()->thousands_sep[0] != '.')
    {
      setlocale (LC_ALL, "C");

      if (getenv ("MPFR_CHECK_LOCALES") == NULL)
        return;

      fprintf (stderr,
               "Cannot test the da_DK locale (not found or inconsistent).\n");
      exit (1);
    }

  mpfr_init2 (x, p);

  /* positive numbers */
  mpfr_set_str (x, "18993474.61279296875", 10, MPFR_RNDN);
  mpfr_init2 (y, 59);
  mpfr_set (y, x, MPFR_RNDN);

  /* simplest case right justified with thousands separator */
  check_sprintf ("1,899347461279296875000000000000000000000e+07", "%'30Re", x);
  check_sprintf ("      1,899347461279296875e+07", "%'30Re", y);
  check_sprintf ("                   1,89935e+07", "%'30Rg", x);
  check_sprintf ("        18.993.474,61279296875", "%'30.19Rg", x);
  check_sprintf ("             18.993.474,612793", "%'30Rf", x);

  /* sign or space, pad, thousands separator with leading zeros */
  check_sprintf (" 1,899347461279296875000000000000000000000E+07", "%' 030RE", x);
  check_sprintf (" 000001,899347461279296875E+07", "%' 030RE", y);
  check_sprintf (" 0000000000000000001,89935E+07", "%' 030RG", x);
  check_sprintf (" 000000018.993.474,61279296875", "%' 030.19RG", x);
  check_sprintf (" 00000000000018.993.474,612793", "%' 030RF", x);

#define T1 "000"
#define T2 ".000"
#define S1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1 T1
#define S2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 T2 ","

  mpfr_set_ui (x, 48, MPFR_RNDN);
  mpfr_exp10 (x, x, MPFR_RNDN);
  check_sprintf ("1" S1, "%.0Rf", x);
  check_sprintf ("1" S2, "%'#.0Rf", x);
  check_sprintf ("1" S2 "0000", "%'.4Rf", x);
  mpfr_mul_ui (x, x, 10, MPFR_RNDN);
  check_sprintf ("10" S1, "%.0Rf", x);
  check_sprintf ("10" S2, "%'#.0Rf", x);
  check_sprintf ("10" S2 "0000", "%'.4Rf", x);
  mpfr_mul_ui (x, x, 10, MPFR_RNDN);
  check_sprintf ("100" S1, "%.0Rf", x);
  check_sprintf ("100" S2, "%'#.0Rf", x);
  check_sprintf ("100" S2 "0000", "%'.4Rf", x);

  mpfr_clear (x);
  mpfr_clear (y);

  setlocale (LC_ALL, "C");
}

#endif  /* ... && MPFR_LCONV_DPTS */

/* check concordance between mpfr_asprintf result with a regular mpfr float
   and with a regular double float */
static void
random_double (void)
{
  mpfr_t x; /* random regular mpfr float */
  double y; /* regular double float (equal to x) */

  char flag[] =
    {
      '-',
      '+',
      ' ',
      '#',
      '0', /* no ambiguity: first zeros are flag zero */
      '\'' /* SUS extension */
    };
  /* no 'a': mpfr and glibc do not have the same semantic */
  char specifier[] =
    {
      'e',
      'f',
      'g',
      'E',
      'f', /* SUSv2 doesn't accept %F, but %F and %f are the same for
              regular numbers */
      'G',
    };
  int spec; /* random index in specifier[] */
  int prec; /* random value for precision field */

  /* in the format string for mpfr_t variable, the maximum length is
     reached by something like "%-+ #0'.*Rf", that is 12 characters. */
#define FMT_MPFR_SIZE 12
  char fmt_mpfr[FMT_MPFR_SIZE];
  char *ptr_mpfr;

  /* in the format string for double variable, the maximum length is
     reached by something like "%-+ #0'.*f", that is 11 characters. */
#define FMT_SIZE 11
  char fmt[FMT_SIZE];
  char *ptr;

  int xi;
  char *xs;
  int yi;
  char *ys;

  int i, j, jmax;

  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);

  for (i = 0; i < 1000; ++i)
    {
      /* 1. random double */
      do
        {
          y = DBL_RAND ();
        }
      while (ABS(y) < DBL_MIN);

      if (RAND_BOOL ())
        y = -y;

      mpfr_set_d (x, y, MPFR_RNDN);
      if (y != mpfr_get_d (x, MPFR_RNDN))
        /* conversion error: skip this one */
        continue;

      /* 2. build random format strings fmt_mpfr and fmt */
      ptr_mpfr = fmt_mpfr;
      ptr = fmt;
      *ptr_mpfr++ = *ptr++ = '%';
      /* random specifier 'e', 'f', 'g', 'E', 'F', or 'G' */
      spec = (int) (randlimb() % 6);
      /* random flags, but no ' flag with %e or with non-glibc */
#if __MPFR_GLIBC(1,0)
      jmax = (spec == 0 || spec == 3) ? 5 : 6;
#else
      jmax = 5;
#endif
      for (j = 0; j < jmax; j++)
        {
          if (randlimb() % 3 == 0)
            *ptr_mpfr++ = *ptr++ = flag[j];
        }
      *ptr_mpfr++ = *ptr++ = '.';
      *ptr_mpfr++ = *ptr++ = '*';
      *ptr_mpfr++ = 'R';
      *ptr_mpfr++ = *ptr++ = specifier[spec];
      *ptr_mpfr = *ptr = '\0';
      MPFR_ASSERTN (ptr - fmt < FMT_SIZE);
      MPFR_ASSERTN (ptr_mpfr - fmt_mpfr < FMT_MPFR_SIZE);

      /* advantage small precision */
      prec = RAND_BOOL () ? 10 : prec_max_printf;
      prec = (int) (randlimb () % prec);

      /* 3. calls and checks */
      /* the double float case is handled by the libc asprintf through
         gmp_asprintf */
      xi = mpfr_asprintf (&xs, fmt_mpfr, prec, x);
      yi = mpfr_asprintf (&ys, fmt, prec, y);

      /* test if XS and YS differ, beware that ISO C99 doesn't specify
         the sign of a zero exponent (the C99 rationale says: "The sign
         of a zero exponent in %e format is unspecified.  The committee
         knows of different implementations and choose not to require
         implementations to document their behavior in this case
         (by making this be implementation defined behaviour).  Most
         implementations use a "+" sign, e.g., 1.2e+00; but there is at
         least one implementation that uses the sign of the unlimited
         precision result, e.g., the 0.987 would be 9.87e-01, so could
         end up as 1e-00 after rounding to one digit of precision."),
         while mpfr always uses '+' */
      if (xi != yi
          || ((strcmp (xs, ys) != 0)
              && (spec == 1 || spec == 4
                  || ((strstr (xs, "e+00") == NULL
                       || strstr (ys, "e-00") == NULL)
                      && (strstr (xs, "E+00") == NULL
                          || strstr (ys, "E-00") == NULL)))))
        {
          mpfr_printf ("Error in mpfr_asprintf(\"%s\", %d, %Re)\n",
                       fmt_mpfr, prec, x);
          printf ("expected: %s\n", ys);
          printf ("     got: %s\n", xs);
          printf ("xi=%d yi=%d spec=%d\n", xi, yi, spec);

          exit (1);
        }

      mpfr_free_str (xs);
      mpfr_free_str (ys);
    }

  mpfr_clear (x);
}

static void
bug20080610 (void)
{
  /* bug on icc found on June 10, 2008 */
  /* this is not a bug but a different implementation choice: ISO C99 doesn't
     specify the sign of a zero exponent (see note in random_double above). */
  mpfr_t x;
  double y;
  int xi;
  char *xs;
  int yi;
  char *ys;

  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);

  y = -9.95645044213728791504536275169812142849e-01;
  mpfr_set_d (x, y, MPFR_RNDN);

  xi = mpfr_asprintf (&xs, "%- #0.*Re", 1, x);
  yi = mpfr_asprintf (&ys, "%- #0.*e", 1, y);

  if (xi != yi || strcmp (xs, ys) != 0)
    {
      printf ("Error in bug20080610\n");
      printf ("expected: %s\n", ys);
      printf ("     got: %s\n", xs);
      printf ("xi=%d yi=%d\n", xi, yi);

      exit (1);
    }

  mpfr_free_str (xs);
  mpfr_free_str (ys);
  mpfr_clear (x);
}

static void
bug20081214 (void)
{
 /* problem with glibc 2.3.6, December 14, 2008:
    the system asprintf outputs "-1.0" instead of "-1.". */
  mpfr_t x;
  double y;
  int xi;
  char *xs;
  int yi;
  char *ys;

  mpfr_init2 (x, MPFR_LDBL_MANT_DIG);

  y = -9.90597761233942053494e-01;
  mpfr_set_d (x, y, MPFR_RNDN);

  xi = mpfr_asprintf (&xs, "%- #0.*RG", 1, x);
  yi = mpfr_asprintf (&ys, "%- #0.*G", 1, y);

  if (xi != yi || strcmp (xs, ys) != 0)
    {
      mpfr_printf ("Error in bug20081214\n"
                   "mpfr_asprintf(\"%- #0.*Re\", 1, %Re)\n", x);
      printf ("expected: %s\n", ys);
      printf ("     got: %s\n", xs);
      printf ("xi=%d yi=%d\n", xi, yi);

      exit (1);
    }

  mpfr_free_str (xs);
  mpfr_free_str (ys);
  mpfr_clear (x);
}

static void
bug20111102 (void)
{
  mpfr_t t;
  char s[100];

  mpfr_init2 (t, 84);
  mpfr_set_str (t, "999.99999999999999999999", 10, MPFR_RNDN);
  mpfr_sprintf (s, "%.20RNg", t);
  if (strcmp (s, "1000") != 0)
    {
      printf ("Error in bug20111102, expected 1000, got %s\n", s);
      exit (1);
    }
  mpfr_clear (t);
}

/* In particular, the following test makes sure that the rounding
 * for %Ra and %Rb is not done on the MPFR number itself (as it
 * would overflow). Note: it has been reported on comp.std.c that
 * some C libraries behave differently on %a, but this is a bug.
 */
static void
check_emax_aux (mpfr_exp_t e)
{
  mpfr_t x;
  char *s1, s2[256];
  int i;
  mpfr_exp_t emax;

  MPFR_ASSERTN (e <= LONG_MAX);
  emax = mpfr_get_emax ();
  set_emax (e);

  mpfr_init2 (x, 16);

  mpfr_set_inf (x, 1);
  mpfr_nextbelow (x);

  i = mpfr_asprintf (&s1, "%Ra %.2Ra", x, x);
  MPFR_ASSERTN (i > 0);

  mpfr_snprintf (s2, 256, "0x7.fff8p+%ld 0x8.00p+%ld", e-3, e-3);

  if (strcmp (s1, s2) != 0)
    {
      printf ("Error in check_emax_aux for emax = ");
      if (e > LONG_MAX)
        printf ("(>LONG_MAX)\n");
      else
        printf ("%ld\n", (long) e);
      printf ("Expected '%s'\n", s2);
      printf ("Got      '%s'\n", s1);
      exit (1);
    }

  mpfr_free_str (s1);

  i = mpfr_asprintf (&s1, "%Rb %.2Rb", x, x);
  MPFR_ASSERTN (i > 0);

  mpfr_snprintf (s2, 256, "1.111111111111111p+%ld 1.00p+%ld", e-1, e);

  if (strcmp (s1, s2) != 0)
    {
      printf ("Error in check_emax_aux for emax = ");
      if (e > LONG_MAX)
        printf ("(>LONG_MAX)\n");
      else
        printf ("%ld\n", (long) e);
      printf ("Expected %s\n", s2);
      printf ("Got      %s\n", s1);
      exit (1);
    }

  mpfr_free_str (s1);

  mpfr_clear (x);
  set_emax (emax);
}

static void
check_emax (void)
{
  check_emax_aux (15);
  check_emax_aux (MPFR_EMAX_MAX);
}

static void
check_emin_aux (mpfr_exp_t e)
{
  mpfr_t x;
  char *s1, s2[256];
  int i;
  mpfr_exp_t emin;
  mpz_t ee;

  MPFR_ASSERTN (e >= LONG_MIN);
  emin = mpfr_get_emin ();
  set_emin (e);

  mpfr_init2 (x, 16);
  mpz_init (ee);

  mpfr_setmin (x, e);
  mpz_set_si (ee, e);
  mpz_sub_ui (ee, ee, 1);

  i = mpfr_asprintf (&s1, "%Ra", x);
  MPFR_ASSERTN (i > 0);

  gmp_snprintf (s2, 256, "0x1p%Zd", ee);

  if (strcmp (s1, s2) != 0)
    {
      printf ("Error in check_emin_aux for emin = %ld\n", (long) e);
      printf ("Expected %s\n", s2);
      printf ("Got      %s\n", s1);
      exit (1);
    }

  mpfr_free_str (s1);

  i = mpfr_asprintf (&s1, "%Rb", x);
  MPFR_ASSERTN (i > 0);

  gmp_snprintf (s2, 256, "1p%Zd", ee);

  if (strcmp (s1, s2) != 0)
    {
      printf ("Error in check_emin_aux for emin = %ld\n", (long) e);
      printf ("Expected %s\n", s2);
      printf ("Got      %s\n", s1);
      exit (1);
    }

  mpfr_free_str (s1);

  mpfr_clear (x);
  mpz_clear (ee);
  set_emin (emin);
}

static void
check_emin (void)
{
  check_emin_aux (-15);
  check_emin_aux (mpfr_get_emin ());
  check_emin_aux (MPFR_EMIN_MIN);
}

static void
test20161214 (void)
{
  mpfr_t x;
  char buf[32];
  const char s[] = "0x0.fffffffffffff8p+1024";
  int r;

  mpfr_init2 (x, 64);
  mpfr_set_str (x, s, 16, MPFR_RNDN);
  r = mpfr_snprintf (buf, 32, "%.*RDf", -2, x);
  MPFR_ASSERTN(r == 316);
  r = mpfr_snprintf (buf, 32, "%.*RDf", INT_MIN + 1, x);
  MPFR_ASSERTN(r == 316);
  r = mpfr_snprintf (buf, 32, "%.*RDf", INT_MIN, x);
  MPFR_ASSERTN(r == 316);
  mpfr_clear (x);
}

/* http://gforge.inria.fr/tracker/index.php?func=detail&aid=21056 */
static void
bug21056 (void)
{
  mpfr_t x;
  const char s[] = "0x0.fffffffffffff8p+1024";
  int ndigits, r;

  mpfr_init2 (x, 64);

  mpfr_set_str (x, s, 16, MPFR_RNDN);

  ndigits = 1000;
  r = mpfr_snprintf (0, 0, "%.*RDf", ndigits, x);
  /* the return value should be ndigits + 310 */
  MPFR_ASSERTN(r == ndigits + 310);

  ndigits = INT_MAX - 310;
  r = mpfr_snprintf (0, 0, "%.*RDf", ndigits, x);
  MPFR_ASSERTN(r == INT_MAX);

  ndigits = INT_MAX - 10;
  r = mpfr_snprintf (0, 0, "%.*RDa", ndigits, x);
  MPFR_ASSERTN(r == INT_MAX);

  ndigits = INT_MAX - 7;
  r = mpfr_snprintf (0, 0, "%.*RDe", ndigits, x);
  MPFR_ASSERTN(r == INT_MAX);

  ndigits = 1000;
  r = mpfr_snprintf (0, 0, "%.*RDg", ndigits, x);
  /* since trailing zeros are removed with %g, we get less digits */
  MPFR_ASSERTN(r == 309);

  ndigits = INT_MAX;
  r = mpfr_snprintf (0, 0, "%.*RDg", ndigits, x);
  /* since trailing zeros are removed with %g, we get less digits */
  MPFR_ASSERTN(r == 309);

  ndigits = INT_MAX - 1;
  r = mpfr_snprintf (0, 0, "%#.*RDg", ndigits, x);
  MPFR_ASSERTN(r == ndigits + 1);

  mpfr_clear (x);
}

/* Fails for i = 5, i.e. t[i] = (size_t) UINT_MAX + 1,
   with r11427 on 64-bit machines (4-byte int, 8-byte size_t).
   On such machines, t[5] converted to int typically gives 0.
   Note: the assumed behavior corresponds to the snprintf behavior
   in ISO C, but this conflicts with POSIX:
     https://sourceware.org/bugzilla/show_bug.cgi?id=14771#c2
     https://austingroupbugs.net/view.php?id=761
     https://austingroupbugs.net/view.php?id=1219
     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87096
   The issue was due to a 64-bit size_t converted to a 32-bit int.
   Fixed in r11429 (6b8cf3e2bdc285027627281cac230ed932c1b73f) on 2017-04-07.
*/
static void
snprintf_size (void)
{
  mpfr_t x;
  char buf[12];
  const char s[] = "17.00000000";
  size_t t[] = { 11, 12, 64, INT_MAX, (size_t) INT_MAX + 1,
                 (size_t) UINT_MAX + 1, (size_t) UINT_MAX + 2,
                 (size_t) -1 };
  int i, r;

  mpfr_init2 (x, 64);
  mpfr_set_ui (x, 17, MPFR_RNDN);

  for (i = 0; i < sizeof (t) / sizeof (*t); i++)
    {
      memset (buf, 0, sizeof (buf));
      /* r = snprintf (buf, t[i], "%.8f", 17.0); */
      r = mpfr_snprintf (buf, t[i], "%.8Rf", x);
      if (r != 11 || (t[i] > 11 && strcmp (buf, s) != 0))
        {
          printf ("Error in snprintf_size for i = %d:\n", i);
          printf ("expected r = 11, \"%s\"\n", s);
          printf ("got      r = %d, \"%s\"\n", r, buf);
          exit (1);
        }
    }

  mpfr_clear (x);
}

/* With r11516, n2 gets a random value for i = 0 only!
   valgrind detects a problem for "nchar = buf.curr - buf.start;"
   in the spec.spec == 'n' case. Indeed, there is no buffer when
   size is 0. */
static void
percent_n (void)
{
  int err = 0, i, j;

  for (i = 0; i < 24; i++)
    for (j = 0; j < 3; j++)
      {
        volatile int n1, n2;
        char buffer[64];

        memset (buffer, 0, 64);
        n2 = -17;
        n1 = mpfr_snprintf (buffer, i % 8, "%d%n", 123, &n2);
        if (n1 != 3 || n2 != 3)
          {
            printf ("Error 1 in percent_n: i = %d, n1 = %d, n2 = %d\n",
                    i, n1, n2);
            err = 1;
          }
      }

  if (err)
    exit (1);
}

struct clo
{
  const char *fmt;
  int width, r, e;
};

static void
check_length_overflow (void)
{
  mpfr_t x;
  int i, r, e;
  struct clo t[] = {
    { "%Rg", 0, 1, 0 },
    { "%*Rg", 1, 1, 0 },
    { "%*Rg", -1, 1, 0 },
    { "%5Rg", 0, 5, 0 },
    { "%*Rg", 5, 5, 0 },
    { "%*Rg", -5, 5, 0 },
#if INT_MAX == 2147483647
    { "%2147483647Rg", 0, 2147483647, 0 },
    { "%2147483647Rg ", 0, -1, 1 },
    { "%2147483648Rg", 0, -1, 1 },
    { "%18446744073709551616Rg", 0, -1, 1 },
    { "%*Rg", 2147483647, 2147483647, 0 },
    { "%*Rg", -2147483647, 2147483647, 0 },
# if INT_MIN < -INT_MAX
    { "%*Rg", INT_MIN, -1, 1 },
# endif
#endif
  };

  mpfr_init2 (x, MPFR_PREC_MIN);
  mpfr_set_ui (x, 0, MPFR_RNDN);

  for (i = 0; i < numberof (t); i++)
    {
      errno = 0;
      r = t[i].width == 0 ?
        mpfr_snprintf (NULL, 0, t[i].fmt, x) :
        mpfr_snprintf (NULL, 0, t[i].fmt, t[i].width, x);
      e = errno;
      if ((t[i].r < 0 ? r >= 0 : r != t[i].r)
#ifdef EOVERFLOW
          || (t[i].e && e != EOVERFLOW)
#endif
          )
        {
          printf ("Error in check_length_overflow for i=%d (%s %d)\n",
                  i, t[i].fmt, t[i].width);
          printf ("Expected r=%d, got r=%d\n", t[i].r, r);
#ifdef EOVERFLOW
          if (t[i].e && e != EOVERFLOW)
            printf ("Expected errno=EOVERFLOW=%d, got errno=%d\n",
                    EOVERFLOW, e);
#endif
          exit (1);
        }
    }

  mpfr_clear (x);
}

/* On 2023-03-22, on a 64-bit Linux machine (thus with 32-bit int),
   the case %.2147483648Rg yields an incorrect size computation and
   MPFR wants to allocate 18446744071562070545 bytes. With assertion
   checking (--enable-assert), one gets:
     vasprintf.c:1908: MPFR assertion failed: threshold >= 1
   for the 2nd mpfr_snprintf below (the other calls with %.2147483648Rg
   have the same issue).

   This case should either succeed or fail as reaching an environmental limit
   like with glibc (note that the precision does not fit in an int).
   For MPFR, once this bug is fixed, this case should actually succeed,
   unless mpfr_intmax_t is a 32-bit type[*] (because 2147483648 is not
   representable in mpfr_intmax_t and in an int), so let's assume that
   in the tests below.
   [*] can be tested with
       -std=c90 -Werror -pedantic -Wformat -Wno-error=overlength-strings
       (this is a way to disable intmax_t).
*/
static void
large_prec_for_g (void)
{
  mpfr_t x;
  char buf1[4] = "xxx", buf2[4] = "xxx", buf3[4] = "xxx", buf4[4] = "xxx";
  int allow_fail = (mpfr_uintmax_t) -1 == 0xffffffff;
  int r;

  mpfr_init2 (x, 128);
  mpfr_set_ui (x, 1, MPFR_RNDN);

  r = mpfr_snprintf (NULL, 0, "%.2147483647Rg\n", x);
  MPFR_ASSERTN (r == 2);

  r = mpfr_snprintf (NULL, 0, "%.2147483648Rg\n", x);
  MPFR_ASSERTN (r == 2 || (allow_fail && r < 0));

  r = mpfr_snprintf (buf1, sizeof(buf1), "%.2147483647Rg\n", x);
  MPFR_ASSERTN (r == 2);
  MPFR_ASSERTN (buf1[0] == '1' && buf1[1] == '\n' && buf1[2] == 0);

  r = mpfr_snprintf (buf2, sizeof(buf2), "%.2147483648Rg\n", x);
  if (r < 0)
    MPFR_ASSERTN (allow_fail);
  else
    {
      MPFR_ASSERTN (r == 2);
      MPFR_ASSERTN (buf2[0] == '1' && buf2[1] == '\n' && buf2[2] == 0);
    }

  r = mpfr_sprintf (buf3, "%.2147483647Rg\n", x);
  MPFR_ASSERTN (r == 2);
  MPFR_ASSERTN (buf3[0] == '1' && buf3[1] == '\n' && buf3[2] == 0);

  r = mpfr_sprintf (buf4, "%.2147483648Rg\n", x);
  if (r < 0)
    MPFR_ASSERTN (allow_fail);
  else
    {
      MPFR_ASSERTN (r == 2);
      MPFR_ASSERTN (buf4[0] == '1' && buf4[1] == '\n' && buf4[2] == 0);
    }

  mpfr_clear (x);
}

/* On 2023-12-02, this gives
     vasprintf.c:670: MPFR assertion failed: len <= strlen (s)
   with assertion checking (--enable-assert), or
     [MPFR] tests_free(): bad size 1, should be 2
   with tests_memory_disabled = 0 (default unless mini-gmp is used),
   which means possible memory corruption with custom memory allocators
   that do not ignore the size parameter of the "free" function, or
     Error in check_null
     expected r = 1, s = { 0, 0, 1 }
     got      r = 0, s = { 0, 1, 1 }
   with tests_memory_disabled = 1.
   Fixed in commits
     390e51ef8570da4e338e9806ecaf2d022210d951 (2023-12-03)
     3babf029fe604c08ec517ca6945a5efb155f69d1 (2023-12-13)
*/
static void
check_null (void)
{
#ifndef MPFR_TESTS_SKIP_CHECK_NULL
  int i;

  for (i = 0; i < 2; i++)
    {
      int r;
      char s[3] = { 1, 1, 1 };
      char t[5] = { 1, 1, 1, 1, 1 };

      r = i == 0 ? mpfr_sprintf (s, "%c", 0)
        : mpfr_snprintf (s, 2, "%c", 0) ;
      if (r != 1 || s[0] != 0 || s[1] != 0 || s[2] != 1)
        {
          printf ("Error in check_null, i = %d\n", i);
          printf ("expected r = %d, s = { %d, %d, %d }\n",
                  1, 0, 0, 1);
          printf ("got      r = %d, s = { %d, %d, %d }\n",
                  r, s[0], s[1], s[2]);
          exit (1);
        }

      r = i == 0 ? mpfr_sprintf (t, "%c%c%c", 8, 0, 9)
        : mpfr_snprintf (t, 4, "%c%c%c", 8, 0, 9);
      if (r != 3 ||
          t[0] != 8 || t[1] != 0 || t[2] != 9 || t[3] != 0 || t[4] != 1)
        {
          printf ("Error in check_null, i = %d\n", i);
          printf ("expected r = %d, t = { %d, %d, %d, %d, %d }\n",
                  3, 8, 0, 9, 0, 1);
          printf ("got      r = %d, t = { %d, %d, %d, %d, %d }\n",
                  r, t[0], t[1], t[2], t[3], t[4]);
          exit (1);
        }
    }
#endif
}

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)

/* The following tests should be equivalent to those from test_locale()
   in tprintf.c (remove the \n at the end of the test strings). */

static void
test_locale (void)
{
  const char * const tab_locale[] = {
    "en_US",
    "en_US.iso88591",
    "en_US.iso885915",
    "en_US.utf8"
  };
  int i;
  mpfr_t x;
  char v[] = "99999999999999999999999.5";

  for (i = 0; i < numberof(tab_locale); i++)
    {
      char *s;

      s = setlocale (LC_ALL, tab_locale[i]);

      if (s != NULL && MPFR_THOUSANDS_SEPARATOR == ',')
        break;
    }

  if (i == numberof(tab_locale))
    {
      if (getenv ("MPFR_CHECK_LOCALES") == NULL)
        return;

      fprintf (stderr, "Cannot find a locale with ',' thousands separator.\n"
               "Please install one of the en_US based locales.\n");
      exit (1);
    }

  mpfr_init2 (x, 113);
  mpfr_set_ui (x, 10000, MPFR_RNDN);

  check_sprintf ("(1) 10000=10,000 ", "(1) 10000=%'Rg ", x);
  check_sprintf ("(2) 10000=10,000.000000 ", "(2) 10000=%'Rf ", x);

  mpfr_set_ui (x, 1000, MPFR_RNDN);
  check_sprintf ("(3) 1000=1,000.000000 ", "(3) 1000=%'Rf ", x);

  for (i = 1; i <= sizeof (v) - 3; i++)
    {
      char buf[64];
      int j;

      strcpy (buf, "(4) 10^i=1");
      for (j = i; j > 0; j--)
        strcat (buf, (j % 3 == 0) ? ",0" : "0");
      strcat (buf, " ");
      mpfr_set_str (x, v + sizeof (v) - 3 - i, 10, MPFR_RNDN);
      check_sprintf (buf, "(4) 10^i=%'.0Rf ", x);
    }

#define N0 20

  for (i = 1; i <= N0; i++)
    {
      char s[N0+4], buf[64];
      int j;

      s[0] = '1';
      for (j = 1; j <= i; j++)
        s[j] = '0';
      s[i+1] = '\0';

      strcpy (buf, "(5) 10^i=1");
      for (j = i; j > 0; j--)
        strcat (buf, (j % 3 == 0) ? ",0" : "0");
      strcat (buf, " ");

      mpfr_set_str (x, s, 10, MPFR_RNDN);

      check_sprintf (buf, "(5) 10^i=%'.0RNf ", x);
      check_sprintf (buf, "(5) 10^i=%'.0RZf ", x);
      check_sprintf (buf, "(5) 10^i=%'.0RUf ", x);
      check_sprintf (buf, "(5) 10^i=%'.0RDf ", x);
      check_sprintf (buf, "(5) 10^i=%'.0RYf ", x);

      strcat (s + (i + 1), ".5");
      check_sprintf (buf, "(5) 10^i=%'.0Rf ", x);
    }

  mpfr_set_str (x, "1000", 10, MPFR_RNDN);
  check_sprintf ("00000001e+03", "%'012.3Rg", x);
  check_sprintf ("00000001,000", "%'012.4Rg", x);
  check_sprintf ("000000001,000", "%'013.4Rg", x);

#ifdef PRINTF_GROUPFLAG
  /* Do not test the thousands separator with a precision field larger
     than the number of digits (thus needing leading zeros), such as
     "%0+ -'13.10Pd:" (used up to MPFR 4.2.0), since the GNU libc is
     buggy: https://sourceware.org/bugzilla/show_bug.cgi?id=23432
     We don't know about the other implementations.
     This new test works fine with glibc up to 2.36, but fails with 2.37
     (as reported by Klaus Dittrich in the MPFR mailing-list); this is
     actually a bug introduced in glibc 2.37, not in MPFR:
       https://sourceware.org/bugzilla/show_bug.cgi?id=30068
     Since this bug can yield a buffer overflow (CVE-2023-25139), possibly
     affecting MPFR users, let us rather require a fix in glibc. This bug
     has been fixed in the 2.37 branch:
       https://sourceware.org/git/?p=glibc.git;a=commit;h=07b9521fc6
     If we wanted to check that and avoid a failure of the test because of
     a buggy C library (while MPFR would be consistent with the C library),
     we could compare the MPFR output with both the correct output and the
     output from the C library (possibly buggy). But to do that in a clean
     way, this would require a change in the check_vsprintf() call. */
  check_vsprintf ("+1,234,567   :", "%0+ -'13Pd:", (mpfr_prec_t) 1234567);
#endif

  mpfr_clear (x);
}

#else

static void
test_locale (void)
{
  if (getenv ("MPFR_CHECK_LOCALES") != NULL)
    {
      fprintf (stderr, "Cannot test locales.\n");
      exit (1);
    }
}

#endif

int
main (int argc, char **argv)
{
  int k;

  tests_start_mpfr ();

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
  /* currently, we just check with 'C' locale */
  setlocale (LC_ALL, "C");
#endif

  bug20111102 ();

  for (k = 0; k < 40; k++)
    {
      native_types ();
      special ();
      decimal ();
      hexadecimal ();
      binary ();

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE) && MPFR_LCONV_DPTS
      locale_da_DK ();
#else
      if (getenv ("MPFR_CHECK_LOCALES") != NULL)
        {
          fprintf (stderr, "Cannot test locales.\n");
          exit (1);
        }
#endif
    }

  check_emax ();
  check_emin ();
  test20161214 ();
  bug21056 ();
  snprintf_size ();
  percent_n ();
  mixed ();
  check_length_overflow ();
  large_prec_for_g ();
  check_null ();
  test_locale ();

  if (getenv ("MPFR_CHECK_LIBC_PRINTF"))
    {
      /* check against libc */
      random_double ();
      bug20081214 ();
      bug20080610 ();
    }

  tests_end_mpfr ();
  return 0;
}

#else  /* HAVE_STDARG */

int
main (void)
{
  /* We have nothing to test. */
  return 77;
}

#endif  /* HAVE_STDARG */
