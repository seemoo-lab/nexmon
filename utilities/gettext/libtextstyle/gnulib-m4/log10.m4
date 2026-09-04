# log10.m4
# serial 16
dnl Copyright (C) 2011-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_FUNC_LOG10],
[
  m4_divert_text([DEFAULTS], [gl_log10_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Determine LOG10_LIBM.
  gl_COMMON_DOUBLE_MATHFUNC([log10])

  m4_ifdef([gl_FUNC_LOG10_IEEE], [
    if test $gl_log10_required = ieee; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether log10 works according to ISO C 99 with IEC 60559],
        [gl_cv_func_log10_ieee],
        [
          saved_LIBS="$LIBS"
          LIBS="$LIBS $LOG10_LIBM"
          AC_RUN_IFELSE(
            [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
/* Compare two numbers with ==.
   This is a separate function in order to disable compiler optimizations.  */
static int
numeric_equal (double x, double y)
{
  return x == y;
}
static double dummy (double x) { return 0; }
int main (int argc, char *argv[])
{
  double (* volatile my_log10) (double) = argc ? log10 : dummy;
  /* Test log10(negative).
     This test fails on NetBSD 5.1, Solaris 11.4.  */
  double y = my_log10 (-1.0);
  if (numeric_equal (y, y))
    return 1;
  return 0;
}
            ]])],
            [gl_cv_func_log10_ieee=yes],
            [gl_cv_func_log10_ieee=no],
            [case "$host_os" in
                                   # Guess yes on glibc systems.
               *-gnu* | gnu*)      gl_cv_func_log10_ieee="guessing yes" ;;
                                   # Guess yes on musl systems.
               *-musl* | midipix*) gl_cv_func_log10_ieee="guessing yes" ;;
                                   # Guess yes on native Windows.
               mingw* | windows*)  gl_cv_func_log10_ieee="guessing yes" ;;
                                   # If we don't know, obey --enable-cross-guesses.
               *)                  gl_cv_func_log10_ieee="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$saved_LIBS"
        ])
      case "$gl_cv_func_log10_ieee" in
        *yes) ;;
        *) REPLACE_LOG10=1 ;;
      esac
    fi
  ])
])
