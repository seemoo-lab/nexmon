dnl Configuration for the gettext-tools directory of GNU gettext
dnl Copyright (C) 1995-2026 Free Software Foundation, Inc.
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <https://www.gnu.org/licenses/>.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([gettext-tools], [package], [bug-gettext@gnu.org])
AC_CONFIG_SRCDIR([src/msgfmt.c])
AC_CONFIG_AUX_DIR([../build-aux])
VERSION_NUMBER=`cd $srcdir/.. \
                && build-aux/git-version-gen .tarball-version`
gl_INIT_PACKAGE_VERSION([$VERSION_NUMBER])
AM_INIT_AUTOMAKE([1.11.1 silent-rules parallel-tests])
AC_CONFIG_HEADERS([config.h])

dnl Installation directories.
dnl They should have the suffix '/gettext' by default, not '/gettext-tools'.
if test "$docdir" = '${datarootdir}/doc/${PACKAGE_TARNAME}'; then
  docdir='${datarootdir}/doc/gettext'
fi
gl_CONFIGMAKE_PREP
if test "$pkgdatadir" = '${datadir}/${PACKAGE}'; then
  pkgdatadir='${datadir}/gettext'
fi
if test "$pkgincludedir" = '${includedir}/${PACKAGE}'; then
  pkgincludedir='${includedir}/gettext'
fi
if test "$pkglibdir" = '${libdir}/${PACKAGE}'; then
  pkglibdir='${libdir}/gettext'
fi
if test "$pkglibexecdir" = '${libexecdir}/${PACKAGE}'; then
  pkglibexecdir='${libexecdir}/gettext'
fi

dnl Define bindir_c and bindir_c_make.
gl_BUILD_TO_HOST_BINDIR
dnl Define libdir_c and libdir_c_make.
gl_BUILD_TO_HOST_LIBDIR
dnl Define libexecdir_c and libexecdir_c_make.
gl_BUILD_TO_HOST_LIBEXECDIR
dnl Define pkgdatadir_c and pkgdatadir_c_make.
gl_BUILD_TO_HOST_PKGDATADIR
dnl Define pkglibdir_c and pkglibdir_c_make.
gl_BUILD_TO_HOST_PKGLIBDIR
dnl Define pkglibexecdir_c and pkglibexecdir_c_make.
gl_BUILD_TO_HOST_PKGLIBEXECDIR

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

gt_JAVA_CHOICE
AS_IF([test "$JAVA_CHOICE" != no], [
  gt_JAVAEXEC
  gt_JAVACOMP([1.8])
  AC_CHECK_PROG([JAR], [jar], [jar])
  if test -n "$HAVE_JAVACOMP" && test -n "$JAR" && test "$JAVA_CHOICE" != no; then
    BUILDJAVA=yes
  else
    BUILDJAVA=no
  fi
], [
  gt_JAVAEXEC_DISABLED
  gt_JAVACOMP_DISABLED
  JAR=
  BUILDJAVA=no
])
AC_SUBST([BUILDJAVA])
if test -n "$HAVE_JAVAEXEC" && test $BUILDJAVA = yes; then
  TESTJAVA=yes
else
  TESTJAVA=no
fi
AC_SUBST([TESTJAVA])

gt_CSHARPCOMP
if test -n "$HAVE_CSHARPCOMP" && test "$CSHARP_CHOICE" != no; then
  BUILDCSHARP=yes
else
  BUILDCSHARP=no
fi
AC_SUBST([BUILDCSHARP])

gt_CSHARPEXEC([csharpexec-test.exe], [$srcdir/m4])
if test -n "$HAVE_CSHARPEXEC" && test $BUILDCSHARP = yes; then
  TESTCSHARP=yes
else
  TESTCSHARP=no
fi
AC_SUBST([TESTCSHARP])

gt_D_CHOICE
AS_IF([test "$D_CHOICE" != no],
  [gt_DCOMP
   if test -n "$DC"; then
     BUILDD=yes
   else
     BUILDD=no
   fi
  ],
  [BUILDD=no
  ])
AC_SUBST([BUILDD])

dnl Check for host type.
AC_CANONICAL_HOST

dnl Checks for compiler output filename suffixes.
AC_OBJEXT
AC_EXEEXT

dnl For a 64-bit build on AIX, AC_PROG_RANLIB is not sufficient.
gl_PROG_AR_RANLIB

