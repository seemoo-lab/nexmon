# findprog.m4 serial 2
dnl Copyright (C) 2003, 2009-2016 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FINDPROG],
[
  dnl Prerequisites of lib/findprog.c.
  AC_CHECK_HEADERS_ONCE([unistd.h])
  AC_REQUIRE([gl_FUNC_EACCESS])
])
