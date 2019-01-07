## Makefile for the toplevel directory of GNU gettext
## Copyright (C) 1995-2016 Free Software Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gnulib-local gettext-runtime gettext-tools

changelog_etc = \
  gettext-runtime/ChangeLog.0 \
  gettext-runtime/doc/ChangeLog.0 \
  gettext-runtime/intl-csharp/ChangeLog.0 \
  gettext-runtime/intl-java/ChangeLog.0 \
  gettext-runtime/libasprintf/ChangeLog.0 \
  gettext-runtime/m4/ChangeLog.0 \
  gettext-runtime/man/ChangeLog.0 \
  gettext-runtime/src/ChangeLog.0 \
  gettext-runtime/tests/ChangeLog.0 \
  gettext-tools/ChangeLog.0 \
  gettext-tools/doc/ChangeLog.0 \
  gettext-tools/doc/ChangeLog.1 \
  gettext-tools/examples/ChangeLog.0 \
  gettext-tools/gnulib-tests/ChangeLog.0 \
  gettext-tools/libgettextpo/ChangeLog.0 \
  gettext-tools/libgrep/ChangeLog.0 \
  gettext-tools/m4/ChangeLog.0 \
  gettext-tools/man/ChangeLog.0 \
  gettext-tools/man/ChangeLog.1 \
  gettext-tools/misc/ChangeLog.0 \
  gettext-tools/projects/ChangeLog.0 \
  gettext-tools/src/ChangeLog.0 \
  gettext-tools/src/ChangeLog.1 \
  gettext-tools/styles/ChangeLog.0 \
  gettext-tools/tests/ChangeLog.0 \
  gettext-tools/tests/ChangeLog.1 \
  gnulib-local/ChangeLog.0

EXTRA_DIST = \
  $(changelog_etc) DEPENDENCIES PACKAGING HACKING ChangeLog.0 autogen.sh \
  build-aux/ac-help.sed build-aux/moopp build-aux/git-version-gen \
  m4/fixautomake.m4 m4/woe32-dll.m4 \
  m4/libtool.m4

# Additional dependencies for configure, due to the use of autoconf --trace.
$(srcdir)/configure: $(srcdir)/gettext-runtime/configure.ac $(srcdir)/gettext-tools/configure.ac 

