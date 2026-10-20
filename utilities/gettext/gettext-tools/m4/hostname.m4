# hostname.m4
# serial 3 (gettext-1.0)
dnl Copyright (C) 2001-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

dnl From Bruno Haible.

# Prerequisites of the hostname.c program.
AC_DEFUN([gt_PREREQ_HOSTNAME],
[
  AC_CHECK_HEADERS([arpa/inet.h])
  AC_CHECK_FUNCS([gethostname gethostbyname inet_ntop])

  AC_CACHE_CHECK([for IPv6 sockets],
    [gt_cv_socket_ipv6],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
           #include <sys/types.h>
           #include <sys/socket.h>
           #include <netinet/in.h>
         ]],
         [[int x = AF_INET6; struct in6_addr y; struct sockaddr_in6 z;]])],
       [gt_cv_socket_ipv6=yes],
       [gt_cv_socket_ipv6=no])
    ])
  if test $gt_cv_socket_ipv6 = yes; then
    AC_DEFINE([HAVE_IPV6], [1], [Define if <sys/socket.h> defines AF_INET6.])
  fi
])
