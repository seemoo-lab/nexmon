dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-modula2], [0])
AC_CONFIG_SRCDIR([hello.mod.in])
AM_INIT_AUTOMAKE([1.11])

gt_MODULA2COMP
if test -z "$M2C"; then
  echo "*** Modula-2 compiler not found" 1>&2
  exit 1
fi

M2_LIBRARIES_PREFIX=
M2_LIBRARIES_EXECPREFIX=
AC_ARG_WITH([m2-libraries-prefix],
  [[  --with-m2-libraries-prefix=DIR  search for Modula-2 libraries in DIR/include/m2, DIR/lib]],
  [if test "X$withval" != "X" && test "X$withval" != "Xno"; then
     M2_LIBRARIES_PREFIX="$withval"
     M2_LIBRARIES_EXECPREFIX="$M2_LIBRARIES_PREFIX"
   fi
  ])
AC_SUBST([M2_LIBRARIES_PREFIX])
AC_SUBST([M2_LIBRARIES_EXECPREFIX])
AM_CONDITIONAL([HAVE_M2_LIBRARIES_PREFIX], [test -n "$M2_LIBRARIES_PREFIX"])

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

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([1.0])

AC_CONFIG_FILES([Makefile hello.mod])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])
AC_OUTPUT
