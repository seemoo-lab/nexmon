#!/bin/sh
# Copyright (C) 2003-2016 Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This script populates the build infrastructure in the source tree
# checked-out from VCS.
#
# This script requires:
#   - Autoconf
#   - Automake
#   - Wget
#   - Git
#   - XZ Utils
#
# By default, it fetches Gnulib as a git submodule.  If you already
# have a local copy of Gnulib, you can avoid extra network traffic by
# setting the GNULIB_SRCDIR environment variable pointing to the path.
#
# In addition, it fetches the archive.dir.tar.gz file, which contains
# data files used by the autopoint program.  If you already have the
# file, place it under gettext-tools/misc, before running this script.
#
# Usage: ./autogen.sh [--skip-gnulib] [--no-git]
#
# Usage after a git clone:              ./autogen.sh
# Usage from a released tarball:        ./autogen.sh --skip-gnulib
# This does not use a gnulib checkout.

# Nuisances.
(unset CDPATH) >/dev/null 2>&1 && unset CDPATH

skip_gnulib=false

# Use git to update gnulib sources
use_git=true

while :; do
  case "$1" in
    --skip-gnulib) skip_gnulib=true; shift;;
    --no-git) use_git=false; shift;;
    *) break ;;
  esac
done

$use_git || test -d "$GNULIB_SRCDIR" \
  || die "Error: --no-git requires --gnulib-srcdir"

cleanup_gnulib() {
  status=$?
  rm -fr "$gnulib_path"
  exit $status
}

git_modules_config () {
  test -f .gitmodules && git config --file .gitmodules "$@"
}

gnulib_path=$(git_modules_config submodule.gnulib.path)
test -z "$gnulib_path" && gnulib_path=gnulib

# The tests in gettext-tools/tests are not meant to be executable, because
# they have a TESTS_ENVIRONMENT that specifies the shell explicitly.

