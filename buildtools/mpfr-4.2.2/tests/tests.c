/* Miscellaneous support for test programs.

Copyright 2001-2025 Free Software Foundation, Inc.
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

/* NOTE. Some tests on macro definitions are already done in src/init2.c
 * as static assertions (in general). This allows one to get a failure at
 * build time in case of inconsistency (probably due to search path issues
 * in header file inclusion). This does not need to be done again in the
 * test suite.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <float.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef MPFR_TESTS_FPE_DIV
# ifdef MPFR_TESTS_FPE_TRAP
#  define _GNU_SOURCE /* for feenableexcept */
# endif
# include <fenv.h>
#endif

#ifdef HAVE_GETTIMEOFDAY
# include <sys/time.h>
#else
# include <time.h>
#endif

/* <sys/fpu.h> is needed to have union fpc_csr defined under IRIX64
   (see below). Let's include it only if need be. */
#if defined HAVE_SYS_FPU_H && defined HAVE_FPC_CSR
# include <sys/fpu.h>
#endif

#ifdef MPFR_TESTS_TIMEOUT
#include <sys/resource.h>
#endif

#if defined(HAVE_SIGNAL) || defined(HAVE_SIGACTION)
# include <signal.h>
#endif

#include "mpfr-test.h"

#ifdef MPFR_FPU_PREC
/* This option allows to test MPFR on x86 processors when the FPU
 * rounding precision has been changed. As MPFR is a library, this can
 * occur in practice, either by the calling software or by some other
 * library or plug-in used by the calling software. This option is
 * mainly for developers. If it is used, then the <fpu_control.h>
 * header is assumed to exist and work like under Linux/x86. MPFR does
 * not need to be recompiled. So, a possible usage is the following:
 *
 *   cd tests
 *   make clean
 *   make check CFLAGS="-g -O2 -ffloat-store -DMPFR_FPU_PREC=_FPU_SINGLE"
 *
 * i.e. just add -DMPFR_FPU_PREC=... to the CFLAGS found in Makefile.
 *
 * Notes:
 *   + SSE2 (used to implement double's on x86_64, and possibly on x86
 *     too, depending on the compiler configuration and flags) is not
 *     affected by the dynamic precision.
 *   + When the FPU is set to single precision, the behavior of MPFR
 *     functions that have a native floating-point type (float, double,
 *     long double) as argument or return value is not guaranteed.
 */

#include <fpu_control.h>

static void
set_fpu_prec (void)
{
  fpu_control_t cw;

  _FPU_GETCW(cw);
  cw &= ~(_FPU_EXTENDED|_FPU_DOUBLE|_FPU_SINGLE);
  cw |= (MPFR_FPU_PREC);
  _FPU_SETCW(cw);
}

#endif

char             mpfr_rands_initialized = 0;
gmp_randstate_t  mpfr_rands;

char *locale = NULL;

/* Programs that test GMP's mp_set_memory_functions() need to set
   tests_memory_disabled = 2 before calling tests_start_mpfr(). */
#ifdef MPFR_USE_MINI_GMP
/* disable since mini-gmp does not keep track of old_size in realloc/free */
int tests_memory_disabled = 1;
#else
int tests_memory_disabled = 0;
#endif

static mpfr_exp_t default_emin, default_emax;

static void tests_rand_start (void);
static void tests_rand_end   (void);
static void tests_limit_start (void);

/* We want to always import the function mpfr_dump inside the test
   suite, so that we can use it in GDB. But it doesn't work if we build
   a Windows DLL (initializer element is not a constant) */
#if !__GMP_LIBGMP_DLL
extern void (*dummy_func) (mpfr_srcptr);
void (*dummy_func)(mpfr_srcptr) = mpfr_dump;
#endif

/* Various version checks.
   A mismatch on the GMP version is not regarded as fatal. A mismatch
   on the MPFR version is regarded as fatal, since this means that we
   would not check the MPFR library that has just been built (the goal
   of "make check") but a different library that is already installed,
   i.e. any test result would be meaningless; in such a case, we exit
   immediately with an error (exit status = 1).
   Return value: 0 for no errors, 1 in case of any non-fatal error.
   Note: If the return value is 0, no data must be sent to stdout. */
