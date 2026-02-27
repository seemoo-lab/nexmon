# hostname.m4 serial 1 (gettext-0.11)
dnl Copyright (C) 2001-2002, 2015-2016 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prerequisites of the hostname.c program.
AC_DEFUN([gt_PREREQ_HOSTNAME],
[
  AC_CHECK_HEADERS(arpa/inet.h)
  AC_CHECK_FUNCS(gethostname gethostbyname inet_ntop)

  AC_MSG_CHECKING([for IPv6 sockets])
  AC_CACHE_VAL(gt_cv_socket_ipv6,[
    AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
[int x = AF_INET6; struct in6_addr y; struct sockaddr_in6 z;],
      gt_cv_socket_ipv6=yes, gt_cv_socket_ipv6=no)
  ])
  AC_MSG_RESULT($gt_cv_socket_ipv6)
  if test $gt_cv_socket_ipv6 = yes; then
    AC_DEFINE(HAVE_IPV6, 1, [Define if <sys/socket.h> defines AF_INET6.])
  fi
])