dnl Set preferences for the gnulib module 'threadlib'.
gl_AVOID_WINPTHREAD

dnl Customization of libunistring gnulib modules: The code of these modules
dnl is compiled into libgettextlib, which is associated with the C macro
dnl 'DLL_VARIABLE' (see below).
GL_GNULIB_LIBUNISTRING_DLL_VARIABLE_NAME='DLL_VARIABLE'

dnl Make sure we see all GNU and Solaris extensions.
gl_EARLY
grgl_EARLY
gtpo_EARLY
gttgl_EARLY

dnl Check for build configuration.

gt_MORE_WARNINGS
gl_CC_INHIBIT_WARNINGS

gl_WOE32_DLL

LT_INIT([win32-dll])

dnl Prepares the libtool configuration for handling of Windows resources, and
dnl sets the RC variable to a program that compiles Windows resource files.
LT_LANG([Windows Resource])

dnl On native Windows and Cygwin, we can activate special Makefile rules which
dnl add version information to the shared libraries and executables.
case "$host_os" in
  mingw* | windows* | cygwin*) is_woe32=yes ;;
  *) is_woe32=no ;;
esac
AM_CONDITIONAL([WOE32], [test $is_woe32 = yes])
if test $is_woe32 = yes; then
  dnl Check for a program that compiles Windows resource files.
  AC_CHECK_TOOL([WINDRES], [windres])
fi

dnl Some other special Makefile rules exist for OS/2.
case "$host_os" in
  os2*) special_makefile_rules=os2 ;;
  *) special_makefile_rules=none ;;
esac
AM_CONDITIONAL([OS2], [test $special_makefile_rules = os2])

dnl Some code is only meant to be compiled on native Windows.
case "$host_os" in
  mingw* | windows*) is_windows_native=yes ;;
  *) is_windows_native=no ;;
esac
AM_CONDITIONAL([WINDOWS_NATIVE], [test $is_windows_native = yes])

dnl Linking a shared library works slightly differently on MSVC than on
dnl Cygwin and mingw.
is_msvc=false
case "$host_os" in
  windows*-msvc*) is_msvc=true ;;
  mingw* | windows*)
    AC_EGREP_CPP([Special], [
#ifdef _MSC_VER
 Special
#endif
], [is_msvc=true])
    ;;
esac
gl_CONDITIONAL([WINDOWS_MSVC], [$is_msvc])

dnl Package-private libraries do not need to be installed when we build only
dnl static libraries. Don't install them in this case. This avoids a build
dnl error on AIX with --enable-shared, when some other version of GNU gettext,
dnl built with --disable-shared, is already installed in the same $prefix and
dnl the CFLAGS or LDFLAGS happen to contain "-L$prefix/lib".
AM_CONDITIONAL([INSTALL_PRIVATE_LIBRARIES], [test "$enable_shared" = yes])

dnl Checks for libraries.

dnl These are the only lines required to internationalize the package.
dnl (OK, not quite, the AC_CONFIG_FILES has also some parts.)
AM_GNU_GETTEXT([use-libtool], [need-ngettext], [\${top_builddir}/../gettext-runtime/intl])
dnl When not cross-compiling, use the 'msgmerge' program from this package,
dnl for output consistency with the 'msgfilter' program used to create
dnl en@quot.po and en@boldquot.po.
if test "$cross_compiling" != yes; then
  MSGMERGE='msgmerge'
  MSGMERGE_FOR_MSGFMT_OPTION='--for-msgfmt'
fi

dnl This option allows to build gettext-tools without (re)building libtextstyle.
AC_ARG_WITH([installed-libtextstyle],
  [AS_HELP_STRING([[--with-installed-libtextstyle]],
     [Use an already installed libtextstyle.])],
  [gt_use_installed_libtextstyle=$withval],
  [gt_use_installed_libtextstyle=no])
if test "$gt_use_installed_libtextstyle" != no; then
  gl_LIBTEXTSTYLE
else
  test -f ../libtextstyle/Makefile || {
    AC_MSG_WARN([When building the gettext-tools package without building the entire gettext package, you need to pass the --with-installed-libtextstyle option to configure.])
  }
fi
AM_CONDITIONAL([USE_INSTALLED_LIBTEXTSTYLE],
  [test "$gt_use_installed_libtextstyle" != no])

