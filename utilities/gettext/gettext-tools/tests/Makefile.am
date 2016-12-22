## Makefile for the gettext-tools/tests subdirectory of GNU gettext
## Copyright (C) 1995-1997, 2001-2010, 2015-2016 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.11 gnits no-dependencies color-tests
EXTRA_DIST =
MOSTLYCLEANFILES = core *.stackdump

TESTS = gettext-1 gettext-2 gettext-3 gettext-4 gettext-5 gettext-6 gettext-7 \
	gettext-8 \
	msgattrib-1 msgattrib-2 msgattrib-3 msgattrib-4 msgattrib-5 \
	msgattrib-6 msgattrib-7 msgattrib-8 msgattrib-9 msgattrib-10 \
	msgattrib-11 msgattrib-12 msgattrib-13 msgattrib-14 msgattrib-15 \
	msgattrib-16 msgattrib-17 msgattrib-18 msgattrib-19 \
	msgattrib-properties-1 \
	msgcat-1 msgcat-2 msgcat-3 msgcat-4 msgcat-5 msgcat-6 msgcat-7 \
	msgcat-8 msgcat-9 msgcat-10 msgcat-11 msgcat-12 msgcat-13 msgcat-14 \
	msgcat-15 msgcat-16 msgcat-17 msgcat-18 msgcat-19 msgcat-20 \
	msgcat-properties-1 msgcat-properties-2 \
	msgcat-stringtable-1 \
	msgcmp-1 msgcmp-2 msgcmp-3 msgcmp-4 \
	msgcomm-1 msgcomm-2 msgcomm-3 msgcomm-4 msgcomm-5 msgcomm-6 msgcomm-7 \
	msgcomm-8 msgcomm-9 msgcomm-10 msgcomm-11 msgcomm-12 msgcomm-13 \
	msgcomm-14 msgcomm-15 msgcomm-16 msgcomm-17 msgcomm-18 msgcomm-19 \
	msgcomm-20 msgcomm-21 msgcomm-22 msgcomm-23 msgcomm-24 msgcomm-25 \
	msgcomm-26 msgcomm-27 msgcomm-28 \
	msgconv-1 msgconv-2 msgconv-3 msgconv-4 msgconv-5 msgconv-6 msgconv-7 \
	msgen-1 msgen-2 msgen-3 msgen-4 \
	msgexec-1 msgexec-2 msgexec-3 msgexec-4 msgexec-5 msgexec-6 \
	msgfilter-1 msgfilter-2 msgfilter-3 msgfilter-4 msgfilter-5 \
	msgfilter-6 msgfilter-7 msgfilter-8 \
	msgfilter-sr-latin-1 msgfilter-quote-1 \
	msgfmt-1 msgfmt-2 msgfmt-3 msgfmt-4 msgfmt-5 msgfmt-6 msgfmt-7 \
	msgfmt-8 msgfmt-9 msgfmt-10 msgfmt-11 msgfmt-12 msgfmt-13 msgfmt-14 \
	msgfmt-15 msgfmt-16 msgfmt-17 msgfmt-18 \
	msgfmt-properties-1 \
	msgfmt-qt-1 msgfmt-qt-2 \
	msgfmt-desktop-1 msgfmt-desktop-2 \
	msgfmt-xml-1 msgfmt-xml-2 \
	msggrep-1 msggrep-2 msggrep-3 msggrep-4 msggrep-5 msggrep-6 msggrep-7 \
	msggrep-8 msggrep-9 msggrep-10 msggrep-11 \
	msginit-1 msginit-2 msginit-3 msginit-4 \
	msgmerge-1 msgmerge-2 msgmerge-3 msgmerge-4 msgmerge-5 msgmerge-6 \
	msgmerge-7 msgmerge-8 msgmerge-9 msgmerge-10 msgmerge-11 msgmerge-12 \
	msgmerge-13 msgmerge-14 msgmerge-15 msgmerge-16 msgmerge-17 \
	msgmerge-18 msgmerge-19 msgmerge-20 msgmerge-21 msgmerge-22 \
	msgmerge-23 msgmerge-24 msgmerge-25 \
	msgmerge-compendium-1 msgmerge-compendium-2 msgmerge-compendium-3 \
	msgmerge-compendium-4 msgmerge-compendium-5 msgmerge-compendium-6 \
	msgmerge-properties-1 msgmerge-properties-2 \
	msgmerge-update-1 msgmerge-update-2 msgmerge-update-3 \
	msgmerge-update-4 \
	msgunfmt-1 msgunfmt-2 msgunfmt-3 \
	msgunfmt-csharp-1 \
	msgunfmt-java-1 \
	msgunfmt-properties-1 \
	msgunfmt-tcl-1 \
	msguniq-1 msguniq-2 msguniq-3 msguniq-4 msguniq-5 msguniq-6 msguniq-7 \
	recode-sr-latin-1 recode-sr-latin-2 \
	xgettext-2 xgettext-3 xgettext-4 xgettext-5 xgettext-6 \
	xgettext-7 xgettext-8 xgettext-9 xgettext-10 xgettext-11 xgettext-12 \
	xgettext-13 xgettext-14 \
	xgettext-appdata-1 \
	xgettext-awk-1 xgettext-awk-2 \
	xgettext-c-2 xgettext-c-3 xgettext-c-4 xgettext-c-5 \
	xgettext-c-6 xgettext-c-7 xgettext-c-8 xgettext-c-9 xgettext-c-10 \
	xgettext-c-11 xgettext-c-12 xgettext-c-13 xgettext-c-14 xgettext-c-15 \
	xgettext-c-16 xgettext-c-17 xgettext-c-18 xgettext-c-19 xgettext-c-20 \
	xgettext-c-21 \
	xgettext-csharp-1 xgettext-csharp-2 xgettext-csharp-3 \
	xgettext-csharp-4 xgettext-csharp-5 xgettext-csharp-6 \
	xgettext-csharp-7 xgettext-csharp-8 \
	xgettext-elisp-1 xgettext-elisp-2 \
	xgettext-glade-1 xgettext-glade-2 xgettext-glade-3 xgettext-glade-4 \
	xgettext-glade-5 xgettext-glade-6 xgettext-glade-7 \
	xgettext-java-1 xgettext-java-2 xgettext-java-3 xgettext-java-4 \
	xgettext-java-5 xgettext-java-6 xgettext-java-7 \
	xgettext-librep-1 xgettext-librep-2 \
	xgettext-lisp-1 xgettext-lisp-2 \
	xgettext-objc-1 xgettext-objc-2 \
	xgettext-perl-1 xgettext-perl-2 xgettext-perl-3 xgettext-perl-4 \
	xgettext-perl-5 xgettext-perl-6 xgettext-perl-7 xgettext-perl-8 \
	xgettext-php-1 xgettext-php-2 xgettext-php-3 xgettext-php-4 \
	xgettext-po-1 \
	xgettext-properties-1 \
	xgettext-python-1 xgettext-python-2 xgettext-python-3 \
	xgettext-python-4 \
	xgettext-scheme-1 xgettext-scheme-2 xgettext-scheme-3 \
	xgettext-scheme-4 \
	xgettext-sh-1 xgettext-sh-2 xgettext-sh-3 xgettext-sh-4 xgettext-sh-5 \
	xgettext-sh-6 \
	xgettext-smalltalk-1 xgettext-smalltalk-2 \
	xgettext-stringtable-1 \
	xgettext-tcl-1 xgettext-tcl-2 xgettext-tcl-3 xgettext-tcl-4 \
	xgettext-ycp-1 xgettext-ycp-2 xgettext-ycp-3 xgettext-ycp-4 \
	xgettext-lua-1 xgettext-lua-2 \
	xgettext-javascript-1 xgettext-javascript-2 xgettext-javascript-3 \
	xgettext-javascript-4 xgettext-javascript-5 xgettext-javascript-6 \
	xgettext-vala-1 \
	xgettext-gsettings-1 \
	xgettext-desktop-1 \
	xgettext-its-1 xgettext-its-2 \
	format-awk-1 format-awk-2 \
	format-boost-1 format-boost-2 \
	format-c-1 format-c-2 format-c-3 format-c-4 format-c-5 \
	format-csharp-1 format-csharp-2 \
	format-elisp-1 format-elisp-2 \
	format-gcc-internal-1 format-gcc-internal-2 \
	format-gfc-internal-1 format-gfc-internal-2 \
	format-java-1 format-java-2 \
	format-kde-1 format-kde-2 \
	format-kde-kuit-1 format-kde-kuit-2 \
	format-librep-1 format-librep-2 \
	format-lisp-1 format-lisp-2 \
	format-php-1 format-php-2 \
	format-python-1 format-python-2 \
	format-python-brace-1 format-python-brace-2 \
	format-pascal-1 format-pascal-2 \
	format-perl-1 format-perl-2 \
	format-perl-brace-1 format-perl-brace-2 \
	format-perl-mixed-1 format-perl-mixed-2 \
	format-qt-1 format-qt-2 \
	format-qt-plural-1 format-qt-plural-2 \
	format-scheme-1 format-scheme-2 \
	format-sh-1 format-sh-2 \
	format-tcl-1 format-tcl-2 \
	format-ycp-1 format-ycp-2 \
	format-lua-1 format-lua-2 \
	format-javascript-1 format-javascript-2 \
	plural-1 plural-2 \
	gettextpo-1 sentence-1 \
	lang-c lang-c++ lang-objc lang-sh lang-bash lang-python-1 \
	lang-python-2 lang-clisp lang-elisp lang-librep lang-guile \
	lang-smalltalk lang-java lang-csharp lang-gawk lang-pascal \
	lang-ycp lang-tcl lang-perl-1 lang-perl-2 lang-php lang-po lang-rst \
	lang-lua lang-javascript lang-vala \
	autopoint-1 autopoint-2 autopoint-3 cldr-plurals-1

