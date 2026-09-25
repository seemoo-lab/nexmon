# strerrorname_np.m4
# serial 8
dnl Copyright (C) 2020-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_FUNC_STRERRORNAME_NP],
[
  AC_REQUIRE([gl_STRING_H_DEFAULTS])

  AC_REQUIRE([gl_CHECK_STRERRORNAME_NP])
  if test $ac_cv_func_strerrorname_np = yes; then
    case "$gl_cv_func_strerrorname_np_works" in
      *yes) ;;
      *) REPLACE_STRERRORNAME_NP=1 ;;
    esac
  else
    HAVE_STRERRORNAME_NP=0
    case "$gl_cv_onwards_func_strerrorname_np" in
      future*) REPLACE_STRERRORNAME_NP=1 ;;
    esac
  fi
])

# Check for a working strerrorname_np function.
# Sets ac_cv_func_strerrorname_np, gl_cv_onwards_func_strerrorname_np,
# gl_cv_func_strerrorname_np_works.
AC_DEFUN([gl_CHECK_STRERRORNAME_NP],
[
  dnl Persuade glibc <string.h> to declare strerrorname_np().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  gl_CHECK_FUNCS_ANDROID([strerrorname_np], [[#include <string.h>]])
  if test $ac_cv_func_strerrorname_np = yes; then
    dnl In glibc 2.32, strerrorname_np returns English error descriptions, not
    dnl error names.
    dnl See <https://sourceware.org/PR26555>.
    dnl In glibc 2.36, strerrorname_np returns NULL for EDEADLOCK on powerpc and
    dnl sparc platforms.
    dnl See <https://sourceware.org/PR29545>.
    dnl In glibc 2.37, strerrorname_np returns NULL for ENOSYM and
    dnl EREMOTERELEASE on hppa platforms.
    dnl See <https://sourceware.org/PR31080>.
    dnl In Solaris 11 OmniOS, strerrorname_np returns NULL for ERESTART
    dnl and ESTRPIPE.
    dnl see <https://www.illumos.org/issues/17134>.
    AC_CACHE_CHECK([whether strerrorname_np works],
      [gl_cv_func_strerrorname_np_works],
      [AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <errno.h>
              #include <string.h>
            ]],
            [[return
                strcmp (strerrorname_np (EINVAL), "EINVAL") != 0
                #ifdef EDEADLOCK
                || strerrorname_np (EDEADLOCK) == NULL
                #endif
                #ifdef ENOSYM
                || strerrorname_np (ENOSYM) == NULL
                #endif
                #ifdef ERESTART
                || strerrorname_np (ERESTART) == NULL
                #endif
                #ifdef ESTRPIPE
                || strerrorname_np (ESTRPIPE) == NULL
                #endif
                ;
            ]])],
         [gl_cv_func_strerrorname_np_works=yes],
         [gl_cv_func_strerrorname_np_works=no],
         [case "$host_os" in
            # Guess no on glibc systems.
            *-gnu* | gnu*)
              gl_cv_func_strerrorname_np_works="guessing no" ;;
            # Otherwise obey --enable-cross-guesses.
            *)
              gl_cv_func_strerrorname_np_works="$gl_cross_guess_normal" ;;
          esac
         ])
      ])
  fi
])

# Prerequisite for using strerrorname_np when available.
AC_DEFUN_ONCE([gl_OPTIONAL_STRERRORNAME_NP],
[
  AC_REQUIRE([gl_CHECK_STRERRORNAME_NP])
  if test $ac_cv_func_strerrorname_np = yes; then
    case "$gl_cv_func_strerrorname_np_works" in
      *yes)
        AC_DEFINE([HAVE_WORKING_STRERRORNAME_NP], [1],
          [Define to 1 if the function strerrorname_np exists and works.])
        ;;
    esac
  fi
])