if ! $skip_gnulib; then
  # Get gnulib files.
  case ${GNULIB_SRCDIR--} in
  -)
    if git_modules_config submodule.gnulib.url >/dev/null; then
      echo "$0: getting gnulib files..."
      git submodule init || exit $?
      git submodule update || exit $?

    elif [ ! -d "$gnulib_path" ]; then
      echo "$0: getting gnulib files..."

      trap cleanup_gnulib 1 2 13 15

      shallow=
      git clone -h 2>&1 | grep -- --depth > /dev/null && shallow='--depth 2'
      git clone $shallow git://git.sv.gnu.org/gnulib "$gnulib_path" ||
        cleanup_gnulib

      trap - 1 2 13 15
    fi
    GNULIB_SRCDIR=$gnulib_path
    ;;
  *)
    # Use GNULIB_SRCDIR as a reference.
    if $use_git && test -d "$GNULIB_SRCDIR"/.git && \
          git_modules_config submodule.gnulib.url >/dev/null; then
      echo "$0: getting gnulib files..."
      if git submodule -h|grep -- --reference > /dev/null; then
        # Prefer the one-liner available in git 1.6.4 or newer.
        git submodule update --init --reference "$GNULIB_SRCDIR" \
          "$gnulib_path" || exit $?
      else
        # This fallback allows at least git 1.5.5.
        if test -f "$gnulib_path"/gnulib-tool; then
          # Since file already exists, assume submodule init already complete.
          git submodule update || exit $?
        else
          # Older git can't clone into an empty directory.
          rmdir "$gnulib_path" 2>/dev/null
          git clone --reference "$GNULIB_SRCDIR" \
            "$(git_modules_config submodule.gnulib.url)" "$gnulib_path" \
            && git submodule init && git submodule update \
            || exit $?
        fi
      fi
      GNULIB_SRCDIR=$gnulib_path
    fi
    ;;
  esac
  # Now it should contain a gnulib-tool.
  if test -f "$GNULIB_SRCDIR"/gnulib-tool; then
    GNULIB_TOOL="$GNULIB_SRCDIR"/gnulib-tool
  else
    echo "** warning: gnulib-tool not found" 1>&2
  fi
  # Skip the gnulib-tool step if gnulib-tool was not found.
  if test -n "$GNULIB_TOOL"; then
    # In gettext-runtime:
    GNULIB_MODULES_RUNTIME_FOR_SRC='
      atexit
      basename
      closeout
      error
      getopt-gnu
      gettext-h
      havelib
      memmove
      progname
      propername
      relocatable-prog
      setlocale
      sigpipe
      stdbool
      stdio
      stdlib
      strtoul
      unlocked-io
      xalloc
    '
    GNULIB_MODULES_RUNTIME_OTHER='
      gettext-runtime-misc
      ansi-c++-opt
      csharpcomp-script
      java
      javacomp-script
    '
    $GNULIB_TOOL --dir=gettext-runtime --lib=libgrt --source-base=gnulib-lib --m4-base=gnulib-m4 --no-libtool --local-dir=gnulib-local --local-symlink \
      --import $GNULIB_MODULES_RUNTIME_FOR_SRC $GNULIB_MODULES_RUNTIME_OTHER || exit $?
    # In gettext-runtime/libasprintf:
    GNULIB_MODULES_LIBASPRINTF='
      alloca
      errno
      verify
      xsize
    '
    GNULIB_MODULES_LIBASPRINTF_OTHER='
    '
    $GNULIB_TOOL --dir=gettext-runtime/libasprintf --source-base=. --m4-base=gnulib-m4 --lgpl=2 --makefile-name=Makefile.gnulib --libtool --local-dir=gnulib-local --local-symlink \
      --import $GNULIB_MODULES_LIBASPRINTF $GNULIB_MODULES_LIBASPRINTF_OTHER || exit $?
    $GNULIB_TOOL --copy-file m4/intmax_t.m4 gettext-runtime/libasprintf/gnulib-m4/intmax_t.m4 || exit $?
    # In gettext-tools:
    GNULIB_MODULES_TOOLS_FOR_SRC='
      alloca-opt
      atexit
      backupfile
      basename
      binary-io
      bison-i18n
      byteswap
      c-ctype
      c-strcase
      c-strcasestr
      c-strstr
      clean-temp
      closedir
      closeout
      copy-file
      csharpcomp
      csharpexec
      error
      error-progname
      execute
      fd-ostream
      file-ostream
      filename
      findprog
      fnmatch
      fopen
      fstrcmp
      full-write
      fwriteerror
      gcd
      getline
      getopt-gnu
      gettext
      gettext-h
      hash
      html-styled-ostream
      iconv
      javacomp
      javaexec
      libunistring-optional
      localcharset
      locale
      localename
      lock
      memmove
      memset
      minmax
      obstack
      open
      opendir
      openmp
      ostream
      pipe-filter-ii
      progname
      propername
      readdir
      relocatable-prog
      relocatable-script
      setlocale
      sh-quote
      sigpipe
      sigprocmask
      spawn-pipe
      stdbool
      stdio
      stdlib
      stpcpy
      stpncpy
      strcspn
      strerror
      strpbrk
      strtol
      strtoul
      styled-ostream
      sys_select
      sys_stat
      sys_time
      term-styled-ostream
      trim
      unictype/ctype-space
      unilbrk/ulc-width-linebreaks
      uniname/uniname
      unistd
      unistr/u8-mbtouc
      unistr/u8-mbtoucr
      unistr/u8-uctomb
      unistr/u16-mbtouc
      uniwidth/width
      unlocked-io
      vasprintf
      wait-process
      write
      xalloc
      xconcat-filename
      xerror
      xmalloca
      xmemdup0
      xsetenv
      xstriconv
      xstriconveh
      xvasprintf
    '
    # Common dependencies of GNULIB_MODULES_TOOLS_FOR_SRC and GNULIB_MODULES_TOOLS_FOR_LIBGREP.
    GNULIB_MODULES_TOOLS_FOR_SRC_COMMON_DEPENDENCIES='
      alloca-opt
      extensions
      gettext-h
      include_next
      locale
      localcharset
      malloc-posix
      mbrtowc
      mbsinit
      multiarch
      snippet/arg-nonnull
      snippet/c++defs
      snippet/warn-on-use
      ssize_t
      stdbool
      stddef
      stdint
      stdlib
      streq
      unistd
      verify
      wchar
      wctype-h
    '
    GNULIB_MODULES_TOOLS_OTHER='
      gettext-tools-misc
      ansi-c++-opt
      csharpcomp-script
      csharpexec-script
      gcj
      java
      javacomp-script
      javaexec-script
      stdint
    '
    GNULIB_MODULES_TOOLS_LIBUNISTRING_TESTS='
      unilbrk/u8-possible-linebreaks-tests
      unilbrk/ulc-width-linebreaks-tests
      unistr/u8-mbtouc-tests
      unistr/u8-mbtouc-unsafe-tests
      uniwidth/width-tests
    '
    $GNULIB_TOOL --dir=gettext-tools --lib=libgettextlib --source-base=gnulib-lib --m4-base=gnulib-m4 --tests-base=gnulib-tests --makefile-name=Makefile.gnulib --libtool --with-tests --local-dir=gnulib-local --local-symlink \
      --import --avoid=hash-tests `for m in $GNULIB_MODULES_TOOLS_LIBUNISTRING_TESTS; do echo --avoid=$m; done` $GNULIB_MODULES_TOOLS_FOR_SRC $GNULIB_MODULES_TOOLS_FOR_SRC_COMMON_DEPENDENCIES $GNULIB_MODULES_TOOLS_OTHER || exit $?
    # In gettext-tools/libgrep:
    GNULIB_MODULES_TOOLS_FOR_LIBGREP='
      mbrlen
      regex
    '
    $GNULIB_TOOL --dir=gettext-tools --macro-prefix=grgl --lib=libgrep --source-base=libgrep --m4-base=libgrep/gnulib-m4 --witness-c-macro=IN_GETTEXT_TOOLS_LIBGREP --makefile-name=Makefile.gnulib --local-dir=gnulib-local --local-symlink \
      --import `for m in $GNULIB_MODULES_TOOLS_FOR_SRC_COMMON_DEPENDENCIES; do if test \`$GNULIB_TOOL --extract-applicability $m\` != all; then echo --avoid=$m; fi; done` $GNULIB_MODULES_TOOLS_FOR_LIBGREP || exit $?
    # In gettext-tools/libgettextpo:
    # This is a subset of the GNULIB_MODULES_FOR_SRC.
    GNULIB_MODULES_LIBGETTEXTPO='
      basename
      close
      c-ctype
      c-strcase
      c-strstr
      error
      error-progname
      file-ostream
      filename
      fopen
      fstrcmp
      fwriteerror
      gcd
      getline
      gettext-h
      hash
      iconv
      libunistring-optional
      markup
      minmax
      open
      ostream
      progname
      relocatable-lib
      sigpipe
      stdbool
      stdio
      stdlib
      stpcpy
      stpncpy
      strchrnul
      strerror
      unictype/ctype-space
      unilbrk/ulc-width-linebreaks
      unistr/u8-mbtouc
      unistr/u8-mbtoucr
      unistr/u8-uctomb
      unistr/u16-mbtouc
      uniwidth/width
      unlocked-io
      vasprintf
      xalloc
      xconcat-filename
      xmalloca
      xerror
      xstriconv
      xvasprintf
    '
    GNULIB_MODULES_LIBGETTEXTPO_OTHER='
    '
    $GNULIB_TOOL --dir=gettext-tools --source-base=libgettextpo --m4-base=libgettextpo/gnulib-m4 --macro-prefix=gtpo --makefile-name=Makefile.gnulib --libtool --local-dir=gnulib-local --local-symlink \
      --import $GNULIB_MODULES_LIBGETTEXTPO $GNULIB_MODULES_LIBGETTEXTPO_OTHER || exit $?
    # Import build tools.  We use --copy-file to avoid directory creation.
    $GNULIB_TOOL --copy-file tests/init.sh gettext-tools || exit $?
    $GNULIB_TOOL --copy-file build-aux/git-version-gen || exit $?
    $GNULIB_TOOL --copy-file build-aux/gitlog-to-changelog || exit $?
    $GNULIB_TOOL --copy-file build-aux/update-copyright || exit $?
    $GNULIB_TOOL --copy-file build-aux/useless-if-before-free || exit $?
    $GNULIB_TOOL --copy-file build-aux/vc-list-files || exit $?
    $GNULIB_TOOL --copy-file top/GNUmakefile . || exit $?
    $GNULIB_TOOL --copy-file top/maint.mk . || exit $?
  fi
