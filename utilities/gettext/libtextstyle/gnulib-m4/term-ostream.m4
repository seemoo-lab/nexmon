# term-ostream.m4
# serial 6
dnl Copyright (C) 2006-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

dnl From Bruno Haible.

AC_DEFUN([gl_TERM_OSTREAM],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_CHECK_FUNCS_ONCE([getsid tcdrain])
])