dnl This option allows to build gettext-tools without (re)building GNU.Gettext.dll.
AC_ARG_WITH([installed-csharp-dll],
  [AS_HELP_STRING([[--with-installed-csharp-dll]],
     [Use an already installed C# DLL.])],
  [gt_use_installed_csharp_dll=$withval],
  [gt_use_installed_csharp_dll=no])
if test $BUILDCSHARP = yes && test "$gt_use_installed_csharp_dll" = no; then
  test -f ../gettext-runtime/intl-csharp/Makefile || {
    AC_MSG_WARN([When building the gettext-tools package without building the entire gettext package, you need to pass the --with-installed-csharp-dll option to configure.])
  }
fi
AM_CONDITIONAL([USE_INSTALLED_CSHARP_DLL],
  [test "$gt_use_installed_csharp_dll" != no])

dnl Check for bison. (cldr-plural.y requires bison >= 3.0.)
gl_PROG_BISON([TOOLS_BISON], [3.0])
gl_BISON
dnl This line internationalizes the bison generated parsers.
BISON_I18N

dnl Test whether msgmerge must be linked against libm. This is the case on
dnl most systems; but BeOS has all <math.h> functions in libc and doesn't have
dnl a libm.
MSGMERGE_LIBM=?
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([[
     #ifndef __NO_MATH_INLINES
     # define __NO_MATH_INLINES 1 /* for glibc */
     #endif
     #include <math.h>
     double x;
     ]],
     [[x = ceil(x); x = sqrt(x);]])],
  [MSGMERGE_LIBM=])
if test "$MSGMERGE_LIBM" = "?"; then
  save_LIBS="$LIBS"
  LIBS="$LIBS -lm"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([[
       #ifndef __NO_MATH_INLINES
       # define __NO_MATH_INLINES 1 /* for glibc */
       #endif
       #include <math.h>
       double x;
       ]],
       [[x = ceil(x); x = sqrt(x);]])],
    [MSGMERGE_LIBM="-lm"])
  LIBS="$save_LIBS"
fi
if test "$MSGMERGE_LIBM" = "?"; then
  MSGMERGE_LIBM=
fi
AC_SUBST([MSGMERGE_LIBM])

dnl Test whether msgmerge can make use of OpenMP.
if test "$gl_use_threads" != no; then
  AC_OPENMP
else
  OPENMP_CFLAGS=
fi

dnl Checks for header files, functions and declarations.
gl_INIT
grgl_INIT
gtpo_INIT
gttgl_INIT

dnl Allow libgrep/locale.h to use setlocale_null.h from gnulib-lib/.
GL_GRGL_GNULIB_SETLOCALE_NULL=1

dnl Checks for header files.
AC_CHECK_HEADERS([pwd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_UNSIGNED_LONG_LONG_INT
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])
gt_TYPE_SSIZE_T
AC_TYPE_PID_T

dnl Checks for library functions.
AC_CHECK_FUNCS([select])

AC_C_BIGENDIAN([endianness=1], [endianness=0],
  [echo "AC-C-BIGENDIAN fails to work on your system." | sed -e 's,-,_,g' 1>&2
   echo "Please report this as a bug to bug-autoconf@gnu.org" 1>&2
   exit 1],
  [endianness=universal])
if test $endianness != universal; then
  AC_DEFINE_UNQUOTED([ENDIANNESS], [$endianness],
    [Define according to the byte order of the target machine: 1 for big endian, 0 for little endian.])
fi
dnl For possibly universal binaries, it's not possible to determine the
dnl endianness at configure time. So determine it at compile time.
AH_BOTTOM([
#ifndef ENDIANNESS
# if defined __BIG_ENDIAN__
#  define ENDIANNESS 1
# endif
# if defined __LITTLE_ENDIAN__
#  define ENDIANNESS 0
# endif
#endif
])

gt_PREREQ_HOSTNAME

dnl Parameterization of the 'libxml' module:
dnl When INCLUDED_LIBXML is true and building static libraries, tell
dnl libxml's xmlexports.h to omit '__declspec(dllimport)' on Windows.
if test "$gl_cv_libxml_use_included" = yes && test "$enable_shared" != yes; then
  AC_DEFINE([LIBXML_STATIC], [1],
    [Define if libxml is being used as a static library.])
fi

