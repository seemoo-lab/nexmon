# modula2.m4
# serial 1
dnl Copyright (C) 2025 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# Sets MODULA2_CHOICE to 'yes' or 'no', depending on the preferred use of
# Modula-2.
AC_DEFUN([gt_MODULA2_CHOICE],
[
  AC_MSG_CHECKING([whether to use Modula-2])
  AC_ARG_ENABLE(modula2,
    [  --disable-modula2       do not build Modula-2 sources],
    [MODULA2_CHOICE="$enableval"],
    [MODULA2_CHOICE=yes])
  AC_MSG_RESULT([$MODULA2_CHOICE])
  AC_SUBST([MODULA2_CHOICE])
])
