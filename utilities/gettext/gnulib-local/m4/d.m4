# d.m4
# serial 1
dnl Copyright (C) 2025 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# Sets D_CHOICE to 'yes' or 'no', depending on the preferred use of D.
AC_DEFUN([gt_D_CHOICE],
[
  AC_MSG_CHECKING([whether to use D])
  AC_ARG_ENABLE(d,
    [  --disable-d             do not build D sources],
    [D_CHOICE="$enableval"],
    [D_CHOICE=yes])
  AC_MSG_RESULT([$D_CHOICE])
  AC_SUBST([D_CHOICE])
])
