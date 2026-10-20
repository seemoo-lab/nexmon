dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-d], [0])
AC_CONFIG_SRCDIR([hello.d.in])
AM_INIT_AUTOMAKE([1.11])

gt_DCOMP
if test -z "$DC"; then
  echo "*** D compiler not found" 1>&2
  exit 1
fi

D_PACKAGES_PREFIX='$(srcdir)/depends'
D_PACKAGES_EXECPREFIX='./depends'
AC_ARG_WITH([d-packages-prefix],
  [[  --with-d-packages-prefix=DIR  search for D packages in DIR/include/d, DIR/lib]],
  [if test "X$withval" != "X" && test "X$withval" != "Xno"; then
     D_PACKAGES_PREFIX="$withval"
     D_PACKAGES_EXECPREFIX="$D_PACKAGES_PREFIX"
   fi
  ])
AC_SUBST([D_PACKAGES_PREFIX])
AC_SUBST([D_PACKAGES_EXECPREFIX])

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

AC_CONFIG_FILES([Makefile hello.d])
AC_CONFIG_FILES([dcomp.sh])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])
AC_OUTPUT
