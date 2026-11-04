dnl  MPFR specific autoconf macros

dnl  Copyright 2000, 2002-2025 Free Software Foundation, Inc.
dnl  Contributed by the Pascaline and Caramba projects, INRIA.
dnl
dnl  This file is part of the GNU MPFR Library.
dnl
dnl  The GNU MPFR Library is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License as
dnl  published by the Free Software Foundation; either version 3 of the
dnl  License, or (at your option) any later version.
dnl
dnl  The GNU MPFR Library is distributed in the hope that it will be useful, but
dnl  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
dnl  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
dnl  License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public License
dnl  along with the GNU MPFR Library; see the file COPYING.LESSER.
dnl  If not, see <https://www.gnu.org/licenses/>.

dnl  autoconf 2.60 is necessary because of the use of AC_PROG_SED.
dnl  The following line allows the autoconf wrapper (when installed)
dnl  to work as expected.
dnl  If you change the required version, please update README.dev too!
AC_PREREQ(2.60)

dnl ------------------------------------------------------------
dnl You must put in MPFR_CONFIGS everything that configures MPFR except:
dnl   - Everything dealing with CC and CFLAGS in particular the ABI
dnl     but the IEEE-754 specific flags must be set here.
dnl   - Tests that depend on gmp.h (see MPFR_CHECK_DBL2INT_BUG as an example:
dnl     a function needs to be defined and called in configure.ac).
dnl   - GMP's linkage.
dnl   - Libtool stuff.
dnl   - Handling of special arguments of MPFR's configure.
AC_DEFUN([MPFR_CONFIGS],
[
AC_REQUIRE([AC_OBJEXT])
AC_REQUIRE([MPFR_CHECK_LIBM])
AC_REQUIRE([MPFR_CHECK_LIBQUADMATH])
AC_REQUIRE([AC_CANONICAL_HOST])

dnl Features for the MPFR shared cache. This needs to be done
dnl quite early since this may change CC, CFLAGS and LIBS, which
dnl may affect the other tests.

if test "$enable_shared_cache" = yes; then

dnl Prefer ISO C11 threads (as in mpfr-thread.h).
  MPFR_CHECK_C11_THREAD()

  if test "$mpfr_c11_thread_ok" != yes; then
dnl Check for POSIX threads. Since the AX_PTHREAD macro is not standard
dnl (it is provided by autoconf-archive), we need to detect whether it
dnl is left unexpanded, otherwise the configure script won't fail and
dnl "make distcheck" won't give any error, yielding buggy tarballs!
dnl The \b is necessary to avoid an error with recent ax_pthread.m4
dnl (such as with Debian's autoconf-archive 20160320-1), which contains
dnl AX_PTHREAD_ZOS_MISSING, etc. It is not documented, but see:
dnl   https://lists.gnu.org/archive/html/autoconf/2015-03/msg00011.html
dnl
dnl AX_PTHREAD is now in the MPFR repository (m4/ax_pthread.m4), but we
dnl should leave this test, just in case there is some issue loading it
dnl (or any other reason).
dnl
dnl Note: each time a change is done in m4_pattern_forbid, autogen.sh
dnl should be tested with and without ax_pthread.m4 availability (in
dnl the latter case, there should be an error).
    m4_pattern_forbid([AX_PTHREAD\b])
    AX_PTHREAD([])
    if test "$ax_pthread_ok" = yes; then
      CC="$PTHREAD_CC"
      CXX="$PTHREAD_CXX"
      CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
      CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
      LIBS="$LIBS $PTHREAD_LIBS"
dnl Do a compilation test, as this is currently not done by AX_PTHREAD.
dnl Moreover, MPFR needs pthread_rwlock_t, which is conditionally defined
dnl in glibc's bits/pthreadtypes.h (via <pthread.h>), not sure why...
      AC_MSG_CHECKING([for pthread_rwlock_t])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <pthread.h>
]], [[
pthread_rwlock_t lock; (void) lock;
]])],
        [AC_MSG_RESULT([yes])
         mpfr_pthread_ok=yes],
        [AC_MSG_RESULT([no])
         mpfr_pthread_ok=no])
    else
      mpfr_pthread_ok=no
    fi
  fi

  AC_MSG_CHECKING(if shared cache can be supported)
  if test "$mpfr_c11_thread_ok" = yes; then
    AC_MSG_RESULT([yes, with ISO C11 threads])
  elif test "$mpfr_pthread_ok" = yes; then
    AC_MSG_RESULT([yes, with pthread])
  else
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([shared cache needs C11 threads or pthread support])
  fi

fi

dnl End of features for the MPFR shared cache.

AC_CHECK_HEADER([limits.h],, AC_MSG_ERROR([limits.h not found]))
AC_CHECK_HEADER([float.h],,  AC_MSG_ERROR([float.h not found]))
AC_CHECK_HEADER([string.h],, AC_MSG_ERROR([string.h not found]))

dnl Check for locales
AC_CHECK_HEADERS([locale.h])

dnl Check for wide characters (wchar_t and wint_t)
AC_CHECK_HEADERS([wchar.h])

dnl Check for stdargs
AC_CHECK_HEADER([stdarg.h],[AC_DEFINE([HAVE_STDARG],1,[Define if stdarg])],
  [AC_CHECK_HEADER([varargs.h],,
    AC_MSG_ERROR([stdarg.h or varargs.h not found]))])

dnl sys/fpu.h - MIPS specific
AC_CHECK_HEADERS([sys/fpu.h])

dnl Android has a <locale.h>, but not the following members.
AC_CHECK_MEMBERS([struct lconv.decimal_point, struct lconv.thousands_sep],,,
  [#include <locale.h>])

dnl Check how to get `alloca'
AC_FUNC_ALLOCA

dnl Define uintptr_t if not available.
AC_TYPE_UINTPTR_T

dnl va_copy macro
AC_MSG_CHECKING([how to copy va_list])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdarg.h>
]], [[
   va_list ap1, ap2;
   va_copy(ap1, ap2);
]])], [
   AC_MSG_RESULT([va_copy])
   AC_DEFINE(HAVE_VA_COPY)
], [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdarg.h>
]], [[
   va_list ap1, ap2;
   __va_copy(ap1, ap2);
]])], [AC_DEFINE([HAVE___VA_COPY]) AC_MSG_RESULT([__va_copy])],
   [AC_MSG_RESULT([memcpy])])])

dnl Options like -Werror can yield a failure as AC_CHECK_FUNCS uses some
dnl fixed prototype for function detection, generally incompatible with
dnl the standard one. For instance, with GCC, if the function is defined
dnl as a builtin, one can get a "conflicting types for built-in function"
dnl error [-Werror=builtin-declaration-mismatch], even though the header
dnl is not included; when these functions were tested, this occurred with
dnl memmove and memset. Even though no failures are known with the tested
dnl functions at this time, let's remove options starting with "-Werror"
dnl since failures may still be possible.
dnl
dnl The gettimeofday function is not defined with MinGW.
saved_CFLAGS="$CFLAGS"
CFLAGS=`[echo " $CFLAGS" | $SED 's/ -Werror[^ ]*//g']`
AC_CHECK_FUNCS([setlocale gettimeofday signal])
CFLAGS="$saved_CFLAGS"

dnl We cannot use AC_CHECK_FUNCS on sigaction, because while this
dnl function may be provided by the C library, its prototype and
dnl associated structure may not be available, e.g. when compiling
dnl with "gcc -std=c99".
AC_MSG_CHECKING(for sigaction and its associated structure)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <signal.h>
static int f (int (*func)(int, const struct sigaction *, struct sigaction *))
{ return 0; }
]], [[
 return f(sigaction);
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_SIGACTION, 1,
    [Define if you have a working sigaction function.])
],[AC_MSG_RESULT(no)])

dnl check for long long
AC_CHECK_TYPE([long long int],
   AC_DEFINE(HAVE_LONG_LONG, 1, [Define if compiler supports long long]),,)

dnl intmax_t is C99
AC_CHECK_TYPES([intmax_t])
if test "$ac_cv_type_intmax_t" = yes; then
  AC_CACHE_CHECK([for working INTMAX_MAX], mpfr_cv_have_intmax_max, [
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I$srcdir/src -DMPFR_NEED_INTMAX_H"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [[#include "mpfr-intmax.h"]],
        [[intmax_t x = INTMAX_MAX; (void) x;]]
      )],
      mpfr_cv_have_intmax_max=yes, mpfr_cv_have_intmax_max=no)
    CPPFLAGS="$saved_CPPFLAGS"
  ])
  if test "$mpfr_cv_have_intmax_max" = "yes"; then
    AC_DEFINE(MPFR_HAVE_INTMAX_MAX,1,[Define if you have a working INTMAX_MAX.])
  fi
fi

AC_CHECK_TYPE( [union fpc_csr],
   AC_DEFINE(HAVE_FPC_CSR,1,[Define if union fpc_csr is available]), ,
[
#if HAVE_SYS_FPU_H
#  include <sys/fpu.h>
#endif
])

dnl Check for _Noreturn function specifier (ISO C11)
AC_CACHE_CHECK([for _Noreturn], mpfr_cv_have_noreturn, [
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[_Noreturn void foo(int);]])],
    mpfr_cv_have_noreturn=yes, mpfr_cv_have_noreturn=no)
])
if test "$mpfr_cv_have_noreturn" = "yes"; then
  AC_DEFINE(MPFR_HAVE_NORETURN,1,[Define if the _Noreturn function specifier is supported.])
fi

dnl Check for __builtin_unreachable
AC_CACHE_CHECK([for __builtin_unreachable], mpfr_cv_have_builtin_unreachable,
[
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
      [[int x;]],
      [[if (x) __builtin_unreachable(); ]]
    )],
    mpfr_cv_have_builtin_unreachable=yes,
    mpfr_cv_have_builtin_unreachable=no)
])
if test "$mpfr_cv_have_builtin_unreachable" = "yes"; then
  AC_DEFINE(MPFR_HAVE_BUILTIN_UNREACHABLE, 1,
   [Define if the __builtin_unreachable GCC built-in is supported.])
fi

dnl Check for attribute constructor and destructor
MPFR_CHECK_CONSTRUCTOR_ATTR()

dnl Check for fesetround
AC_CACHE_CHECK([for fesetround], mpfr_cv_have_fesetround, [
saved_LIBS="$LIBS"
LIBS="$LIBS $MPFR_LIBM"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <fenv.h>]], [[fesetround(FE_TONEAREST);]])],
  mpfr_cv_have_fesetround=yes, mpfr_cv_have_fesetround=no)
LIBS="$saved_LIBS"
])
if test "$mpfr_cv_have_fesetround" = "yes"; then
  AC_DEFINE(MPFR_HAVE_FESETROUND,1,[Define if you have the `fesetround' function via the <fenv.h> header file.])
fi

dnl Check for gcc float-conversion bug; if need be, -ffloat-store is used to
dnl force the conversion to the destination type when a value is stored to
dnl a variable (see ISO C99 standard 5.1.2.3#13, 6.3.1.5#2, 6.3.1.8#2). This
dnl is important concerning the exponent range. Note that this doesn't solve
dnl the double-rounding problem.
if test -n "$GCC"; then
  AC_CACHE_CHECK([for gcc float-conversion bug], mpfr_cv_gcc_floatconv_bug, [
  saved_LIBS="$LIBS"
  LIBS="$LIBS $MPFR_LIBM"
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <float.h>
#ifdef MPFR_HAVE_FESETROUND
#include <fenv.h>
#endif
static double get_max (void);
int main (void) {
  double x = 0.5;
  double y;
  int i;
  for (i = 1; i <= 11; i++)
    x *= x;
  if (x != 0)
    return 1;
#ifdef MPFR_HAVE_FESETROUND
  /* Useful test for the G4 PowerPC */
  fesetround(FE_TOWARDZERO);
  x = y = get_max ();
  x *= 2.0;
  if (x != y)
    return 1;
#endif
  return 0;
}
static double get_max (void) { static volatile double d = DBL_MAX; return d; }
  ]])],
     [mpfr_cv_gcc_floatconv_bug="no"],
     [mpfr_cv_gcc_floatconv_bug="yes, use -ffloat-store"],
     [mpfr_cv_gcc_floatconv_bug="cannot test, use -ffloat-store"])
  LIBS="$saved_LIBS"
  ])
  if test "$mpfr_cv_gcc_floatconv_bug" != "no"; then
    CFLAGS="$CFLAGS -ffloat-store"
  fi
