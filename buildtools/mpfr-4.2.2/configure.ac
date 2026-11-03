dnl Process this file with autoconf to produce a configure script.

dnl WARNING! C code starting with # (preprocessor directives) must not
dnl be indented!

AC_COPYRIGHT([
Copyright 1999-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 3 of the License, or (at
your option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>.
])

dnl Add check-news when it checks for more than 15 lines
AC_INIT([MPFR],[4.2.2])

dnl AC_CANONICAL_HOST is needed by this configure.ac file.
dnl AC_CANONICAL_TARGET is not explicitly needed, but may be required by
dnl some other macros (e.g. AX_PTHREAD 29, committed on 2021-02-19) and
dnl must be called quite early in order to avoid the following warning:
dnl   warning: AC_ARG_PROGRAM was called before AC_CANONICAL_TARGET
dnl (we don't use AC_ARG_PROGRAM explicitly, but again, it is invoked by
dnl another macro, apparently AM_INIT_AUTOMAKE). This has eventually been
dnl fixed in AX_PTHREAD 31.
dnl Due to this bug in AX_PTHREAD 29, AC_CANONICAL_HOST was moved here and
dnl AC_CANONICAL_TARGET was added in r14481. However, it is unlikely that
dnl we need AC_CANONICAL_TARGET in the future; see the comments at
dnl   https://github.com/autoconf-archive/autoconf-archive/commit/2567e0ce0f3a11b535c6b527386197fb49ff172b
dnl (introducing AX_PTHREAD 29). Thus it is probably better to avoid it,
dnl as it could cause confusion. But let's keep AC_CANONICAL_HOST here,
dnl since this may be a better place than later.
AC_CANONICAL_HOST

dnl Older Automake versions than 1.13 may still be supported, but no longer
dnl tested, and many things have changed in 1.13. Moreover the INSTALL file
dnl and MPFR manual assume that MPFR has been built using Automake 1.13+
dnl (due to parallel tests, introduced by default in Automake 1.13).
dnl The subdir-objects option is needed due to configuration related to
dnl mini-gmp, which has sources in an external directory.
AM_INIT_AUTOMAKE([1.13 no-define dist-bzip2 dist-xz dist-zip subdir-objects])
AM_MAINTAINER_MODE(enable)

AC_CONFIG_MACRO_DIR([m4])

dnl Some AC_RUN_IFELSE programs need to be able to return several values
dnl (e.g., in a format detection, one for each possible format). But the
dnl Autoconf manual says: "This exit status might be that of a failed
dnl compilation, or it might be that of a failed program execution."
dnl Unfortunately, we cannot know whether a non-zero exit status comes
dnl from a failed compilation, so that the detection may be incorrect.
dnl Since failures generally occur with a small exit status, the value 77
dnl is reserved for skipped tests by Autoconf, and values larger than 125
dnl have special meanings in POSIX[*], good candidates for success are 0
dnl and values from 80 to 125.
dnl https://tldp.org/LDP/abs/html/exitcodes.html suggests the range 64-113
dnl but note that /usr/include/sysexits.h now allocates previously unused
dnl exit codes from 64 - 78 (for various kinds of errors).
dnl
dnl Alternatively, the test program could output the result to a file and
dnl return with the 0 exit status if it could do that successfully.
dnl
dnl [*] 2.8.2 Exit Status for Commands
dnl https://pubs.opengroup.org/onlinepubs/9699919799//utilities/V3_chap02.html#tag_18_08_02

test_CFLAGS=${CFLAGS+set}

dnl Check if user request its CC and CFLAGS
if test -n "$CFLAGS" || test -n "$CC" ; then
 user_redefine_cc=yes
fi

dnl Basic Autoconf macros. At this point, they must not make Autoconf
dnl choose a compiler because of the CC and CFLAGS setup from gmp.h!

AC_PROG_EGREP
AC_PROG_SED

dnl To use a separate config header.
dnl There is still some problem with GMP's HAVE_CONFIG
dnl AC_CONFIG_HEADERS([mpfrconf.h:mpfrconf.in])

unset gmp_lib_path GMP_CFLAGS GMP_CC

unset dev_version
if $EGREP -q -e '-dev$' $srcdir/VERSION; then
  dev_version=1
fi

dnl ********************************************************************
dnl Extra arguments to configure (AC_ARG_WITH and AC_ARG_ENABLE)

AC_ARG_WITH(gmp_include,
   [  --with-gmp-include=DIR  GMP include directory ],
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   CPPFLAGS="$CPPFLAGS -I$withval")

AC_ARG_WITH(gmp_lib,
   [  --with-gmp-lib=DIR      GMP lib directory ], [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   LDFLAGS="$LDFLAGS -L$withval"
   gmp_lib_path="$withval"
  ])

AC_ARG_WITH(gmp,
   [  --with-gmp=DIR          GMP install directory ], [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   if test -z "$with_gmp_lib" && test -z "$with_gmp_include" ; then
      CPPFLAGS="$CPPFLAGS -I$withval/include"
      LDFLAGS="$LDFLAGS -L$withval/lib"
      gmp_lib_path="$withval/lib"
   else
      AC_MSG_FAILURE([Do not use --with-gmp and --with-gmp-include/--with-gmp-lib options simultaneously.])
   fi
  ])

AC_ARG_WITH(gmp_build,
   [  --with-gmp-build=DIR    GMP build directory (please read INSTALL file)],
   [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   if test -z "$gmp_lib_path" && test -z "$with_gmp_include" ; then
      CPPFLAGS="$CPPFLAGS -I$withval -I$withval/tune"
      LDFLAGS="$LDFLAGS -L$withval -L$withval/.libs -L$withval/tune"
      gmp_lib_path="$withval$PATH_SEPARATOR$withval/.libs$PATH_SEPARATOR$withval/tune"
      if test -r $withval/Makefile ; then
         GMP_CFLAGS=`$SED -n 's/^CFLAGS = //p' $withval/Makefile`
         GMP_CC=`$SED -n 's/^CC = //p' $withval/Makefile`
         GMP_SOURCE=`$SED -n 's/^srcdir = *//p' $withval/Makefile`
         case "$GMP_SOURCE" in
           .)  GMP_SOURCE="" ;;
           /*) ;;
           ?*) GMP_SOURCE="$withval/$GMP_SOURCE" ;;
         esac
         if test -d "$GMP_SOURCE" ; then
            CPPFLAGS="$CPPFLAGS -I$GMP_SOURCE -I$GMP_SOURCE/tune"
         fi
      fi
      use_gmp_build=yes
   else
      AC_MSG_FAILURE([Do not use --with-gmp-build and other --with-gmp options simultaneously.])
   fi
   ])

AC_ARG_WITH(mini_gmp,
   [  --with-mini-gmp=DIR     use mini-gmp (sources in DIR) instead of GMP
                          (experimental, please read doc/mini-gmp file)],
   [
   MPFR_PARSE_DIRECTORY(["$withval"],[withval])
   if test -z "$gmp_lib_path" && test -z "$with_gmp_include" && \
      test -z "$use_gmp_build"; then
     if test -f "$withval/mini-gmp.c" && test -f "$withval/mini-gmp.h"; then
       AC_DEFINE([MPFR_USE_MINI_GMP],1,[Use mini-gmp])
       mini_gmp_path="$withval"
       AC_SUBST(mini_gmp_path)
     else
       AC_MSG_FAILURE([mini-gmp.{c,h} not found in $withval])
     fi
   else
     AC_MSG_FAILURE([Do not use --with-mini-gmp and other --with-gmp options simultaneously.])
   fi
  ])

AC_ARG_WITH(mulhigh_size,
   [  --with-mulhigh-size=NUM internal threshold table for mulhigh],
   AC_DEFINE_UNQUOTED([MPFR_MULHIGH_SIZE],$withval, [Mulhigh size]))

AC_ARG_ENABLE(gmp-internals,
   [  --enable-gmp-internals  enable use of GMP undocumented functions [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([WANT_GMP_INTERNALS],1,[Want GMP internals]) ;;
      no)  ;;
      *) AC_MSG_ERROR([bad value for --enable-gmp-internals: yes or no]) ;;
     esac])

AC_ARG_ENABLE(assert,
   [  --enable-assert         enable ASSERT checking [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_WANT_ASSERT],1,[Want all simple assertions]) ;;
      none) AC_DEFINE([MPFR_WANT_ASSERT],-1,[Do no want any assertion]) ;;
      no)  ;;
      full) AC_DEFINE([MPFR_WANT_ASSERT],2,[Want full assertions]) ;;
      *) AC_MSG_ERROR([bad value for --enable-assert: yes, no, none or full]) ;;
     esac])

AC_ARG_ENABLE(logging,
   [  --enable-logging        enable MPFR logging (needs nested functions
                          and the 'cleanup' attribute) [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_USE_LOGGING],1,[Enable MPFR logging support]) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-logging: yes or no]) ;;
     esac])

AC_ARG_ENABLE(thread-safe,
   [  --disable-thread-safe   explicitly disable TLS support
  --enable-thread-safe    build MPFR as thread safe, i.e. with TLS support
                          (the system must support it) [[default=autodetect]]],
   [ case $enableval in
      yes) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-thread-safe: yes or no]) ;;
     esac])

AC_ARG_ENABLE(shared-cache,
   [  --enable-shared-cache   enable use of caches shared by all threads,
                          for all MPFR constants.  It usually makes MPFR
                          dependent on PTHREAD [[default=no]]],
   [ case $enableval in
      yes)
         AC_DEFINE([MPFR_WANT_SHARED_CACHE],1,[Want shared cache]) ;;
      no)  ;;
      *) AC_MSG_ERROR([bad value for --enable-shared-cache: yes or no]) ;;
     esac])

AC_ARG_ENABLE(warnings,
   [  --enable-warnings       allow MPFR to output warnings to stderr [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_USE_WARNINGS],1,[Allow MPFR to output warnings to stderr]) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-warnings: yes or no]) ;;
     esac])

dnl For timeout support (in the testsuite), the case "yes" and absent
dnl --enable-tests-timeout option will be handled in a consistent way
dnl in MPFR_CONFIGS (see acinclude.m4).
AC_ARG_ENABLE(tests-timeout,
   [  --enable-tests-timeout=NUM
                          [[for developers]] enable timeout for test programs
                          (NUM seconds, <= 9999) [[default=no]]; if this is
                          enabled, the environment variable $MPFR_TESTS_TIMEOUT
                          overrides NUM (0: no timeout)],
   [ case $enableval in
      no)   ;;
      yes)  ;;
      [[0-9]]|[[0-9]][[0-9]]|[[0-9]][[0-9]][[0-9]]|[[0-9]][[0-9]][[0-9]][[0-9]])
       AC_DEFINE_UNQUOTED([MPFR_TESTS_TIMEOUT], $enableval, [timeout limit]) ;;
      *)    AC_MSG_ERROR([bad value for --enable-tests-timeout]) ;;
     esac])

AC_ARG_ENABLE(tune-for-coverage,
   [  --enable-tune-for-coverage
                          [[for developers]] tune MPFR for coverage tests],
   [ case $enableval in
      no)   ;;
      yes)  AC_DEFINE([MPFR_TUNE_COVERAGE], 1, [tune for coverage]) ;;
      *)    AC_MSG_ERROR([bad value for --enable-tune-for-coverage]) ;;
     esac])

dnl Support for _Decimal64 and _Decimal128 (ISO/IEC TS 18661).
dnl See acinclude.m4 for more information and tests.
dnl FIXME: differentiate the support of _Decimal64 and _Decimal128, e.g.
dnl   --enable-decimal64  for _Decimal64
dnl   --enable-decimal128 for _Decimal128
dnl   --enable-decimal-float would explicitly enable both (or fail).
dnl   --disable-decimal-float would explicitly disable both.
AC_ARG_ENABLE(decimal-float,
   [  --disable-decimal-float explicitly disable decimal floats support
  --enable-decimal-float  build conversion functions from/to decimal floats
                          (see INSTALL file for details) [[default=auto]]],
   [ case $enableval in
      yes|no|auto|bid|dpd|generic) ;;
      *) AC_MSG_ERROR([bad value for --enable-decimal-float]) ;;
     esac])

dnl Warning! Not to be confused with _Decimal128. Thus it is better
dnl to say binary128 in the description. It can correspond to either
dnl _Float128 (ISO C23) or __float128 (old type name).
AC_ARG_ENABLE(float128,
   [  --disable-float128      explicitly disable binary128 support
  --enable-float128       build conversion functions from/to binary128
                          (_Float128 or __float128) [[default=autodetect]]],
   [ case $enableval in
      yes) ;;
      no)  ;;
      *)   AC_MSG_ERROR([bad value for --enable-float128: yes or no]) ;;
     esac])

AC_ARG_ENABLE(debug-prediction,
   [  --enable-debug-prediction
                          [[for developers]] enable debug of branch prediction
                          (for x86 and x86-64 with GCC, static libs)],
   [ case $enableval in
      yes)  if test "$enable_shared" != no; then
              AC_MSG_ERROR([--enable-debug-prediction can only work in static mode (--disable-shared)])
            fi
            AC_DEFINE([MPFR_DEBUG_PREDICTION],1,
	       [Enable debug of branch prediction]) ;;
      no)   ;;
      *)    AC_MSG_ERROR([bad value for --enable-debug-prediction: yes or no]) ;;
     esac])

AC_ARG_ENABLE(lto,
   [  --enable-lto            build MPFR with link-time-optimization
                          (experimental) [[default: no]]],
   [ case $enableval in
      yes)  if test "$enable_shared" != "no"; then
              AC_MSG_ERROR([--enable-lto can only work in static mode (--disable-shared)])
            fi
            enable_lto=yes
            ;;
      no)   ;;
      *)    AC_MSG_ERROR([bad value for --enable-lto: yes or no]) ;;
     esac])

AC_ARG_ENABLE(formally-proven-code,
   [  --enable-formally-proven-code
                          use formally proven code when available
                          (needs a C99 compiler) [[default=no]]],
   [ case $enableval in
      yes) AC_DEFINE([MPFR_WANT_PROVEN_CODE],1,[Want formally proven code]) ;;
      no)  ;;
      *) AC_MSG_ERROR([bad value for --enable-formally-proven-code: yes or no]) ;;
     esac])

dnl Makefile.am files can use "if MINI_GMP" / ... / "endif".
AM_CONDITIONAL([MINI_GMP], [test -n "$mini_gmp_path"])

dnl First, detect incompatibilities between the above configure options.
AC_MSG_CHECKING([whether configure options are compatible])
if test "$enable_logging" = yes; then
  if test "$enable_thread_safe" = yes; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([enable either logging or thread-safe, not both])
  fi
dnl The following test is done only to output a specific error message,
dnl as there would otherwise be an error due to enable_thread_safe=no.
  if test "$enable_shared_cache" = yes; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([shared cache does not work with logging support])
  fi
  enable_thread_safe=no
fi
if test "$enable_shared_cache" = yes; then
  if test "$enable_thread_safe" = no; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([shared cache needs thread-safe support])
  fi
  enable_thread_safe=yes
fi
AC_MSG_RESULT([yes])


dnl
dnl Setup CC and CFLAGS
dnl

dnl ********************************************************************
dnl Check for CC and CFLAGS in gmp.h

dnl Warning! The following tests must be done before Autoconf selects
dnl a compiler. This means that some macros such as AC_PROG_CC and
dnl AM_PROG_AR must be put after the following code.

dnl We do not do this check if mini-gmp is used.

if test -z "$user_redefine_cc" && \
   test "$cross_compiling" != yes && \
   test "${with_mini_gmp+set}" != set ; then

dnl We need to guess the C preprocessor instead of using AC_PROG_CPP,
dnl since AC_PROG_CPP implies AC_PROG_CC, which chooses a compiler
dnl (before we have the chance to get it from gmp.h) and does some
dnl checking related to this compiler (such as dependency tracking
dnl options); if the compiler changes due to __GMP_CC in gmp.h, one
dnl would have incorrect settings.
if test -z "$GMP_CC$GMP_CFLAGS" ; then
   AC_MSG_CHECKING(for CC and CFLAGS in gmp.h)
   GMP_CC=__GMP_CC
   GMP_CFLAGS=__GMP_CFLAGS
   # /lib/cpp under Solaris doesn't support some environment variables
   # used by GCC, such as C_INCLUDE_PATH. If the user has set up such
   # environment variables, he probably wants to use them. So, let us
   # prefer cpp and gcc to /lib/cpp. However, note that this won't
   # work if GCC has been installed with versioned names only (e.g.
   # with cpp-5 and gcc-5).
   for cpp in "cpp -P" "cpp" "gcc -P -E" "gcc -E" "/lib/cpp -P" "/lib/cpp" "cc -P -E" "cc -E" "c99 -P -E" "c99 -E" "clang -E" "cl -E" "icl -E"
   do
     # Get CC
     echo "#include \"gmp.h\"" > conftest.c
     echo "MPFR_OPTION __GMP_CC" >> conftest.c
     $cpp $CPPFLAGS conftest.c 2> /dev/null > conftest.txt
     test $? -ne 0 && continue
     GMP_CC=`$EGREP MPFR_OPTION conftest.txt | $SED -e 's/MPFR_OPTION //g;s/ *" *//g'`
     # Get CFLAGS
     echo "#include \"gmp.h\"" >  conftest.c
     echo "MPFR_OPTION __GMP_CFLAGS" >> conftest.c
     $cpp $CPPFLAGS conftest.c 2> /dev/null > conftest.txt
     test $? -ne 0 && continue
     [GMP_CFLAGS=`$EGREP MPFR_OPTION conftest.txt | $SED -e 's/[ "]-pedantic[ "]/ /g;s/MPFR_OPTION //g;s/ *" *//g'`]
     break
   done
   rm -f conftest*
   if test "x$GMP_CC" = "x__GMP_CC" || test "x$GMP_CFLAGS" = "x__GMP_CFLAGS" ; then
      AC_MSG_RESULT(failed)
      GMP_CFLAGS=
      GMP_CC=
   else
      AC_MSG_RESULT([CC=$GMP_CC CFLAGS=$GMP_CFLAGS])
   fi
fi

dnl But these variables may be invalid, so we must check them first.
dnl Note: we do not use AC_RUN_IFELSE, as it implies AC_PROG_CC.
if test -n "$GMP_CC$GMP_CFLAGS" ; then
   AC_MSG_CHECKING(for CC=$GMP_CC and CFLAGS=$GMP_CFLAGS)
   echo "int main (void) { return 0; }" > conftest.c
   if $GMP_CC $GMP_CFLAGS -o conftest conftest.c 2> /dev/null ; then
     AC_MSG_RESULT(yes)
     CFLAGS=$GMP_CFLAGS
     CC=$GMP_CC
   else
     AC_MSG_RESULT(no)
   fi
   rm -f conftest*
fi

fi

dnl ********************************************************************

AC_PROG_CC
AC_PROG_CPP
AC_LANG(C)

dnl AM_PROG_AR is needed so that ar-lib (wrapper for Microsoft lib.exe)
dnl gets installed by "automake -i" (if absent, "automake --warnings=all"
dnl gives a warning).
dnl However, if GMP chooses a non-default mode (a.k.a. ABI), this test may
dnl fail. The reason is that GMP provides GMP_CC and GMP_CFLAGS to get the
dnl right commands to compile (and link?) in the chosen non-default mode,
dnl but does not provide a macro for the archiver (if such a special command
dnl is needed). In case of failure, the user needs to either force GMP to
dnl use the default mode or provide an AR variable for MPFR's configure.
dnl Example: on power-aix, "gcc -c -O2 -maix64 -mpowerpc64 -mtune=power7"
dnl is used, but "ar cru" fails. Solution: use AR="ar -X64" NM="nm -B -X64"
dnl as said in the INSTALL file ("Notes on AIX/PowerPC").
AM_PROG_AR

dnl This must done before MPFR_CONFIGS.
LT_INIT(win32-dll)

dnl Warning: This test is *only* for CFLAGS settings.
dnl If the compiler is ICC, add some specific flags, except on MS-Windows.
dnl Don't add warnings flags (Otherwise you'll get more than 20000 warnings).
dnl Add -long_double flags? Don't use -pc64 !
dnl Notes (VL):
dnl   * With icc 10.1 20080212 on itanium, the __ICC macro is not defined,
dnl     even when the -icc option is used (contrary to what is documented
dnl     on the icc man page).
dnl   * When ICC is correctly detected (__ICC macro defined), unsetting
dnl     the GCC variable confuses libtool. See:
dnl       https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=485421
dnl   * If need be, the gcc predefined macros __GNUC_* can be disabled
dnl     thanks to the -no-gcc option.
dnl   * Now use -mieee-fp instead of -mp (ICC 13 says: option '-mp' is
dnl     deprecated and will be removed in a future release.). According
dnl     to "icc -help", both options are equivalent; it also suggests
dnl     to use -fp-model <arg> instead of -mp, but what is <arg>? Anyway
dnl     -mieee-fp solves the tset_d failure due to a negative zero. This
dnl     option has been introduced in ICC 9.0 for Linux (2005-06):
dnl     https://climserv.ipsl.polytechnique.fr/documentation/intel-icc/ReleaseNotes.htm
AC_MSG_CHECKING(for non-MS-Windows ICC)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if (!defined(__ICC) && !defined(__INTEL_COMPILER)) || defined(_WIN32)
# error "Not a non-MS-Windows ICC"
error
#endif
]], [[]])],[
 AC_MSG_RESULT(yes)
 CFLAGS="-fp_port -mieee-fp -wd1572 -wd265 -wd186 -wd239 $CFLAGS"
],[AC_MSG_RESULT(no)])

