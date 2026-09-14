dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-objc-gnome2], [0])
AC_CONFIG_SRCDIR([hello.m])
AM_INIT_AUTOMAKE([1.11])

dnl Test for gobjc before gcc, since that's what the EPEL 9 package 'gcc-objc'
dnl installs (on systems where gcc does not support Objective C).
m4_pushdef([AC_MSG_FAILURE], m4_defn([AC_MSG_WARN]))
AC_PROG_OBJC([gobjc gcc objcc objc cc CC clang])
m4_popdef([AC_MSG_FAILURE])

GNOME_INIT
OBGNOME_INCLUDES=
obgnome_prefix=`obgnome-config --prefix`
if test -n "$obgnome_prefix"; then
  OBGNOME_INCLUDES="-I $obgnome_prefix/include"
fi
AC_SUBST([OBGNOME_INCLUDES])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([1.0])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile.in])
AC_OUTPUT