fi

# Fetch config.guess, config.sub.
if test -n "$GNULIB_TOOL"; then
  for file in config.guess config.sub; do
    $GNULIB_TOOL --copy-file build-aux/$file; chmod a+x build-aux/$file || exit $?
  done
else
  for file in config.guess config.sub; do
    echo "$0: getting $file..."
    wget -q --timeout=5 -O build-aux/$file.tmp "http://git.savannah.gnu.org/gitweb/?p=gnulib.git;a=blob_plain;f=build-aux/${file};hb=HEAD" \
      && mv build-aux/$file.tmp build-aux/$file \
      && chmod a+x build-aux/$file
    retval=$?
    rm -f build-aux/$file.tmp
    test $retval -eq 0 || exit $retval
  done
fi

# Fetch gettext-tools/misc/archive.dir.tar.
if ! test -f gettext-tools/misc/archive.dir.tar; then
  if ! test -f gettext-tools/misc/archive.dir.tar.xz; then
    echo "$0: getting gettext-tools/misc/archive.dir.tar..."
    wget -q --timeout=5 -O gettext-tools/misc/archive.dir.tar.xz-t "ftp://alpha.gnu.org/gnu/gettext/archive.dir-latest.tar.xz" \
      && mv gettext-tools/misc/archive.dir.tar.xz-t gettext-tools/misc/archive.dir.tar.xz
    retval=$?
    rm -f gettext-tools/misc/archive.dir.tar.xz-t
    test $retval -eq 0 || exit $retval
  fi
  xz -d -c < gettext-tools/misc/archive.dir.tar.xz > gettext-tools/misc/archive.dir.tar-t \
    && mv gettext-tools/misc/archive.dir.tar-t gettext-tools/misc/archive.dir.tar
  retval=$?
  rm -f gettext-tools/misc/archive.dir.tar-t
  test $retval -eq 0 || exit $retval
