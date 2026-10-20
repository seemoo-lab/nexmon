# isnanf.m4
# serial 22
dnl Copyright (C) 2007-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

dnl Check how to get or define isnanf().

AC_DEFUN([gl_FUNC_ISNANF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  ISNANF_LIBM=
  gl_HAVE_ISNANF_NO_LIBM
  if test $gl_cv_func_isnanf_no_libm = no; then
    gl_HAVE_ISNANF_IN_LIBM
    if test $gl_cv_func_isnanf_in_libm = yes; then
      ISNANF_LIBM=-lm
    fi
  fi
  dnl The variable gl_func_isnanf set here is used by isnan.m4.
  if test $gl_cv_func_isnanf_no_libm = yes || test -n "$ISNANF_LIBM"; then
    saved_LIBS="$LIBS"
    LIBS="$LIBS $ISNANF_LIBM"
    gl_ISNANF_WORKS
    LIBS="$saved_LIBS"
    case "$gl_cv_func_isnanf_works" in
      *yes) gl_func_isnanf=yes ;;
      *)    gl_func_isnanf=no; ISNANF_LIBM= ;;
    esac
  else
    gl_func_isnanf=no
  fi
  if test $gl_func_isnanf != yes; then
    HAVE_ISNANF=0
  fi
  AC_SUBST([ISNANF_LIBM])
])

dnl Check how to get or define isnanf() without linking with libm.

AC_DEFUN([gl_FUNC_ISNANF_NO_LIBM],
[
  gl_HAVE_ISNANF_NO_LIBM
  if test $gl_cv_func_isnanf_no_libm = yes; then
    gl_ISNANF_WORKS
  fi
  if test $gl_cv_func_isnanf_no_libm = yes \
     && { case "$gl_cv_func_isnanf_works" in
            *yes) true;;
            *) false;;
          esac
        }; then
    gl_func_isnanf_no_libm=yes
    AC_DEFINE([HAVE_ISNANF_IN_LIBC], [1],
      [Define if the isnan(float) function is available in libc.])
  else
    gl_func_isnanf_no_libm=no
  fi
])

dnl Prerequisites of replacement isnanf definition. It does not need -lm.
AC_DEFUN([gl_PREREQ_ISNANF],
[
  gl_FLOAT_EXPONENT_LOCATION
])

dnl Test whether isnanf() can be used without libm.
AC_DEFUN([gl_HAVE_ISNANF_NO_LIBM],
[
  AC_CACHE_CHECK([whether isnan(float) can be used without linking with libm],
    [gl_cv_func_isnanf_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#include <math.h>
             #if (__GNUC__ >= 4) || (__clang_major__ >= 4)
             # undef isnanf
             # define isnanf(x) __builtin_isnan ((float)(x))
             #elif defined isnan
             # undef isnanf
             # define isnanf(x) isnan ((float)(x))
             #endif
             float x;]],
           [[return isnanf (x);]])],
        [gl_cv_func_isnanf_no_libm=yes],
        [gl_cv_func_isnanf_no_libm=no])
    ])
])

dnl Test whether isnanf() can be used with libm.
AC_DEFUN([gl_HAVE_ISNANF_IN_LIBM],
[
  AC_CACHE_CHECK([whether isnan(float) can be used with libm],
    [gl_cv_func_isnanf_in_libm],
    [
      saved_LIBS="$LIBS"
      LIBS="$LIBS -lm"
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#include <math.h>
             #if (__GNUC__ >= 4) || (__clang_major__ >= 4)
             # undef isnanf
             # define isnanf(x) __builtin_isnan ((float)(x))
             #elif defined isnan
             # undef isnanf
             # define isnanf(x) isnan ((float)(x))
             #endif
             float x;]],
           [[return isnanf (x);]])],
        [gl_cv_func_isnanf_in_libm=yes],
        [gl_cv_func_isnanf_in_libm=no])
      LIBS="$saved_LIBS"
    ])
])

dnl Test whether isnanf() rejects Infinity (this fails on Solaris 2.5.1).
AC_DEFUN([gl_ISNANF_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_REQUIRE([gl_FLOAT_EXPONENT_LOCATION])
  AC_CACHE_CHECK([whether isnan(float) works], [gl_cv_func_isnanf_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <math.h>
#if (__GNUC__ >= 4) || (__clang_major__ >= 4)
# undef isnanf
# define isnanf(x) __builtin_isnan ((float)(x))
#elif defined isnan
# undef isnanf
# define isnanf(x) isnan ((float)(x))
#endif
int main()
{
  int result = 0;

  if (isnanf (1.0f / 0.0f))
    result |= 1;

  return result;
}]])],
        [gl_cv_func_isnanf_works=yes],
        [gl_cv_func_isnanf_works=no],
        [case "$host_os" in
           solaris*) gl_cv_func_isnanf_works="guessing no" ;;
           mingw* | windows*) # Guess yes on mingw, no on MSVC.
             AC_EGREP_CPP([Known], [
#ifdef __MINGW32__
 Known
#endif
               ],
               [gl_cv_func_isnanf_works="guessing yes"],
               [gl_cv_func_isnanf_works="guessing no"])
             ;;
           *) gl_cv_func_isnanf_works="guessing yes" ;;
         esac
        ])
    ])
])