# Verify that some files are the same.
distcheck-hook:
	cmp -s gettext-runtime/po/Makefile.in.in gettext-tools/po/Makefile.in.in
	cmp -s gettext-runtime/po/Rules-quot gettext-tools/po/Rules-quot
	cmp -s gettext-runtime/po/boldquot.sed gettext-tools/po/boldquot.sed
	cmp -s gettext-runtime/po/quot.sed gettext-tools/po/quot.sed
	cmp -s gettext-runtime/po/en@quot.header gettext-tools/po/en@quot.header
	cmp -s gettext-runtime/po/en@boldquot.header gettext-tools/po/en@boldquot.header
	cmp -s gettext-runtime/po/insert-header.sin gettext-tools/po/insert-header.sin
	cmp -s gettext-runtime/po/remove-potcdate.sin gettext-tools/po/remove-potcdate.sin
	cmp -s gettext-runtime/po/remove-potcdate.sin gettext-tools/examples/po/remove-potcdate.sin
	cmp -s gettext-runtime/m4/codeset.m4 gettext-tools/gnulib-m4/codeset.m4
	cmp -s gettext-runtime/m4/extern-inline.m4 gettext-tools/gnulib-m4/extern-inline.m4
	cmp -s gettext-runtime/m4/fcntl-o.m4 gettext-tools/gnulib-m4/fcntl-o.m4
	cmp -s gettext-runtime/m4/gettext.m4 gettext-tools/gnulib-m4/gettext.m4
	cmp -s gettext-runtime/m4/glibc2.m4 gettext-tools/gnulib-m4/glibc2.m4
	cmp -s gettext-runtime/m4/glibc21.m4 gettext-tools/gnulib-m4/glibc21.m4
	cmp -s gettext-runtime/m4/iconv.m4 gettext-tools/gnulib-m4/iconv.m4
	cmp -s gettext-runtime/m4/intdiv0.m4 gettext-tools/gnulib-m4/intdiv0.m4
	cmp -s gettext-runtime/m4/intl.m4 gettext-tools/gnulib-m4/intl.m4
	cmp -s gettext-runtime/m4/intldir.m4 gettext-tools/gnulib-m4/intldir.m4
	cmp -s gettext-runtime/m4/intlmacosx.m4 gettext-tools/gnulib-m4/intlmacosx.m4
	cmp -s gettext-runtime/m4/intmax.m4 gettext-tools/gnulib-m4/intmax.m4
	cmp -s gettext-runtime/m4/inttypes-pri.m4 gettext-tools/gnulib-m4/inttypes-pri.m4
	cmp -s gettext-runtime/m4/inttypes_h.m4 gettext-tools/gnulib-m4/inttypes_h.m4
	cmp -s gettext-runtime/m4/lcmessage.m4 gettext-tools/gnulib-m4/lcmessage.m4
	cmp -s gettext-runtime/m4/lock.m4 gettext-tools/gnulib-m4/lock.m4
	cmp -s gettext-runtime/m4/longlong.m4 gettext-tools/gnulib-m4/longlong.m4
	cmp -s gettext-runtime/m4/nls.m4 gettext-tools/gnulib-m4/nls.m4
	cmp -s gettext-runtime/m4/po.m4 gettext-tools/gnulib-m4/po.m4
	cmp -s gettext-runtime/m4/gettext.m4 gettext-tools/gnulib-m4/gettext.m4
	cmp -s gettext-runtime/m4/printf-posix.m4 gettext-tools/gnulib-m4/printf-posix.m4
	cmp -s gettext-runtime/m4/progtest.m4 gettext-tools/gnulib-m4/progtest.m4
	cmp -s gettext-runtime/m4/size_max.m4 gettext-tools/gnulib-m4/size_max.m4
	cmp -s gettext-runtime/m4/stdint_h.m4 gettext-tools/gnulib-m4/stdint_h.m4
	cmp -s gettext-runtime/m4/threadlib.m4 gettext-tools/gnulib-m4/threadlib.m4
	cmp -s gettext-runtime/m4/uintmax_t.m4 gettext-tools/gnulib-m4/uintmax_t.m4
	cmp -s gettext-runtime/m4/visibility.m4 gettext-tools/gnulib-m4/visibility.m4
	cmp -s gettext-runtime/m4/wchar_t.m4 gettext-tools/gnulib-m4/wchar_t.m4
	cmp -s gettext-runtime/m4/wint_t.m4 gettext-tools/gnulib-m4/wint_t.m4
	cmp -s gettext-runtime/m4/xsize.m4 gettext-tools/gnulib-m4/xsize.m4
	cmp -s gettext-runtime/libasprintf/gnulib-m4/intmax_t.m4 gettext-tools/gnulib-m4/intmax_t.m4
	test "`sed 1,17d gettext-runtime/intl/config.charset | md5sum`" = "`sed 1,17d gettext-tools/gnulib-lib/config.charset | md5sum`"
	test "`sed 1,16d gettext-runtime/intl/localcharset.h | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/localcharset.h | md5sum`"
	test "`sed 1,16d gettext-runtime/intl/localcharset.c | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/localcharset.c | md5sum`"
	test "`sed 1,16d gettext-runtime/intl/localename.c | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/localename.c | md5sum`"
	test "`sed 1,15d gettext-runtime/intl/lock.h | md5sum`" = "`sed 1,15d gettext-tools/gnulib-lib/glthread/lock.h | md5sum`"
	test "`sed 1,15d gettext-runtime/intl/lock.c | md5sum`" = "`sed -e 1,15d -e 's,glthread/,,g' gettext-tools/gnulib-lib/glthread/lock.c | md5sum`"
	test "`sed 1,16d gettext-runtime/intl/verify.h | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/verify.h | md5sum`"
	cmp -s gettext-runtime/intl/printf-args.h gettext-runtime/libasprintf/printf-args.h
	cmp -s gettext-runtime/intl/printf-args.c gettext-runtime/libasprintf/printf-args.c
	cmp -s gettext-runtime/intl/printf-parse.h gettext-runtime/libasprintf/printf-parse.h
	cmp -s gettext-runtime/intl/printf-parse.c gettext-runtime/libasprintf/printf-parse.c
	cmp -s gettext-runtime/intl/verify.h gettext-runtime/libasprintf/verify.h
	test "`sed 1,16d gettext-runtime/intl/ref-add.sin | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/ref-add.sin | md5sum`"
	test "`sed 1,16d gettext-runtime/intl/ref-del.sin | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/ref-del.sin | md5sum`"
	test "`sed 1,18d gettext-runtime/intl/relocatable.h | md5sum`" = "`sed 1,18d gettext-tools/gnulib-lib/relocatable.h | md5sum`"
	test "`sed 1,18d gettext-runtime/intl/relocatable.c | md5sum`" = "`sed 1,18d gettext-tools/gnulib-lib/relocatable.c | md5sum`"
	cmp -s gettext-runtime/intl/vasnprintf.h gettext-runtime/libasprintf/vasnprintf.h
	test "`sed 1,16d gettext-runtime/intl/vasnprintf.c | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/vasnprintf.c | md5sum`"
	cmp -s gettext-runtime/intl/vasnprintf.c gettext-runtime/libasprintf/vasnprintf.c
	cmp -s gettext-runtime/intl/xsize.h gettext-runtime/libasprintf/xsize.h
	test "`sed 1,16d gettext-runtime/intl/xsize.h | md5sum`" = "`sed 1,16d gettext-tools/gnulib-lib/xsize.h | md5sum`"
	cmp -s gettext-runtime/man/help2man gettext-tools/man/help2man
	cmp -s gettext-runtime/man/x-to-1.in gettext-tools/man/x-to-1.in
	cmp -s gettext-runtime/libasprintf/texi2html gettext-tools/doc/texi2html
	cmp -s gettext-tools/examples/hello-java-awt/m4/TestAWT.java gettext-tools/examples/hello-java-swing/m4/TestAWT.java
	cmp -s gettext-tools/examples/hello-java-awt/m4/TestAWT.class gettext-tools/examples/hello-java-swing/m4/TestAWT.class
	test "`sed 1,15d gnulib-local/lib/alloca.in.h | md5sum`" = "`sed 1,15d gettext-runtime/libasprintf/alloca.in.h | md5sum`"

