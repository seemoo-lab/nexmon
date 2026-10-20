# htonl.m4
# serial 2
dnl Copyright (C) 2024-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

dnl Written by Collin Funk.

AC_DEFUN([gl_FUNC_HTONL],
[
  AC_REQUIRE([gl_ARPA_INET_H_DEFAULTS])

  dnl On Windows htonl and friends require -lws2_32 and inclusion of
  dnl winsock2.h.
  HTONL_LIB=
  gl_PREREQ_SYS_H_WINSOCK2
  if test $HAVE_WINSOCK2_H = 1; then
    HTONL_LIB="-lws2_32"
  elif test $ac_cv_header_arpa_inet_h = yes; then
    AC_CHECK_DECLS([htons, htonl, ntohs, ntohl],,, [[#include <arpa/inet.h>]])
  fi

  AC_SUBST([HTONL_LIB])
])
