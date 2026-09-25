dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-ocaml], [0])
AC_CONFIG_SRCDIR([hello.ml.in])
AM_INIT_AUTOMAKE([1.11])

dnl Check for availability of the 'dune' program.
AC_CHECK_PROGS([DUNE], [dune])
if test -z "$DUNE"; then
  echo "*** Essential program dune not found" 1>&2
  exit 1
fi

dnl The installed program must know where to find its message catalogs.
dnl Unfortunately, prefix gets only finally determined at the end of configure.
if test "X$prefix" = "XNONE"; then
  final_prefix="$ac_default_prefix"
else
  final_prefix="$prefix"
fi
save_prefix="$prefix"
prefix="$final_prefix"
eval "datarootdir=\"${datarootdir}\""
eval "localedir=\"${datarootdir}/locale\""
prefix="$save_prefix"
AC_SUBST([localedir])

dnl Support for the po directory.
AM_PO_SUBDIRS

AC_CONFIG_FILES([Makefile hello.ml])
dnl Support for VPATH builds:
dnl We need a copy of dune and dune-project in the build directory. Otherwise
dnl "dune build ..." creates its '_build' directory in the source directory.
AC_CONFIG_FILES([dune dune-project])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])
AC_OUTPUT