fi

dnl Check if subnormal numbers are supported.
dnl For the binary64 format, the smallest normal number is 2^(-1022).
dnl For the binary32 format, the smallest normal number is 2^(-126).
dnl Do not use the corresponding HAVE_SUBNORM_* macros as they
dnl are not available when cross-compiling. For the tests, use
dnl the have_subnorm_* functions if need be.
dnl Note: "volatile" is needed to avoid -ffast-math optimizations
dnl (default in icx 2021.2.0, which also sets the FZ and DAZ bits
dnl of the x86-64 MXCSR register to disregard subnormals).
AC_CACHE_CHECK([for subnormal double-precision numbers],
mpfr_cv_have_subnorm_dbl, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
int main (void) {
  volatile double x = 2.22507385850720138309e-308, y;
  y = x / 2.0;
  fprintf (stderr, "%e\n", y);
  return 2.0 * y != x;
}
]])],
   [mpfr_cv_have_subnorm_dbl="yes"],
   [mpfr_cv_have_subnorm_dbl="no"],
   [mpfr_cv_have_subnorm_dbl="cannot test, assume no"])
])
if test "$mpfr_cv_have_subnorm_dbl" = "yes"; then
  AC_DEFINE(HAVE_SUBNORM_DBL, 1,
   [Define if the double type fully supports subnormals.])
fi
AC_CACHE_CHECK([for subnormal single-precision numbers],
mpfr_cv_have_subnorm_flt, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
int main (void) {
  volatile float x = 1.17549435082229e-38, y;
  y = x / 2.0f;
  fprintf (stderr, "%e\n", (double) y);
  return 2.0f * y != x;
}
]])],
   [mpfr_cv_have_subnorm_flt="yes"],
   [mpfr_cv_have_subnorm_flt="no"],
   [mpfr_cv_have_subnorm_flt="cannot test, assume no"])
])
if test "$mpfr_cv_have_subnorm_flt" = "yes"; then
  AC_DEFINE(HAVE_SUBNORM_FLT, 1,
   [Define if the float type fully supports subnormals.])
fi

dnl Check if signed zeros are supported. Note: the test will fail
dnl if the division by 0 generates a trap.
AC_CACHE_CHECK([for signed zeros], mpfr_cv_have_signedz, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main (void) {
  return 1.0 / 0.0 == 1.0 / -0.0;
}
]])],
   [mpfr_cv_have_signedz="yes"],
   [mpfr_cv_have_signedz="no"],
   [mpfr_cv_have_signedz="cannot test, assume no"])
])
if test "$mpfr_cv_have_signedz" = "yes"; then
  AC_DEFINE(HAVE_SIGNEDZ,1,[Define if signed zeros are supported.])
fi

dnl Check the FP division by 0 fails (e.g. on a non-IEEE-754 platform).
dnl In such a case, MPFR_ERRDIVZERO is defined to disable the tests
dnl involving a FP division by 0.
dnl For the developers: to check whether all these tests are disabled,
dnl configure MPFR with "-DMPFR_TESTS_FPE_DIV -DMPFR_ERRDIVZERO".
AC_CACHE_CHECK([if the FP division by 0 fails], mpfr_cv_errdivzero, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main (void) {
  volatile double d = 0.0, x;
  x = 0.0 / d;
  x = 1.0 / d;
  (void) x;
  return 0;
}
]])],
   [mpfr_cv_errdivzero="no"],
   [mpfr_cv_errdivzero="yes"],
   [mpfr_cv_errdivzero="cannot test, assume no"])
])
if test "$mpfr_cv_errdivzero" = "yes"; then
  AC_DEFINE(MPFR_ERRDIVZERO,1,[Define if the FP division by 0 fails.])
  AC_MSG_WARN([The floating-point division by 0 fails instead of])
  AC_MSG_WARN([returning a special value: NaN or infinity. Tests])
  AC_MSG_WARN([involving a FP division by 0 will be disabled.])
fi

dnl Check whether NAN != NAN (as required by the IEEE-754 standard,
dnl but not by the ISO C standard). For instance, this is false with
dnl MIPSpro 7.3.1.3m under IRIX64. By default, assume this is true.
dnl Note that this test may not detect all issues. For instance, with
dnl icx 2021.2.0 (and default fast-math), the result depends on whether
dnl the identifier has internal or external linkage:
dnl   https://community.intel.com/t5/Intel-oneAPI-Base-Toolkit/icx-2021-2-0-bug-incorrect-NaN-comparison-using-an-identifier/m-p/1286869
dnl TODO: change "NAN == NAN" to "NaN is supported" and rename
dnl the MPFR_NANISNAN macro?
AC_CACHE_CHECK([if NAN == NAN], mpfr_cv_nanisnan, [
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <math.h>
#ifndef NAN
# define NAN (0.0/0.0)
#endif
int main (void) {
  double d;
  d = NAN;
  return d != d;
}
]])],
   [mpfr_cv_nanisnan="yes"],
   [mpfr_cv_nanisnan="no"],
   [mpfr_cv_nanisnan="cannot test, assume no"])
])
if test "$mpfr_cv_nanisnan" = "yes"; then
  AC_DEFINE(MPFR_NANISNAN,1,[Define if NAN == NAN.])
  AC_MSG_WARN([The test NAN != NAN is false. The probable reason is that])
  AC_MSG_WARN([your compiler optimizes floating-point expressions in an])
  AC_MSG_WARN([unsafe way because some option, such as -ffast-math or])
  AC_MSG_WARN([-fast (depending on the compiler), has been used.  You])
  AC_MSG_WARN([should NOT use such an option, otherwise MPFR functions])
  AC_MSG_WARN([such as mpfr_get_d and mpfr_set_d may return incorrect])
  AC_MSG_WARN([results on special FP numbers (e.g. NaN or signed zeros).])
  AC_MSG_WARN([If you did not use such an option, please send us a bug])
  AC_MSG_WARN([report so that we can try to find a workaround for your])
  AC_MSG_WARN([platform and/or document the behavior.])
