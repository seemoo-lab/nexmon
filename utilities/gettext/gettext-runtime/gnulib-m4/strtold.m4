# strtold.m4
# serial 11
dnl Copyright (C) 2002-2003, 2006-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_FUNC_STRTOLD],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CHECK_FUNCS_ONCE([strtold])
  if test $ac_cv_func_strtold != yes; then
    HAVE_STRTOLD=0
  else
    AC_CACHE_CHECK([whether strtold obeys POSIX], [gl_cv_func_strtold_works],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <string.h>
/* Compare two numbers with ==.
   This is a separate function in order to disable compiler optimizations.  */
static int
numeric_equal (long double x, long double y)
{
  return x == y;
}
]], [[
  int result = 0;
  {
    /* Under Solaris 2.4, strtod returns the wrong value for the
       terminating character under some conditions.  */
    const char *string = "NaN";
    char *term;
    strtold (string, &term);
    if (term != string && *(term - 1) == 0)
      result |= 1;
  }
  {
    /* Older glibc and Cygwin mis-parse "-0x".  */
    const char *string = "-0x";
    char *term;
    long double value = strtold (string, &term);
    long double zero = 0.0L;
    if (1.0L / value != -1.0L / zero || term != (string + 2))
      result |= 2;
  }
  {
    /* mingw does not parse hex floats.  */
    const char *string = "0XaP+1";
    char *term;
    long double value = strtold (string, &term);
    if (value != 20.0L || term != (string + 6))
      result |= 4;
  }
  {
    /* HP-UX 11.31/ia64 parses inf, but mistakenly sets errno.  */
    const char *string = "inf";
    char *term;
    long double value;
    errno = 0;
    value = strtold (string, &term);
    if (value != HUGE_VAL || term != (string + 3) || errno)
      result |= 8;
  }
  {
    /* glibc-2.3.2, mingw, Haiku misparse "nan()".  */
    const char *string = "nan()";
    char *term;
    long double value = strtold (string, &term);
    if (numeric_equal (value, value) || term != (string + 5))
      result |= 16;
  }
  {
    /* Mac OS X 10.5 misparses "nan(".  */
    const char *string = "nan(";
    char *term;
    long double value = strtold (string, &term);
    if (numeric_equal (value, value) || term != (string + 3))
      result |= 16;
  }
#ifndef _MSC_VER /* On MSVC, this is expected behaviour.  */
  {
    /* In Cygwin 2.9 and mingw 5.0, strtold does not set errno upon
       gradual underflow.  */
# if LDBL_MAX_EXP > 10000
    const char *string = "1e-4950";
# else
    const char *string = "1e-320";
# endif
    char *term;
    long double value;
    errno = 0;
    value = strtold (string, &term);
    if (term != (string + strlen (string))
        || (value > 0.0L && value <= LDBL_MIN && errno != ERANGE))
      result |= 32;
  }
#endif
  {
    /* In Cygwin 2.9, strtold does not set errno upon
       flush-to-zero underflow.  */
    const char *string = "1E-100000";
    char *term;
    long double value;
    errno = 0;
    value = strtold (string, &term);
    if (term != (string + 9) || (value == 0.0L && errno != ERANGE))
      result |= 64;
  }
  return result;
]])],
        [gl_cv_func_strtold_works=yes],
        [result=$?
         if expr $result '>=' 64 >/dev/null; then
           gl_cv_func_strtold_works="no (underflow problem)"
         else
           if expr $result '>=' 32 >/dev/null; then
             gl_cv_func_strtold_works="no (gradual underflow problem)"
           else
             gl_cv_func_strtold_works=no
           fi
         fi
        ],
        [dnl The last known bugs in glibc strtold(), as of this writing,
         dnl were fixed in version 2.8
         AC_EGREP_CPP([Lucky user],
           [
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8) || (__GLIBC__ > 2)) \
     && !defined __UCLIBC__
  Lucky user
 #endif
#endif
           ],
           [gl_cv_func_strtold_works="guessing yes"],
           [case "$host_os" in
                                  # Guess yes on musl systems.
              *-musl* | midipix*) gl_cv_func_strtold_works="guessing yes" ;;
                                  # Guess 'no (underflow problem)' on Cygwin.
              cygwin*)            gl_cv_func_strtold_works="guessing no (underflow problem)" ;;
                                  # Guess 'no (gradual underflow problem)' on mingw.
                                  # (Although the results may vary depending on
                                  # __USE_MINGW_ANSI_STDIO.)
              mingw* | windows*)  gl_cv_func_strtold_works="guessing no (gradual underflow problem)" ;;
              *)                  gl_cv_func_strtold_works="$gl_cross_guess_normal" ;;
            esac
           ])
        ])
      ])
    case "$gl_cv_func_strtold_works" in
      *yes) ;;
      *)
        REPLACE_STRTOLD=1
        case "$gl_cv_func_strtold_works" in
          *"no (underflow problem)")
            AC_DEFINE([STRTOLD_HAS_UNDERFLOW_BUG], [1],
              [Define to 1 if strtold does not set errno upon flush-to-zero underflow.])
            ;;
          *"no (gradual underflow problem)")
            AC_DEFINE([STRTOLD_HAS_GRADUAL_UNDERFLOW_PROBLEM], [1],
              [Define to 1 if strtold does not set errno upon gradual underflow.])
            ;;
        esac
        ;;
    esac
  fi
])

# Prerequisites of lib/strtold.c.
AC_DEFUN([gl_PREREQ_STRTOLD], [
  AC_REQUIRE([gl_CHECK_LDEXPL_NO_LIBM])
  if test $gl_cv_func_ldexpl_no_libm = yes; then
    AC_DEFINE([HAVE_LDEXPL_IN_LIBC], [1],
      [Define if the ldexpl function is available in libc.])
  fi
  gl_CHECK_FUNCS_ANDROID([nl_langinfo], [[#include <langinfo.h>]])
])
