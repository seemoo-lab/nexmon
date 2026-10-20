dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-go-http], [0])
AC_CONFIG_SRCDIR([example1/go.mod.in])
AM_INIT_AUTOMAKE([1.11])

gt_GOCOMP
if test -z "$GO"; then
  echo "*** Essential program go or go-<version> not found" 1>&2
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

AC_CONFIG_FILES([Makefile])
dnl Support for VPATH builds:
dnl We need a copy of go.mod in the build directory, otherwise we get an error
dnl "go.mod file not found in current directory or any parent directory;
dnl  see 'go help modules'".
AC_CONFIG_FILES([example1/Makefile example1/go.mod example1/hello1ml.go])
AC_CONFIG_FILES([gocomp.sh])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])
AC_OUTPUT