dnl Compilation on native Windows and Cygwin needs special Makefile rules,
dnl because
dnl 1. when we install a shared library, we must arrange to export
dnl    auxiliary pointer variables for every exported variable,
dnl 2. when we install a shared library and a static library simultaneously,
dnl    the include file specifies __declspec(dllimport) and therefore we
dnl    must arrange to define the auxiliary pointer variables for the
dnl    exported variables _also_ in the static library.
if test "$enable_shared" = yes; then
  case "$host_os" in
    mingw* | windows* | cygwin*) is_woe32dll=yes ;;
    *) is_woe32dll=no ;;
  esac
else
  is_woe32dll=no
fi
AM_CONDITIONAL([WOE32DLL], [test $is_woe32dll = yes])
if test $is_woe32dll = yes; then
  AC_DEFINE([WOE32DLL], [1],
    [Define when --enable-shared is used on Windows.])
fi

dnl Tell the source files that the error facility is overridden by gnulib,
dnl to make sure to prepend "rpl_" prefix to the error_* symbols.
dnl This is only necessary on Cygwin and on Android.
if test "$ac_cv_lib_error_at_line" = no; then
  case "$host_os" in
    cygwin* | linux*-android*)
      AC_DEFINE([GNULIB_REPLACE_ERROR], [1],
        [Define when the error facility is replaced by gnulib.])
      ;;
  esac
fi

dnl Put some default definitions into config.h.
AH_TOP([
/* Default value for alignment of strings in .mo file.  */
#define DEFAULT_OUTPUT_ALIGNMENT 1
])
AH_BOTTOM([
/* Parameterization of the 'gcd' module.  */
#define GCD_WORD_T size_t

/* A file name cannot consist of any character possible.  INVALID_PATH_CHAR
   contains the characters not allowed.  */
#if defined _MSC_VER || defined __MINGW32__
/* Woe32.  This string is valid for Windows NT/2000.  On Windows 95/98/ME some
   few characters in the range 0x80..0xff are invalid as well, but this doesn't
   matter much for our purposes.  */
# define INVALID_PATH_CHAR "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\"*/:<>?\\|"
#elif defined MSDOS
/* Something like this for MSDOG.  */
# define INVALID_PATH_CHAR "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 \177\\:."
#else
/* Unix.  */
# define INVALID_PATH_CHAR "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 \177/"
#endif

/* This is the page width for the message_print function.  It should
   not be set to more than 79 characters (Emacs users will appreciate
   it).  It is used to wrap the msgid and msgstr strings, and also to
   wrap the file position (#:) comments.  */
#define PAGE_WIDTH 79

/* On Windows, variables that may be in a DLL must be marked specially.
   The symbols marked with DLL_VARIABLE should be exported if and only if the
   object file gets included in a DLL.  Libtool, on Windows platforms, defines
   the C macro DLL_EXPORT (together with PIC) when compiling for a shared
   library (called DLL under Windows) and does not define it when compiling
   an object file meant to be linked statically into some executable.  */
/* DLL_VARIABLE               is used for variables defined in libgettextlib.
   LIBGETTEXTSRC_DLL_VARIABLE is used for variables defined in libgettextsrc.
 */
#if (defined _WIN32 || defined __CYGWIN__) && WOE32DLL && !defined IN_RELOCWRAPPER
# if IN_LIBGETTEXTLIB
#  if defined DLL_EXPORT
#   define DLL_VARIABLE __declspec (dllexport)
#  else
#   define DLL_VARIABLE
#  endif
# else
#  define DLL_VARIABLE __declspec (dllimport)
# endif
#else
# define DLL_VARIABLE
#endif
#if (defined _WIN32 || defined __CYGWIN__) && WOE32DLL && !defined IN_RELOCWRAPPER
# if IN_LIBGETTEXTSRC
#  if defined DLL_EXPORT
#   define LIBGETTEXTSRC_DLL_VARIABLE __declspec (dllexport)
#  else
#   define LIBGETTEXTSRC_DLL_VARIABLE
#  endif
# else
#  define LIBGETTEXTSRC_DLL_VARIABLE __declspec (dllimport)
# endif
#else
# define LIBGETTEXTSRC_DLL_VARIABLE
#endif

/* Extra OS/2 (emx+gcc) defines.  */
#if defined __EMX__ && !defined __KLIBC__
# include "../gettext-runtime/intl/os2compat.h"
#endif
])
dnl Rename some macros and functions used for locking.
dnl Needed for compiling ../gettext-runtime/intl/localealias.c.
AH_BOTTOM([
#define __libc_lock_t                   gl_lock_t
#define __libc_lock_define              gl_lock_define
#define __libc_lock_define_initialized  gl_lock_define_initialized
#define __libc_lock_init                gl_lock_init
#define __libc_lock_lock                gl_lock_lock
#define __libc_lock_unlock              gl_lock_unlock
])