int
test_version (void)
{
  const char *version;
  char buffer[256];
  int err = 0;

#ifndef MPFR_USE_MINI_GMP
  sprintf (buffer, "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
           __GNU_MP_VERSION_PATCHLEVEL);
  if (strcmp (buffer, gmp_version) != 0 &&
      (__GNU_MP_VERSION_PATCHLEVEL != 0 ||
       (sprintf (buffer, "%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR),
        strcmp (buffer, gmp_version) != 0)))
    err = 1;

  /* In some cases, it may be acceptable to have different versions for
     the header and the library, in particular when shared libraries are
     used (e.g., after a bug-fix upgrade of the library, and versioning
     ensures that this can be done only when the binary interface is
     compatible). However, when recompiling software like here, this
     should never happen (except if GMP has been upgraded between two
     "make check" runs, but there's no reason for that). A difference
     between the versions of gmp.h and libgmp probably indicates either
     a bad configuration or some other inconsistency in the development
     environment, and it is better to fail (in particular for automatic
     installations). */
  if (err)
    {
      printf ("ERROR! The versions of gmp.h (%s) and libgmp (%s) do not "
              "match.\nThe possible causes are:\n", buffer, gmp_version);
      printf ("  * A bad configuration in your include/library search paths.\n"
              "  * An inconsistency in the include/library search paths of\n"
              "    your development environment; an example:\n"
              "      "
              "https://gcc.gnu.org/legacy-ml/gcc-help/2010-11/msg00359.html\n"
              "  * GMP has been upgraded after the first \"make check\".\n"
              "    In such a case, try again after a \"make clean\".\n"
              "  * A new or non-standard version naming is used in GMP.\n"
              "    In this case, a patch may already be available on the\n"
              "    MPFR web site.  Otherwise please report the problem.\n");
      printf ("In the first two cases, this may lead to errors, in particular"
              " with MPFR.\nIf some other tests fail, please solve that"
              " problem first.\n");
    }
#endif

  /* VL: I get the following error on an OpenSUSE machine, and changing
     the value of shlibpath_overrides_runpath in the libtool file from
     'no' to 'yes' fixes the problem. */
  version = mpfr_get_version ();
  if (strcmp (MPFR_VERSION_STRING, version) == 0)
    {
      int i;

      sprintf (buffer, "%d.%d.%d", MPFR_VERSION_MAJOR, MPFR_VERSION_MINOR,
               MPFR_VERSION_PATCHLEVEL);
      for (i = 0; buffer[i] == version[i]; i++)
        if (buffer[i] == '\0')
          return err;
      if (buffer[i] == '\0' && version[i] == '-')
        return err;
      printf ("%sMPFR_VERSION_MAJOR.MPFR_VERSION_MINOR.MPFR_VERSION_PATCHLEVEL"
              " (%s)\nand MPFR_VERSION_STRING (%s) do not match!\nIt seems "
              "that the mpfr.h file has been corrupted.\n", err ? "\n" : "",
              buffer, version);
    }
  else
    printf (
      "%sIncorrect MPFR version! (%s header vs %s library)\n"
      "Nothing else has been tested since for this reason, any other test\n"
      "may fail.  Please fix this problem first, as suggested below.  It\n"
      "probably comes from libtool (included in the MPFR tarball), which\n"
      "is responsible for setting up the search paths depending on the\n"
      "platform, or automake.\n"
      "  * On some platforms such as Solaris, $LD_LIBRARY_PATH overrides\n"
      "    the rpath, and if the MPFR library is already installed in a\n"
      "    $LD_LIBRARY_PATH directory, you typically get this error.  Do\n"
      "    not use $LD_LIBRARY_PATH permanently on such platforms; it may\n"
      "    also break other things.\n"
      "  * You may have an ld option that specifies a library search path\n"
      "    where MPFR can be found, taking the precedence over the path\n"
      "    added by libtool.  Check your environment variables, such as\n"
      "    LD_OPTIONS under Solaris.  Moreover, under Solaris, the run path\n"
      "    generated by libtool 2.4.6 may be incorrect: the build directory\n"
      "    may not appear first in the run path; set $LD_LIBRARY_PATH to\n"
      "    /path/to/builddir/src/.libs for the tests as a workaround.\n"
      "  * Then look at https://www.mpfr.org/mpfr-current/ for any update.\n"
      "  * Try again on a completely clean source (some errors might come\n"
      "    from a previous build or previous source changes).\n"
      "  * If the error still occurs, you can try to change the value of\n"
      "    shlibpath_overrides_runpath ('yes' or 'no') in the \"libtool\"\n"
      "    file and rebuild MPFR (make clean && make && make check).  You\n"
      "    may want to report the problem to the libtool and/or automake\n"
      "    developers, with the effect of this change.\n",
      err ? "\n" : "", MPFR_VERSION_STRING, version);
  /* Note about $LD_LIBRARY_PATH under Solaris:
   *   https://en.wikipedia.org/wiki/Rpath#Solaris_ld.so
   * This cause has been confirmed by a user who got this error.
   * And about the libtool 2.4.6 bug also concerning Solaris:
   *   https://debbugs.gnu.org/cgi/bugreport.cgi?bug=30222
   *   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=888059
   */
  exit (1);
}

/* The inexact exception occurs very often, and is normal.
   The underflow exception also might occur, for example in test_generic
   for mpfr_xxx_d functions. Same for overflow. Thus we only check for
   the division-by-zero and invalid exceptions, which should not occur
   inside MPFR. */
#define FPE_FLAGS (FE_DIVBYZERO | FE_INVALID)

void
tests_start_mpfr (void)
{
  /* Don't buffer, so output is not lost if a test causes a segv, etc.
     For stdout, this is important as it will typically be fully buffered
     by default with "make check". For stderr, the C standard just says
     that it is not fully buffered (it may be line buffered by default);
     disabling buffering completely might be useful in some cases.
     Warning! No operations must have already been done on stdout/stderr
     (this is a requirement of ISO C, and this is important on AIX).
     Thus tests_start_mpfr should be called at the beginning of main(),
     possibly after some variable settings. */
  setbuf (stdout, NULL);
  setbuf (stderr, NULL);

  test_version ();

#if defined HAVE_LOCALE_H && defined HAVE_SETLOCALE
  /* Added on 2005-07-09. This allows to test MPFR under various
     locales. New bugs will probably be found, in particular with
     LC_ALL="tr_TR.ISO8859-9" because of the i/I character... */
  locale = setlocale (LC_ALL, "");
#endif

#ifdef MPFR_FPU_PREC
  set_fpu_prec ();
#endif

#ifdef MPFR_TESTS_FPE_DIV
  /* Define to test the use of MPFR_ERRDIVZERO */
  feclearexcept (FE_ALL_EXCEPT);
# ifdef MPFR_TESTS_FPE_TRAP
  /* to trap the corresponding FP exceptions */
  feenableexcept (FPE_FLAGS);
# endif
#endif

  if (tests_memory_disabled != 2)
    {
      if (tests_memory_disabled == 0)
        tests_memory_start ();
      tests_rand_start ();
    }
  tests_limit_start ();

  default_emin = mpfr_get_emin ();
  default_emax = mpfr_get_emax ();
}

void
tests_end_mpfr (void)
{
  int err = 0;

  if (mpfr_get_emin () != default_emin)
    {
      printf ("Default emin value has not been restored!\n");
      err = 1;
    }

  if (mpfr_get_emax () != default_emax)
    {
      printf ("Default emax value has not been restored!\n");
      err = 1;
    }

  mpfr_free_cache ();
  mpfr_free_cache2 (MPFR_FREE_GLOBAL_CACHE);
  if (tests_memory_disabled != 2)
    {
      tests_rand_end ();
      if (tests_memory_disabled == 0)
        tests_memory_end ();
    }

#ifdef MPFR_TESTS_FPE_DIV
  /* Define to test the use of MPFR_ERRDIVZERO */
  if (fetestexcept (FPE_FLAGS))
    {
      /* With MPFR_ERRDIVZERO, such exceptions should never occur
         because the purpose of defining MPFR_ERRDIVZERO is to avoid
         all the FP divisions by 0. */
      printf ("Some floating-point exception(s) occurred:");
      if (fetestexcept (FE_DIVBYZERO))
        printf (" DIVBYZERO");  /* e.g. from 1.0 / 0.0 to generate an inf */
      if (fetestexcept (FE_INVALID))
        printf (" INVALID");    /* e.g. from 0.0 / 0.0 to generate a NaN */
      printf ("\n");
      err = 1;
    }
#endif

  if (err)
    exit (err);
}

static void
tests_limit_start (void)
{
#ifdef MPFR_TESTS_TIMEOUT
  struct rlimit rlim[1];
  char *timeoutp;
  int timeout;

  timeoutp = getenv ("MPFR_TESTS_TIMEOUT");
  timeout = timeoutp != NULL ? atoi (timeoutp) : MPFR_TESTS_TIMEOUT;
  if (timeout > 0)
    {
      /* We need to call getrlimit first to initialize rlim_max to
         an acceptable value for setrlimit. When enabled, timeouts
         are regarded as important: we don't want to take too much
         CPU time on machines shared with other users. So, if we
         can't set the timeout, we exit immediately. */
      if (getrlimit (RLIMIT_CPU, rlim))
        {
          printf ("Error: getrlimit failed\n");
          exit (1);
        }
      rlim->rlim_cur = timeout;
      if (setrlimit (RLIMIT_CPU, rlim))
        {
          printf ("Error: setrlimit failed\n");
          exit (1);
        }
    }
#endif
}

static void
tests_rand_start (void)
{
  char           *perform_seed;
  unsigned long  seed;

  if (mpfr_rands_initialized)
    {
      printf (
        "Please let tests_start() initialize the global mpfr_rands, i.e.\n"
        "ensure that function is called before the first use of RANDS.\n");
      exit (1);
    }

  gmp_randinit_default (mpfr_rands);
  mpfr_rands_initialized = 1;

  perform_seed = getenv ("GMP_CHECK_RANDOMIZE");
  if (perform_seed != NULL)
    {
      seed = strtoul (perform_seed, NULL, 10);
      if (! (seed == 0 || seed == 1))
        {
          printf ("Re-seeding with GMP_CHECK_RANDOMIZE=%lu\n", seed);
          gmp_randseed_ui (mpfr_rands, seed);
        }
      else
        {
#ifdef HAVE_GETTIMEOFDAY
          struct timeval  tv;
          gettimeofday (&tv, NULL);
          /* Note: If time_t is a "floating type" (as allowed by ISO C99),
             the cast below can yield undefined behavior. But this would
             be uncommon (gettimeofday() is specified by POSIX only and
             POSIX requires time_t to be an integer type) and this line
             is not executed by default. So, this should be OK. Moreover,
             gettimeofday() is marked obsolescent by POSIX.1-2008. */
          seed = 1000000 * (unsigned long) tv.tv_sec + tv.tv_usec;
#else
          time_t  tv;
          time (&tv);
          seed = tv;
#endif
          gmp_randseed_ui (mpfr_rands, seed);
          printf ("Seed GMP_CHECK_RANDOMIZE=%lu "
                  "(include this in bug reports)\n", seed);
        }
    }
  else
    gmp_randseed_ui (mpfr_rands, 0x2143FEDC);
}

static void
tests_rand_end (void)
{
  RANDS_CLEAR ();
}

/* true if subnormals are supported for double */
int
have_subnorm_dbl (void)
{
  volatile double x = DBL_MIN, y;

  y = x / 2.0;
  return 2.0 * y == x;
}

/* true if subnormals are supported for float */
int
have_subnorm_flt (void)
{
  volatile float x = FLT_MIN, y;

  y = x / 2.0f;
  return 2.0f * y == x;
}

/* initialization function for tests using the hardware floats
   Not very useful now. */
void
mpfr_test_init (void)
{
#ifdef HAVE_FPC_CSR
  /* to get subnormal numbers on IRIX64 */
  union fpc_csr exp;

  exp.fc_word = get_fpc_csr();
  exp.fc_struct.flush = 0;
  set_fpc_csr(exp.fc_word);
#endif

  /* generate DBL_EPSILON with a loop to avoid that the compiler
     optimizes the code below in non-IEEE 754 mode, deciding that
     c = d is always false. */
#if 0
  for (eps = 1.0; eps != DBL_EPSILON; eps /= 2.0);
  c = 1.0 + eps;
  d = eps * (1.0 - eps) / 2.0;
  d += c;
  if (c != d)
    {
      printf ("Warning: IEEE 754 standard not fully supported\n"
              "         (maybe extended precision not disabled)\n"
              "         Some tests may fail\n");
    }
#endif
}


/* generate a random limb */
mp_limb_t
randlimb (void)
{
  mp_limb_t limb;

  mpfr_rand_raw (&limb, RANDS, GMP_NUMB_BITS);
  return limb;
}

unsigned long
randulong (void)
{
#ifdef MPFR_LONG_WITHIN_LIMB

  return randlimb ();

#else

  unsigned long u = 0, v = 0;

  while (u |= randlimb (), v |= MPFR_LIMB_MAX, v != ULONG_MAX)
    {
      u <<= GMP_NUMB_BITS;
      v <<= GMP_NUMB_BITS;
    }

  return u;

#endif
}

long
randlong (void)
{
  unsigned long u = randulong ();

  return ULONG2LONG (u);
}

/* returns ulp(x) for x a 'normal' double-precision number */
double
Ulp (double x)
{
   double y, eps;

   if (x < 0) x = -x;

   y = x * 2.220446049250313080847263336181640625e-16 ; /* x / 2^52 */

   /* as ulp(x) <= y = x/2^52 < 2*ulp(x),
   we have x + ulp(x) <= x + y <= x + 2*ulp(x),
   therefore o(x + y) = x + ulp(x) or x + 2*ulp(x) */

   eps =  x + y;
   eps = eps - x; /* ulp(x) or 2*ulp(x) */

   return (eps > y) ? 0.5 * eps : eps;
}

/* returns the number of ulp's between a and b,
   where a and b can be any floating-point number, except NaN
 */
int
ulp (double a, double b)
{
  double twoa;

  if (a == b) return 0; /* also deals with a=b=inf or -inf */

  twoa = a + a;
  if (twoa == a) /* a is +/-0.0 or +/-Inf */
    return ((b < a) ? INT_MAX : -INT_MAX);

  return (int) ((a - b) / Ulp (a));
}

/* return double m*2^e */
double
dbl (double m, int e)
{
  if (e >=0 )
    while (e-- > 0)
      m *= 2.0;
  else
    while (e++ < 0)
      m /= 2.0;
  return m;
}

/* Warning: NaN values cannot be distinguished if MPFR_NANISNAN is defined. */
int
Isnan (double d)
{
  return (d) != (d);
}

void
d_trace (const char *name, double d)
{
  double x = d;
  unsigned char *p = (unsigned char *) &x;
  int  i;

  if (name != NULL && name[0] != '\0')
    printf ("%s = ", name);

  printf ("[");
  for (i = 0; i < (int) sizeof (double); i++)
    {
      if (i != 0)
        printf (" ");
      printf ("%02X", (unsigned int) p[i]);
    }
  printf ("] %.20g\n", d);
}

void
ld_trace (const char *name, long double ld)
{
  long double x = ld;
  unsigned char *p = (unsigned char *) &x;
  int  i;

  if (name != NULL && name[0] != '\0')
    printf ("%s = ", name);

  printf ("[");
  for (i = 0; i < (int) sizeof (long double); i++)
    {
      if (i != 0)
        printf (" ");
      printf ("%02X", (unsigned int) p[i]);
    }
  printf ("] %.20Lg\n", ld);
}

void
n_trace (const char *name, mp_limb_t *p, mp_size_t n)
{
  unsigned char *buf;
  size_t bufsize;
  mp_size_t i, m;

  if (name != NULL && name[0] != '\0')
    printf ("%s=", name);

  /* similar to gmp_printf ("%NX\n",...), which is not available
     with mini-gmp */
  bufsize = 2 + ((mpfr_prec_t) n * GMP_NUMB_BITS - 1) / 4;
  buf = (unsigned char *) tests_allocate (bufsize);
  m = mpn_get_str (buf, 16, p, n);
  i = 0;
  while (i < m - 1 && buf[i] == 0)
    i++;  /* skip leading zeros (keeping at least one digit) */
  while (i < m)
    putchar ("0123456789ABCDEF"[buf[i++]]);
  putchar ('\n');
  tests_free (buf, bufsize);
}

/* Open a file in the SRCDIR directory, i.e. the "tests" source directory,
   which is different from the current directory when objdir is different
   from srcdir. One should generally use this function instead of fopen
   directly. */
FILE *
src_fopen (const char *filename, const char *mode)
{
#ifndef SRCDIR
  return fopen (filename, mode);
#else
  const char *srcdir = SRCDIR;
  char *buffer;
  size_t buffsize;
  FILE *f;

  buffsize = strlen (filename) + strlen (srcdir) + 2;
  buffer = (char *) tests_allocate (buffsize);
  if (buffer == NULL)
    {
      printf ("src_fopen: failed to alloc memory)\n");
      exit (1);
    }
  sprintf (buffer, "%s/%s", srcdir, filename);
  f = fopen (buffer, mode);
  tests_free (buffer, buffsize);
  return f;
#endif
}

void
set_emin (mpfr_exp_t exponent)
{
  if (mpfr_set_emin (exponent))
    {
      printf ("set_emin: setting emin to %" MPFR_EXP_FSPEC "d failed\n",
              (mpfr_eexp_t) exponent);
      exit (1);
    }
}

void
set_emax (mpfr_exp_t exponent)
{
  if (mpfr_set_emax (exponent))
    {
      printf ("set_emax: setting emax to %" MPFR_EXP_FSPEC "d failed\n",
              (mpfr_eexp_t) exponent);
      exit (1);
    }
}

/* pos is 512 times the proportion of negative numbers.
   If pos=256, half of the numbers are negative.
   If pos=0, all generated numbers are positive.
*/
void
tests_default_random (mpfr_ptr x, int pos, mpfr_exp_t emin, mpfr_exp_t emax,
                      int always_scale)
{
  MPFR_ASSERTN (emin <= emax);
  MPFR_ASSERTN (emin >= MPFR_EMIN_MIN);
  MPFR_ASSERTN (emax <= MPFR_EMAX_MAX);
  /* but it isn't required that emin and emax are in the current
     exponent range (see below), so that underflow/overflow checks
     can be done on 64-bit machines without a manual change of the
     exponent range (well, this is a bit ugly...). */

  mpfr_urandomb (x, RANDS);
  if (MPFR_IS_PURE_FP (x) && (emin >= 1 || always_scale || RAND_BOOL ()))
    {
      mpfr_exp_t e;
      e = emin + (mpfr_exp_t) (randlimb () % (emax - emin + 1));
      /* Note: There should be no overflow here because both terms are
         between MPFR_EMIN_MIN and MPFR_EMAX_MAX. */
      MPFR_ASSERTD (e >= emin && e <= emax);
      if (mpfr_set_exp (x, e))
        {
          /* The random number doesn't fit in the current exponent range.
             In this case, test the function in the extended exponent range,
             which should be restored by the caller. */
          set_emin (MPFR_EMIN_MIN);
          set_emax (MPFR_EMAX_MAX);
          mpfr_set_exp (x, e);
        }
    }
  if (randlimb () % 512 < pos)
    mpfr_neg (x, x, MPFR_RNDN);
}

/* If sb = -1, then the function is tested in only one rounding mode
   (the one provided in rnd) and the ternary value is not checked.
   Otherwise, the function is tested in the 5 rounding modes, rnd must
   initially be MPFR_RNDZ, y = RNDZ(f(x)), and sb is 0 if f(x) is exact,
   1 if f(x) is inexact (in which case, y must be a regular number,
   i.e. not the result of an overflow or an underflow); the successive
   rounding modes are:
     * MPFR_RNDZ, MPFR_RNDD, MPFR_RNDA, MPFR_RNDU, MPFR_RNDN for positive y;
     * MPFR_RNDZ, MPFR_RNDU, MPFR_RNDA, MPFR_RNDD, MPFR_RNDN for negative y;
   for the last test MPFR_RNDN, the target precision is decreased by 1 in
   order to be able to deduce the result (anyway, for a hard-to-round case
   in directed rounding modes, if yprec is chosen to be minimum precision
   preserving this hard-to-round case, then one has a hard-to-round case
   in round-to-nearest for precision yprec-1). If the target precision was
   MPFR_PREC_MIN, then skip the MPFR_RNDN test; thus to test exact special
   cases, use a target precision larger than MPFR_PREC_MIN.
   Note: if y is a regular number, sb corresponds to the sticky bit when
   considering round-to-nearest with precision yprec-1.
   As examples of use, see the calls to test5rm from the data_check and
   bad_cases functions. */
static void
test5rm (int (*fct) (FLIST), mpfr_srcptr x, mpfr_ptr y, mpfr_ptr z,
         mpfr_rnd_t rnd, int sb, const char *name)
{
  mpfr_prec_t yprec = MPFR_PREC (y);
  mpfr_rnd_t rndnext = MPFR_RND_MAX;  /* means uninitialized */
  int expected_inex = INT_MIN;

  MPFR_ASSERTN (sb == -1 || rnd == MPFR_RNDZ);
  mpfr_set_prec (z, yprec);
  while (1)
    {
      int inex;

      MPFR_ASSERTN (rnd != MPFR_RND_MAX);
      inex = fct (z, x, rnd);
      if (sb == -1)
        expected_inex = inex;  /* not checked */
      else if (rnd != MPFR_RNDN)
        expected_inex =
          sb == 0 ? 0 : MPFR_IS_LIKE_RNDD (rnd, MPFR_SIGN (y)) ? -1 : 1;
      MPFR_ASSERTN (expected_inex != INT_MIN);
      if (!(SAME_VAL (y, z) && SAME_SIGN (inex, expected_inex)))
        {
          printf ("test5rm: error for %s with xprec=%lu, yprec=%lu, rnd=%s\n"
                  "x = ",
                  name, (unsigned long) MPFR_PREC (x), (unsigned long) yprec,
                  mpfr_print_rnd_mode (rnd));
          mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
          printf ("\nexpected ");
          mpfr_out_str (stdout, 16, 0, y, MPFR_RNDN);
          printf ("\ngot      ");
          mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
          printf ("\n");
          if (sb != -1)
            printf ("Expected inex = %d, got %d\n", expected_inex, inex);
          exit (1);
        }

      if (sb == -1 || rnd == MPFR_RNDN)
        break;
      else if (rnd == MPFR_RNDZ)
        {
          rnd = MPFR_IS_NEG (y) ? MPFR_RNDU : MPFR_RNDD;
          rndnext = MPFR_RNDA;
        }
      else
        {
          rnd = rndnext;
          if (rnd == MPFR_RNDA)
            {
              if (sb)
                mpfr_nexttoinf (y);
              rndnext = MPFR_IS_NEG (y) ? MPFR_RNDD : MPFR_RNDU;
            }
          else if (rndnext != MPFR_RNDN)
            rndnext = MPFR_RNDN;
          else
            {
              if (yprec == MPFR_PREC_MIN)
                break;
              /* If sb = 1, then mpfr_nexttoinf was called on y for the
                 MPFR_RNDA test, i.e. y = RNDA(yprec,f(x)); we use MPFR_RNDZ
                 since one has the property RNDN(p,w) = RNDZ(p,RNDA(p+1,w))
                 when w is not a midpoint in precision p. If sb = 0, then
                 y = f(x), so that RNDN(yprec-1,f(x)) = RNDN(yprec-1,y). */
              inex = mpfr_prec_round (y, --yprec, sb ? MPFR_RNDZ : MPFR_RNDN);
              expected_inex = sb ?
                MPFR_SIGN (y) * (inex == 0 ? 1 : -1) : inex;
              mpfr_set_prec (z, yprec);
            }
        }
    }
}

/* Check data in file f for function foo, with name 'name'.
   Each line consists of the file f one:

   xprec yprec rnd x y

   where:

   xprec is the input precision
   yprec is the output precision
   rnd is the rounding mode (n, z, u, d, a, Z, *)
   x is the input (hexadecimal format)
   y is the expected output (hexadecimal format) for foo(x) with rounding rnd

   If rnd is Z, then y is the expected output in round-toward-zero and
   it is assumed to be inexact; the four directed rounding modes are
   tested, and the round-to-nearest mode is tested in precision yprec-1.
   See details in the description of test5rm above.

   If rnd is *, y must be an exact case (possibly a special case).
   All the rounding modes are tested and the ternary value is checked
   (it must be 0).
 */
void
data_check (const char *f, int (*foo) (FLIST), const char *name)
{
  FILE *fp;
  long int xprec, yprec;  /* not mpfr_prec_t because of the fscanf */
  mpfr_t x, y, z;
  mpfr_rnd_t rnd;
  char r;
  int c;

  fp = fopen (f, "r");
  if (fp == NULL)
    fp = src_fopen (f, "r");
  if (fp == NULL)
    {
      char *v = (char *) MPFR_VERSION_STRING;

      /* In the '-dev' versions, assume that the data file exists and
         return an error if the file cannot be opened to make sure
         that such failures are detected. */
      while (*v != '\0')
        v++;
      if (v[-4] == '-' && v[-3] == 'd' && v[-2] == 'e' && v[-1] == 'v')
        {
          printf ("Error: unable to open file '%s'\n", f);
          exit (1);
        }
      else
        return;
    }

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (z);

  while (!feof (fp))
    {
      /* skip whitespace, for consistency */
      if (fscanf (fp, " ") == EOF)
        {
          if (ferror (fp))
            {
              perror ("data_check");
              exit (1);
            }
          else
            break;  /* end of file */
        }

      if ((c = getc (fp)) == EOF)
        {
          if (ferror (fp))
            {
              perror ("data_check");
              exit (1);
            }
          else
            break;  /* end of file */
        }

      if (c == '#') /* comment: read entire line */
        {
          do
            {
              c = getc (fp);
            }
          while (!feof (fp) && c != '\n');
        }
      else
        {
          ungetc (c, fp);

          c = fscanf (fp, "%ld %ld %c", &xprec, &yprec, &r);
          MPFR_ASSERTN (MPFR_PREC_COND (xprec));
          MPFR_ASSERTN (MPFR_PREC_COND (yprec));
          if (c == EOF)
            {
              perror ("data_check");
              exit (1);
            }
          else if (c != 3)
            {
              printf ("Error: corrupted line in file '%s'\n", f);
              exit (1);
            }

          switch (r)
            {
            case 'n':
              rnd = MPFR_RNDN;
              break;
            case 'z': case 'Z':
              rnd = MPFR_RNDZ;
              break;
            case 'u':
              rnd = MPFR_RNDU;
              break;
            case 'd':
              rnd = MPFR_RNDD;
              break;
            case '*':
              rnd = MPFR_RND_MAX; /* non-existing rounding mode */
              break;
            default:
              printf ("Error: unexpected rounding mode"
                      " in file '%s': %c\n", f, (int) r);
              exit (1);
            }
          mpfr_set_prec (x, xprec);
          mpfr_set_prec (y, yprec);
          if (mpfr_inp_str (x, fp, 0, MPFR_RNDN) == 0)
            {
              printf ("Error: corrupted argument in file '%s'\n", f);
              exit (1);
            }
          if (mpfr_inp_str (y, fp, 0, MPFR_RNDN) == 0)
            {
              printf ("Error: corrupted result in file '%s'\n", f);
              exit (1);
            }
          if (getc (fp) != '\n')
            {
              printf ("Error: result not followed by \\n in file '%s'\n", f);
              exit (1);
            }
          /* Skip whitespace, in particular at the end of the file. */
          if (fscanf (fp, " ") == EOF && ferror (fp))
            {
              perror ("data_check");
              exit (1);
            }
          if (r == '*')
            test5rm (foo, x, y, z, MPFR_RNDZ, 0, name);
          else
            test5rm (foo, x, y, z, rnd, r == 'Z' ? 1 : -1, name);
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);

  fclose (fp);
}

/* Test n random bad cases. A precision py in [pymin,pymax] and
 * a number y of precision py are chosen randomly. One computes
 * x = inv(y) in precision px = py + psup (rounded to nearest).
 * Then (in general), y is a bad case for fct in precision py (in
 * the directed rounding modes, but also in the rounding-to-nearest
 * mode for some lower precision: see data_check).
 * fct, inv, name: data related to the function.
 * pos, emin, emax: arguments for tests_default_random.
 * For debugging purpose (e.g. in case of crash or infinite loop),
 * you can set the MPFR_DEBUG_BADCASES environment variable to 1 in
 * order to output information about the tested worst cases. You can
 * also enable logging (when supported), but this may give too much
 * information.
 */
void
bad_cases (int (*fct)(FLIST), int (*inv)(FLIST), const char *name,
           int pos, mpfr_exp_t emin, mpfr_exp_t emax,
           mpfr_prec_t pymin, mpfr_prec_t pymax, mpfr_prec_t psup,
           int n)
{
  mpfr_t x, y, z;
  char *dbgenv;
  int cnt = 0, i, dbg;
  mpfr_exp_t old_emin, old_emax;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  dbgenv = getenv ("MPFR_DEBUG_BADCASES");
  dbg = dbgenv != 0 ? atoi (dbgenv) : 0;  /* debug level */
  mpfr_inits2 (MPFR_PREC_MIN, x, y, z, (mpfr_ptr) 0);
  for (i = 0; i < n; i++)
    {
      mpfr_prec_t px, py, pz;
      int inex_inv, inex, sb;

      if (dbg)
        printf ("bad_cases: %s, i = %d\n", name, i);
      py = pymin + (randlimb () % (pymax - pymin + 1));
      mpfr_set_prec (y, py);
      tests_default_random (y, pos, emin, emax, 0);
      if (dbg)
        {
          printf ("bad_cases: yprec =%6ld, y = ", (long) py);
          mpfr_out_str (stdout, 16, 0, y, MPFR_RNDN);
          printf ("\n");
        }
      px = py + psup;
      mpfr_set_prec (x, px);
      if (dbg)
        printf ("bad_cases: xprec =%6ld\n", (long) px);
      mpfr_clear_flags ();
      inex_inv = inv (x, y, MPFR_RNDN);
      if (mpfr_nanflag_p () || mpfr_overflow_p () || mpfr_underflow_p ())
        {
          if (dbg)
            printf ("bad_cases: no normal inverse\n");
          goto next_i;
        }
      if (dbg > 1)
        {
          printf ("bad_cases: x = ");
          mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
          printf ("\n");
        }
      pz = px;
      do
        {
          pz += 32;
          mpfr_set_prec (z, pz);
          sb = fct (z, x, MPFR_RNDN) != 0;
          if (!sb)
            {
              if (dbg)
                {
                  printf ("bad_cases: exact case z = ");
                  mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
                  printf ("\n");
                }
              if (inex_inv)
                {
                  if (dbg)
                    printf ("bad_cases: f exact while f^(-1) inexact\n");
                  goto does_not_match;
                }
              inex = 0;
              break;
            }
          if (dbg)
            {
              if (dbg > 1)
                {
                  printf ("bad_cases: %s(x) ~= ", name);
                  mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
                }
              else
                {
                  printf ("bad_cases:   [MPFR_RNDZ]  ~= ");
                  mpfr_out_str (stdout, 16, 40, z, MPFR_RNDZ);
                }
              printf ("\n");
            }
          inex = mpfr_prec_round (z, py, MPFR_RNDN);
          if (mpfr_nanflag_p () || mpfr_overflow_p () || mpfr_underflow_p ()
              || ! mpfr_equal_p (z, y))
            {
              /* This may occur when psup is not large enough: evaluating
                 x = (f^(-1))(y) then z = f(x) may not give back y if the
                 precision of x is too small. */
            does_not_match:
              if (dbg)
                {
                  printf ("bad_cases: inverse doesn't match for %s\ny = ",
                          name);
                  mpfr_out_str (stdout, 16, 0, y, MPFR_RNDN);
                  printf ("\nz = ");
                  mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
                  printf ("\n");
                }
              goto next_i;
            }
        }
      while (inex == 0);
      /* We really have a bad case (or some special case). */
      if (mpfr_zero_p (z))
        {
          /* This can occur on tlog (GMP_CHECK_RANDOMIZE=1630879377004032
             in r14570, 2021-09-07):
             y = -0, giving x = 1 and z = 0. We have y = z, but here,
             y and z have different signs. Since test5rm will test f(x)
             and its sign (in particular for 0), we need to take the
             sign of f(x), i.e. of z.
             Note: To avoid this special case, one might want to detect and
             ignore y = 0 (of any sign) when taking the random number above,
             as this case should be redundant with some other tests. */
          mpfr_set (y, z, MPFR_RNDN);
          py = MPFR_PREC_MIN;
        }
      else
        {
          do
            py--;
          while (py >= MPFR_PREC_MIN &&
                 mpfr_prec_round (z, py, MPFR_RNDZ) == 0);
          py++;
        }
      /* py is now the smallest output precision such that we have
         a bad case in the directed rounding modes. */
      if (mpfr_prec_round (y, py, MPFR_RNDZ) != 0)
        {
          printf ("Internal error for i = %d\n", i);
          exit (1);
        }
      if ((inex > 0 && MPFR_IS_POS (z)) ||
          (inex < 0 && MPFR_IS_NEG (z)))
        {
          mpfr_nexttozero (y);
          if (mpfr_zero_p (y))
            goto next_i;
        }
      if (dbg)
        {
          printf ("bad_cases: yprec =%6ld, y = ", (long) py);
          mpfr_out_str (stdout, 16, 0, y, MPFR_RNDN);
          printf ("\n");
        }
      /* Note: y is now the expected result rounded toward zero. */
      test5rm (fct, x, y, z, MPFR_RNDZ, sb, name);
      cnt++;
    next_i:
      /* In case the exponent range has been changed by
         tests_default_random()... */
      set_emin (old_emin);
      set_emax (old_emax);
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);

  if (dbg)
    printf ("bad_cases: %d bad cases over %d generated values for %s\n",
            cnt, n, name);

  if (getenv ("MPFR_CHECK_BADCASES") && n - cnt > n/10)
    {
      printf ("bad_cases: too few bad cases (%d over %d generated values)"
              " for %s\n", cnt, n, name);
      exit (1);
    }
}

void
flags_out (unsigned int flags)
{
  int none = 1;

  if (flags & MPFR_FLAGS_UNDERFLOW)
    none = 0, printf (" underflow");
  if (flags & MPFR_FLAGS_OVERFLOW)
    none = 0, printf (" overflow");
  if (flags & MPFR_FLAGS_NAN)
    none = 0, printf (" nan");
  if (flags & MPFR_FLAGS_INEXACT)
    none = 0, printf (" inexact");
  if (flags & MPFR_FLAGS_ERANGE)
    none = 0, printf (" erange");
  if (none)
    printf (" none");
  printf (" (%u)\n", flags);
}

static void
abort_called (int x)
{
  /* Ok, abort has been called */
  exit (0);
}

/* This function has to be called for a test
   that will call the abort function */
void
tests_expect_abort (void)
{
#if defined(HAVE_SIGACTION)
  struct sigaction act;
  int ret;

  memset (&act, 0, sizeof act);
  act.sa_handler = abort_called;
  ret = sigaction (SIGABRT, &act, NULL);
  if (ret != 0)
    {
      /* Can't register error handler: Skip test */
      exit (77);
    }
#elif defined(HAVE_SIGNAL)
  signal (SIGABRT, abort_called);
#else
  /* Can't register error handler: Skip test */
  exit (77);
#endif
}

/* Guess whether the test runs within Valgrind.
   Note: This should work at least under Linux and Solaris.
   If need be, support for macOS (with DYLD_INSERT_LIBRARIES) and
   i386 FreeBSD on amd64 (with LD_32_PRELOAD) could be added; thanks
   to Paul Floyd for the information.
   Up-to-date information should be found at
   <https://stackoverflow.com/a/62364698/3782797>. */
int
tests_run_within_valgrind (void)
{
  char *p;

  p = getenv ("LD_PRELOAD");
  if (p == NULL)
    return 0;
  return (strstr (p, "/valgrind/") != NULL ||
          strstr (p, "/vgpreload") != NULL);
}
