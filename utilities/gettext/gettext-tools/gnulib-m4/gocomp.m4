# gocomp.m4
# serial 1
dnl Copyright (C) 2025-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

# There are two Go implementations, that differ essentially only regarding
# the compiler and the used runtime version:
#   * The golang Go is the original implementation, typically a little more
#     up-to-date. It supports only the major computing platforms.
#   * The GCC Go implementation is part of the GNU Compiler Collection.
#     It lags behind a bit. It supports all platforms supported by GCC.
# The golang Go implementation produces smaller executables, and the
# binary packages needed for development are smaller as well.
#
# Therefore the preferred implementation is
#   * golang where available,
#   * GCC on the platforms where golang is not available. This includes in
#     particular:
#     - Linux/alpha, Linux/hppa, Linux/m68k, Linux/sparc, Linux/powerpc (32-bit)
#     - GNU/Hurd
#     - Solaris/sparc
#     - Haiku

# Sets GO_CHOICE to the preferred Go compiler implementation:
# 'golang' or 'gcc' or 'any' or 'no'.
# Here
#   - 'golang' means the original Go compiler.
#   - 'gcc' means GCC Go compiler.
# The runtime library of both is the same (possibly in different versions,
# though).
AC_DEFUN([gt_GO_CHOICE],
[
  AC_MSG_CHECKING([for preferred Go compiler])
  AC_ARG_ENABLE([go],
    [  --enable-go[[=IMPL]]      choose preferred Go compiler (golang, gcc)],
    [GO_CHOICE="$enableval"],
    [GO_CHOICE=any])
  AC_SUBST([GO_CHOICE])
  AC_MSG_RESULT([$GO_CHOICE])
])

# How to run Go programs?
# Assume you want to distribute a Go program. In which form? And what are
# the dependencies to do so?
#
# There are three ways to do so:
#
#   * Run the program by compiling it on-the-fly in a temporary directory
#     and running it from there:
#       ${GO} run foo.go
#     You would distribute foo.go (very small).
#     But the dependencies are large: the Go development environment.
#     On Ubuntu 24.04 this is:
#     - with 'golang': the package 'golang-go'
#       11 MB for /usr/lib/go-1.22/bin/go,
#       85 MB for /usr/lib/go-1.22/pkg/tool/linux_amd64/*
#       = 96 MB in total
#     - with 'gccgo': the package 'gccgo-13'
#       60 MB for /usr/lib/x86_64-linux-gnu/libgo.so.22.0.0,
#       > 40 MB for /usr/libexec/gcc/x86_64-linux-gnu/13/go1
#               and /usr/bin/x86_64-linux-gnu-go-13,
#       116 MB for /usr/lib/gcc/x86_64-linux-gnu/13/libgo.a
#       = 216 MB in total.
#
#   * You distribute the binary, linked against the shared Go runtime library.
#     Running the program is just invoking that binary.
#     On Ubuntu the dependencies are:
#     - with 'golang': unsupported.
#     - with 'gccgo': the package 'libgo22'
#       60 MB for /usr/lib/x86_64-linux-gnu/libgo.so.22.0.0
#       = 60 MB in total.
#
#   * You distribute the binary, linked statically with the needed parts
#     of the Go runtime library:
#       ${GO} build ${GOCOMPFLAGS} foo.go
#     Running the program is just invoking that binary.
#     On Ubuntu the dependencies are:
#     - with 'golang': No dependencies; the binary is statically linked.
#       The stripped executable's size is >= 1 MB.
#     - with 'gccgo': No dependencies; the binary is statically linked
#       (when using GOCOMPFLAGS='-static') or statically linked except for
#       the standard C library (when using GOCOMPFLAGS='-static-libgo').
#       The stripped executable's size is 5 MB or 6 MB, respectively.
#       Distros generally prefer avoiding dynamic linking with libc,
#       so let's use that.
#
# It is clear that the third approach will be preferred for small programs.

# Prerequisites of gocomp.sh.
# Checks for a Go implementation.
# Sets HAVE_GOCOMP to nonempty if gocomp.sh will work.
# Also sets GO and GOCOMPFLAGS (options that can be used with "$GO build").
AC_DEFUN([gt_GOCOMP],
[
  AC_REQUIRE([gt_GO_CHOICE])
  AC_MSG_CHECKING([for Go compiler])
  HAVE_GOCOMP=1
  pushdef([AC_MSG_CHECKING],[:])dnl
  pushdef([AC_CHECKING],[:])dnl
  pushdef([AC_MSG_RESULT],[:])dnl
  dnl Prefer golang over gcc by default, because it produces much smaller
  dnl executables (see above).
  if test "$GO_CHOICE" = gcc; then
    AC_PATH_PROGS([GO], [go-30 go-29 go-28 go-27 go-26 go-25 go-24 go-23 go-22 go-21 go-20 go-19 go-18 go-17 go-16 go-15 go-14 go-13 go-12 go-11 go-10 go-9 go-8 go-7 go-6 go-5 go])
  else
    AC_PATH_PROGS([GO], [go go-30 go-29 go-28 go-27 go-26 go-25 go-24 go-23 go-22 go-21 go-20 go-19 go-18 go-17 go-16 go-15 go-14 go-13 go-12 go-11 go-10 go-9 go-8 go-7 go-6 go-5])
  fi
  popdef([AC_MSG_RESULT])dnl
  popdef([AC_CHECKING])dnl
  popdef([AC_MSG_CHECKING])dnl
  if test -n "$GO"; then
    case "$GO" in
      go | */go ) GOCOMPFLAGS= ;;
      *)          GOCOMPFLAGS='-gccgoflags=-static-libgo' ;;
    esac
    ac_result="$GO"
  else
    HAVE_GOCOMP=
    ac_result="no"
  fi
  AC_MSG_RESULT([$ac_result])
  AC_SUBST([GO])
  AC_SUBST([GOCOMPFLAGS])
])