dnl Check for nm output filter that yields the exported symbols.
gt_GLOBAL_SYMBOL_PIPE

gt_MODULA2_CHOICE
AS_IF([test "$MODULA2_CHOICE" != no],
  [gt_MODULA2COMP
   if test -n "$M2C"; then
     BUILDMODULA2=yes
   else
     BUILDMODULA2=no
   fi
  ],
  [BUILDMODULA2=no
  ])
AC_SUBST([BUILDMODULA2])

dnl Check for the libjson-c library, version 0.13 or newer.
dnl Set INCJSON_C to the -I option for the include files.
dnl Set LIBJSON_C to the -L and -l options for the library.
AC_MSG_CHECKING([for the libjson-c library])
LIBJSON_C=?
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
     [[#include <json-c/json_c_version.h>
       int version_check[2 * (JSON_C_MAJOR_VERSION + (JSON_C_MINOR_VERSION >= 13) > 0) - 1];
     ]],
     [[]])],
  [],
  [dnl The include files are not present or are too old.
   LIBJSON_C=
  ])
if test "$LIBJSON_C" = "?"; then
  save_LIBS="$LIBS"
  LIBS="$LIBS -ljson-c"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
       [[#include <json-c/json_c_version.h>]],
       [[return json_c_version () [0] != '0';]])
    ],
    [LIBJSON_C="-ljson-c"],
    [dnl The library is not present.
     LIBJSON_C=
    ])
  LIBS="$save_LIBS"
fi
dnl Find the location of the include files <json-c/> subdirectory.
INCJSON_C=
if test -n "$LIBJSON_C"; then
  gl_ABSOLUTE_HEADER_ONE([json-c/json_c_version.h])
  if test -n "$gl_cv_absolute_json_c_json_c_version_h"; then
    INCJSON_C="-I"`echo "$gl_cv_absolute_json_c_json_c_version_h" | sed -e 's|.json_c_version[.]h$||'`
  else
    dnl Could not find the include files <json-c/> subdirectory.
    LIBJSON_C=
  fi
fi
AC_SUBST([INCJSON_C])
AC_SUBST([LIBJSON_C])
if test -n "$LIBJSON_C"; then
  gt_libjson_c_found=yes
else
  gt_libjson_c_found=no
fi
AC_MSG_RESULT([$gt_libjson_c_found])

dnl Check for the libcurl library.
dnl Set INCCURL to the -I option for the include files.
dnl Set LIBCURL to the -L and -l options for the library.
dnl TODO: Support linking with libcurl.a and its many dependencies.
AC_MSG_CHECKING([for the libcurl library])
INCCURL=
LIBCURL=?
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <curl/curlver.h>]], [[]])],
  [],
  [dnl The include files are not present.
   LIBCURL=
  ])
if test "$LIBCURL" = "?"; then
  save_LIBS="$LIBS"
  LIBS="$LIBS -lcurl"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
       [[#include <curl/curl.h>]],
       [[curl_global_init (CURL_GLOBAL_DEFAULT);]])
    ],
    [LIBCURL="-lcurl"],
    [dnl The library is not present.
     LIBCURL=
    ])
  LIBS="$save_LIBS"
fi
AC_SUBST([INCCURL])
AC_SUBST([LIBCURL])
if test -n "$LIBCURL"; then
  gt_libcurl_found=yes
else
  gt_libcurl_found=no
fi
AC_MSG_RESULT([$gt_libcurl_found])

dnl Check in which form to install the 'spit' program.
AM_CONDITIONAL([BUILD_SPIT_IN_C], [test -n "$LIBJSON_C" && test -n "$LIBCURL"])