fi

dnl Check if the chars '0' to '9', 'a' to 'z', and 'A' to 'Z' are
dnl consecutive values.
dnl The const is necessary with GCC's "-Wwrite-strings -Werror".
AC_MSG_CHECKING([if charset has consecutive values])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
const char *number = "0123456789";
const char *lower  = "abcdefghijklmnopqrstuvwxyz";
const char *upper  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
]],[[
 int i;
 unsigned char *p;
 for (p = (unsigned char*) number, i = 0; i < 9; i++)
   if ( (*p)+1 != *(p+1) ) return 1;
 for (p = (unsigned char*) lower, i = 0; i < 25; i++)
   if ( (*p)+1 != *(p+1) ) return 1;
 for (p = (unsigned char*) upper, i = 0; i < 25; i++)
   if ( (*p)+1 != *(p+1) ) return 1;
]])], [AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_DEFINE(MPFR_NO_CONSECUTIVE_CHARSET,1,[Charset is not consecutive])
], [AC_MSG_RESULT(cannot test)])

dnl Must be checked with the LIBM
dnl but we don't want to add the LIBM to MPFR dependency.
dnl Can't use AC_CHECK_FUNCS since the function may be in LIBM but
dnl not exported in math.h
saved_LIBS="$LIBS"
LIBS="$LIBS $MPFR_LIBM"
dnl AC_CHECK_FUNCS([round trunc floor ceil nearbyint])
AC_MSG_CHECKING(for math/round)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
static int f (double (*func)(double)) { return 0; }
]], [[
 return f(round);
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_ROUND, 1,[Have ISO C99 round function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/trunc)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
static int f (double (*func)(double)) { return 0; }
]], [[
 return f(trunc);
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_TRUNC, 1,[Have ISO C99 trunc function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/floor)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
static int f (double (*func)(double)) { return 0; }
]], [[
 return f(floor);
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_FLOOR, 1,[Have ISO C99 floor function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/ceil)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
static int f (double (*func)(double)) { return 0; }
]], [[
 return f(ceil);
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_CEIL, 1,[Have ISO C99 ceil function])
],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for math/nearbyint)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
static int f (double (*func)(double)) { return 0; }
]], [[
 return f(nearbyint);
]])], [
   AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_NEARBYINT, 1,[Have ISO C99 nearbyint function])
],[AC_MSG_RESULT(no)])

LIBS="$saved_LIBS"

dnl Try to determine the format of double
MPFR_C_REALFP_FORMAT(double,)
case $mpfr_cv_c_double_format in
  "IEEE double, big endian"*)
    AC_DEFINE(HAVE_DOUBLE_IEEE_BIG_ENDIAN, 1)
    ;;
  "IEEE double, little endian"*)
    AC_DEFINE(HAVE_DOUBLE_IEEE_LITTLE_ENDIAN, 1)
    ;;
  unknown*)
    ;;
  *)
    AC_MSG_WARN([format of `double' unsupported or not recognized: $mpfr_cv_c_double_format])
    ;;
esac

dnl Now try to determine the format of long double
MPFR_C_REALFP_FORMAT(long double,L)
case $mpfr_cv_c_long_double_format in
  "IEEE double, big endian"*)
    AC_DEFINE(HAVE_LDOUBLE_IS_DOUBLE, 1)
    ;;
  "IEEE double, little endian"*)
    AC_DEFINE(HAVE_LDOUBLE_IS_DOUBLE, 1)
    ;;
  "IEEE extended, little endian"*)
    AC_DEFINE(HAVE_LDOUBLE_IEEE_EXT_LITTLE, 1)
    ;;
  "IEEE extended, big endian"*)
    AC_DEFINE(HAVE_LDOUBLE_IEEE_EXT_BIG, 1)
    ;;
  "IEEE quad, big endian"*)
    AC_DEFINE(HAVE_LDOUBLE_IEEE_QUAD_BIG, 1)
    ;;
  "IEEE quad, little endian"*)
    AC_DEFINE(HAVE_LDOUBLE_IEEE_QUAD_LITTLE, 1)
    ;;
  "possibly double-double, big endian"*)
    AC_MSG_WARN([This format is known on GCC/PowerPC platforms,])
    AC_MSG_WARN([but due to GCC PR26374, we can't test further.])
    AC_MSG_WARN([You can safely ignore this warning, though.])
    AC_DEFINE(HAVE_LDOUBLE_MAYBE_DOUBLE_DOUBLE, 1)
    ;;
  "possibly double-double, little endian"*)
    AC_MSG_WARN([This format is known on GCC/PowerPC platforms,])
    AC_MSG_WARN([but due to GCC PR26374, we can't test further.])
    AC_MSG_WARN([You can safely ignore this warning, though.])
    AC_DEFINE(HAVE_LDOUBLE_MAYBE_DOUBLE_DOUBLE, 1)
    ;;
  unknown*)
    ;;
  *)
    AC_MSG_WARN([format of `long double' unsupported or not recognized: $mpfr_cv_c_long_double_format])
    ;;
esac

dnl Check if thread-local variables are supported.
dnl At least two problems can occur in practice:
dnl 1. The compilation fails, e.g. because the compiler doesn't know
dnl    about the __thread keyword.
dnl 2. The compilation succeeds, but the system doesn't support TLS or
dnl    there is some ld configuration problem. One of the effects can
dnl    be that thread-local variables always evaluate to 0. So, it is
dnl    important to run the test below.
if test "$enable_thread_safe" != no; then
AC_MSG_CHECKING(for TLS support using C11)
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$srcdir/src"
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define MPFR_USE_THREAD_SAFE 1
#define MPFR_USE_C11_THREAD_SAFE 1
#include "mpfr-thread.h"
MPFR_THREAD_ATTR int x = 17;
int main (void) {
  return x != 17;
}
  ]])],
     [AC_MSG_RESULT(yes)
      AC_DEFINE([MPFR_USE_THREAD_SAFE],1,[Build MPFR as thread safe])
      AC_DEFINE([MPFR_USE_C11_THREAD_SAFE],1,[Build MPFR as thread safe using C11])
      tls_c11_support=yes
      enable_thread_safe=yes
     ],
     [AC_MSG_RESULT(no)
     ],
     [AC_MSG_RESULT([cannot test, assume no])
     ])
CPPFLAGS="$saved_CPPFLAGS"

if test "$tls_c11_support" != "yes"
then

 AC_MSG_CHECKING(for TLS support)
 saved_CPPFLAGS="$CPPFLAGS"
 CPPFLAGS="$CPPFLAGS -I$srcdir/src"
 AC_RUN_IFELSE([AC_LANG_SOURCE([[
 #define MPFR_USE_THREAD_SAFE 1
 #include "mpfr-thread.h"
 MPFR_THREAD_ATTR int x = 17;
 int main (void) {
   return x != 17;
 }
   ]])],
      [AC_MSG_RESULT(yes)
       AC_DEFINE([MPFR_USE_THREAD_SAFE],1,[Build MPFR as thread safe])
       enable_thread_safe=yes
      ],
      [AC_MSG_RESULT(no)
       if test "$enable_thread_safe" = yes; then
         AC_MSG_ERROR([please configure with --disable-thread-safe])
       fi
      ],
      [if test "$enable_thread_safe" = yes; then
         AC_MSG_RESULT([cannot test, assume yes])
         AC_DEFINE([MPFR_USE_THREAD_SAFE],1,[Build MPFR as thread safe])
       else
         AC_MSG_RESULT([cannot test, assume no])
       fi
      ])
 CPPFLAGS="$saved_CPPFLAGS"
 fi
fi

if test "$enable_decimal_float" != no; then

