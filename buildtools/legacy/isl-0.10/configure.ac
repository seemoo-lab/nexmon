AC_INIT([isl], [0.10], [isl-development@googlegroups.com])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AC_SUBST(versioninfo)
versioninfo=10:0:0

AC_PROG_CC
AC_PROG_CXX

AX_CC_MAXOPT
AX_GCC_WARN_UNUSED_RESULT
AX_C___ATTRIBUTE__

AC_PROG_LIBTOOL

AC_CHECK_PROG(PERL, perl, perl, [])
AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex, [])
AC_CHECK_PROG(POD2HTML, pod2html, pod2html, [])

AM_CONDITIONAL(GENERATE_DOC, test -n "$PERL" -a -n "$PDFLATEX" -a -n "$POD2HTML")

AX_CREATE_STDINT_H(include/isl/stdint.h)

AX_SUBMODULE(gmp,system|build,system)

AC_SUBST(GMP_CPPFLAGS)
AC_SUBST(GMP_LDFLAGS)
AC_SUBST(GMP_LIBS)
case "$with_gmp" in
system)
	if test "x$with_gmp_prefix" != "x"; then
		isl_configure_args="$isl_configure_args --with-gmp=$with_gmp_prefix"
		GMP_CPPFLAGS="-I$with_gmp_prefix/include"
		GMP_LDFLAGS="-L$with_gmp_prefix/lib"
	fi
	GMP_LIBS=-lgmp
	;;
build)
	GMP_CPPFLAGS="-I$gmp_srcdir -I$with_gmp_builddir"
	GMP_LIBS="$with_gmp_builddir/libgmp.la"
	;;
esac
SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$GMP_CPPFLAGS $CPPFLAGS"
need_get_memory_functions=false
AC_CHECK_DECLS(mp_get_memory_functions,[],[
	need_get_memory_functions=true
],[#include <gmp.h>])
CPPFLAGS="$SAVE_CPPFLAGS"
AM_CONDITIONAL(NEED_GET_MEMORY_FUNCTIONS, test x$need_get_memory_functions = xtrue)

AX_SUBMODULE(piplib,no|system|build,no)

have_piplib=false
AC_SUBST(PIPLIB_CPPFLAGS)
AC_SUBST(PIPLIB_LDFLAGS)
AC_SUBST(PIPLIB_LIBS)
case "$with_piplib" in
	build)
		PIPLIB_CPPFLAGS="-I$piplib_srcdir/include"
		PIPLIB_LIBS="$with_piplib_builddir/libpiplibMP.la"
	;;
	system)
		PIPLIB_LIBS="-lpiplibMP"
		if test "x$with_piplib_prefix" != "x"; then
			PIPLIB_CPPFLAGS="-I$with_piplib_prefix/include"
			PIPLIB_LDFLAGS="-L$with_piplib_prefix/lib"
		fi
		SAVE_CPPFLAGS="$CPPFLAGS"
		SAVE_LDFLAGS="$LDFLAGS"
		CPPFLAGS="$PIPLIB_CPPFLAGS $CPPFLAGS"
		LDFLAGS="$PIPLIB_LDFLAGS $LDFLAGS"
		AC_CHECK_LIB(piplibMP, pip_solve,[
			AC_CHECK_MEMBER(PipOptions.Urs_parms, [], [
				AC_MSG_ERROR([Piplib too old; please install version 1.3.6 or newer])
			],[#include <piplib/piplibMP.h>])
		],[
			AC_MSG_ERROR([Piplib not found])
		])
		CPPFLAGS="$SAVE_CPPFLAGS"
		LDFLAGS="$SAVE_LDFLAGS"
	;;
	no)
	;;
	*)
		AC_MSG_ERROR(unsupported)
	;;
esac
if test "$with_piplib" != "no"; then
	AC_DEFINE(ISL_PIPLIB,,piplib is available)
	have_piplib=true
fi
AM_CONDITIONAL(HAVE_PIPLIB, test x$have_piplib = xtrue)

