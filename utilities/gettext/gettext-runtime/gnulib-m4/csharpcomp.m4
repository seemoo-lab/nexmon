# csharpcomp.m4
# serial 11
dnl Copyright (C) 2003-2005, 2007, 2009-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# Prerequisites of csharpcomp.sh.
# Checks for a C# compiler.
# Sets at most one of HAVE_MCS, HAVE_DOTNET_SDK, HAVE_DOTNET_CSC, HAVE_CSC.
# Sets HAVE_CSHARPCOMP to nonempty if csharpcomp.sh will work.
# Also sets CSHARPCOMPFLAGS.
AC_DEFUN([gt_CSHARPCOMP],
[
  AC_REQUIRE([gt_CSHARP_CHOICE])
  AC_MSG_CHECKING([for C[#] compiler])
  HAVE_CSHARPCOMP=1
  pushdef([AC_MSG_CHECKING],[:])dnl
  pushdef([AC_CHECKING],[:])dnl
  pushdef([AC_MSG_RESULT],[:])dnl
  AC_CHECK_PROG([HAVE_MCS_IN_PATH], [mcs], [yes])
  AC_CHECK_PROG([HAVE_DOTNET_IN_PATH], [dotnet], [yes])
  AC_CHECK_PROG([HAVE_CSC_IN_PATH], [csc], [yes])
  popdef([AC_MSG_RESULT])dnl
  popdef([AC_CHECKING])dnl
  popdef([AC_MSG_CHECKING])dnl
  for impl in "$CSHARP_CHOICE" mono dotnet sscli no; do
    case "$impl" in
      mono)
        if test -n "$HAVE_MCS_IN_PATH" \
           && mcs --version >/dev/null 2>/dev/null \
           && mcs --version 2>/dev/null | grep Mono >/dev/null; then
          HAVE_MCS=1
          ac_result="mcs"
          break
        fi
        ;;
      dotnet)
        # The dotnet compiler is called "Roslyn".
        # <https://en.wikipedia.org/wiki/Roslyn_(compiler)>
        # There are two situations:
        # - A dotnet SDK, that contains a 'dotnet' program and the Roslyn
        #   compiler as csc.dll.
        # - An MSVC installation, that contains the Roslyn compiler as csc.exe
        #   (e.g. in C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\Roslyn\csc.exe).
        # In the first case, the user only has to make sure that 'dotnet' is
        # found in $PATH.
        # In the second case, they need to make sure that both 'dotnet' and
        # 'csc' are found in $PATH.
        if test -n "$HAVE_DOTNET_IN_PATH" \
           && dotnet --list-runtimes >/dev/null 2>/dev/null \
           && test -n "`dotnet --list-sdks 2>/dev/null`"; then
          HAVE_DOTNET_SDK=1
          ac_result="dotnet"
          break
        else
          if test -n "$HAVE_CSC_IN_PATH" \
             && csc -help 2>/dev/null | grep analyzer >/dev/null \
             && { if csc -help 2>/dev/null | grep -i chicken > /dev/null; then false; else true; fi; }; then
            HAVE_DOTNET_CSC=1
            ac_result="dotnet"
            break
          fi
        fi
        ;;
      sscli)
        if test -n "$HAVE_CSC_IN_PATH" \
           && csc -help >/dev/null 2>/dev/null \
           && { if csc -help 2>/dev/null | grep -i chicken > /dev/null; then false; else true; fi; }; then
          HAVE_CSC=1
          ac_result="csc"
          break
        fi
        ;;
      no)
        HAVE_CSHARPCOMP=
        ac_result="no"
        break
        ;;
    esac
  done
  AC_MSG_RESULT([$ac_result])
  AC_SUBST([HAVE_MCS])
  AC_SUBST([HAVE_DOTNET_SDK])
  AC_SUBST([HAVE_DOTNET_CSC])
  AC_SUBST([HAVE_CSC])
  dnl Provide a default for CSHARPCOMPFLAGS.
  if test -z "${CSHARPCOMPFLAGS+set}"; then
    CSHARPCOMPFLAGS="-O -g"
  fi
  AC_SUBST([CSHARPCOMPFLAGS])
])
