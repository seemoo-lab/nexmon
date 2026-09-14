# dcomp.m4
# serial 3
dnl Copyright (C) 2025-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# There are three D implementations, see
# <https://en.wikipedia.org/wiki/D_(programming_language)#Implementations>
# <https://wiki.dlang.org/Compilers>
# <https://dub.pm/dub-reference/build_settings/#buildoptions>
# Although each has different possible options, a few options are accepted by
# all of the implementations: -c, -I, -g, -O.  Some essential options, however,
# are different:
#          gdc          ldc2                  dmd
#          ----------   -------------------   ------------
#          -oFILE       -of=FILE, --of=FILE   -of=FILE
#          -lLIBRARY    -L -lLIBRARY          -L=-lLIBRARY
#          -LDIR        -L -LDIR              -L=-LDIR
#          -Wl,OPTION   -L OPTION             -L=OPTION

# Checks for a D implementation.
# Sets DC and DFLAGS (options that can be used with "$DC").
AC_DEFUN([gt_DCOMP],
[
  AC_MSG_CHECKING([for D compiler])
  pushdef([AC_MSG_CHECKING],[:])dnl
  pushdef([AC_CHECKING],[:])dnl
  pushdef([AC_MSG_RESULT],[:])dnl
  AC_ARG_VAR([DC], [D compiler command])
  AC_ARG_VAR([DFLAGS], [D compiler options])
  dnl On OpenBSD, gdc is called 'egdc' and works less well than dmd.
  dnl Like AC_CHECK_TOOLS([DC], [gdc ldc2 dmd egdc]) but check whether the
  dnl compiler can actually compile a trivial program. We may simplify this
  dnl once <https://savannah.gnu.org/support/index.php?111256> is implemented.
  if test -z "$DC"; then
    echo > empty.d
    if test -n "${host_alias}"; then
      if test -z "$DC"; then
        DC="${host_alias}-gdc"
        echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
        if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
      fi
      if test -z "$DC"; then
        DC="${host_alias}-ldc2"
        echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
        if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
      fi
      if test -z "$DC"; then
        DC="${host_alias}-dmd"
        echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
        if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
      fi
      if test -z "$DC"; then
        DC="${host_alias}-egdc"
        echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
        if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
      fi
    fi
    if test -z "$DC"; then
      DC="gdc"
      echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
      if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
    fi
    if test -z "$DC"; then
      DC="ldc2"
      echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
      if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
    fi
    if test -z "$DC"; then
      DC="dmd"
      echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
      if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
    fi
    if test -z "$DC"; then
      DC="egdc"
      echo "$as_me:${as_lineno-$LINENO}: trying ${DC}..." >&AS_MESSAGE_LOG_FD
      if ${DC} -c empty.d 2>&AS_MESSAGE_LOG_FD; then :; else DC= ; fi
    fi
    rm -f empty.d empty.o empty.obj
  fi
  popdef([AC_MSG_RESULT])dnl
  popdef([AC_CHECKING])dnl
  popdef([AC_MSG_CHECKING])dnl
  if test -n "$DC"; then
    ac_result="$DC"
  else
    ac_result="no"
  fi
  AC_MSG_RESULT([$ac_result])
  AC_SUBST([DC])
  if test -z "$DFLAGS" && test -n "$DC"; then
    case `$DC --version | sed -e 's/ .*//' -e 1q` in
      gdc | *-gdc | egdc | *-egdc | LDC*) DFLAGS="-g -O2" ;;
      *)                                  DFLAGS="-g -O" ;;
    esac
  fi
  AC_SUBST([DFLAGS])
])