dnl Check for Emacs and where to install .elc files.
dnl Sometimes Emacs is badly installed. Allow the user to work around it.
AC_ARG_WITH([emacs],
  [  --without-emacs         do not use Emacs, don't install po-mode],
  [gt_use_emacs=$withval],
  [gt_use_emacs=yes])
if test "$EMACS" != no; then
  AC_MSG_CHECKING([whether Emacs support is requested])
  AC_MSG_RESULT([$gt_use_emacs])
  if test "$gt_use_emacs" = no; then
    EMACS=no
  fi
fi
dnl If $EMACS = no, we won't install any Emacs Lisp (*.el) files,
dnl therefore it's useless to determine $(lispdir).
if test "$EMACS" != no; then
  AM_PATH_LISPDIR
fi

aclocaldir='${datadir}/aclocal'
AC_SUBST([aclocaldir])

dnl Check how to install the set of previous versions, for autopoint.
AC_ARG_WITH([git],
  [  --without-git           don't use git to compress the infrastructure archive],
  [gt_use_git=$withval],
  [gt_use_git=maybe])
AC_ARG_WITH([cvs],
  [  --with-cvs              use cvs to compress the infrastructure archive
                          (deprecated)],
  [gt_use_cvs=$withval],
  [gt_use_cvs=no])
AC_ARG_WITH([bzip2],
  [  --without-bzip2         don't use bzip2 to compress the infrastructure archive],
  [gt_use_bzip2=$withval],
  [gt_use_bzip2=maybe])
AC_ARG_WITH([xz],
  [  --without-xz            don't use xz to compress the infrastructure archive],
  [gt_use_xz=$withval],
  [gt_use_xz=maybe])
# The CVS format is deprecated, because "cvs init" does not work in all
# circumstances
# (see <https://lists.gnu.org/archive/html/bug-cvs/2010-05/msg00003.html>)
# and we are not allowed to distribute the cvs infrastructure files ourselves
# (see <https://lists.gnu.org/archive/html/bug-cvs/2010-06/msg00011.html>).
#
# With the archive from version 0.18.1, we can observe the following
# compression rates:
#    Size           File
#  16916480   archive.dir.tar
#   3528757   archive.dir.tar.gz
#   2621440   archive.cvs.tar
#   2082492   archive.dir.tar.bz2
#    696320   archive.git.tar
#    599046   archive.git.tar.bz2
#    592989   archive.git.tar.gz
#    588124   archive.git.tar.xz
#    457190   archive.cvs.tar.gz
#    335808   archive.cvs.tar.bz2
#    308880   archive.dir.tar.xz
#    299124   archive.cvs.tar.xz
# Among these:
#   - We don't offer the uncompressed ones, because at least gzip can be
#     assumed to be present everywhere.
#   - Among archive.git.tar.*, all have approximately the same size, because
#     git already compresses most of its repository data. Offer only .gz here.
#   - Among archive.cvs.tar.*, offer only .gz, for simplicity, since --with-cvs
#     is deprecated.
# The remaining ones are:
#    Size           File            ARCHIVE_FORMAT
#   3528757   archive.dir.tar.gz      dirgz
#   2082492   archive.dir.tar.bz2     dirbz2
#    592989   archive.git.tar.gz      git
#    457190   archive.cvs.tar.gz      cvs
#    308880   archive.dir.tar.xz      dirxz
if test "$gt_use_xz" != no && test "$gt_use_xz" != maybe; then
  # --with-xz is specified. Even if --with-git or --with-cvs or --with-bzip2 is
  # also specified, we use the dirxz format.
  ARCHIVE_FORMAT=dirxz
else
  if test "$gt_use_git" != no && test "$gt_use_git" != maybe; then
    # --with-git is specified. Even if --with-cvs or --with-bzip2 is also
    # specified, we use the git format.
    ARCHIVE_FORMAT=git
  else
    if test "$gt_use_cvs" != no; then
      # --with-cvs is specified. Even if --with-bzip2 is also specified, we use
      # the cvs format.
      ARCHIVE_FORMAT=cvs
    else
      if test "$gt_use_bzip2" != no && test "$gt_use_bzip2" != maybe; then
        # --with-bzip2 is specified. We use the dirbz2 format.
        ARCHIVE_FORMAT=dirbz2
      else
        # Neither --with-git nor --with-cvs nor --with-bzip2 nor --with-xz is
        # specified. Determine which programs are present.
        if test "$gt_use_xz" != no \
           && (xz --version) >/dev/null 2>&1 \
           && xz --version | grep lzma >/dev/null; then
          # --without-xz is not specified, and the xz program exists.
          ARCHIVE_FORMAT=dirxz
        else
          # --without-xz is specified, or the xz program is missing.
changequote(,)dnl
          if test "$gt_use_git" != no \
             && (git --version) >/dev/null 2>&1 \
             && { case `git --version | sed -e 's/^[^0-9]*//'` in
                    0.* | 1.[0-5].*) false ;;
                    *) true ;;
                  esac
                }; then
changequote([,])dnl
            # --without-git is not specified, and the git program exists
            # in version 1.6 or newer. Use git format.
            ARCHIVE_FORMAT=git
          else
            # --without-git is specified, or the git program is missing.
changequote(,)dnl
            if test "$gt_use_bzip2" != no \
               && (: | bzip2 --version) >/dev/null 2>&1 \
               && { sed_bzip2_version1='s/ \([0-9][0-9.]*\).*/ \1/'
                    sed_bzip2_version2='s/^.* //'
                    case `(: | bzip2 --version) 2>&1 \
                          | sed -e 1q \
                          | sed -e "$sed_bzip2_version1" -e "$sed_bzip2_version2"` in
                      0.*) false ;;
                      *) true ;;
                    esac
                  }; then