dnl Check if decimal floats are available.
dnl For the different cases, we try to use values that will not be returned
dnl by build tools. For instance, 1 must not be used as it can be returned
dnl by gcc or by ld in case of link failure.
dnl Note: We currently reread the 64-bit data memory as a double and compare
dnl it with constants. However, if there is any issue with double, such as
dnl the use of an extended precision, this may fail. Possible solutions:
dnl   1. Use the hex format for the double constants (this format should be
dnl      supported if _Decimal64 is).
dnl   2. Use more precision in the double constants (more decimal digits),
dnl      just in case.
dnl   3. Use uint64_t (or unsigned long long, though this type might not be
dnl      on 64 bits) instead of or in addition to the test on double.
dnl   4. Use an array of 8 unsigned char's instead of or in addition to the
dnl      test on double, considering the 2 practical cases of endianness.
  AC_MSG_CHECKING(if compiler knows _Decimal64)
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[_Decimal64 x;]])],
    [AC_MSG_RESULT(yes)
     AC_MSG_CHECKING(decimal float format)
     AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
]], [[
volatile _Decimal64 x = 1;
union { double d; _Decimal64 d64; } y;
if (x != x) return 83;
y.d64 = 1234567890123456.0dd;
return y.d == 0.14894469406741037E-123 ? 80 :
       y.d == 0.59075095508629822E-68  ? 81 : 82;
]])], [AC_MSG_RESULT(internal error)
       AC_MSG_FAILURE(unexpected exit status 0)],
      [d64_exit_status=$?
       case "$d64_exit_status" in
         80) AC_MSG_RESULT(DPD)
             if test "$enable_decimal_float" = bid; then
               AC_MSG_ERROR([encoding mismatch (BID requested).])
             fi
             if test "$enable_decimal_float" != generic; then
               enable_decimal_float=dpd
             fi ;;
         81) AC_MSG_RESULT(BID)
             if test "$enable_decimal_float" = dpd; then
               AC_MSG_ERROR([encoding mismatch (DPD requested).])
             fi
             if test "$enable_decimal_float" != generic; then
               enable_decimal_float=bid
             fi ;;
         82) AC_MSG_RESULT(neither DPD nor BID)
             if test "$enable_decimal_float" = dpd; then
               AC_MSG_ERROR([encoding mismatch (DPD requested).])
             fi
             if test "$enable_decimal_float" = bid; then
               AC_MSG_ERROR([encoding mismatch (BID requested).])
             fi
             enable_decimal_float=generic
  AC_MSG_WARN([The _Decimal64 encoding is non-standard or there was an])
  AC_MSG_WARN([issue with its detection.  The generic code will be used.])
  AC_MSG_WARN([Please do not forget to test with `make check'.])
  AC_MSG_WARN([In case of failure of a decimal test, you should rebuild])
  AC_MSG_WARN([MPFR without --enable-decimal-float.]) ;;
         *)  AC_MSG_RESULT(error (exit status $d64_exit_status))
             case "$enable_decimal_float" in
               yes|bid|dpd|generic) AC_MSG_FAILURE([internal or link error.
Please build MPFR without --enable-decimal-float.]) ;;
               *) enable_decimal_float=no ;;
             esac ;;
       esac],
      [AC_MSG_RESULT(cannot test)
       dnl Since the _Decimal64 type exists, we assume that it is correctly
       dnl supported. The detection of the encoding may still be done at
       dnl compile time. We do not add a configure test for it so that this
       dnl can be done on platforms where configure cannot be used.
       enable_decimal_float=compile-time])
    ],
    [AC_MSG_RESULT(no)
     case "$enable_decimal_float" in
       yes|bid|dpd|generic)
         AC_MSG_FAILURE([compiler doesn't know _Decimal64 (ISO/IEC TR 24732).
Please use another compiler or build MPFR without --enable-decimal-float.]) ;;
       *) enable_decimal_float=no ;;
     esac])
  if test "$enable_decimal_float" != no; then
    AC_DEFINE([MPFR_WANT_DECIMAL_FLOATS],1,
              [Build decimal float functions])
    case "$enable_decimal_float" in
      dpd) AC_DEFINE([DECIMAL_DPD_FORMAT],1,[]) ;;
      bid) AC_DEFINE([DECIMAL_BID_FORMAT],1,[]) ;;
      generic) AC_DEFINE([DECIMAL_GENERIC_CODE],1,[]) ;;
      compile-time) ;;
      *) AC_MSG_ERROR(internal error) ;;
    esac
  fi

dnl Check the bit-field ordering for _Decimal128.
dnl Little endian: sig=0 comb=49400 t0=0 t1=0 t2=0 t3=10
dnl Big endian: sig=0 comb=8 t0=0 t1=0 t2=0 t3=570933248
dnl Note: If the compilation fails, the compiler should exit with
dnl an exit status less than 80.
  AC_MSG_CHECKING(bit-field ordering for _Decimal128)
  AC_RUN_IFELSE([AC_LANG_PROGRAM([[
  ]], [[
    union ieee_decimal128
    {
      struct
      {
        unsigned int t3:32;
        unsigned int t2:32;
        unsigned int t1:32;
        unsigned int t0:14;
        unsigned int comb:17;
        unsigned int sig:1;
      } s;
      _Decimal128 d128;
    } x;

    x.d128 = 1.0dl;
    if (x.s.sig == 0 && x.s.comb == 49400 &&
        x.s.t0 == 0 && x.s.t1 == 0 && x.s.t2 == 0 && x.s.t3 == 10)
       return 80; /* little endian */
    else if (x.s.sig == 0 && x.s.comb == 8 &&
             x.s.t0 == 0 && x.s.t1 == 0 && x.s.t2 == 0 && x.s.t3 == 570933248)
       return 81; /* big endian */
    else
       return 82; /* unknown encoding */
  ]])], [AC_MSG_RESULT(internal error)],
        [d128_exit_status=$?
         case "$d128_exit_status" in
           80) AC_MSG_RESULT(little endian)
               AC_DEFINE([HAVE_DECIMAL128_IEEE_LITTLE_ENDIAN],1) ;;
           81) AC_MSG_RESULT(big endian)
               AC_DEFINE([HAVE_DECIMAL128_IEEE_BIG_ENDIAN],1) ;;
           *)  AC_MSG_RESULT(unavailable or unknown) ;;
         esac],
        [AC_MSG_RESULT(cannot test)])
  
fi
# End of decimal float checks

dnl Check if _Float128 or __float128 is available. We also require the
dnl compiler to support hex constants with the f128 or q suffix respectively.
dnl If _Float128 is supported, then the mpfr_float128 macro should be
dnl defined as this type. We do not define it here because this will be
dnl done in mpfr.h, and not defining it here is the only way to ensure
dnl that under "make check", mpfr.h really defines it.
dnl Note: We use AC_LINK_IFELSE instead of AC_COMPILE_IFELSE since an
dnl error may occur only at link time, such as under NetBSD:
dnl   https://mail-index.netbsd.org/pkgsrc-users/2018/02/02/msg026220.html
dnl   https://mail-index.netbsd.org/pkgsrc-users/2018/02/05/msg026238.html
dnl By using volatile and making the exit code depend on the value of
dnl this variable, we also make sure that optimization doesn't make
dnl the "undefined reference" error disappear.
if test "$enable_float128" != no; then
   AC_MSG_CHECKING(if _Float128 with hex constants is supported)
   AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[
volatile _Float128 x = 0x1.fp+16383f128;
return x == 0;
]])],
      [AC_MSG_RESULT(yes)
       AC_DEFINE([MPFR_WANT_FLOAT128],1,[Build float128 functions])],
      [AC_MSG_RESULT(no)
       AC_MSG_CHECKING(if __float128 can be used as a fallback)
dnl Use the q suffix in this case.
       AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[
volatile __float128 x = 0x1.fp+16383q;
return x == 0;
]])],
          [AC_MSG_RESULT(yes)
           AC_DEFINE([MPFR_WANT_FLOAT128],2,
                     [Build float128 functions with __float128 fallback])
           AC_DEFINE([mpfr_float128],[__float128],[__float128 fallback])],
          [AC_MSG_RESULT(no)
           if test "$enable_float128" = yes; then
              AC_MSG_ERROR(
[compiler doesn't know _Float128 or __float128 with hex constants.
Please use another compiler or build MPFR without --enable-float128.])
       fi])
      ])
fi

dnl Check if Static Assertions are supported.
AC_MSG_CHECKING(for Static Assertion support)
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$srcdir/src"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#define MPFR_USE_STATIC_ASSERT 1
#include "mpfr-sassert.h"

/* Test if Static Assertions work */

int main (void) {
  int x;
  (void) (x = 1);  /* cast to void: avoid a warning, at least with GCC */
  /* Test of the macro after a declaraction and a statement. */
  MPFR_STAT_STATIC_ASSERT(sizeof(short) <= sizeof(int));
  return 0;
}
  ]])],
     [AC_MSG_RESULT(yes)
      AC_DEFINE([MPFR_USE_STATIC_ASSERT],1,[Build MPFR with Static Assertions])
     ],
     [AC_MSG_RESULT(no)
     ])
CPPFLAGS="$saved_CPPFLAGS"

if test "$enable_lto" = "yes" ; then
   MPFR_LTO
fi

dnl Logging support needs nested functions and the 'cleanup' attribute.
dnl This is checked at the end because the change of CC and/or CFLAGS that
dnl could occur before may have an influence on this test. The tested code
dnl is very similar to what is used in MPFR (mpfr-impl.h).
if test "$enable_logging" = yes; then
AC_MSG_CHECKING(for nested functions and 'cleanup' attribute)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
int main (void) {
  auto void f_cleanup (int *p);
  void f_cleanup (int *p) { int v = *p; (void) v; }
  int v __attribute__ ((cleanup (f_cleanup)));
  v = 0;
  return 0;
}
  ]])],
     [AC_MSG_RESULT(yes)],
     [AC_MSG_RESULT(no)
      AC_MSG_ERROR([logging support needs nested functions and the 'cleanup' attribute])
     ])
fi

dnl Timeout support in the testsuite.
dnl If the --enable-tests-timeout option has not been provided, let's
dnl enable timeout support only in -dev versions and if we detect that
dnl the needed POSIX features seem to be available (this is not much
dnl useful for releases, where an infinite loop is very unlikely).
dnl When supported, default is 0 (no timeout).
if test -z "$enable_tests_timeout" && test -n "$dev_version"; then
AC_MSG_CHECKING(if timeout can be supported)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/resource.h>
]], [[
  struct rlimit rlim[1];
  if (getrlimit (RLIMIT_CPU, rlim))
    return 1;
  rlim->rlim_cur = 1;
  if (setrlimit (RLIMIT_CPU, rlim))
    return 1;
]])], [
  AC_MSG_RESULT(yes)
  enable_tests_timeout=yes
],[AC_MSG_RESULT(no)])
fi
if test "$enable_tests_timeout" = yes; then
  AC_DEFINE([MPFR_TESTS_TIMEOUT], 0, [timeout limit])
fi

])
dnl end of MPFR_CONFIGS


dnl MPFR_CHECK_GMP
dnl --------------
dnl Check GMP library vs header. Useful if the user provides --with-gmp
dnl with a directory containing a GMP version that doesn't have the
dnl correct ABI: the previous tests won't trigger the error if the same
dnl GMP version with the right ABI is installed on the system, as this
dnl library is automatically selected by the linker, while the header
dnl (which depends on the ABI) of the --with-gmp include directory is
dnl used.
dnl Note: if the error is changed to a warning due to that fact that
dnl libtool is not used, then the same thing should be done for the
dnl other tests based on GMP.
AC_DEFUN([MPFR_CHECK_GMP], [
AC_REQUIRE([MPFR_CONFIGS])dnl
AC_CACHE_CHECK([for GMP library vs header correctness], mpfr_cv_check_gmp, [
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <limits.h>
#include "gmp.h"
]], [[
  fprintf (stderr, "GMP_NAIL_BITS     = %d\n", (int) GMP_NAIL_BITS);
  fprintf (stderr, "GMP_NUMB_BITS     = %d\n", (int) GMP_NUMB_BITS);
  fprintf (stderr, "mp_bits_per_limb  = %d\n", (int) mp_bits_per_limb);
  fprintf (stderr, "sizeof(mp_limb_t) = %d\n", (int) sizeof(mp_limb_t));
  if (GMP_NAIL_BITS != 0)
    {
      fprintf (stderr, "GMP_NAIL_BITS != 0\n");
      return 81;
    }
  if (GMP_NUMB_BITS != mp_bits_per_limb)
    {
      fprintf (stderr, "GMP_NUMB_BITS != mp_bits_per_limb\n");
      return 82;
    }
  if (GMP_NUMB_BITS != sizeof(mp_limb_t) * CHAR_BIT)
    {
      fprintf (stderr, "GMP_NUMB_BITS != sizeof(mp_limb_t) * CHAR_BIT\n");
      return 83;
    }
  return 0;
]])], [mpfr_cv_check_gmp="yes"],
      [mpfr_cv_check_gmp="no (exit status is $?)"],
      [mpfr_cv_check_gmp="cannot test, assume yes"])
])
case $mpfr_cv_check_gmp in
no*)
  AC_MSG_ERROR([bad GMP library or header - ABI problem?
See 'config.log' for details.]) ;;
esac
])


dnl MPFR_CHECK_DBL2INT_BUG
dnl ----------------------
dnl Check for double-to-integer conversion bug
dnl https://gforge.inria.fr/tracker/index.php?func=detail&aid=14435
dnl The following problem has been seen under Solaris in config.log,
dnl i.e. the failure to link with libgmp wasn't detected in the first
dnl test:
dnl   configure: checking if gmp.h version and libgmp version are the same
dnl   configure: gcc -o conftest -Wall -Wmissing-prototypes [...]
dnl   configure: $? = 0
dnl   configure: ./conftest
dnl   ld.so.1: conftest: fatal: libgmp.so.10: open failed: No such file [...]
dnl   configure: $? = 0
dnl   configure: result: yes
dnl   configure: checking for double-to-integer conversion bug
dnl   configure: gcc -o conftest -Wall -Wmissing-prototypes [...]
dnl   configure: $? = 0
dnl   configure: ./conftest
dnl   ld.so.1: conftest: fatal: libgmp.so.10: open failed: No such file [...]
dnl   ./configure[1680]: eval: line 1: 1971: Killed
dnl   configure: $? = 9
dnl   configure: program exited with status 9
AC_DEFUN([MPFR_CHECK_DBL2INT_BUG], [
AC_REQUIRE([MPFR_CONFIGS])dnl
AC_CACHE_CHECK([for double-to-integer conversion bug], mpfr_cv_dbl_int_bug, [
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include "gmp.h"
]], [[
  double d;
  mp_limb_t u;
  int i;

  d = 1.0;
  for (i = 0; i < GMP_NUMB_BITS - 1; i++)
    d = d + d;
  u = (mp_limb_t) d;
  for (; i > 0; i--)
    {
      if (u & 1)
        break;
      u = u >> 1;
    }
  if (i == 0 && u == 1UL)
    return 0;
  fprintf (stderr, "Failure: i = %d, (unsigned long) u = %lu\n",
           i, (unsigned long) u);
  return 1;
]])], [mpfr_cv_dbl_int_bug="no"],
      [mpfr_cv_dbl_int_bug="yes or failed to exec (exit status is $?)"],
      [mpfr_cv_dbl_int_bug="cannot test, assume not present"])
])
case $mpfr_cv_dbl_int_bug in
yes*)
  AC_MSG_ERROR([double-to-integer conversion is incorrect.
You need to use another compiler (or lower the optimization level).]) ;;
esac
])

dnl MPFR_CHECK_MP_LIMB_T_VS_LONG
dnl ----------------------------
dnl Check whether a long fits in mp_limb_t.
dnl If static assertions are not supported, one gets "no" even though a long
dnl fits in mp_limb_t. Therefore, code without MPFR_LONG_WITHIN_LIMB defined
dnl needs to be portable.
dnl According to the GMP developers, a limb is always as large as a long,
dnl except when __GMP_SHORT_LIMB is defined. It is currently never defined:
dnl https://gmplib.org/list-archives/gmp-discuss/2018-February/006190.html
dnl but it is not clear whether this could change in the future.
AC_DEFUN([MPFR_CHECK_MP_LIMB_T_VS_LONG], [
AC_REQUIRE([MPFR_CONFIGS])
AC_CACHE_CHECK([for long to fit in mp_limb_t], mpfr_cv_long_within_limb, [
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$srcdir/src"
dnl AC_LINK_IFELSE is safer than AC_COMPILE_IFELSE, as it will detect
dnl undefined function-like macros (which otherwise may be regarded
dnl as valid function calls with AC_COMPILE_IFELSE since prototypes
dnl are not required by the C standard).
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
/* Make sure that a static assertion is used (not MPFR_ASSERTN). */
#undef MPFR_USE_STATIC_ASSERT
#define MPFR_USE_STATIC_ASSERT 1
#include "mpfr-sassert.h"
]], [[
  MPFR_STAT_STATIC_ASSERT ((mp_limb_t) -1 >= (unsigned long) -1);
  return 0;
]])], [mpfr_cv_long_within_limb="yes"],
      [mpfr_cv_long_within_limb="no"])
])
case $mpfr_cv_long_within_limb in
yes*)
      AC_DEFINE([MPFR_LONG_WITHIN_LIMB],1,[long can be stored in mp_limb_t]) ;;
esac
CPPFLAGS="$saved_CPPFLAGS"
])

dnl MPFR_CHECK_MP_LIMB_T_VS_INTMAX
dnl ------------------------------
dnl Check that an intmax_t can fit in a mp_limb_t.
AC_DEFUN([MPFR_CHECK_MP_LIMB_T_VS_INTMAX], [
AC_REQUIRE([MPFR_CONFIGS])
if test "$ac_cv_type_intmax_t" = yes; then
AC_CACHE_CHECK([for intmax_t to fit in mp_limb_t], mpfr_cv_intmax_within_limb, [
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$srcdir/src -DMPFR_NEED_INTMAX_H"
dnl AC_LINK_IFELSE is safer than AC_COMPILE_IFELSE, as it will detect
dnl undefined function-like macros (which otherwise may be regarded
dnl as valid function calls with AC_COMPILE_IFELSE since prototypes
dnl are not required by the C standard).
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
/* Make sure that a static assertion is used (not MPFR_ASSERTN). */
#undef MPFR_USE_STATIC_ASSERT
#define MPFR_USE_STATIC_ASSERT 1
#include "mpfr-sassert.h"
#include "mpfr-intmax.h"
]], [[
  MPFR_STAT_STATIC_ASSERT ((mp_limb_t) -1 >= (uintmax_t) -1);
  return 0;
]])], [mpfr_cv_intmax_within_limb="yes"],
      [mpfr_cv_intmax_within_limb="no"])
])
case $mpfr_cv_intmax_within_limb in
yes*)
      AC_DEFINE([MPFR_INTMAX_WITHIN_LIMB],1,[intmax_t can be stored in mp_limb_t]) ;;
esac
CPPFLAGS="$saved_CPPFLAGS"
fi
])

dnl MPFR_PARSE_DIRECTORY
dnl Input:  $1 = a string to a relative or absolute directory
dnl Output: $2 = the variable to set with the absolute directory
AC_DEFUN([MPFR_PARSE_DIRECTORY],
[
 dnl Check if argument is a directory
 if test -d $1 ; then
    dnl Get the absolute path of the directory in case of relative directory
    dnl with the realpath command. If the output is empty, the cause may be
    dnl that this command has not been found, and we do an alternate test,
    dnl the same as what autoconf does for the generated configure script to
    dnl determine whether a pathname is absolute or relative.
    local_tmp=`realpath $1 2>/dev/null`
    if test "$local_tmp" != "" ; then
       if test -d "$local_tmp" ; then
           $2="$local_tmp"
       else
           $2=$1
       fi
    else
       dnl The quadrigraphs @<:@, @:>@ and @:}@ produce [, ] and )
       dnl respectively (see Autoconf manual). We cannot use quoting here
       dnl as the result depends on the context in which this macro is
       dnl invoked! To detect that, one needs to look at every instance
       dnl of the macro expansion in the generated configure script.
       case $1 in
         @<:@\\/@:>@* | ?:@<:@\\/@:>@* @:}@ $2=$1 ;;
         *@:}@ $2="$PWD"/$1 ;;
       esac
    fi
    dnl Check for space in the directory
    if test `echo $1|cut -d' ' -f1` != $1 ; then
        AC_MSG_ERROR($1 directory shall not contain any space.)
    fi
 else
    AC_MSG_ERROR($1 shall be a valid directory)
 fi
])


dnl MPFR_C_REALFP_FORMAT
dnl --------------------
dnl Determine the format of a real floating type (first argument),
dnl actually either double or long double. The second argument is
dnl the printf length modifier.
dnl
dnl The object file is grepped, so as to work when cross-compiling.
dnl Start and end sequences are included to avoid false matches, and
dnl allowance is made for the desired data crossing an "od -b" line
dnl boundary.  The test number is a small integer so it should appear
dnl exactly, without rounding or truncation, etc.
dnl
dnl "od -b" is supported even by Unix V7, and the awk script used doesn't
dnl have functions or anything, so even an "old" awk should suffice.
dnl
dnl Concerning long double: The 10-byte IEEE extended format is generally
dnl padded with null bytes to either 12 or 16 bytes for alignment purposes.
dnl The SVR4 i386 ABI is 12 bytes, or i386 gcc -m128bit-long-double selects
dnl 16 bytes. IA-64 is 16 bytes in LP64 mode, or 12 bytes in ILP32 mode.
dnl The relevant part in all cases (big and little endian) consists of the
dnl first 10 bytes.
dnl
dnl We compile and link (with "-o conftest$EXEEXT") instead of just
dnl compiling (with "-c"), so that this test works with GCC's and
dnl clang's LTO (-flto). If we just compile with LTO, the generated
dnl object file does not contain the structure as is. This new test
dnl is inspired by the one used by GMP for the double type:
dnl   https://gmplib.org/repo/gmp/rev/33eb0998a052
dnl   https://gmplib.org/repo/gmp/rev/cbc6dbf95a10
dnl "$EXEEXT" had to be added, otherwise the test was failing on
dnl MS-Windows (see Autoconf manual).

AC_DEFUN([MPFR_C_REALFP_FORMAT],
[AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_AWK])
AC_REQUIRE([AC_OBJEXT])
AS_VAR_PUSHDEF([my_Type_var], [mpfr_cv_c_$1_format])dnl
AC_CACHE_CHECK([format of floating-point type `$1'],
                my_Type_var,
[my_Type_var=unknown
 cat >conftest.c <<\EOF
[
#include <stdio.h>
/* "before" is 16 bytes to ensure there's no padding between it and "x".
   We're not expecting any type bigger than 16 bytes or with
   alignment requirements stricter than 16 bytes.  */
typedef struct {
  char         before[16];
  $1           x;
  char         after[8];
} foo_t;

foo_t foo = {
  { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\001', '\043', '\105', '\147', '\211', '\253', '\315', '\357' },
  -123456789.0,
  { '\376', '\334', '\272', '\230', '\166', '\124', '\062', '\020' }
};

int main (void) {
  int i;
  for (i = 0; i < 8; i++)
    printf ("%d %$2f\n", foo.before[i] + foo.after[i], foo.x);
  return 0;
}
]
EOF
 mpfr_compile="$CC $CFLAGS $CPPFLAGS $LDFLAGS conftest.c -o conftest$EXEEXT >&AS_MESSAGE_LOG_FD 2>&1"
 if AC_TRY_EVAL(mpfr_compile); then
   cat >conftest.awk <<\EOF
[
BEGIN {
  found = 0
}

# got[] holds a sliding window of bytes read the input.  got[0] is the most
# recent byte read, and got[31] the oldest byte read, so when looking to
# match some data the indices are "reversed".
#
{
  for (f = 2; f <= NF; f++)
    {
      # new byte, shift others up
      for (i = 31; i >= 0; i--)
        got[i+1] = got[i];
      got[0] = $f;

      # end sequence
      if (got[7] != "376") continue
      if (got[6] != "334") continue
      if (got[5] != "272") continue
      if (got[4] != "230") continue
      if (got[3] != "166") continue
      if (got[2] != "124") continue
      if (got[1] != "062") continue
      if (got[0] != "020") continue

      # start sequence, with 8-byte body
      if (got[23] == "001" && \
          got[22] == "043" && \
          got[21] == "105" && \
          got[20] == "147" && \
          got[19] == "211" && \
          got[18] == "253" && \
          got[17] == "315" && \
          got[16] == "357")
        {
          saw = " (" got[15] \
                 " " got[14] \
                 " " got[13] \
                 " " got[12] \
                 " " got[11] \
                 " " got[10] \
                 " " got[9]  \
                 " " got[8] ")"

          if (got[15] == "301" && \
              got[14] == "235" && \
              got[13] == "157" && \
              got[12] == "064" && \
              got[11] == "124" && \
              got[10] == "000" && \
              got[9] ==  "000" && \
              got[8] ==  "000")
            {
              print "IEEE double, big endian"
              found = 1
              exit
            }

          if (got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "124" && \
              got[11] == "064" && \
              got[10] == "157" && \
              got[9] ==  "235" && \
              got[8] ==  "301")
            {
              print "IEEE double, little endian"
              found = 1
              exit
            }
        }

      # start sequence, with 12-byte body
      if (got[27] == "001" && \
          got[26] == "043" && \
          got[25] == "105" && \
          got[24] == "147" && \
          got[23] == "211" && \
          got[22] == "253" && \
          got[21] == "315" && \
          got[20] == "357")
        {
          saw = " (" got[19] \
                 " " got[18] \
                 " " got[17] \
                 " " got[16] \
                 " " got[15] \
                 " " got[14] \
                 " " got[13] \
                 " " got[12] \
                 " " got[11] \
                 " " got[10] \
                 " " got[9]  \
                 " " got[8] ")"

          if (got[19] == "000" && \
              got[18] == "000" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "240" && \
              got[14] == "242" && \
              got[13] == "171" && \
              got[12] == "353" && \
              got[11] == "031" && \
              got[10] == "300")
            {
              print "IEEE extended, little endian (12 bytes)"
              found = 1
              exit
            }

          if (got[19] == "300" && \
              got[18] == "031" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "353" && \
              got[14] == "171" && \
              got[13] == "242" && \
              got[12] == "240" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[09] == "000" && \
              got[08] == "000")
            {
              # format found on m68k
              print "IEEE extended, big endian (12 bytes)"
              found = 1
              exit
            }
        }

      # start sequence, with 16-byte body
      if (got[31] == "001" && \
          got[30] == "043" && \
          got[29] == "105" && \
          got[28] == "147" && \
          got[27] == "211" && \
          got[26] == "253" && \
          got[25] == "315" && \
          got[24] == "357")
        {
          saw = " (" got[23] \
                 " " got[22] \
                 " " got[21] \
                 " " got[20] \
                 " " got[19] \
                 " " got[18] \
                 " " got[17] \
                 " " got[16] \
                 " " got[15] \
                 " " got[14] \
                 " " got[13] \
                 " " got[12] \
                 " " got[11] \
                 " " got[10] \
                 " " got[9]  \
                 " " got[8] ")"

          if (got[23] == "000" && \
              got[22] == "000" && \
              got[21] == "000" && \
              got[20] == "000" && \
              got[19] == "240" && \
              got[18] == "242" && \
              got[17] == "171" && \
              got[16] == "353" && \
              got[15] == "031" && \
              got[14] == "300")
            {
              print "IEEE extended, little endian (16 bytes)"
              found = 1
              exit
            }

          if (got[23] == "300" && \
              got[22] == "031" && \
              got[21] == "326" && \
              got[20] == "363" && \
              got[19] == "105" && \
              got[18] == "100" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "000" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[9]  == "000" && \
              got[8]  == "000")
            {
              # format used on HP 9000/785 under HP-UX
              print "IEEE quad, big endian"
              found = 1
              exit
            }

          if (got[23] == "000" && \
              got[22] == "000" && \
              got[21] == "000" && \
              got[20] == "000" && \
              got[19] == "000" && \
              got[18] == "000" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "100" && \
              got[12] == "105" && \
              got[11] == "363" && \
              got[10] == "326" && \
              got[9]  == "031" && \
	      got[8]  == "300")
            {
              print "IEEE quad, little endian"
              found = 1
              exit
            }

          if (got[23] == "301" && \
              got[22] == "235" && \
              got[21] == "157" && \
              got[20] == "064" && \
              got[19] == "124" && \
              got[18] == "000" && \
              got[17] == "000" && \
              got[16] == "000" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "000" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[9]  == "000" && \
              got[8]  == "000")
            {
              # format used on 32-bit PowerPC (Mac OS X and Debian GNU/Linux)
              print "possibly double-double, big endian"
              found = 1
              exit
            }

          if (got[23] == "000" && \
              got[22] == "000" && \
              got[21] == "000" && \
              got[20] == "124" && \
              got[19] == "064" && \
              got[18] == "157" && \
              got[17] == "235" && \
              got[16] == "301" && \
              got[15] == "000" && \
              got[14] == "000" && \
              got[13] == "000" && \
              got[12] == "000" && \
              got[11] == "000" && \
              got[10] == "000" && \
              got[9]  == "000" && \
              got[8]  == "000")
            {
              # format used on ppc64le
              print "possibly double-double, little endian"
              found = 1
              exit
            }
        }
    }
}

END {
  if (! found)
    print "unknown", saw
}
]
EOF
   my_Type_var=`od -b conftest$EXEEXT | $AWK -f conftest.awk`
   case $my_Type_var in
   unknown*)
     echo "cannot match anything, conftest$EXEEXT contains" >&AS_MESSAGE_LOG_FD
     od -b conftest$EXEEXT >&AS_MESSAGE_LOG_FD
     ;;
   esac
 else
   AC_MSG_WARN([oops, cannot compile test program])
 fi
 rm -f conftest*
])])


