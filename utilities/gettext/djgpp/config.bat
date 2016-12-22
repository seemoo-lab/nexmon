@echo off
echo Configuring GNU Gettext for DJGPP v2.x...

Rem The SmallEnv tests protect against fixed and too small size
Rem of the environment in stock DOS shell.

Rem Find out if NLS is wanted or not,
Rem if dependency-tracking is wanted or not,
Rem if caching is wanted or not
Rem if static or shared libraries are wanted
Rem and where the sources are.
Rem We always default to NLS support,
Rem no dependency tracking, static library
Rem and to in place configuration.
set ARGS=
set NLS=enabled
if not "%NLS%" == "enabled" goto SmallEnv
set CACHING=enabled
if not "%CACHING%" == "enabled" goto SmallEnv
set DEPENDENCY_TRACKING=disabled
if not "%DEPENDENCY_TRACKING%" == "disabled" goto SmallEnv
set LIBICONV_PREFIX=disabled
if not "%LIBICONV_PREFIX%" == "disabled" goto SmallEnv
set LIBINTL_PREFIX=disabled
if not "%LIBINTL_PREFIX%" == "disabled" goto SmallEnv
set STATIC_LIBRARY=enabled
if not "%STATIC_LIBRARY%" == "enabled" goto SmallEnv
set XSRC=.
if not "%XSRC%" == "." goto SmallEnv

Rem Loop over all arguments.
Rem Special arguments are: NLS, XSRC, CACHE, STATIC_LIBS, LIBICONV_PREFIX, LIBINTL_PREFIX and DEPS.
Rem All other arguments are stored into ARGS.
:ArgLoop
if "%1" == "nls" goto NextArgument
if "%1" == "NLS" goto NextArgument
if "%1" == "no-nls" goto NoNLS
if "%1" == "no-NLS" goto NoNLS
if "%1" == "NO-NLS" goto NoNLS
goto CachingOption
:NoNLS
if "%1" == "no-nls" set NLS=disabled
if "%1" == "no-NLS" set NLS=disabled
if "%1" == "NO-NLS" set NLS=disabled
if not "%NLS%" == "disabled" goto SmallEnv
goto NextArgument
:CachingOption
if "%1" == "cache" goto NextArgument
if "%1" == "CACHE" goto NextArgument
if "%1" == "no-cache" goto NoCaching
if "%1" == "no-CACHE" goto NoCaching
if "%1" == "NO-CACHE" goto NoCaching
goto DependencyOption
:NoCaching
if "%1" == "no-cache" set CACHING=disabled
if "%1" == "no-CACHE" set CACHING=disabled
if "%1" == "NO-CACHE" set CACHING=disabled
if not "%CACHING%" == "disabled" goto SmallEnv
goto NextArgument
:DependencyOption
if "%1" == "no-dep" goto NextArgument
if "%1" == "no-DEP" goto NextArgument
if "%1" == "NO-DEP" goto NextArgument
if "%1" == "dep" goto DependecyTraking
if "%1" == "DEP" goto DependecyTraking
goto LibiconvPrefixOption
:DependecyTraking
if "%1" == "dep" set DEPENDENCY_TRACKING=enabled
if "%1" == "DEP" set DEPENDENCY_TRACKING=enabled
if not "%DEPENDENCY_TRACKING%" == "enabled" goto SmallEnv
goto NextArgument
:LibiconvPrefixOption
if "%1" == "no-libiconvprefix" goto NextArgument
if "%1" == "no-LIBICONVPREFIX" goto NextArgument
if "%1" == "NO-LIBICONVPREFIX" goto NextArgument
if "%1" == "libiconvprefix" goto WithLibiconvPrefix
if "%1" == "LIBICONVPREFIX" goto WithLibiconvPrefix
goto LibintlPrefixOption
:WithLibiconvPrefix
if "%1" == "libiconvprefix" set LIBICONV_PREFIX=enabled
if "%1" == "LIBICONVPREFIX" set LIBICONV_PREFIX=enabled
if not "%LIBICONV_PREFIX%" == "enabled" goto SmallEnv
goto NextArgument
:LibintlPrefixOption
if "%1" == "no-libiconvprefix" goto NextArgument
if "%1" == "no-LIBICONVPREFIX" goto NextArgument
if "%1" == "NO-LIBICONVPREFIX" goto NextArgument
if "%1" == "libintlprefix" goto _WithLibintlPrefix
if "%1" == "LIBINTLPREFIX" goto _WithLibintlPrefix
goto StaticLibraryOption
:_WithLibintlPrefix
if "%1" == "libintlprefix" set LIBINTL_PREFIX=enabled
if "%1" == "LIBINTLPREFIX" set LIBINTL_PREFIX=enabled
if not "%LIBINTL_PREFIX%" == "enabled" goto SmallEnv
goto NextArgument
:StaticLibraryOption
if "%1" == "static" goto NextArgument
if "%1" == "STATIC" goto NextArgument
if "%1" == "shared" goto SharedLibrary
if "%1" == "SHARED" goto SharedLibrary
goto SrcDirOption
:SharedLibrary
if "%1" == "shared" set STATIC_LIBRARY=disabled
if "%1" == "SHARED" set STATIC_LIBRARY=disabled
if not "%STATIC_LIBRARY%" == "disabled" goto SmallEnv
goto NextArgument
:SrcDirOption
echo %1 | grep -q "/"
if errorlevel 1 goto CollectArgument
set XSRC=%1
if not "%XSRC%" == "%1" goto SmallEnv
goto NextArgument
:CollectArgument
set _ARGS=%ARGS% %1
if not "%_ARGS%" == "%ARGS% %1" if not "%_ARGS%" == "%ARGS%%1" goto SmallEnv
echo %_ARGS% | grep -q "[^ ]"
if not errorlevel 0 set ARGS=%_ARGS%
set _ARGS=
:NextArgument
shift
if not "%1" == "" goto ArgLoop

