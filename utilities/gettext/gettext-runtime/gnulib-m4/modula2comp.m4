# modula2comp.m4
# serial 1
dnl Copyright (C) 2025-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# Checks for a Modula-2 implementation.
# Sets M2C and M2FLAGS (options that can be used with "$M2C").
AC_DEFUN([gt_MODULA2COMP],
[
  AC_MSG_CHECKING([for GNU Modula-2 compiler])
  pushdef([AC_MSG_CHECKING],[:])dnl
  pushdef([AC_CHECKING],[:])dnl
  pushdef([AC_MSG_RESULT],[:])dnl
  AC_ARG_VAR([M2C], [Modula-2 compiler command])
  AC_ARG_VAR([M2FLAGS], [Modula-2 compiler options])
  AC_CHECK_TOOLS([M2C], [gm2])
  popdef([AC_MSG_RESULT])dnl
  popdef([AC_CHECKING])dnl
  popdef([AC_MSG_CHECKING])dnl
  if test -n "$M2C"; then
    ac_result="$M2C"
  else
    ac_result="no"
  fi
  AC_MSG_RESULT([$ac_result])
  AC_SUBST([M2C])
  if test -z "$M2FLAGS" && test -n "$M2C"; then
    M2FLAGS="-g -O2"
  fi
  AC_SUBST([M2FLAGS])
])