dnl  MPFR_HAVE_LIB
dnl  -------------
dnl
dnl  Similar to AC_CHECK_LIB, but without checking any function.
dnl  This is useful, because AC_CHECK_LIB has compatibility issues
dnl  with GCC's -Werror, such has
dnl    error: infinite recursion detected [-Werror=infinite-recursion]
dnl  if "main" is checked, or
dnl    error: conflicting types for built-in function 'floor'; expected
dnl    'double(double)' [-Werror=builtin-declaration-mismatch]
dnl  if "floor" is checked (since "char floor ();" is used by autoconf).
dnl
dnl  In the future, use AC_SEARCH_LIBS instead?

AC_DEFUN([MPFR_HAVE_LIB], [
saved_LIBS="$LIBS"
LIBS="-l$1 $LIBS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])], $2)
LIBS="$saved_LIBS"
])


dnl  MPFR_CHECK_LIBM
dnl  ---------------
dnl  Determine math libraries to use.
dnl  The actual functions will individually be checked later.

AC_DEFUN([MPFR_CHECK_LIBM],
[AC_REQUIRE([AC_CANONICAL_HOST])
AC_SUBST(MPFR_LIBM,'')
case $host in
  *-*-beos* | *-*-cegcc* | *-*-cygwin* | *-*-haiku* | *-*-pw32* | *-*-darwin*)
    # According to libtool.m4:
    #   These systems don't have libm, or don't need it.
    ;;
  *-*-solaris*)
    # On Solaris, some additional math functions are in -lm9x.
    # For MPFR, https://docs.oracle.com/cd/E19957-01/806-3568/ncg_lib.html
    # says that ceil, floor and rint are provided by libm. We would also
    # like nearbyint when available, but there is no mention of it in this
    # doc. Just in case, let's check for it in m9x, e.g. if it is added in
    # the future.
    MPFR_HAVE_LIB(m9x, MPFR_LIBM="-lm9x")
    MPFR_HAVE_LIB(m, MPFR_LIBM="$MPFR_LIBM -lm")
    ;;
  *-ncr-sysv4.3*)
    # The following AC_CHECK_LIB line about -lmw is copied from libtool.m4,
    # but do we need it? This has never been tested in MPFR. See commits
    #   6d34bd85f038abeaeeb77aa8f65b562623cc38bc (1999-02-13)
    #   e65f46d3fc4eb98d25ee94ad8e6f51c5846c8fe3 (1999-03-20)
    # in the libtool repository.
    AC_CHECK_LIB(mw, _mwvalidcheckl, MPFR_LIBM="-lmw")
    MPFR_HAVE_LIB(m, MPFR_LIBM="$MPFR_LIBM -lm")
    ;;
  *)
    MPFR_HAVE_LIB(m, MPFR_LIBM="-lm")
    ;;