dnl If CFLAGS has not been set explicitly and the compiler is GCC, then
dnl use some specific flags. But don't touch user other flags.
dnl Note: This is done even when CFLAGS has been set from GMP's CFLAGS
dnl (__GMP_CFLAGS macro in gmp.h) above. The consequence is that this
dnl might yield a compilation failure if the -Werror option appears in
dnl __GMP_CFLAGS. But in this case, since -Werror is not used by default,
dnl one may expect that the user would also set CFLAGS for MPFR.
if test "$test_CFLAGS" != set && test -n "$GCC"; then
  CFLAGS="-Wpointer-arith $CFLAGS"
  AC_MSG_CHECKING(whether the selected language is C++)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined (__cplusplus)
# error "C++"
error
#endif
  ]], [[]])],[
    AC_MSG_RESULT(no)
    dnl The option -Wc++-compat is available in GCC 4.1.0 and newer.
    [
    case `$CC --version | $SED -e 's/^[^ ]* //' -e 's/([^)]*) //' -e 1q` in
      2.*|3.*|4.0*) ;;
      *) CFLAGS="-Wc++-compat $CFLAGS" ;;
    esac
    CFLAGS="-Wmissing-prototypes $CFLAGS"
    ]
  ],[
    AC_MSG_RESULT(yes)
    CFLAGS="-Wmissing-declarations -Wno-sign-compare $CFLAGS"
  ])
  CFLAGS="-Wall $CFLAGS"
  AC_MSG_NOTICE([using CFLAGS="$CFLAGS"])