changequote([,])dnl
              # --without-bzip2 is not specified, and the bzip2 program exists
              # in version 1.0 or newer. Use dirbz2 format.
              ARCHIVE_FORMAT=dirbz2
            else
              # --without-bzip2 is specified, or the bzip2 program is missing.
              ARCHIVE_FORMAT=dirgz
            fi
          fi
        fi
      fi
    fi
  fi
fi
AC_SUBST([ARCHIVE_FORMAT])

dnl Get versions of files brought in by autopull.sh.
ARCHIVE_VERSION=1.0
AC_SUBST([ARCHIVE_VERSION])
dnl We don't offer a --with-tree-sitter-prefix option, but instead always use
dnl the tree-sitter version included in the tarball. This is required because
dnl there is no stability guarantee for grammar elements: grammar elements can
dnl be changed, extended, or even removed in future tree-sitter-<lang> versions,
dnl whereas the x-<lang>.c code assumes that certain grammar elements have a
dnl specific structure. We may need to change x-<lang>.c so that it works with
dnl a newer version of tree-sitter-<lang>.
. $srcdir/tree-sitter.cfg
AC_SUBST([TREE_SITTER_VERSION])
AC_SUBST([TREE_SITTER_GO_VERSION])
AC_SUBST([TREE_SITTER_RUST_VERSION])
AC_SUBST([TREE_SITTER_TYPESCRIPT_VERSION])
AC_SUBST([TREE_SITTER_OCAML_VERSION])
AC_SUBST([TREE_SITTER_D_VERSION])

dnl Determine the directory name to use for version-specific data files.
PACKAGE_SUFFIX="-$PACKAGE_VERSION"
AC_SUBST([PACKAGE_SUFFIX])
AC_DEFINE_UNQUOTED(PACKAGE_SUFFIX, "$PACKAGE_SUFFIX",
  [Define to the suffix of the directory name for version-specific data files.])

dnl Check for tools needed for formatting the documentation.
ac_aux_dir_abs=`cd $ac_aux_dir && pwd`
AC_PATH_PROG([DVIPS], [dvips], [$ac_aux_dir_abs/missing dvips])
AC_PATH_PROG([TEXI2PDF], [texi2pdf], [$ac_aux_dir_abs/missing texi2pdf])
AC_PATH_PROG([PERL], [perl], [$ac_aux_dir_abs/missing perl])

dnl Check for tools needed for formatting man pages.
CROSS_COMPILING=$cross_compiling
AC_SUBST([CROSS_COMPILING])

dnl Checks for configuration that some tests/ depend on.
gt_GLIBC2

dnl Checks for optional programs for the tests/lang-*, tests/recode-* tests.
gl_PROG_ANSI_CXX([CXX], [ANSICXX])
gt_LOCALE_FR
gt_LOCALE_FR_UTF8
gt_LOCALE_JA
gt_LOCALE_DE_UTF8
dnl Test for gobjc before gcc, since that's what the EPEL 9 package 'gcc-objc'
dnl installs (on systems where gcc does not support Objective C).
AC_PROG_OBJC([gobjc gcc objcc objc cc CC clang])