esac
])

dnl  MPFR_CHECK_LIBQUADMATH
dnl  ----------------------
dnl  Determine a math library -lquadmath to use.

AC_DEFUN([MPFR_CHECK_LIBQUADMATH],
[AC_REQUIRE([AC_CANONICAL_HOST])
AC_SUBST(MPFR_LIBQUADMATH,'')
case $host in
  *)
    MPFR_HAVE_LIB(quadmath, MPFR_LIBQUADMATH="-lquadmath")
    ;;
esac
])


dnl  MPFR_LD_SEARCH_PATHS_FIRST
dnl  --------------------------

AC_DEFUN([MPFR_LD_SEARCH_PATHS_FIRST],
[case "$LD $LDFLAGS" in
  *-Wl,-search_paths_first*) ;;
  *) AC_MSG_CHECKING([if the compiler understands -Wl,-search_paths_first])
     saved_LDFLAGS="$LDFLAGS"
     LDFLAGS="-Wl,-search_paths_first $LDFLAGS"
     AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
       [AC_MSG_RESULT(yes)],
       [AC_MSG_RESULT(no)]
        LDFLAGS="$saved_LDFLAGS")
     ;;
 esac
])


dnl  GMP_C_ATTRIBUTE_MODE
dnl  --------------------
dnl  Introduced in gcc 2.2, but perhaps not in all Apple derived versions.
dnl  Needed for mpfr-longlong.h; this is currently necessary for s390.
dnl
dnl  TODO: Replace this with a cleaner type size detection, as this
dnl  solution only works with gcc and assumes CHAR_BIT == 8. Probably use
dnl  <stdint.h>, and <https://gcc.gnu.org/viewcvs/gcc/trunk/config/stdint.m4>
dnl  as a fallback.