EXTRA_DIST += init.sh init.cfg $(TESTS) \
	test.mo overflow-1.mo overflow-2.mo overflow-3.mo overflow-4.mo \
	overflow-5.mo overflow-6.mo xg-c-1.ok.po mex-test2.ok \
	mm-ko.ascii.pot mm-ko.euc-kr.po mm-ko-comp.euc-kr.po \
	mm-viet.comp.po mm-viet.pot mm-viet.out \
	msguniq-a.in msguniq-a.inp msguniq-a.out \
	qttest_pl.po qttest_pl.qm \
	qttest2_de.po qttest2_de.qm qttest2_de.ts \
	gettext-3-1.po gettext-3-2.po gettext-4.po gettext-5.po \
	gettext-6-1.po gettext-6-2.po gettext-7.po \
	gettextpo-1.de.po \
	xgettext-1 \
	xgettext-c-1 xg-c-21.c \
	common/supplemental/plurals.xml

XGETTEXT = ../src/xgettext

# For debugging memory leaks and memory allocation bugs.
# You should build with --disable-shared when using valgrind.
CHECKER =
#CHECKER = valgrind --tool=memcheck --suppressions=$(abs_srcdir)/../gnulib-lib/malloca.valgrind --suppressions=$(abs_srcdir)/../gnulib-lib/libunistring.valgrind --num-callers=20 --leak-check=yes --leak-resolution=high --show-reachable=yes
#CHECKER = valgrind --tool=massif --format=html --depth=10 --alloc-fn=xmalloc --alloc-fn=xrealloc --stacks=no

TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) \
		    abs_srcdir=$(abs_srcdir) \
		    abs_top_srcdir=$(abs_top_srcdir) \
		    GETTEXT="$(CHECKER) tstgettext" \
		    NGETTEXT="$(CHECKER) tstngettext" \
		    XGETTEXT="$(CHECKER) xgettext" \
		    MSGATTRIB="$(CHECKER) msgattrib" \
		    MSGCAT="$(CHECKER) msgcat" \
		    MSGCMP="$(CHECKER) msgcmp" \
		    MSGCOMM="$(CHECKER) msgcomm" \
		    MSGCONV="$(CHECKER) msgconv" \
		    MSGEN="$(CHECKER) msgen" \
		    MSGEXEC="$(CHECKER) msgexec" \
		    MSGFILTER="$(CHECKER) msgfilter" \
		    MSGFMT="$(CHECKER) msgfmt" \
		    MSGGREP="$(CHECKER) msggrep" \
		    MSGINIT="$(CHECKER) msginit" \
		    MSGMERGE="$(CHECKER) msgmerge" \
		    MSGUNFMT="$(CHECKER) msgunfmt" \
		    MSGUNIQ="$(CHECKER) msguniq" \
		    RECODE="$(CHECKER) recode" \
		    CONFIG_SHELL='$(SHELL)'

