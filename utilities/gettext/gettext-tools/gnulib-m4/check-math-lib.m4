# check-math-lib.m4
# serial 6
dnl Copyright (C) 2007, 2009-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.
dnl
dnl gl_CHECK_MATH_LIB (VARIABLE, TYPE, EXPRESSION [, EXTRA-CODE])
dnl
dnl Sets the shell VARIABLE according to the libraries needed by EXPRESSION
dnl (that operates on a variable x of type TYPE) to compile and link: to the
dnl empty string if no extra libraries are needed, to "-lm" if -lm is needed,
dnl or to "missing" if it does not compile and link either way.
dnl
dnl Example: gl_CHECK_MATH_LIB([ROUNDF_LIBM], [float], [x = roundf (x);])
AC_DEFUN([gl_CHECK_MATH_LIB], [
  saved_LIBS="$LIBS"
  $1=missing
  for libm in "" "-lm"; do
    LIBS="$saved_LIBS $libm"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
         #ifndef __NO_MATH_INLINES
         # define __NO_MATH_INLINES 1 /* for glibc */
         #endif
         #include <math.h>
         $4
         $2 x;]],
      [$3])],
      [$1=$libm
break])
  done
  LIBS="$saved_LIBS"
])
