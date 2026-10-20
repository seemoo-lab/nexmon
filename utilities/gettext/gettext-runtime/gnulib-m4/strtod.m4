# strtod.m4
# serial 32
dnl Copyright (C) 2002-2003, 2006-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_FUNC_STRTOD],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  m4_ifdef([gl_FUNC_STRTOD_OBSOLETE], [
    dnl Test whether strtod is declared.
    dnl Don't call AC_FUNC_STRTOD, because it does not have the right guess
    dnl when cross-compiling.
    dnl Don't call AC_CHECK_FUNCS([strtod]) because it would collide with the
    dnl ac_cv_func_strtod variable set by the AC_FUNC_STRTOD macro.
    AC_CHECK_DECLS_ONCE([strtod])
    if test $ac_cv_have_decl_strtod != yes; then
      HAVE_STRTOD=0
    fi
  ])
  if test $HAVE_STRTOD = 1; then
    AC_CACHE_CHECK([whether strtod obeys C99], [gl_cv_func_strtod_works],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <errno.h>
/* Compare two numbers with ==.
   This is a separate function in order to disable compiler optimizations.  */
static int
numeric_equal (double x, double y)
{
  return x == y;
}
]], [[
  int result = 0;
  {
    /* In some old versions of Linux (2000 or before), strtod mis-parses
       strings with leading '+'.  */
    const char *string = " +69";
    char *term;
    double value = strtod (string, &term);
    if (value != 69 || term != (string + 4))
      result |= 1;
  }
  {
    /* Under Solaris 2.4, strtod returns the wrong value for the
       terminating character under some conditions.  */
    const char *string = "NaN";
    char *term;
    strtod (string, &term);
    if (term != string && *(term - 1) == 0)
      result |= 1;
  }
  {
    /* Older glibc and Cygwin mis-parse "-0x".  */
    const char *string = "-0x";
    char *term;
    double value = strtod (string, &term);
    double zero = 0.0;
    if (1.0 / value != -1.0 / zero || term != (string + 2))
      result |= 2;
  }
  {
    /* Many platforms do not parse hex floats.  */
    const char *string = "0XaP+1";
    char *term;
    double value = strtod (string, &term);
    if (value != 20.0 || term != (string + 6))
      result |= 4;
  }
  {
    /* Many platforms do not parse infinities.  HP-UX 11.31 parses inf,
       but mistakenly sets errno.  */
    const char *string = "inf";
    char *term;
    double value;
    errno = 0;
    value = strtod (string, &term);
    if (value != HUGE_VAL || term != (string + 3) || errno)
      result |= 8;
  }
  {
    /* glibc 2.7 and cygwin 1.5.24 misparse "nan()".  */
    const char *string = "nan()";
    char *term;
    double value = strtod (string, &term);
    if (numeric_equal (value, value) || term != (string + 5))
      result |= 16;
  }
  {
    /* Darwin 10.6.1 (macOS 10.6.6) misparses "nan(".  */
    const char *string = "nan(";
    char *term;
    double value = strtod (string, &term);
    if (numeric_equal (value, value) || term != (string + 3))
      result |= 16;
  }
#ifndef _MSC_VER /* On MSVC, this is expected behaviour.  */
  {
    /* In Cygwin 2.9, strtod does not set errno upon
       gradual underflow.  */
    const char *string = "1e-320";
    char *term;
    double value;
    errno = 0;
    value = strtod (string, &term);
    if (term != (string + 6)
        || (value > 0.0 && value <= DBL_MIN && errno != ERANGE))
      result |= 32;
  }
#endif
  {
    /* strtod could not set errno upon
       flush-to-zero underflow.  */
    const char *string = "1E-100000";
    char *term;
    double value;
    errno = 0;
    value = strtod (string, &term);
    if (term != (string + 9) || (value == 0.0L && errno != ERANGE))
      result |= 64;
  }
  return result;
]])],
        [gl_cv_func_strtod_works=yes],
        [result=$?
         if expr $result '>=' 64 >/dev/null; then
           gl_cv_func_strtod_works="no (underflow problem)"
         else
           if expr $result '>=' 32 >/dev/null; then
             gl_cv_func_strtod_works="no (gradual underflow problem)"
           else
             gl_cv_func_strtod_works=no
           fi
         fi
        ],
        [dnl The last known bugs in glibc strtod(), as of this writing,
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
           [gl_cv_func_strtod_works="guessing yes"],
           [case "$host_os" in
                                  # Guess yes on musl systems.
              *-musl* | midipix*) gl_cv_func_strtod_works="guessing yes" ;;
                                  # Guess 'no (gradual underflow problem)' on Cygwin.
              cygwin*)            gl_cv_func_strtod_works="guessing no (gradual underflow problem)" ;;
                                  # Guess yes on native Windows.
              mingw* | windows*)  gl_cv_func_strtod_works="guessing yes" ;;
              *)                  gl_cv_func_strtod_works="$gl_cross_guess_normal" ;;
            esac
           ])
        ])
      ])
    case "$gl_cv_func_strtod_works" in
      *yes) ;;
      *)
        REPLACE_STRTOD=1
        case "$gl_cv_func_strtod_works" in
          *"no (underflow problem)")
            AC_DEFINE([STRTOD_HAS_UNDERFLOW_BUG], [1],
              [Define to 1 if strtod does not set errno upon flush-to-zero underflow.])
            ;;
          *"no (gradual underflow problem)")
            AC_DEFINE([STRTOD_HAS_GRADUAL_UNDERFLOW_PROBLEM], [1],
              [Define to 1 if strtod does not set errno upon gradual underflow.])
            ;;
        esac
        ;;
    esac
  fi
])

# Prerequisites of lib/strtod.c.
AC_DEFUN([gl_PREREQ_STRTOD], [
  AC_REQUIRE([gl_CHECK_LDEXP_NO_LIBM])
  if test $gl_cv_func_ldexp_no_libm = yes; then
    AC_DEFINE([HAVE_LDEXP_IN_LIBC], [1],
      [Define if the ldexp function is available in libc.])
  fi
  gl_CHECK_FUNCS_ANDROID([nl_langinfo], [[#include <langinfo.h>]])
])
