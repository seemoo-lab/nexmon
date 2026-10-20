# csharp.m4
# serial 5
dnl Copyright (C) 2004-2005, 2009-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# Sets CSHARP_CHOICE to the preferred C# implementation:
# 'mono' or 'dotnet' or 'any' or 'no'.
# Here
#   - 'mono' means <https://en.wikipedia.org/wiki/Mono_(software)>.
#   - 'dotnet' means the (newer) .NET <https://en.wikipedia.org/wiki/.NET>,
#     *not* the .NET framework <https://en.wikipedia.org/wiki/.NET_Framework>.
AC_DEFUN([gt_CSHARP_CHOICE],
[
  AC_MSG_CHECKING([for preferred C[#] implementation])
  AC_ARG_ENABLE([csharp],
    [  --enable-csharp[[=IMPL]]  choose preferred C[#] implementation (mono, dotnet)],
    [CSHARP_CHOICE="$enableval"],
    CSHARP_CHOICE=any)
  AC_SUBST([CSHARP_CHOICE])
  AC_MSG_RESULT([$CSHARP_CHOICE])
  case "$CSHARP_CHOICE" in
    mono)
      AC_DEFINE([CSHARP_CHOICE_MONO], [1],
        [Define if mono is the preferred C# implementation.])
      ;;
    dotnet)
      AC_DEFINE([CSHARP_CHOICE_DOTNET], [1],
        [Define if dotnet is the preferred C# implementation.])
      ;;
  esac
])