fi

# Generate configure script in each subdirectories.
dir0=`pwd`

echo "$0: generating configure in gettext-runtime/libasprintf..."
cd gettext-runtime/libasprintf
aclocal -I ../../m4 -I ../m4 -I gnulib-m4 \
  && autoconf \
  && autoheader \
  && touch ChangeLog config.h.in \
  && automake --add-missing --copy \
  || exit $?
cd "$dir0"

echo "$0: generating configure in gettext-runtime..."
cd gettext-runtime
aclocal -I m4 -I ../m4 -I gnulib-m4 \
  && autoconf \
  && autoheader \
  && touch ChangeLog intl/ChangeLog config.h.in \
  && automake --add-missing --copy \
  || exit $?
cd "$dir0"

echo "$0: generating configure in gettext-tools/examples..."
cd gettext-tools/examples
aclocal -I ../../gettext-runtime/m4 -I ../../m4 \
  && autoconf \
  && touch ChangeLog \
  && automake --add-missing --copy \
  || exit $?
cd "$dir0"

echo "$0: copying common files from gettext-runtime to gettext-tools..."
cp -p gettext-runtime/ABOUT-NLS gettext-tools/ABOUT-NLS
cp -p gettext-runtime/po/Makefile.in.in gettext-tools/po/Makefile.in.in
cp -p gettext-runtime/po/Rules-quot gettext-tools/po/Rules-quot
cp -p gettext-runtime/po/boldquot.sed gettext-tools/po/boldquot.sed
cp -p gettext-runtime/po/quot.sed gettext-tools/po/quot.sed
cp -p gettext-runtime/po/en@quot.header gettext-tools/po/en@quot.header
cp -p gettext-runtime/po/en@boldquot.header gettext-tools/po/en@boldquot.header
cp -p gettext-runtime/po/insert-header.sin gettext-tools/po/insert-header.sin
cp -p gettext-runtime/po/remove-potcdate.sin gettext-tools/po/remove-potcdate.sin
# Those two files might be newer than Gnulib's.
sed_extract_serial='s/^#.* serial \([^ ]*\).*/\1/p
1q'
for file in intl.m4 po.m4; do
  existing_serial=`sed -n -e "$sed_extract_serial" < "gettext-tools/gnulib-m4/$file"`
  gettext_serial=`sed -n -e "$sed_extract_serial" < "gettext-runtime/m4/$file"`
  if test -n "$existing_serial" && test -n "$gettext_serial" \
        && test "$existing_serial" -ge "$gettext_serial" 2> /dev/null; then
    :
  else
    cp -p "gettext-runtime/m4/$file" "gettext-tools/gnulib-m4/$file"
  fi
done

echo "$0: generating configure in gettext-tools..."
cd gettext-tools
aclocal -I m4 -I ../gettext-runtime/m4 -I ../m4 -I gnulib-m4 -I libgrep/gnulib-m4 -I libgettextpo/gnulib-m4 \
  && autoconf \
  && autoheader && touch ChangeLog config.h.in \
  && test -d intl || mkdir intl \
  && automake --add-missing --copy \
  || exit $?
cd "$dir0"

aclocal -I m4 && autoconf && touch ChangeLog && automake || exit $?

echo "$0: done.  Now you can run './configure'."
