AC_DEFUN([ISL_DETECT_CLANG], [
AC_SUBST(CLANG_CXXFLAGS)
AC_SUBST(CLANG_LDFLAGS)
AC_SUBST(CLANG_RFLAG)
AC_SUBST(CLANG_LIBS)
AC_PROG_GREP
AC_PROG_SED
if test "x$with_clang_prefix" != "x"; then
	LLVM_CONFIG="$with_clang_prefix/bin/llvm-config"
fi
AC_PATH_PROG([LLVM_CONFIG], ["llvm-config"])
if test -z "$LLVM_CONFIG" || test ! -x "$LLVM_CONFIG"; then
	AC_MSG_ERROR([llvm-config not found])
fi
CLANG_CXXFLAGS=`$LLVM_CONFIG --cxxflags | \
	$SED -e 's/-Wcovered-switch-default//' \
	     -e 's/-gsplit-dwarf//' \
	     -e 's/-Wl,--no-keep-files-mapped//'`
CLANG_LDFLAGS=`$LLVM_CONFIG --ldflags`
# Construct a -R argument for libtool.
# This is needed in case some of the clang libraries are shared libraries.
CLANG_RFLAG=`echo "$CLANG_LDFLAGS" | $SED -e 's/-L/-R/g'`

CLANG_VERSION=`$LLVM_CONFIG --version`
CLANG_LIB="LLVM-$CLANG_VERSION"

SAVE_LDFLAGS="$LDFLAGS"
LDFLAGS="$CLANG_LDFLAGS $LDFLAGS"
AC_CHECK_LIB([$CLANG_LIB], [main], [have_lib_llvm=yes], [have_lib_llvm=no])
LDFLAGS="$SAVE_LDFLAGS"

# Use single libLLVM shared library when available.
# Otherwise, try and figure out all the required libraries
if test "$have_lib_llvm" = yes; then
	CLANG_LIBS="-l$CLANG_LIB"
else
	targets=`$LLVM_CONFIG --targets-built`
	components="$targets asmparser bitreader support mc"
	# Link in option and frontendopenmp components when available
	# since they may be used by the clang libraries.
	for c in option frontendopenmp; do
		$LLVM_CONFIG --components | $GREP $c > /dev/null 2> /dev/null
		if test $? -eq 0; then
			components="$components $c"
		fi
	done
	CLANG_LIBS=`$LLVM_CONFIG --libs $components`
fi
systemlibs=`$LLVM_CONFIG --system-libs 2> /dev/null | tail -1`
if test $? -eq 0; then
	CLANG_LIBS="$CLANG_LIBS $systemlibs"
fi

AC_LANG_PUSH(C++)

SAVE_CPPFLAGS="$CPPFLAGS"
SAVE_LDFLAGS="$LDFLAGS"
SAVE_LIBS="$LIBS"

CPPFLAGS="$CLANG_CXXFLAGS -I$srcdir $CPPFLAGS"
LDFLAGS="$CLANG_LDFLAGS $LDFLAGS"

# A test program for checking whether linking against libclang-cpp works.
m4_define([_ISL_DETECT_CLANG_PROGRAM], [AC_LANG_PROGRAM(
	[[#include <clang/Frontend/CompilerInstance.h>]],
	[[
		new clang::CompilerInstance();
	]])])

# Use single libclang-cpp shared library when available.
# Otherwise, use a selection of clang libraries that appears to work.
AC_CHECK_LIB([clang-cpp], [main], [have_lib_clang=yes], [have_lib_clang=no])
if test "$have_lib_clang" = yes; then
	# The LLVM libraries may be linked into libclang-cpp already.
	# Linking against them again can cause errors about options
	# being registered more than once.
	# Check whether linking against libclang-cpp requires
	# linking against the LLVM libraries as well.
	# Fail if linking fails with or without the LLVM libraries.
	AC_MSG_CHECKING([whether libclang-cpp needs LLVM libraries])
	LIBS="-lclang-cpp $SAVE_LIBS"
	AC_LINK_IFELSE([_ISL_DETECT_CLANG_PROGRAM], [clangcpp_needs_llvm=no], [
		LIBS="-lclang-cpp $CLANG_LIBS $SAVE_LIBS"
		AC_LINK_IFELSE([_ISL_DETECT_CLANG_PROGRAM],
			[clangcpp_needs_llvm=yes],
			[clangcpp_needs_llvm=unknown])
	])
	AC_MSG_RESULT([$clangcpp_needs_llvm])
	AS_IF([test "$clangcpp_needs_llvm" = "no"],
			[CLANG_LIBS="-lclang-cpp"],
	      [test "$clangcpp_needs_llvm" = "yes"],
			[CLANG_LIBS="-lclang-cpp $CLANG_LIBS"],
	      [AC_MSG_FAILURE([unable to link against libclang-cpp])])
else
	_ISL_DETECT_CLANG_ADD_CLANG_LIB([clangSupport])
	CLANG_LIBS="-lclangDriver -lclangBasic $CLANG_LIBS"
	_ISL_DETECT_CLANG_ADD_CLANG_LIB([clangOptions])
	_ISL_DETECT_CLANG_ADD_CLANG_LIB([clangASTMatchers])
	CLANG_LIBS="-lclangAnalysis -lclangAST -lclangLex $CLANG_LIBS"
	_ISL_DETECT_CLANG_ADD_CLANG_LIB([clangAnalysisLifetimeSafety])
	_ISL_DETECT_CLANG_ADD_CLANG_LIB([clangEdit])
	_ISL_DETECT_CLANG_ADD_CLANG_LIB([clangAPINotes])
	CLANG_LIBS="-lclangParse -lclangSema $CLANG_LIBS"
	CLANG_LIBS="-lclangFrontend -lclangSerialization $CLANG_LIBS"
fi

CPPFLAGS="$SAVE_CPPFLAGS"
LDFLAGS="$SAVE_LDFLAGS"
LIBS="$SAVE_LIBS"

AC_LANG_POP
])

# Check if the specified (clang) library exists and, if so,
# add it to CLANG_LIBS.
# SAVE_LIBS is assumed to hold the original LIBS.
m4_define([_ISL_DETECT_CLANG_ADD_CLANG_LIB], [
	LIBS="$CLANG_LIBS $SAVE_LIBS"
	AC_CHECK_LIB($1, [main], [CLANG_LIBS="-l$1 $CLANG_LIBS"])
])