Rem Create an arguments file for the configure script.
echo --srcdir=%XSRC% > arguments
if "%CACHING%" == "enabled"              echo --cache-file=%XSRC%/djgpp/config.cache >> arguments
if "%DEPENDENCY_TRACKING%" == "enabled"  echo --enable-dependency-tracking >> arguments
if "%DEPENDENCY_TRACKING%" == "disabled" echo --disable-dependency-tracking >> arguments
if "%LIBICONV_PREFIX%" == "enabled"      echo --with-libiconv-prefix >> arguments
if "%LIBICONV_PREFIX%" == "disabled"     echo --without-libiconv-prefix >> arguments
if "%LIBINTL_PREFIX%" == "enabled"       echo --with-libintl-prefix >> arguments
if "%LIBINTL_PREFIX%" == "disabled"      echo --without-libintl-prefix >> arguments
if "%STATIC_LIBRARY%" == "enabled"       echo --enable-static --disable-shared >> arguments
if "%STATIC_LIBRARY%" == "disabled"      echo --enable-shared --disable-static >> arguments
if not "%ARGS%" == ""                    echo %ARGS% >> arguments
set ARGS=
set CACHING=
set DEPENDENCY_TRACKING=
set LIBICONV_PREFIX=
set LIBINTL_PREFIX=
set STATIC_LIBRARY=

if "%XSRC%" == "." goto InPlace

:NotInPlace
redir -e /dev/null update %XSRC%/configure.orig ./configure
test -f ./configure
if errorlevel 1 update %XSRC%/configure ./configure

:InPlace
Rem Update configuration files
echo Updating configuration scripts...
test -f ./configure.orig
if errorlevel 1 update configure configure.orig
sed -f %XSRC%/djgpp/config.sed configure.orig > configure
if errorlevel 1 goto SedError

Rem Make sure they have a config.site file
set CONFIG_SITE=%XSRC%/djgpp/config.site
if not "%CONFIG_SITE%" == "%XSRC%/djgpp/config.site" goto SmallEnv