dnl Checks for functions for the tests/*.c tests.
AC_CHECK_FUNCS_ONCE([getrlimit setrlimit uselocale])

dnl Prepares the libtool configuration for languages used by the tests.
LT_LANG([C++])

dnl Check whether to execute tests that rely on libasprintf.
dnl This test must be consistent with gettext-runtime/configure.ac.
if test "${enable_libasprintf+set}" = set; then
  if test "$enable_libasprintf" != no; then
    TESTLIBASPRINTF=yes
  else
    TESTLIBASPRINTF=no
  fi
else
  TESTLIBASPRINTF=yes
fi
if test "$CXX" = no; then
  TESTLIBASPRINTF=no
fi
AC_SUBST([TESTLIBASPRINTF])

dnl Check whether to execute tests of the system (vendor-provided) libintl API.
dnl We execute these tests if and only if we don't install our own libintl.
AM_CONDITIONAL([TEST_SYSTEM_LIBINTL], [test $USE_INCLUDED_LIBINTL = no])

dnl Checks for optional programs for the its/* tests.
AC_PATH_PROGS([XMLLINT], [xmllint], [:])
AC_SUBST([HAVE_JAVAEXEC])

dnl Test for features used in install-tests.
dnl shlibpath_var and PATH_SEPARATOR are set by LT_INIT.
if test $USE_INCLUDED_LIBINTL = yes; then
  INSTALLED_LIBINTL="-lintl $LIBICONV $INTL_MACOSX_LIBS $INTL_WINDOWS_LIBS"
  INSTALLED_LIBGETTEXTPO_DEPS="$INSTALLED_LIBINTL"
else
  INSTALLED_LIBINTL=
  INSTALLED_LIBGETTEXTPO_DEPS="$LIBICONV"
fi
AC_SUBST([INSTALLED_LIBGETTEXTPO_DEPS])
AC_SUBST([PATH_SEPARATOR])
if test -n "${shlibpath_var}"; then
  if test "${shlibpath_var}" = PATH; then
    AUGMENT_SHLIBPATH="PATH='\$(bindir)'${PATH_SEPARATOR}\"\$\$PATH\";"
  else
    AUGMENT_SHLIBPATH="${shlibpath_var}='\$(libdir)':\"\$\$${shlibpath_var}\"; export ${shlibpath_var};"
  fi
else
  AUGMENT_SHLIBPATH=
fi
AC_SUBST([AUGMENT_SHLIBPATH])

AC_CONFIG_SUBDIRS([examples])

AC_CONFIG_FILES([Makefile],
                [FIX_MAKEFILE_DISTRIB])

AC_CONFIG_FILES([doc/Makefile])

AC_CONFIG_FILES([its/Makefile])

AC_CONFIG_FILES([gnulib-lib/Makefile])

AC_CONFIG_FILES([libgrep/Makefile])
AC_CONFIG_FILES([libgrep/gnulib-lib/Makefile])

AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/user-email:src/user-email.sh.in])
AC_CONFIG_FILES([src/spit.py], [chmod a+x src/spit.py])

AC_CONFIG_FILES([libgettextpo/Makefile])
AC_CONFIG_FILES([libgettextpo/exported.sh])

AC_CONFIG_FILES([po/Makefile.in])

AC_CONFIG_FILES([projects/Makefile])

AC_CONFIG_FILES([styles/Makefile])

AC_CONFIG_FILES([wizard/Makefile])
AC_CONFIG_FILES([wizard/gettextize], [chmod a+x wizard/gettextize])

AC_CONFIG_FILES([autotools/Makefile])
AC_CONFIG_FILES([autotools/autopoint], [chmod a+x autotools/autopoint])
AC_CONFIG_FILES([autotools/convert-archive], [chmod a+x autotools/convert-archive])

AC_CONFIG_FILES([misc/Makefile])
AC_CONFIG_FILES([misc/po-fetch], [chmod a+x misc/po-fetch])

AC_CONFIG_FILES([emacs/Makefile])

AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([man/x-to-1])

AC_CONFIG_FILES([m4/Makefile])

AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([tests/init-env])
AC_CONFIG_FILES([tests/gnulib-lib/Makefile])

AC_CONFIG_FILES([system-tests/Makefile])

AC_CONFIG_FILES([gnulib-tests/Makefile])

AC_CONFIG_FILES([install-tests/Makefile])

AC_CONFIG_FILES([build-aux/xml-validate-10],
                [chmod a+x build-aux/xml-validate-10])
AC_CONFIG_FILES([build-aux/xml-validate-11],
                [chmod a+x build-aux/xml-validate-11])

AC_OUTPUT