AC_DEFUN([GMP_C_ATTRIBUTE_MODE],
[AC_CACHE_CHECK([whether gcc __attribute__ ((mode (XX))) works],
               gmp_cv_c_attribute_mode,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[typedef int SItype __attribute__ ((mode (SI)));]], [[]])],
 gmp_cv_c_attribute_mode=yes, gmp_cv_c_attribute_mode=no)
])
if test $gmp_cv_c_attribute_mode = yes; then
 AC_DEFINE(HAVE_ATTRIBUTE_MODE, 1,
 [Define to 1 if the compiler accepts gcc style __attribute__ ((mode (XX)))])
fi
])


dnl  MPFR_FUNC_GMP_PRINTF_SPEC
dnl  ------------------------------------
dnl  MPFR_FUNC_GMP_PRINTF_SPEC(spec, type, [includes], [if-true], [if-false])
dnl  Check if sprintf and gmp_sprintf support the conversion specifier 'spec'
dnl  with type 'type'.
dnl  Expand 'if-true' if printf supports 'spec', 'if-false' otherwise.

AC_DEFUN([MPFR_FUNC_GMP_PRINTF_SPEC],[
AC_MSG_CHECKING(if gmp_printf supports "%$1")
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <string.h>
$3
#include "gmp.h"
]], [[
  char s[256];
  $2 a = 17;

  /* Contrary to the gmp_sprintf test, do not use the 0 flag with the
     precision, as -Werror=format yields an error, even though this
     flag is allowed by the ISO C standard (it is just ignored).
     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70813 */
  if (sprintf (s, "(%.0$1)(%d)", a, 42) != 8 ||
      strcmp (s, "(17)(42)") != 0)
    return 1;

  if (gmp_sprintf (s, "(%0.0$1)(%d)", a, 42) == -1 ||
      strcmp (s, "(17)(42)") != 0)
    return 1;

  return 0;
]])],
  [AC_MSG_RESULT(yes)
  $4],
  [AC_MSG_RESULT(no)
  $5],
  [AC_MSG_RESULT(cross-compiling, assuming yes)
  $4])
])


dnl MPFR_CHECK_PRINTF_SPEC
dnl ----------------------
dnl Check if gmp_printf supports some optional length modifiers.
dnl Defined symbols are negative to shorten the gcc command line.

