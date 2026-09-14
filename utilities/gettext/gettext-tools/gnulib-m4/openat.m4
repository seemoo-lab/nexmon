# openat.m4
# serial 47
dnl Copyright (C) 2004-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# See if we need to use our replacement for Solaris' openat et al functions.

# Written by Jim Meyering.

AC_DEFUN([gl_FUNC_OPENAT],
[
  AC_REQUIRE([gl_FCNTL_H_DEFAULTS])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_FUNCS_ONCE([openat])
  AC_REQUIRE([gl_FCNTL_O_FLAGS])
  AC_REQUIRE([gl_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
  AC_REQUIRE([gl_PREPROC_O_CLOEXEC])
  AS_CASE([$ac_cv_func_openat+$gl_cv_header_working_fcntl_h+$gl_cv_func_lstat_dereferences_slashed_symlink+$gl_cv_macro_O_CLOEXEC],
    [yes+*O_DIRECTORY*+*+* | yes+*no+*+*], [REPLACE_OPENAT=1],
    [yes+*+*yes+yes], [],
    [yes+*], [REPLACE_OPENAT=1],
    [HAVE_OPENAT=0])
])

# Prerequisites of lib/openat.c.
AC_DEFUN([gl_PREREQ_OPENAT],
[
  AC_REQUIRE([gl_PROMOTED_TYPE_MODE_T])
  :
])
