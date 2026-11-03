AC_DEFUN([AX_DETECT_CLANG], [
ISL_DETECT_CLANG

CLANG_PREFIX=`$LLVM_CONFIG --prefix`
AC_DEFINE_UNQUOTED(ISL_CLANG_PREFIX,
	["$CLANG_PREFIX"], [Clang installation prefix])

AC_MSG_CHECKING([for clang resource directory])
CLANG_RESOURCE_DIR=`$CLANG_PREFIX/bin/clang -print-resource-dir 2>/dev/null`
if test $? -eq 0 && test "x$CLANG_RESOURCE_DIR" != "x"; then
	AC_MSG_RESULT([$CLANG_RESOURCE_DIR])
	AC_DEFINE_UNQUOTED(ISL_CLANG_RESOURCE_DIR,
		["$CLANG_RESOURCE_DIR"], [Clang resource directory])
else
	AC_MSG_RESULT([not found or not supported])
fi

# If $CLANG_PREFIX/bin/clang cannot find the standard include files,
# then see if setting sysroot to `xcode-select -p`/SDKs/MacOSX.sdk helps.
# This may be required on some versions of OS X since they lack /usr/include.
# If so, set ISL_CLANG_SYSROOT accordingly.
SAVE_CC="$CC"
CC="$CLANG_PREFIX/bin/clang"
AC_MSG_CHECKING(
	[whether $CLANG_PREFIX/bin/clang can find standard include files])
found_header=no
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[]])],
	[found_header=yes])
AC_MSG_RESULT([$found_header])
if test "x$found_header" != "xyes"; then
	AC_CHECK_PROG(XCODE_SELECT, xcode-select, xcode-select, [])
	if test -z "$XCODE_SELECT"; then
		AC_MSG_ERROR([Cannot find xcode-select])
	fi
	sysroot=`$XCODE_SELECT -p`/SDKs/MacOSX.sdk
	SAVE_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS -isysroot $sysroot"
	AC_MSG_CHECKING(
		[whether standard include files can be found with sysroot set])
	found_header=no
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[]])],
		[found_header=yes])
	AC_MSG_RESULT([$found_header])
	CPPFLAGS="$SAVE_CPPFLAGS"
	if test "x$found_header" != "xyes"; then
		AC_MSG_ERROR([Cannot find standard include files])
	else
		AC_DEFINE_UNQUOTED([ISL_CLANG_SYSROOT], ["$sysroot"],
			[Define to sysroot if needed])
	fi
fi
CC="$SAVE_CC"

AC_LANG_PUSH(C++)

SAVE_CPPFLAGS="$CPPFLAGS"

CPPFLAGS="$CLANG_CXXFLAGS -I$srcdir $CPPFLAGS"
AC_CHECK_HEADER([clang/Basic/SourceLocation.h], [],
	[AC_MSG_ERROR([clang header file not found])])
AC_CHECK_HEADER([llvm/TargetParser/Host.h],
	[AC_DEFINE([HAVE_TARGETPARSER_HOST_H], [],
		   [Define if llvm/TargetParser/Host.h exists])],
	[AC_EGREP_HEADER([getDefaultTargetTriple], [llvm/Support/Host.h], [],
		[AC_DEFINE([getDefaultTargetTriple], [getHostTriple],
		[Define to getHostTriple for older versions of clang])])
	])
AC_EGREP_HEADER([getExpansionLineNumber], [clang/Basic/SourceLocation.h], [],
	[AC_DEFINE([getExpansionLineNumber], [getInstantiationLineNumber],
	[Define to getInstantiationLineNumber for older versions of clang])])
AC_EGREP_HEADER([getImmediateExpansionRange], [clang/Basic/SourceManager.h],
	[],
	[AC_DEFINE([getImmediateExpansionRange],
	[getImmediateInstantiationRange],
	[Define to getImmediateInstantiationRange for older versions of clang])]
)
AC_EGREP_HEADER([ArrayRef.*CommandLineArgs],
	[clang/Frontend/CompilerInvocation.h],
	[AC_DEFINE([CREATE_FROM_ARGS_TAKES_ARRAYREF], [],
		[Define if CompilerInvocation::CreateFromArgs takes
		 ArrayRef])
	])