LOG_COMPILER = $(SHELL)

# Update the expected test results.
update-expected: xg-c-1.ok.po
xg-c-1.ok.po: $(top_srcdir)/src/xgettext.c $(top_srcdir)/src/msgfmt.c
	$(XGETTEXT) -d xg-c-1.ok -p $(srcdir) \
	  -k_ --omit-header --no-location \
	  $(top_srcdir)/src/xgettext.c $(top_srcdir)/src/msgfmt.c

# Three auxiliary programs used by the tests.
AM_CPPFLAGS = \
  -I.. \
  -I../gnulib-lib -I$(top_srcdir)/gnulib-lib \
  -I../intl
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
# INTL_MACOSX_LIBS is needed because the programs depend on libintl.la
# but libtool doesn't put -Wl,-framework options into .la files.
LDADD = $(LDADD_@USE_INCLUDED_LIBINTL@) @INTL_MACOSX_LIBS@
LDADD_yes = ../intl/libintl.la @LTLIBTHREAD@
LDADD_no = ../intl/libgnuintl.la @LTLIBTHREAD@ @LTLIBINTL@
check_PROGRAMS = tstgettext tstngettext testlocale gettext-3-prg gettext-4-prg gettext-5-prg gettext-6-prg gettext-7-prg gettext-8-prg cake fc3 fc4 fc5 gettextpo-1-prg sentence-1-prg
tstgettext_SOURCES = tstgettext.c setlocale.c
tstgettext_CFLAGS = -DINSTALLDIR=\".\"
tstgettext_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
tstngettext_SOURCES = tstngettext.c setlocale.c
tstngettext_CFLAGS = -DINSTALLDIR=\".\"
tstngettext_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
testlocale_SOURCES = testlocale.c
gettext_3_prg_SOURCES = gettext-3-prg.c
gettext_3_prg_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
gettext_4_prg_SOURCES = gettext-4-prg.c
gettext_4_prg_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
gettext_5_prg_SOURCES = gettext-5-prg.c
gettext_5_prg_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
gettext_6_prg_SOURCES = gettext-6-prg.c
gettext_6_prg_CFLAGS = $(gettext_6_prg_CFLAGS_@HAVE_NEWLOCALE@)
gettext_6_prg_CFLAGS_1 = -DUSE_POSIX_THREADS
gettext_6_prg_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD) $(gettext_6_prg_LDADD_@HAVE_NEWLOCALE@)
gettext_6_prg_LDADD_1 = -lpthread
gettext_7_prg_SOURCES = gettext-7-prg.c
gettext_7_prg_CFLAGS = $(gettext_7_prg_CFLAGS_@HAVE_NEWLOCALE@)
gettext_7_prg_CFLAGS_1 = -DUSE_POSIX_THREADS
gettext_7_prg_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD) $(gettext_7_prg_LDADD_@HAVE_NEWLOCALE@)
gettext_7_prg_LDADD_1 = -lpthread
gettext_8_prg_SOURCES = gettext-8-prg.c
gettext_8_prg_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
cake_SOURCES = plural-1-prg.c setlocale.c
cake_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
fc3_SOURCES = format-c-3-prg.c setlocale.c
fc3_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
fc4_SOURCES = format-c-4-prg.c setlocale.c
fc4_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
fc5_SOURCES = format-c-5-prg.c
fc5_LDADD = ../gnulib-lib/libgettextlib.la $(LDADD)
gettextpo_1_prg_SOURCES = gettextpo-1-prg.c
# Don't add 'gnulib-lib' to the include path here.  Some header files
# there may interfere with the ones in 'libgettextpo'.
gettextpo_1_prg_CPPFLAGS = \
  -I.. \
  -I../libgettextpo -I$(top_srcdir)/libgettextpo \
  -I../intl
# Don't add more libraries here. This test must check whether libgettextpo is
# self contained.
gettextpo_1_prg_LDADD = ../libgettextpo/libgettextpo.la $(LDADD)
sentence_1_prg_SOURCES = sentence-1-prg.c
sentence_1_prg_CPPFLAGS = $(AM_CPPFLAGS) -I../src -I$(top_srcdir)/src
sentence_1_prg_LDADD = ../src/libgettextsrc.la $(LDADD)

# Clean up after Solaris cc.
clean-local:
	rm -rf SunWS_cache