Rem Make sure crucial file names are not munged by unpacking
test -f %XSRC%/config.h.in
if not errorlevel 1 mv -f %XSRC%/config.h.in %XSRC%/config.h-in
test -f %XSRC%/configh.in
if not errorlevel 1 mv -f %XSRC%/config.h.in %XSRC%/config.h-in
test -f %XSRC%/config.h-in
if errorlevel 1 mv -f %XSRC%/config.h %XSRC%/config.h-in
test -f %XSRC%/po/Makefile.in.in
if not errorlevel 1 mv -f %XSRC%/po/Makefile.in.in %XSRC%/po/Makefile.in-in

Rem While building the binaries in src/ subdir an intermediary
Rem file called po-gram-gen2.h is generated from po-gram-gen.h.
Rem Both resolve to the same 8.3 filename. po-gram-gen2.h will
Rem be renamed to po-gram_gen2.h and src/po-lex.c must be fixed
Rem accordingly.
test -f %XSRC%/src/po-lex.orig
if errorlevel 1 update %XSRC%/src/po-lex.c %XSRC%/src/po-lex.orig
sed "s/po-gram-gen2.h/po-gram_gen2.h/g" %XSRC%/src/po-lex.orig > po-lex.tmp
if errorlevel 1 goto SedError
mv ./po-lex.tmp %XSRC%/src/po-lex.c

Rem Starting with gettext-0.11 posix function unsetenv() is needed.
Rem As long as djdev204 has not been released, we will provide
Rem unsetenv.c from djdev204 CVS tree.
test -f %XSRC%/lib/unsetenv.c
if errorlevel 1 update %XSRC%/djgpp/unsetenv.c %XSRC%/lib/unsetenv.c

Rem Starting with gettext-0.11 pw_gecos is needed.
Rem As long as djdev204 has not been released, we will provide
Rem getpwman.c and pwd.h (djpwd.h) from djdev204 CVS tree.
test -f %XSRC%/lib/djpwd.h
if errorlevel 1 update %XSRC%/djgpp/djpwd.h %XSRC%/lib/djpwd.h
test -f %XSRC%/lib/getpwnam.c
if errorlevel 1 update %XSRC%/djgpp/getpwnam.c %XSRC%/lib/getpwnam.c

Rem src/msginit.c must use the distributed CVS tree pwd.h
Rem instead of the system's one.
test -f %XSRC%/src/msginit.orig
if errorlevel 1 update %XSRC%/src/msginit.c %XSRC%/src/msginit.orig
sed -f %XSRC%/djgpp/msginit.sed %XSRC%/src/msginit.orig > msginit.tmp
if errorlevel 1 goto SedError
mv ./msginit.tmp %XSRC%/src/msginit.c

Rem POTFILES.in must be adjusted to reflect the changed names
Rem according to fnchange.lst.
test -f %XSRC%/po/POTFILES.orig
if errorlevel 1 update %XSRC%/po/POTFILES.in %XSRC%/po/POTFILES.orig
sed "s/format-librep/format_librep/;s/format-pascal/format_pascal/" %XSRC%/po/POTFILES.orig > POTFILES.tmp
if errorlevel 1 goto SedError
mv ./POTFILES.tmp %XSRC%/po/POTFILES.in

