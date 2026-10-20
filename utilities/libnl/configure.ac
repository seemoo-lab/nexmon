# SPDX-License-Identifier: LGPL-2.1-only

#
# Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
#

# copied from glib
m4_define([libnl_major_version], [3])
m4_define([libnl_minor_version], [12])
m4_define([libnl_micro_version], [0])
m4_define([libnl_git_sha], [m4_esyscmd([ ( [ -d ./.git/ ] && [ "$(readlink -f ./.git/)" = "$(readlink -f "$(git rev-parse --git-dir 2>/dev/null)" 2>/dev/null)" ] && git rev-parse --verify -q HEAD 2>/dev/null ) || true ])])


# The following explanation may help to understand the above rules a bit
# better: consider that there are three possible kinds of reactions from
# users of your library to changes in a shared library:
#
# 1. Programs using the previous version may use the new version as drop-in
#    replacement, and programs using the new version can also work with the
#    previous one. In other words, no recompiling nor relinking is needed.
#    In this case, bump revision only, don't touch current nor age.
#
# 2. Programs using the previous version may use the new version as drop-in
#    replacement, but programs using the new version may use APIs not
#    present in the previous one. In other words, a program linking against
#    the new version may fail with “unresolved symbols” if linking against
#    the old version at runtime: set revision to 0, bump current and age.
#
# 3. Programs may need to be changed, recompiled, relinked in order to use
#    the new version. Bump current, set revision and age to 0.

m4_define([libnl_lt_current],    [226])
m4_define([libnl_lt_revision],   [0])
m4_define([libnl_lt_age],        [26])

m4_define([libnl_version],
	  [libnl_major_version.libnl_minor_version.libnl_micro_version])

AC_INIT([libnl],[libnl_version],[],[],[http://www.infradead.org/~tgr/libnl/])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)], [])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

MAJ_VERSION=libnl_major_version
AC_SUBST(MAJ_VERSION)
MIN_VERSION=libnl_minor_version
AC_SUBST(MIN_VERSION)
MIC_VERSION=libnl_micro_version
AC_SUBST(MIC_VERSION)
LIBNL_GIT_SHA=libnl_git_sha
LIBNL_VERSION=libnl_version
AC_SUBST(LIBNL_VERSION)

LT_CURRENT=libnl_lt_current
AC_SUBST(LT_CURRENT)
LT_REVISION=libnl_lt_revision
AC_SUBST(LT_REVISION)
LT_AGE=libnl_lt_age
AC_SUBST(LT_AGE)

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL
LT_INIT
AC_PROG_MKDIR_P
AC_CHECK_PROGS(FLEX, 'flex')
AC_CHECK_PROGS(YACC, 'bison -y')

AC_C_CONST
AC_C_INLINE

AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[#ifndef __cplusplus
    #error "broken C++"
    #endif]])],,
  [CXX=;])
AC_LANG_POP([C++])
AM_CONDITIONAL([HAS_CXX], [test "x$CXX" != x])

PKG_CHECK_MODULES([CHECK], [check >= 0.9.0],
	[has_check="yes"],
	[AC_MSG_WARN([*** Disabling building of unit tests])
	 has_check="no"])
AM_CONDITIONAL(WITH_CHECK, [test "$has_check" = 'yes'])

AC_ARG_WITH([pkgconfigdir], AS_HELP_STRING([--with-pkgconfigdir=PATH],
	[Path to the pkgconfig directory [[LIBDIR/pkgconfig]]]),
	[pkgconfigdir="$withval"], [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])

AC_ARG_ENABLE([cli],
	AS_HELP_STRING([--enable-cli=yes|no|no-inst|bin|sbin], [Whether to build command line interface utils. Defaults to 'yes' which is a synonym for 'bin'. 'no-inst' means only build, not installing. 'bin'/'sbin' means installing to bin/sbin directory]),
	[enable_cli="$enableval"], [enable_cli="yes"])
if test "$enable_cli" != "no" &&
   test "$enable_cli" != "no-inst" &&
   test "$enable_cli" != "sbin"; then
	enable_cli="bin"
fi
AM_CONDITIONAL([ENABLE_CLI], [test "$enable_cli" != "no"])
AM_CONDITIONAL([ENABLE_CLI_INSTALL_BIN], [test "$enable_cli" = "bin"])
AM_CONDITIONAL([ENABLE_CLI_INSTALL_SBIN], [test "$enable_cli" = "sbin"])

AC_CHECK_HEADERS(dlfcn.h, [], [])

AC_ARG_ENABLE([pthreads],
	AS_HELP_STRING([--disable-pthreads], [Disable pthreads support]),
	[enable_pthreads="$enableval"], [enable_pthreads="yes"])
AM_CONDITIONAL([DISABLE_PTHREADS], [test "$enable_pthreads" = "no"])
if test "x$enable_pthreads" = "xno"; then
    AC_DEFINE([DISABLE_PTHREADS], [1], [Define to 1 to disable pthreads])
else
    AC_CHECK_LIB([pthread], [pthread_mutex_lock], [], AC_MSG_ERROR([libpthread is required]))
fi

AM_CONDITIONAL([ENABLE_STATIC], [test "$enable_static" != "no"])

AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--disable-debug], [Do not include debugging statements]),
        [enable_debug="$enableval"], [enable_debug="yes"])
if test "$enable_debug" = "yes"; then
    AC_DEFINE([NL_DEBUG], [1], [Define to 1 to enable debugging])
else
    enable_debug=no
    AC_DEFINE([NL_DEBUG], [0], [Define to 1 to enable debugging])
fi

AC_CONFIG_SUBDIRS([doc])

AC_CHECK_FUNCS([strerror_l])

AC_CHECK_DECLS([getprotobyname_r, getprotobynumber_r], [], [], [[
    #include <netdb.h>
]])

AC_CONFIG_FILES([
Makefile
libnl-3.0.pc
libnl-route-3.0.pc
libnl-genl-3.0.pc
libnl-nf-3.0.pc
libnl-cli-3.0.pc
libnl-xfrm-3.0.pc
libnl-idiag-3.0.pc
python/setup.py
include/netlink/version.h
])

ac_errcount=0
if test -z "$YACC"; then
    AC_MSG_WARN(bison not found. Please install before continuing.)
    ac_errcount=$((ac_errcount + 1))
fi
if test -z "$FLEX"; then
    AC_MSG_WARN(flex not found. Please install before continuing.)
    ac_errcount=$((ac_errcount + 1))
fi
if test $ac_errcount -gt 0; then
    AC_MSG_ERROR(Required packages are missing. Please install them and rerun ./configure)
fi

AC_OUTPUT

CXX_MSG="none"
if test "x$CXX" != x ; then
	CXX_MSG="[$CXX]"
fi

echo
echo "libnl $LIBNL_VERSION configuration${LIBNL_GIT_SHA:+ (git:$LIBNL_GIT_SHA)}:"
echo "    --enable-pthreads=$enable_pthreads"
echo "    --enable-static=$enable_static"
echo
echo "    --enable-debug=$enable_debug"
echo "    --enable-cli=$enable_cli"
echo
echo "    check: $has_check"
echo "    CXX: $CXX_MSG (only used for tests)"
echo