# DJGPP port.

EXTRA_DIST += djgpp/COPYING.DJ djgpp/Makefile.maint djgpp/README.DJ \
              djgpp/README.in djgpp/README djgpp/config.bat djgpp/config.in \
              djgpp/config.sed djgpp/config.site djgpp/djpwd.h \
              djgpp/edtests.bat djgpp/fnchange.in djgpp/fnchange.lst \
              djgpp/getpwnam.c djgpp/msginit.sed djgpp/tscript.sed \
              djgpp/unsetenv.c


# OS/2 port.

EXTRA_DIST += os2/README.OS2 os2/Makefile os2/backward.def os2/configure.awk


# Woe32 support.

EXTRA_DIST += README.woe32 windows/windres-options windows/gettext.rc

# Versioning based on Git release tags.

EXTRA_DIST += $(top_srcdir)/.version
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook: gen-ChangeLogs
	echo $(VERSION) > $(distdir)/.tarball-version

# Generate ChangeLog.

gen_start_date = 2015-10-13
.PHONY: gen-ChangeLogs
gen-ChangeLogs:
	$(AM_V_GEN)if test -d .git; then				\
	  log_fix="$(srcdir)/build-aux/git-log-fix";			\
	  test -e "$$log_fix"						\
	    && amend_git_log="--amend=$$log_fix"			\
	    || amend_git_log=;						\
	  for d in . gettext-runtime/intl gettext-runtime/po		\
		gettext-tools/po; do					\
	    test "$$d" = "."						\
	      && cldir="$(distdir)"					\
	      || cldir="$(distdir)/$$d";				\
	    $(top_srcdir)/build-aux/gitlog-to-changelog			\
	      $$amend_git_log --since=$(gen_start_date) -- "$$d"	\
		> "$$cldir/cl-t" &&					\
	    { rm -f "$$cldir/ChangeLog" &&				\
	      mv "$$cldir/cl-t" "$$cldir/ChangeLog"; };			\
	  done;								\
	fi

# PO files update.

maintainer-update-po-local: $(top_builddir)/config.status