AC_EGREP_HEADER([void HandleTopLevelDecl\(], [clang/AST/ASTConsumer.h],
	[AC_DEFINE([HandleTopLevelDeclReturn], [void],
		   [Return type of HandleTopLevelDeclReturn])
	 AC_DEFINE([HandleTopLevelDeclContinue], [],
		   [Return type of HandleTopLevelDeclReturn])],
	[AC_DEFINE([HandleTopLevelDeclReturn], [bool],
		   [Return type of HandleTopLevelDeclReturn])
	 AC_DEFINE([HandleTopLevelDeclContinue], [true],
		   [Return type of HandleTopLevelDeclReturn])])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <clang/Basic/TargetInfo.h>]], [[
	using namespace clang;
	std::shared_ptr<TargetOptions> TO;
	DiagnosticsEngine *Diags;
	TargetInfo::CreateTargetInfo(*Diags, TO);
]])], [AC_DEFINE([CREATETARGETINFO_TAKES_SHARED_PTR], [],
	      [Define if TargetInfo::CreateTargetInfo takes shared_ptr])])
AC_EGREP_HEADER([getReturnType],
	[clang/AST/CanonicalType.h], [],
	[AC_DEFINE([getReturnType], [getResultType],
	    [Define to getResultType for older versions of clang])])
AC_EGREP_HEADER([initializeBuiltins],
	[clang/Basic/Builtins.h], [],
	[AC_DEFINE([initializeBuiltins], [InitializeBuiltins],
		[Define to InitializeBuiltins for older versions of clang])])
AC_EGREP_HEADER([IK_C], [clang/Frontend/FrontendOptions.h], [],
	[AC_CHECK_HEADER([clang/Basic/LangStandard.h],
		[IK_C=Language::C], [IK_C=InputKind::C])
	 AC_DEFINE_UNQUOTED([IK_C], [$IK_C],
	 [Define to Language::C or InputKind::C for newer versions of clang])
	])
# llvmorg-15-init-7544-g93471e65df48
AC_EGREP_HEADER([setLangDefaults], [clang/Basic/LangOptions.h],
	[SETLANGDEFAULTS=LangOptions],
	[SETLANGDEFAULTS=CompilerInvocation])
AC_DEFINE_UNQUOTED([SETLANGDEFAULTS], [$SETLANGDEFAULTS],
	[Define to class with setLangDefaults method])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#include <clang/Basic/TargetOptions.h>
	#include <clang/Lex/PreprocessorOptions.h>
	#include <clang/Frontend/CompilerInstance.h>

	#include "include/isl-interface/set_lang_defaults_arg4.h"
]], [[
	using namespace clang;
	CompilerInstance *Clang;
	TargetOptions TO;
	llvm::Triple T(TO.Triple);
	PreprocessorOptions PO;
	SETLANGDEFAULTS::setLangDefaults(Clang->getLangOpts(), IK_C,
			T, setLangDefaultsArg4(PO),
			LangStandard::lang_unspecified);
]])], [AC_DEFINE([SETLANGDEFAULTS_TAKES_5_ARGUMENTS], [],
	[Define if CompilerInvocation::setLangDefaults takes 5 arguments])])
AC_CHECK_HEADER([llvm/Option/Arg.h],
	[AC_DEFINE([HAVE_LLVM_OPTION_ARG_H], [],
		   [Define if llvm/Option/Arg.h exists])])
# llvmorg-20-init-12950-gbdd10d9d249b
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
	       [[#include <clang/Frontend/CompilerInstance.h>]], [[
       clang::CompilerInstance *Clang;
       Clang->createDiagnostics(*llvm::vfs::getRealFileSystem());
]])], [AC_DEFINE([CREATEDIAGNOSTICS_TAKES_VFS], [],
       [Define if CompilerInstance::createDiagnostics takes VFS])])

CPPFLAGS="$SAVE_CPPFLAGS"

AC_LANG_POP
])