Rem This is required because DOS/Windows are case-insensitive
Rem to file names, and "make install" will do nothing if Make
Rem finds a file called `install'.
if exist INSTALL ren INSTALL INSTALL.txt

Rem Set SHELL to a sane default or some configure tests stop working
Rem if the package is configured across partitions.
if not "%SHELL%" == "" goto HomeName
set SHELL=/bin/sh
if not "%SHELL%" == "/bin/sh" goto SmallEnv
echo No SHELL found in the environment, using default value

:HomeName
Rem Set HOME to a sane default so configure stops complaining.
if not "%HOME%" == "" goto HostName
set HOME=%XSRC%/djgpp
if not "%HOME%" == "%XSRC%/djgpp" goto SmallEnv
echo No HOME found in the environment, using default value

:HostName
Rem Set HOSTNAME so it shows in config.status
if not "%HOSTNAME%" == "" goto hostdone
if "%windir%" == "" goto msdos
set OS=MS-Windows
if not "%OS%" == "MS-Windows" goto SmallEnv
goto haveos
:msdos
set OS=MS-DOS
if not "%OS%" == "MS-DOS" goto SmallEnv
:haveos
if not "%USERNAME%" == "" goto haveuname
if not "%USER%" == "" goto haveuser
echo No USERNAME and no USER found in the environment, using default values
set HOSTNAME=Unknown PC
if not "%HOSTNAME%" == "Unknown PC" goto SmallEnv
goto userdone
:haveuser
set HOSTNAME=%USER%'s PC
if not "%HOSTNAME%" == "%USER%'s PC" goto SmallEnv
goto userdone
:haveuname
set HOSTNAME=%USERNAME%'s PC
if not "%HOSTNAME%" == "%USERNAME%'s PC" goto SmallEnv
:userdone
set _HOSTNAME=%HOSTNAME%, %OS%
if not "%_HOSTNAME%" == "%HOSTNAME%, %OS%" goto SmallEnv
set HOSTNAME=%_HOSTNAME%
:hostdone
set _HOSTNAME=
set OS=

Rem install-sh is required by the configure script but clashes with the
Rem various Makefile install-foo targets, so we MUST have it before the
Rem script runs and rename it afterwards
test -f %XSRC%/install-sh
if not errorlevel 1 goto NoRen0
test -f %XSRC%/install-sh.sh
if not errorlevel 1 mv -f %XSRC%/install-sh.sh %XSRC%/install-sh
:NoRen0

if "%NLS%" == "disabled" goto WithoutNLS

:WithNLS
Rem Recreate the files in the %XSRC%/po subdir with our ported tools.
redir -e /dev/null rm %XSRC%/po/*.gmo
redir -e /dev/null rm %XSRC%/po/gettext.pot
redir -e /dev/null rm %XSRC%/po/cat-id-tbl.c
redir -e /dev/null rm %XSRC%/po/stamp-cat-id

Rem Update the arguments file for the configure script.
echo --enable-nls --with-included-gettext >> arguments
goto ConfigurePackage

:WithoutNLS
Rem Update the arguments file for the configure script.
echo --disable-nls >> arguments

:ConfigurePackage
echo Running the ./configure script...
sh ./configure @arguments
if errorlevel 1 goto CfgError
rm arguments
echo Done.

:ScriptEditing
Rem DJGPP specific editing of test scripts.
test -f %XSRC%/tests/stamp-test
if not errorlevel 1 goto End
if "%XSRC%" == "." goto NoDirChange
cd | sed "s|:.*$|:|" > cd_BuildDir.bat
cd | sed "s|^.:|cd |" >> cd_BuildDir.bat
mv -f cd_BuildDir.bat %XSRC%/cd_BuildDir.bat
echo %XSRC% | sed -e "s|^/dev/||" -e "s|/|:|" -e "s|:.*$|:|g" > cd_SrcDir.bat
echo %XSRC% | sed -e "s|^/dev/||" -e "s|/|:/|" -e "s|^.*:|cd |" -e "s|^\.\.|cd &|" -e "s|/|\\|g" >> cd_SrcDir.bat
call cd_SrcDir.bat
call djgpp\edtests.bat
call cd_BuildDir.bat
rm -f cd_SrcDir.bat cd_BuildDir.bat %XSRC%/cd_BuildDir.bat
goto End
:NoDirChange
call djgpp\edtests.bat
goto End

:SedError
echo ./configure script editing failed!
goto End

:CfgError
echo ./configure script exited abnormally!
goto End

:SmallEnv
echo Your environment size is too small.  Enlarge it and run me again.
echo Configuration NOT done!

:End
test -f %XSRC%/install-sh.sh
if not errorlevel 1 goto NoRen1
test -f %XSRC%/install-sh
if not errorlevel 1 mv -f %XSRC%/install-sh %XSRC%/install-sh.sh
:NoRen1
if "%SHELL%" == "/bin/sh" set SHELL=
if "%HOME%" == "%XSRC%/djgpp" set HOME=
set CONFIG_SITE=
set HOSTNAME=
set NLS=
set CACHING=
set DEPENDENCY_TRACKING=
set XSRC=