fi

AM_PROG_CC_C_O

case $host in
  *-apple-darwin*)
dnl This allows to take the first GMP library in the library paths,
dnl whether it is dynamic or static. This behavior is more sensible,
dnl in particular because it is the only way to link with a version
dnl only available in static form when another version is available
dnl in dynamic, and also for consistency, because the compiler will
dnl take the first gmp.h found in the include paths (so, we need to
dnl take a library that corresponds to this header file). This is a
dnl common problem with darwin.
    MPFR_LD_SEARCH_PATHS_FIRST ;;
esac

AC_C_CONST
AC_C_VOLATILE
dnl Determine the endianness of integer. But since MPFR doesn't use
dnl AC_CONFIG_HEADERS, configure cannot provide such information for
dnl universal binaries containing variants with different endianness
dnl (i.e. generic code must be used).
AC_C_BIGENDIAN([AC_DEFINE(HAVE_BIG_ENDIAN)],[AC_DEFINE(HAVE_LITTLE_ENDIAN)],
[true],[true])

# (Based on GMP 5.1)
# clock_gettime is in librt on *-*-osf5.1 and on glibc < 2.17, so add -lrt to
# TUNE_LIBS if needed (e.g. if clock_gettime is not already in the C library).
# On linux (tested on x86_32, 2.6.26), clock_getres reports ns accuracy,
# while in a quick test on osf, clock_getres said only 1 millisecond.
old_LIBS="$LIBS"
AC_SEARCH_LIBS(clock_gettime, rt, [
  AC_DEFINE([HAVE_CLOCK_GETTIME],1,[Define to 1 if you have the `clock_gettime' function])])
TUNE_LIBS="$LIBS"
LIBS="$old_LIBS"
AC_SUBST(TUNE_LIBS)

dnl Under Linux, make sure that the old dtags are used if LD_LIBRARY_PATH
dnl is defined. The issue is that with the new dtags, LD_LIBRARY_PATH has
dnl the precedence over the run path, so that if a compatible MPFR library
dnl is installed in some directory from $LD_LIBRARY_PATH, then the tested
dnl MPFR library will be this library instead of the MPFR library from the
dnl build tree. Other OS with the same issue might be added later.
dnl
dnl References:
dnl   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=859732
dnl   https://lists.gnu.org/archive/html/libtool/2017-05/msg00000.html
dnl
dnl We need to check whether --disable-new-dtags is supported as alternate
dnl linkers may be used (e.g., with tcc: CC=tcc LD=tcc).
dnl
dnl [2023-08]
dnl On Android/Termux, -Wl,--disable-new-dtags is supported by clang, but
dnl the DT_RPATH directories are stripped away when running the program.
dnl Moreover, config.guess identifies the host as GNU/Linux (*-*-linux-gnu)
dnl instead of Android (linux*android*), so that we need to explicitly
dnl exclude Termux.
dnl
case $host in
  *-*-linux-gnu)
    if test -n "$LD_LIBRARY_PATH" && test -z "$TERMUX_VERSION"; then
      saved_LDFLAGS="$LDFLAGS"
      LDFLAGS="$LDFLAGS -Wl,--disable-new-dtags"
      AC_MSG_CHECKING(whether --disable-new-dtags is supported by the linker)
      AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main (void) { return 0; }
      ]])],
      [AC_MSG_RESULT(yes (use it since LD_LIBRARY_PATH is set))],
      [AC_MSG_RESULT(no)
       LDFLAGS="$saved_LDFLAGS"
      ])
    fi
    ;;
esac

dnl MPFR_CONFIGS uses LDFLAGS, thus must be invoked after LDFLAGS has
dnl been determined completely.
MPFR_CONFIGS

dnl
dnl For mpfr-longlong.h - TODO: should be replaced (see acinclude.m4).
dnl

GMP_C_ATTRIBUTE_MODE


dnl
dnl Setup related to GMP / mini-gmp
dnl

if test -z "$mini_gmp_path" ; then

dnl Setup for GMP

dnl Check GMP Header
AC_MSG_CHECKING(for gmp.h)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include "gmp.h"
]])],[AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_MSG_ERROR([gmp.h can't be found, or is unusable.])
])

dnl Configs for Windows DLLs.
dnl libtool requires "-no-undefined" for win32 dll
dnl
dnl "-Wl,output-def" is used to get a .def file for use by MS lib to make
dnl a .lib import library, described in the manual.
dnl
dnl Incidentally, libtool does generate an import library libmpfr.dll.a,
dnl but it's "ar" format and cannot be used by the MS linker.  There
dnl doesn't seem to be any GNU tool for generating or converting to .lib.
dnl
dnl The DLL version (the number in libmpfr-*.dll.def below) must be the
dnl version of the oldest supported interface (i.e. CURRENT - AGE in
dnl src/Makefile.am).
AC_SUBST(MPFR_LDFLAGS)
AC_SUBST(LIBMPFR_LDFLAGS)
case $host in
  *-*-cygwin* | *-*-mingw* | *-*-pw32* | *-*-os2*)
   AC_MSG_CHECKING(for DLL/static GMP)
   if test "$enable_shared" = yes; then
     MPFR_LDFLAGS="$MPFR_LDFLAGS -no-undefined"
     LIBMPFR_LDFLAGS="$LIBMPFR_LDFLAGS -Wl,--output-def,.libs/libmpfr-6.dll.def"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
#if !__GMP_LIBGMP_DLL
# error "Dead man"
error
#endif
     ]], [[]])],[AC_MSG_RESULT(DLL)],[
  AC_MSG_RESULT(static)
  AC_MSG_ERROR([libgmp isn't provided as a DLL: use --enable-static --disable-shared]) ])
   else
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
#if __GMP_LIBGMP_DLL
# error "Dead man"
error
#endif
     ]], [[]])],[AC_MSG_RESULT(static)],[
  AC_MSG_RESULT(DLL)
  AC_MSG_ERROR([libgmp is provided as a DLL: use --disable-static --enable-shared]) ])
  fi
  ;;
esac

dnl Check minimal GMP version
dnl We only guarantee that with a *functional* and recent enough GMP version,
dnl MPFR will compile; we do not guarantee that GMP will compile.
dnl In particular fat builds are broken in GMP 4.3.2 and GMP 5.0.0
dnl (at least on 64-bit Core 2 under Linux),
dnl see <https://gmplib.org/list-archives/gmp-bugs/2011-August/002345.html>.
AC_MSG_CHECKING(for recent GMP)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include "gmp.h"
#if (__GNU_MP_VERSION*100+__GNU_MP_VERSION_MINOR*10 < 500)
# error "GMP 5.0.0 or newer is required"
error
#endif
  ]])],[AC_MSG_RESULT(yes)],[
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([GMP 5.0.0 or newer is required])
])

dnl Check if gmp.h is usable at link time; this may detect errors such as
dnl with GMP 4.1, which uses "extern __inline__" unconditionally with all
dnl GCC versions, which breaks by default with GCC 5 (this problem with
dnl GMP 4.1 is no longer possible as we now require GMP 5.0.0 or newer,
dnl but the same kind of problem may occur in the future).
dnl Note: No linking is done against the GMP library at this time, as we
dnl do not use any GMP symbol. The goal of this test is to avoid obscure
dnl errors with the following gmp.h tests.
AC_MSG_CHECKING(usable gmp.h at link time)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
]],[[]])],[AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_MSG_ERROR([there is an incompatibility between gmp.h and the compiler.
See 'config.log' for details.])
])

dnl Check if we can use internal header files of GMP (only --with-gmp-build)
if test "$use_gmp_build" = yes ; then
   AC_MSG_CHECKING(for gmp internal files)
   AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
   #include "gmp.h"
   #include "gmp-impl.h"
   #include "longlong.h"
   ]])],[
     AC_MSG_RESULT(yes)
     AC_DEFINE([MPFR_HAVE_GMP_IMPL],1,[Use GMP Internal Files])
   ],[
     AC_MSG_ERROR([header files gmp-impl.h and longlong.h not found])
   ])
fi

dnl Check for GMP_NUMB_BITS and sizeof(mp_limb_t) consistency.
dnl Problems may occur if gmp.h was generated with some ABI
dnl and is used with another ABI (or if nails are used).
dnl This test doesn't need to link with libgmp (at least it shouldn't).
AC_MSG_CHECKING(for GMP_NUMB_BITS and sizeof(mp_limb_t) consistency)
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <limits.h>
#include "gmp.h"
]], [[
  if (GMP_NUMB_BITS == sizeof(mp_limb_t) * CHAR_BIT)
    return 0;
  fprintf (stderr, "GMP_NUMB_BITS     = %ld\n", (long) GMP_NUMB_BITS);
  fprintf (stderr, "sizeof(mp_limb_t) = %ld\n", (long) sizeof(mp_limb_t));
  fprintf (stderr, "sizeof(mp_limb_t) * CHAR_BIT = %ld != GMP_NUMB_BITS\n",
           (long) (sizeof(mp_limb_t) * CHAR_BIT));
  return 1;
]])], [AC_MSG_RESULT(yes)], [
       AC_MSG_RESULT(no)
       AC_MSG_ERROR([GMP_NUMB_BITS and sizeof(mp_limb_t) are not consistent.
You probably need to change some of the GMP or MPFR compile options.
See 'config.log' for details (search for GMP_NUMB_BITS).])],
       [AC_MSG_RESULT([cannot test])])

MPFR_CHECK_MP_LIMB_T_VS_LONG
MPFR_CHECK_MP_LIMB_T_VS_INTMAX

dnl Now we will need the GMP library (not just gmp.h).
dnl Do not use AC_CHECK_LIB as it is based on a deprecated ISO C feature.
LIBS="-lgmp $LIBS"

dnl Check if we can link with GMP
AC_MSG_CHECKING(if we can link with GMP)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include "gmp.h"
mpz_t m;
]],[[
mpz_init (m);
]])],[AC_MSG_RESULT(yes)],[
 AC_MSG_RESULT(no)
 AC_MSG_ERROR([libgmp not found or uses a different ABI (including static vs shared).
Please read the INSTALL file -- see "In case of problem".])
])

dnl Check for corresponding 'gmp.h' and libgmp
AC_MSG_CHECKING(if gmp.h version and libgmp version are the same)
dnl We do not set LD_LIBRARY_PATH, as it is not possible to set it just
dnl before the test program is run, and we do not want to affect other
dnl programs (such as the compiler), because the behavior could be
dnl incorrect and even have security implications.
dnl WARNING! LD_RUN_PATH is not taken into account by the GNU gold ld,
dnl e.g. from binutils-gold 2.22-5 under Debian; see
dnl   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=660813
dnl   https://sourceware.org/bugzilla/show_bug.cgi?id=13764
saved_LD_RUN_PATH="$LD_RUN_PATH"
LD_RUN_PATH="${LD_RUN_PATH:+$LD_RUN_PATH$PATH_SEPARATOR}$gmp_lib_path"
export LD_RUN_PATH
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <string.h>
#include "gmp.h"
  ]], [[
  char buffer[100];
  sprintf (buffer, "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
           __GNU_MP_VERSION_PATCHLEVEL);
  printf ("(%s/%s) ", buffer, gmp_version);
  fflush (stdout);
  /* Also put information in config.log (stderr) */
  fprintf (stderr, "gmp.h: %s / libgmp: %s\n", buffer, gmp_version);
  if (strcmp (buffer, gmp_version) == 0)
    return 0;
  if (__GNU_MP_VERSION_PATCHLEVEL != 0)
    return 1;
  sprintf (buffer, "%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR);
  return (strcmp (buffer, gmp_version) != 0) ? 1 : 0;
  ]])],
   [AC_MSG_RESULT(yes)
    MPFR_CHECK_GMP
    MPFR_CHECK_DBL2INT_BUG
    MPFR_CHECK_PRINTF_SPEC
    MPFR_CHECK_PRINTF_GROUPFLAG],
   [AC_MSG_RESULT(no)
    AC_MSG_WARN([==========================================================])
    AC_MSG_WARN(['gmp.h' and 'libgmp' seem to have different versions or])
    AC_MSG_WARN([we cannot run a program linked with GMP (if you cannot])
    AC_MSG_WARN([see the version numbers above). A cause may be different])
    AC_MSG_WARN([GMP versions with different ABI's or the use of --with-gmp])
    AC_MSG_WARN([or --with-gmp-include with a system include directory])
    AC_MSG_WARN([(such as /usr/include or /usr/local/include). Another])
    AC_MSG_WARN([cause may be that LD_RUN_PATH is not honored (problem])
    AC_MSG_WARN([seen under OpenBSD 6.6 and with the GNU gold linker).])
    AC_MSG_WARN([However, since we can't use 'libtool' inside the configure,])
    AC_MSG_WARN([we can't be sure. See 'config.log' for details.])
    AC_MSG_WARN([CC="$CC"])
    AC_MSG_WARN([CFLAGS="$CFLAGS"])
    AC_MSG_WARN([CPPFLAGS="$CPPFLAGS"])
    AC_MSG_WARN([LDFLAGS="$LDFLAGS"])
    AC_MSG_WARN([LIBS="$LIBS"])
    AC_MSG_WARN([Temporary LD_RUN_PATH was "$LD_RUN_PATH".])
    AC_MSG_WARN([==========================================================])
    ],AC_MSG_RESULT([cannot test])
  )
LD_RUN_PATH="$saved_LD_RUN_PATH"

dnl __gmpn_sbpi1_divappr_q is an internal GMP symbol; thus its behavior
dnl may change or this symbol may be removed in the future (without being
dnl handled by the library versioning system, which is even worse, as this
dnl can mean undetected incompatibilities in case of GMP library upgrade,
dnl without rebuilding MPFR). So, this symbol must not be used, unless
dnl WANT_GMP_INTERNALS is defined. Only the GMP public API should be used
dnl by default, in particular by binary distributions. Moreover the check
dnl below may yield an incorrect result as libtool isn't used in configure
dnl (see above).
AC_CHECK_FUNCS([__gmpn_sbpi1_divappr_q])
dnl same for other GMP internal functions
AC_CHECK_FUNCS([__gmpn_invert_limb])
dnl mpn_rsblsh1_n(rp, up, vp, n) does {rp, n} <- 2 * {vp, n} - {up, n}
AC_CHECK_FUNCS([__gmpn_rsblsh1_n])

else

dnl Setup for mini-gmp

dnl Let us make the configure script clean up the mini-gmp.* files
dnl and add the symbolic links to mini-gmp.{c,h} instead of relying
dnl on timestamp-based make rules, which may not work when testing
dnl several mini-gmp versions (whose source can go back in time).
mpfr_build_src="${ac_top_build_prefix}src"
mkdir -p "$mpfr_build_src"
rm -f -- "$mpfr_build_src"/mini-gmp.*
for i in c h
do
  ln -s -- "$mini_gmp_path/mini-gmp.$i" "$mpfr_build_src/mini-gmp.$i"
done

dnl First check whether mini-gmp defines GMP_NUMB_BITS. If it doesn't,
dnl then guess the value from the size of mp_limb_t.
AC_MSG_CHECKING(for GMP_NUMB_BITS)
how="from mini-gmp.h"
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$mpfr_build_src"
AC_COMPUTE_INT(mini_gmp_numb_bits, [(GMP_NUMB_BITS)],
   [#include <mini-gmp.h>],
   [how="not in mini-gmp.h; guessed"
    AC_COMPUTE_INT(mini_gmp_numb_bits, [(sizeof(mp_limb_t) * CHAR_BIT)],
       [#include <limits.h>
        #include <mini-gmp.h>],
       [AC_MSG_FAILURE([cannot define GMP_NUMB_BITS])])
    AC_DEFINE_UNQUOTED([GMP_NUMB_BITS], $mini_gmp_numb_bits, [number of bits in a limb])
  ])
CPPFLAGS="$saved_CPPFLAGS"
AC_MSG_RESULT([$mini_gmp_numb_bits bits ($how)])

fi

dnl End of setup related to GMP / mini-gmp

dnl The getrusage function is needed for MPFR bench (cf tools/bench)
AC_CHECK_FUNCS([getrusage])

dnl Remove also many macros (AC_DEFINE), which are unused by MPFR and
dnl pollute (and slow down because libtool has to parse them) the build.
if test -f confdefs.h; then
  for i in PACKAGE_ HAVE_STRING HAVE_DLFCN_H HAVE_MEM STDC_HEADERS \
           HAVE_STDLIB_H HAVE_UNISTD_H HAVE_STDC_HEADERS \
           HAVE_SYS_STAT_H HAVE_SYS_TYPES_H PROTOTYPES __PROTOTYPES
  do
    $SED "/#define $i/d" < confdefs.h > confdefs.tmp
    mv confdefs.tmp confdefs.h
  done
fi

if test -n "$dev_version"; then
  AC_SUBST([DATAFILES])dnl
  DATAFILES=`echo \`$SED -n \
    's/^ *data_check *("\(data\/[[^"]]*\)".*/tests\/\1/p' \
    $srcdir/tests/*.c\``
fi

dnl Output
AC_CONFIG_FILES([Makefile mpfr.pc doc/Makefile src/Makefile tests/Makefile tune/Makefile src/mparam.h:src/mparam_h.in tools/bench/Makefile])
AC_OUTPUT

dnl NEWS README AUTHORS Changelog
