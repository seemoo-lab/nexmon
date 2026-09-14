dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-rust], [0])
AC_CONFIG_SRCDIR([src/main.rs.in])
AM_INIT_AUTOMAKE([1.11])

dnl Check for availability of the Cargo program.
AC_PATH_PROG([CARGO], [cargo])
if test -z "$CARGO"; then
  echo "*** Essential program 'cargo' not found" 1>&2
  exit 1
fi
AC_SUBST([CARGO])

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

AC_CONFIG_FILES([Makefile src/main.rs])
dnl Support for VPATH builds:
dnl We need a copy of Cargo.toml in the build directory, because
dnl   - Cargo.lock needs to reside in the build directory and
dnl   - src/main.rs is in the build directory as well.
dnl Cf. <https://doc.rust-lang.org/cargo/guide/project-layout.html>.
AC_CONFIG_FILES([Cargo.toml])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])
AC_OUTPUT