AC_SUBST(CLANG_CXXFLAGS)
AC_SUBST(CLANG_LDFLAGS)
AC_SUBST(CLANG_LIBS)
AX_SUBMODULE(clang,system|no,no)
case "$with_clang" in
system)
	llvm_config="llvm-config"
	AC_CHECK_PROG([llvm_config_found], ["$llvm_config"], [yes])
	if test "x$with_clang_prefix" != "x"; then
		llvm_config="$with_clang_prefix/bin/llvm-config"
		if test -x "$llvm_config"; then
			llvm_config_found=yes
		fi
	fi
	if test "$llvm_config_found" != yes; then
		AC_MSG_ERROR([llvm-config not found])
	fi
	CLANG_CXXFLAGS=`$llvm_config --cxxflags`
	CLANG_LDFLAGS=`$llvm_config --ldflags`
	CLANG_LIBS=`$llvm_config --libs analysis`
	CLANG_PREFIX=`$llvm_config --prefix`
	AC_DEFINE_UNQUOTED(CLANG_PREFIX, ["$CLANG_PREFIX"],
				[Clang installation prefix])

	SAVE_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CLANG_CXXFLAGS $CPPFLAGS"
	AC_LANG_PUSH(C++)
	AC_CHECK_HEADER([clang/Basic/SourceLocation.h], [],
		[AC_ERROR([clang header file not found])])
	AC_EGREP_HEADER([getDefaultTargetTriple], [llvm/Support/Host.h], [],
		[AC_DEFINE([getDefaultTargetTriple], [getHostTriple],
		[Define to getHostTriple for older versions of clang])])
	AC_EGREP_HEADER([getExpansionLineNumber],
		[clang/Basic/SourceLocation.h], [],
		[AC_DEFINE([getExpansionLineNumber],
			[getInstantiationLineNumber],
	    [Define to getInstantiationLineNumber for older versions of clang])])
	AC_EGREP_HEADER([DiagnosticsEngine],
		[clang/Basic/Diagnostic.h], [],
		[AC_DEFINE([DiagnosticsEngine],
			[Diagnostic],
	    [Define to Diagnostic for older versions of clang])])
	AC_EGREP_HEADER([ArrayRef], [clang/Driver/Driver.h],
		[AC_DEFINE([USE_ARRAYREF], [],
			[Define if Driver::BuildCompilation takes ArrayRef])])
	AC_EGREP_HEADER([CXXIsProduction], [clang/Driver/Driver.h],
		[AC_DEFINE([HAVE_CXXISPRODUCTION], [],
		[Define if Driver constructor takes CXXIsProduction argument])])
	AC_EGREP_HEADER([void HandleTopLevelDecl\(], [clang/AST/ASTConsumer.h],
		[AC_DEFINE([HandleTopLevelDeclReturn], [void],
			   [Return type of HandleTopLevelDeclReturn])
		 AC_DEFINE([HandleTopLevelDeclContinue], [],
			   [Return type of HandleTopLevelDeclReturn])],
		[AC_DEFINE([HandleTopLevelDeclReturn], [bool],
			   [Return type of HandleTopLevelDeclReturn])
		 AC_DEFINE([HandleTopLevelDeclContinue], [true],
			   [Return type of HandleTopLevelDeclReturn])])
	AC_LANG_POP
	CPPFLAGS="$SAVE_CPPFLAGS"

	SAVE_LDFLAGS="$LDFLAGS"
	LDFLAGS="$CLANG_LDFLAGS $LDFLAGS"
	AC_SUBST(LIB_CLANG_EDIT)
	AC_CHECK_LIB([clangEdit], [main], [LIB_CLANG_EDIT=-lclangEdit], [])
	LDFLAGS="$SAVE_LDFLAGS"
	;;
esac
AM_CONDITIONAL(HAVE_CLANG, test $with_clang = system)

AX_SET_WARNING_FLAGS

AC_SUBST(WARNING_FLAGS)

PACKAGE_CFLAGS="$GMP_CPPFLAGS"
PACKAGE_LDFLAGS="$GMP_LDFLAGS"
PACKAGE_LIBS="-lisl -lgmp"
AX_CREATE_PKGCONFIG_INFO

AX_DETECT_GIT_HEAD
echo '#define GIT_HEAD_ID "'$GIT_HEAD_ID'"' > gitversion.h

AH_BOTTOM([#include <isl_config_post.h>])
AC_CONFIG_HEADERS(isl_config.h)
AC_CONFIG_HEADERS(include/isl/config.h)
AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(doc/Makefile)
if test $with_clang = system; then
	AC_CONFIG_FILES(interface/Makefile)
fi
AC_CONFIG_FILES([bound_test.sh], [chmod +x bound_test.sh])
AC_CONFIG_FILES([pip_test.sh], [chmod +x pip_test.sh])
AC_CONFIG_COMMANDS_POST([
	dnl pass on arguments to subdir configures, but don't
	dnl add them to config.status
	ac_configure_args="$ac_configure_args $isl_configure_args"
])
AC_OUTPUT