AC_DEFUN([MPFR_CHECK_PRINTF_SPEC], [
AC_REQUIRE([MPFR_CONFIGS])dnl
if test "$ac_cv_type_intmax_t" = yes; then
 MPFR_FUNC_GMP_PRINTF_SPEC([jd], [intmax_t], [
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
         ],,
         [AC_DEFINE([NPRINTF_J], 1, [printf/gmp_printf cannot read intmax_t])])
fi

MPFR_FUNC_GMP_PRINTF_SPEC([hhd], [char], [
#include "gmp.h"
         ],,
         [AC_DEFINE([NPRINTF_HH], 1, [printf/gmp_printf cannot use `hh' length modifier])])

MPFR_FUNC_GMP_PRINTF_SPEC([lld], [long long int], [
#include "gmp.h"
         ],,
         [AC_DEFINE([NPRINTF_LL], 1, [printf/gmp_printf cannot read long long int])])

MPFR_FUNC_GMP_PRINTF_SPEC([Lf], [long double], [
#include "gmp.h"
         ],
         [AC_DEFINE([PRINTF_L], 1, [printf/gmp_printf can read long double])],
         [AC_DEFINE([NPRINTF_L], 1, [printf/gmp_printf cannot read long double])])

MPFR_FUNC_GMP_PRINTF_SPEC([td], [ptrdiff_t], [
#if defined (__cplusplus)
#include <cstddef>
#else
#include <stddef.h>
#endif
#include "gmp.h"
    ],
    [AC_DEFINE([PRINTF_T], 1, [printf/gmp_printf can read ptrdiff_t])],
    [AC_DEFINE([NPRINTF_T], 1, [printf/gmp_printf cannot read ptrdiff_t])])

AC_MSG_CHECKING(if gmp_printf supports "%a")
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include "gmp.h"
]], [[
  char s[256];

  /* With the buggy repl-vsnprintf.c (vsnprintf replacement), one gets
     an assertion failure like:
       repl-vsnprintf.c:389: GNU MP assertion failed: len < total_width
     (not all values are affected, but -1.25, used below, is).
     The purpose of this test is to detect this bug only, assuming
     C99 support (which will be checked in the test .c files).
     We do not check the output, which is not fully specified. */
  gmp_snprintf (s, 256, "%a", -1.25);
  return 0;

#if 0
  /* A more complete program, for testing.
     The assertion failure occurs only on the 4th case. */
#include <stdio.h>
#include <gmp.h>
int main (void)
{
  char s[256];
  double x[2] = { 1.25, -1.25 };
  int i;
  for (i = 0; i < 2; i++)
    {
      gmp_sprintf (s, "%a", x[i]);
      printf ("[%s]\n", s);
      gmp_snprintf (s, 256, "%a", x[i]);
      printf ("[%s]\n", s);
    }
  return 0;
}
#endif
]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([PRINTF_A], 1, [printf/gmp_printf supports "%a"])],
  [AC_MSG_RESULT(no)
   AC_MSG_WARN([gmp_snprintf on "%a" terminated the program abnormally.])
   AC_MSG_WARN([This is probably an assertion failure due to the buggy])
   AC_MSG_WARN([repl-vsnprintf.c (vsnprintf replacement). References:])
   AC_MSG_WARN([  https://sympa.inria.fr/sympa/arc/mpfr/2022-10/msg00001.html])
   AC_MSG_WARN([  https://gmplib.org/list-archives/gmp-bugs/2022-October/005200.html])
   AC_MSG_WARN([  https://gmplib.org/list-archives/gmp-bugs/2025-January/005557.html])
   AC_MSG_WARN([%a must not be used in the MPFR formatted output functions])
   AC_MSG_WARN([with this version of GMP; the corresponding tests in the])
   AC_MSG_WARN([MPFR testsuite will be disabled.])
   AC_MSG_WARN([See 'config.log' for details.])
   AC_DEFINE([NPRINTF_A], 1, [printf/gmp_printf does not support "%a"])],
  [AC_MSG_RESULT(cross-compiling, do not assume anything)])
])

dnl MPFR_CHECK_PRINTF_GROUPFLAG
dnl ---------------------------
dnl Check the support of the group flag for native integers, which is
dnl a Single UNIX Specification extension.
dnl This will be used to enable some tests, as the implementation of
dnl the P length modifier for mpfr_*printf relies on this support.

AC_DEFUN([MPFR_CHECK_PRINTF_GROUPFLAG], [
AC_MSG_CHECKING(if gmp_printf supports the ' group flag)
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <string.h>
#include "gmp.h"
]], [[
  char s[256];

  if (gmp_sprintf (s, "%'d", 17) == -1) return 1;
  return (strcmp (s, "17") != 0);
]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([PRINTF_GROUPFLAG], 1, [Define if gmp_printf supports the ' group flag])],
  [AC_MSG_RESULT(no)],
  [AC_MSG_RESULT(cannot test, assume no)])
])
])

dnl MPFR_LTO
dnl --------
dnl To be representative, we need:
dnl * to compile a source,
dnl * to generate a library archive,
dnl * to generate a program with this archive.
AC_DEFUN([MPFR_LTO],[
dnl Check for -flto support
CFLAGS="$CFLAGS -flto"

AC_MSG_CHECKING([if Link Time Optimisation flag '-flto' is supported...])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
int main(void) { return 0; }
  ]])],
     [AC_MSG_RESULT(yes)
     ],
     [AC_MSG_RESULT(no)
      AC_MSG_ERROR([Link Time Optimisation flag '-flto' is not supported.])
     ])

dnl Check if it works...
mpfr_compile_and_link()
{
   echo "int f(int); int f(int n) { return n; }" > conftest-f.c
   echo "int f(int); int main() { return f(0); }" > conftest-m.c
   echo "$CC $CFLAGS -c -o conftest-f.o conftest-f.c" >&2
   $CC $CFLAGS -c -o conftest-f.o conftest-f.c || return 1
   echo "$AR cru conftest-lib.a conftest-f.o" >&2
   $AR cru conftest-lib.a conftest-f.o || return 1
   echo "$RANLIB conftest-lib.a" >&2
   $RANLIB conftest-lib.a || return 1
   echo "$CC $CFLAGS conftest-m.c conftest-lib.a" >&2
   $CC $CFLAGS conftest-m.c conftest-lib.a || return 1
   return 0
}
   AC_MSG_CHECKING([if Link Time Optimisation works with AR=$AR])
   if mpfr_compile_and_link 2> conftest-log1.txt ; then
      cat conftest-log1.txt >&AS_MESSAGE_LOG_FD
      AC_MSG_RESULT(yes)
   else
      cat conftest-log1.txt >&AS_MESSAGE_LOG_FD
      AC_MSG_RESULT(no)
      AR=gcc-ar
      RANLIB=gcc-ranlib
      AC_MSG_CHECKING([if Link Time Optimisation works with AR=$AR])
      if mpfr_compile_and_link 2> conftest-log2.txt; then
         cat conftest-log2.txt >&AS_MESSAGE_LOG_FD
         AC_MSG_RESULT(yes)
      else
        cat conftest-log2.txt >&AS_MESSAGE_LOG_FD
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([Link Time Optimisation is not supported (see config.log for details).])
      fi
   fi
rm -f conftest*
])

dnl MPFR_CHECK_CONSTRUCTOR_ATTR
dnl ---------------------------
dnl Check for constructor/destructor attributes to function.
dnl Output: Define
dnl  * MPFR_HAVE_CONSTRUCTOR_ATTR C define
dnl  * mpfr_have_constructor_destructor_attributes shell variable to yes
dnl if supported.
AC_DEFUN([MPFR_CHECK_CONSTRUCTOR_ATTR], [
AC_MSG_CHECKING([for constructor and destructor attributes])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
int x = 0;
__attribute__((constructor)) static void
call_f(void) { x = 1742; }
__attribute__((destructor)) static void
call_g(void) { x = 1448; }
]], [[
    return (x == 1742) ? 0 : 1;
]])], [AC_MSG_RESULT(yes)
       mpfr_have_constructor_destructor_attributes=yes ],
      [AC_MSG_RESULT(no)]
)

if test "$mpfr_have_constructor_destructor_attributes" = "yes"; then
  AC_DEFINE(MPFR_HAVE_CONSTRUCTOR_ATTR, 1,
   [Define if the constructor/destructor GCC attributes are supported.])
fi
])

dnl MPFR_CHECK_C11_THREAD
dnl ---------------------
dnl Check for C11 thread support
dnl Output: Define
dnl  * MPFR_HAVE_C11_LOCK C define
dnl  * mpfr_c11_thread_ok shell variable to yes
dnl if supported.
dnl We don't check for __STDC_NO_THREADS__ define variable but rather try to mimic our usage.
AC_DEFUN([MPFR_CHECK_C11_THREAD], [
AC_MSG_CHECKING([for ISO C11 thread support])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <assert.h>
#include <threads.h>
 mtx_t lock;
 once_flag once = ONCE_FLAG_INIT;
 thrd_t thd_idx;
 int x = 0;
 void once_call (void) { x = 1; }
]], [[
    int err;
    err = mtx_init(&lock, mtx_plain);
    assert(err == thrd_success);
    err = mtx_lock(&lock);
    assert(err == thrd_success);
    err = mtx_unlock(&lock);
    assert(err == thrd_success);
    mtx_destroy(&lock);
    once_call(&once, once_call);
    return x == 1 ? 0 : -1;
]])], [AC_MSG_RESULT(yes)
       mpfr_c11_thread_ok=yes ],
      [AC_MSG_RESULT(no)]
)

if test "$mpfr_c11_thread_ok" = "yes"; then
  AC_DEFINE(MPFR_HAVE_C11_LOCK, 1,
   [Define if the ISO C11 Thread is supported.])
fi
])


