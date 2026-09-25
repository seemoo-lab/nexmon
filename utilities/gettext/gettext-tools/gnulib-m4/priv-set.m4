# priv-set.m4
# serial 8
dnl Copyright (C) 2009-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# Written by David Bartley.

AC_DEFUN([gl_PRIV_SET],
[
  AC_CHECK_FUNCS([getppriv])
  AC_CHECK_HEADERS_ONCE([priv.h])
])
