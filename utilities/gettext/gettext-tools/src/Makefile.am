## Makefile for the gettext-tools/src subdirectory of GNU gettext
## Copyright (C) 1995-1998, 2000-2011, 2015-2016 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.5 gnits no-dependencies subdir-objects
EXTRA_DIST =
MOSTLYCLEANFILES = core *.stackdump
CLEANFILES =
DISTCLEANFILES =

RM = rm -f

bin_PROGRAMS = \
msgcmp msgfmt msgmerge msgunfmt xgettext \
msgattrib msgcat msgcomm msgconv msgen msgexec msgfilter msggrep msginit msguniq \
recode-sr-latin

noinst_PROGRAMS = hostname urlget cldr-plurals

lib_LTLIBRARIES = libgettextsrc.la

noinst_HEADERS = pos.h message.h po-error.h po-xerror.h po-gram.h po-charset.h \
po-lex.h open-catalog.h read-catalog-abstract.h read-catalog.h \
read-po.h read-properties.h read-stringtable.h \
str-list.h \
color.h write-catalog.h write-po.h write-properties.h write-stringtable.h \
dir-list.h file-list.h po-gram-gen.h po-gram-gen2.h cldr-plural.h \
cldr-plural-exp.h locating-rule.h its.h search-path.h \
msgl-charset.h msgl-equal.h msgl-iconv.h msgl-ascii.h msgl-cat.h msgl-header.h \
msgl-english.h msgl-check.h msgl-fsearch.h msgfmt.h msgunfmt.h \
plural-count.h plural-eval.h plural-distrib.h \
read-mo.h write-mo.h \
read-java.h write-java.h \
read-csharp.h write-csharp.h \
read-resources.h write-resources.h \
read-tcl.h write-tcl.h \
write-qt.h \
read-desktop.h write-desktop.h \
write-xml.h \
po-time.h plural-table.h lang-table.h format.h filters.h \
xgettext.h x-c.h x-po.h x-sh.h x-python.h x-lisp.h x-elisp.h x-librep.h \
x-scheme.h x-smalltalk.h x-java.h x-properties.h x-csharp.h x-awk.h x-ycp.h \
x-tcl.h x-perl.h x-php.h x-stringtable.h x-rst.h x-glade.h x-lua.h \
x-javascript.h x-vala.h x-gsettings.h x-desktop.h x-appdata.h

EXTRA_DIST += FILES project-id

aliaspath = $(localedir)
jardir = $(datadir)/gettext
pkgdatadir = $(datadir)/gettext
projectsdir = $(pkgdatadir)/projects
pkglibdir = $(libdir)/gettext

AM_CPPFLAGS = \
  -I. -I$(srcdir) \
  -I.. -I$(top_srcdir) \
  -I$(top_srcdir)/libgrep \
  -I../gnulib-lib -I$(top_srcdir)/gnulib-lib \
  -I../intl -I$(top_srcdir)/../gettext-runtime/intl
DEFS = \
  -DLOCALEDIR=\"$(localedir)\" -DBISON_LOCALEDIR=\"$(BISON_LOCALEDIR)\" \
  -DLOCALE_ALIAS_PATH=\"$(aliaspath)\" \
  -DUSEJAVA=$(USEJAVA) \
  -DUSEJEXE=$(USEJEXE) \
  -DGETTEXTJEXEDIR=\"$(pkglibdir)\" \
  -DGETTEXTJAR=\"$(jardir)/gettext.jar\" \
  -DLIBDIR=\"$(libdir)\" \
  -DGETTEXTDATADIR=\"$(pkgdatadir)\" \
  -DPROJECTSDIR=\"$(projectsdir)\" @DEFS@
# Ensure that <stdint.h> defines SIZE_MAX in C++ mode, like it does in C mode.
AM_CXXFLAGS = -D__STDC_LIMIT_MACROS

LDADD = ../gnulib-lib/libgettextlib.la $(LTLIBUNISTRING) @LTLIBINTL@ $(WOE32_LDADD)
OTHERPROGDEPENDENCIES = ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)

SED = sed
YACC = @YACC@ -d
GCJ = @GCJ@
GCJFLAGS = @GCJFLAGS@
JAR = @JAR@
JAVACOMP = $(SHELL) ../javacomp.sh
CSHARPCOMP = $(SHELL) ../csharpcomp.sh
CSHARPCOMPFLAGS = @CSHARPCOMPFLAGS@


# All programs deal with message lists.
# All programs must read PO files. (msgunfmt also, for read-java.c,
# read-csharp.c and read-resources.c.)
# message.c -> str-list.c.
# (read-catalog-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> str-list.c.
# (read-catalog-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> open-catalog.c -> dir-list.c -> str-list.c.
# (read-catalog-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> po-charset.c.
# (read-catalog-abstract.c <--> po-gram-gen.y <--> po-lex.c) -> message.c -> str-list.c.
COMMON_SOURCE = message.c po-error.c po-xerror.c \
read-catalog-abstract.c po-lex.c po-gram-gen.y po-charset.c \
read-po.c read-properties.c read-stringtable.c open-catalog.c \
dir-list.c str-list.c

# xgettext and msgfmt deal with format strings.
if !WOE32DLL
FORMAT_SOURCE = format.c
else
FORMAT_SOURCE = ../woe32dll/c++format.cc
endif
FORMAT_SOURCE += \
  format-invalid.h \
  format-c.c format-c-parse.h \
  format-sh.c \
  format-python.c \
  format-python-brace.c \
  format-lisp.c \
  format-elisp.c \
  format-librep.c \
  format-scheme.c \
  format-java.c \
  format-csharp.c \
  format-awk.c \
  format-pascal.c \
  format-ycp.c \
  format-tcl.c \
  format-perl.c \
  format-perl-brace.c \
  format-php.c \
  format-gcc-internal.c \
  format-gfc-internal.c \
  format-qt.c \
  format-qt-plural.c \
  format-kde.c \
  format-kde-kuit.c \
  format-boost.c \
  format-lua.c \
  format-javascript.c

# libgettextsrc contains all code that is needed by at least two programs.
libgettextsrc_la_SOURCES = \
$(COMMON_SOURCE) read-catalog.c \
color.c write-catalog.c write-properties.c write-stringtable.c write-po.c \
msgl-ascii.c msgl-iconv.c msgl-equal.c msgl-cat.c msgl-header.c msgl-english.c \
msgl-check.c file-list.c msgl-charset.c po-time.c plural-exp.c plural-eval.c \
plural-table.c quote.h sentence.h sentence.c \
$(FORMAT_SOURCE) \
read-desktop.c locating-rule.c its.c search-path.c

# msggrep needs pattern matching.
LIBGREP = ../libgrep/libgrep.a

# Source dependencies.
msgcmp_SOURCES = msgcmp.c
msgcmp_SOURCES += msgl-fsearch.c
msgfmt_SOURCES = msgfmt.c
msgfmt_SOURCES += \
  write-mo.c write-java.c write-csharp.c write-resources.c write-tcl.c \
  write-qt.c write-desktop.c write-xml.c \
  ../../gettext-runtime/intl/hash-string.c
if !WOE32DLL
msgmerge_SOURCES = msgmerge.c
else
msgmerge_SOURCES = ../woe32dll/c++msgmerge.cc
endif
msgmerge_SOURCES += msgl-fsearch.c lang-table.c plural-count.c
msgunfmt_SOURCES = msgunfmt.c
msgunfmt_SOURCES += \
  read-mo.c read-java.c read-csharp.c read-resources.c read-tcl.c
if !WOE32DLL
xgettext_SOURCES = xgettext.c
else
xgettext_SOURCES = ../woe32dll/c++xgettext.cc
endif
xgettext_SOURCES += \
  x-c.c x-po.c x-sh.c x-python.c x-lisp.c x-elisp.c x-librep.c x-scheme.c \
  x-smalltalk.c x-java.c x-csharp.c x-awk.c x-ycp.c x-tcl.c x-perl.c x-php.c \
  x-rst.c x-lua.c x-javascript.c x-vala.c \
  x-desktop.c
if !WOE32DLL
msgattrib_SOURCES = msgattrib.c
else
msgattrib_SOURCES = ../woe32dll/c++msgattrib.cc
endif
if !WOE32DLL
msgcat_SOURCES = msgcat.c
else
msgcat_SOURCES = ../woe32dll/c++msgcat.cc
endif
if !WOE32DLL
msgcomm_SOURCES = msgcomm.c
else
msgcomm_SOURCES = ../woe32dll/c++msgcomm.cc
endif
if !WOE32DLL
msgconv_SOURCES = msgconv.c
else
msgconv_SOURCES = ../woe32dll/c++msgconv.cc
endif
if !WOE32DLL
msgen_SOURCES = msgen.c
else
msgen_SOURCES = ../woe32dll/c++msgen.cc
endif
msgexec_SOURCES = msgexec.c
if !WOE32DLL
msgfilter_SOURCES = msgfilter.c
else
msgfilter_SOURCES = ../woe32dll/c++msgfilter.cc
endif
msgfilter_SOURCES += filter-sr-latin.c
msgfilter_SOURCES += filter-quote.c
if !WOE32DLL
msggrep_SOURCES = msggrep.c
else
msggrep_SOURCES = ../woe32dll/c++msggrep.cc
endif
msginit_SOURCES = msginit.c
msginit_SOURCES += lang-table.c plural-count.c
msginit_SOURCES += ../../gettext-runtime/intl/localealias.c
# This is needed because on Solaris, localealias.c requires the symbol
# libintl_thread_in_use which is defined in lock.c. The copy of lock.c inside
# libintl.so is not sufficient, because libintl.so doesn't export the symbol
# libintl_thread_in_use.
msginit_SOURCES += ../../gettext-runtime/intl/lock.c
if !WOE32DLL
msguniq_SOURCES = msguniq.c
else
msguniq_SOURCES = ../woe32dll/c++msguniq.cc
endif
recode_sr_latin_SOURCES = recode-sr-latin.c filter-sr-latin.c
hostname_SOURCES = hostname.c
urlget_SOURCES = urlget.c
cldr_plurals_SOURCES = cldr-plural.y cldr-plural-exp.c cldr-plurals.c
cldr_plurals_CFLAGS = $(AM_CFLAGS) $(INCXML)
cldr_plurals_LDADD = libgettextsrc.la $(LDADD)

# How to build libgettextsrc.la.
# Need ../gnulib-lib/libgettextlib.la.
# Need $(LTLIBUNISTRING) because ulc_width_linebreaks, uc_width, etc. may be
# taken from libunistring, when the configure option --with-libunistring-prefix
# was given.
# Need @LTLIBINTL@ because many source files use gettext().
# Need @LTLIBICONV@ because po-charset.c, po-lex.c, msgl-iconv.c, write-po.c
# use iconv().
libgettextsrc_la_LDFLAGS = \
  -release @VERSION@ \
  ../gnulib-lib/libgettextlib.la $(LTLIBUNISTRING) @LTLIBINTL@ @LTLIBICONV@ -lc -no-undefined

# OS/2 does not support a DLL name longer than 8 characters.
if OS2
libgettextsrc_la_LDFLAGS += -os2dllname gtsrc
endif

libgettextsrc_la_CPPFLAGS = $(AM_CPPFLAGS) $(INCXML)

# Tell the mingw or Cygwin linker which symbols to export.
if WOE32DLL
libgettextsrc_la_SOURCES += ../woe32dll/gettextsrc-exports.c
libgettextsrc_la_LDFLAGS += -Wl,--export-all-symbols
libgettextsrc_la_CPPFLAGS += $(GETTEXTLIB_EXPORTS_FLAGS)
endif

# No need to install libgettextsrc.a, except on AIX.
install-exec-hook: install-exec-clean
install-exec-clean:
	case "@host_os@" in \
	  aix*) ;; \
	  *) $(RM) $(DESTDIR)$(libdir)/libgettextsrc.a ;; \
	esac

# Compile-time flags for particular source files.
msgmerge_CFLAGS = $(AM_CFLAGS) $(OPENMP_CFLAGS)
msgmerge_CXXFLAGS = $(AM_CXXFLAGS) $(OPENMP_CFLAGS)

# Link dependencies.
# INTL_MACOSX_LIBS is needed because the programs depend on libintl.la
# but libtool doesn't put -Wl,-framework options into .la files.
# For msginit, it is also needed because of localename.c.
msgcmp_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ @MSGMERGE_LIBM@ $(WOE32_LDADD)
msgfmt_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgmerge_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ @MSGMERGE_LIBM@ $(WOE32_LDADD) $(OPENMP_CFLAGS)
msgunfmt_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
xgettext_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ @LTLIBICONV@ $(WOE32_LDADD)
msgattrib_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgcat_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgcomm_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgconv_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgen_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgexec_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgfilter_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msggrep_LDADD = $(LIBGREP) libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msginit_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msguniq_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)

# Specify when to relink the programs.
msgcmp_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgfmt_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgmerge_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgunfmt_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
xgettext_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgattrib_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgcat_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgcomm_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgconv_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgen_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgexec_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgfilter_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msggrep_DEPENDENCIES = $(LIBGREP) libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msginit_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msguniq_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
recode_sr_latin_DEPENDENCIES = $(OTHERPROGDEPENDENCIES)
hostname_DEPENDENCIES = $(OTHERPROGDEPENDENCIES)
urlget_DEPENDENCIES = $(OTHERPROGDEPENDENCIES)

# Specify installation directory, for --enable-relocatable.
msgcmp_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgfmt_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgmerge_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgunfmt_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
xgettext_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgattrib_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgcat_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgcomm_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgconv_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgen_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgexec_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msgfilter_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msggrep_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msginit_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
msguniq_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
recode_sr_latin_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(bindir)\"
hostname_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(pkglibdir)\"
urlget_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(pkglibdir)\"
cldr_plurals_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=\"$(pkglibdir)\"
if RELOCATABLE_VIA_LD
msgcmp_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgfmt_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgmerge_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgunfmt_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
xgettext_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgattrib_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgcat_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgcomm_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgconv_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgen_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgexec_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msgfilter_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msggrep_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msginit_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msguniq_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
recode_sr_latin_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
hostname_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibdir)`
urlget_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibdir)`
cldr_plurals_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibdir)`
endif

# Linking with C++ libraries is needed _only_ on mingw and Cygwin.
if !WOE32DLL
libgettextsrc_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(libgettextsrc_la_LDFLAGS) $(LDFLAGS) \
	-o $@
msgattrib_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msgattrib_LDFLAGS) $(LDFLAGS) \
	-o $@
msgcat_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msgcat_LDFLAGS) $(LDFLAGS) \
	-o $@
msgcomm_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msgcomm_LDFLAGS) $(LDFLAGS) \
	-o $@
msgconv_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msgconv_LDFLAGS) $(LDFLAGS) \
	-o $@
msgen_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msgen_LDFLAGS) $(LDFLAGS) \
	-o $@
msgfilter_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msgfilter_LDFLAGS) $(LDFLAGS) \
	-o $@
msggrep_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msggrep_LDFLAGS) $(LDFLAGS) \
	-o $@
msgmerge_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(msgmerge_CFLAGS) $(CFLAGS) $(msgmerge_LDFLAGS) $(LDFLAGS) \
	-o $@
msguniq_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(msguniq_LDFLAGS) $(LDFLAGS) \
	-o $@
xgettext_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(xgettext_LDFLAGS) $(LDFLAGS) \
	-o $@
else
libgettextsrc_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(libgettextsrc_la_LDFLAGS) $(LDFLAGS) \
	-o $@
msgattrib_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msgattrib_LDFLAGS) $(LDFLAGS) \
	-o $@
msgcat_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msgcat_LDFLAGS) $(LDFLAGS) \
	-o $@
msgcomm_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msgcomm_LDFLAGS) $(LDFLAGS) \
	-o $@
msgconv_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msgconv_LDFLAGS) $(LDFLAGS) \
	-o $@
msgen_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msgen_LDFLAGS) $(LDFLAGS) \
	-o $@
msgfilter_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msgfilter_LDFLAGS) $(LDFLAGS) \
	-o $@
msggrep_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msggrep_LDFLAGS) $(LDFLAGS) \
	-o $@
msgmerge_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(msgmerge_CXXFLAGS) $(CXXFLAGS) $(msgmerge_LDFLAGS) $(LDFLAGS) \
	-o $@
msguniq_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(msguniq_LDFLAGS) $(LDFLAGS) \
	-o $@
xgettext_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(xgettext_LDFLAGS) $(LDFLAGS) \
	-o $@
endif

# Special rules for bison and flex generated files.

BUILT_SOURCES = \
  po-gram-gen.c po-gram-gen.h po-gram-gen2.h cldr-plural.c cldr-plural.h

po-lex.o po-lex.lo: po-gram-gen2.h
po-gram-gen2.h: po-gram-gen.h
	srcdir=''; \
	test -f ./po-gram-gen.h || srcdir=$(srcdir)/; \
	$(SED) -e 's/yy/po_gram_/g' -e 's/extern /extern DLL_VARIABLE /' \
	  $${srcdir}po-gram-gen.h > $@-tmp && \
	mv $@-tmp $@
po-gram-gen.h: po-gram-gen.c


# Special rules for installation of auxiliary programs.

install-exec-local:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) hostname$(EXEEXT) $(DESTDIR)$(pkglibdir)/hostname$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) urlget$(EXEEXT) $(DESTDIR)$(pkglibdir)/urlget$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) cldr-plurals$(EXEEXT) $(DESTDIR)$(pkglibdir)/cldr-plurals$(EXEEXT)
	$(INSTALL_SCRIPT) user-email $(DESTDIR)$(pkglibdir)/user-email
	$(INSTALL_SCRIPT) $(srcdir)/project-id $(DESTDIR)$(pkglibdir)/project-id

installdirs-local:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)

uninstall-local:
	$(RM) $(DESTDIR)$(pkglibdir)/hostname$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/urlget$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/cldr-plurals$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/user-email
	$(RM) $(DESTDIR)$(pkglibdir)/project-id

DISTCLEANFILES += user-email


# Special rules for Java compilation.

USEJAVA = $(USEJAVA_@BUILDJAVAEXE@)
USEJAVA_yes = 1
USEJAVA_no = $(USEJAVA_no_@BUILDJAVA@)
USEJAVA_no_yes = 1
USEJAVA_no_no = 0

USEJEXE = $(USEJEXE_@BUILDJAVAEXE@)
USEJEXE_yes = 1
USEJEXE_no = 0

all-local: all-java-@BUILDJAVAEXE@
all-java-yes: gnu.gettext.DumpResource$(EXEEXT) gnu.gettext.GetURL$(EXEEXT)
all-java-no: all-java-no-@BUILDJAVA@
all-java-no-yes: gettext.jar
all-java-no-no:

gnu.gettext.DumpResource$(EXEEXT): $(srcdir)/gnu/gettext/DumpResource.java
	$(GCJ) $(GCJFLAGS) $(srcdir)/gnu/gettext/DumpResource.java --main=gnu.gettext.DumpResource -o $@

gnu.gettext.GetURL$(EXEEXT): $(srcdir)/gnu/gettext/GetURL.java
	$(GCJ) $(GCJFLAGS) $(srcdir)/gnu/gettext/GetURL.java --main=gnu.gettext.GetURL -o $@

gnu/gettext/DumpResource.class: $(srcdir)/gnu/gettext/DumpResource.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/DumpResource.java

gnu/gettext/GetURL.class: $(srcdir)/gnu/gettext/GetURL.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/GetURL.java

gettext.jar: gnu/gettext/DumpResource.class gnu/gettext/GetURL.class
	$(JAR) cf $@ gnu/gettext/DumpResource*.class gnu/gettext/GetURL*.class

EXTRA_DIST += gnu/gettext/DumpResource.java gnu/gettext/GetURL.java

CLEANFILES += gnu.gettext.DumpResource$(EXEEXT) gnu.gettext.GetURL$(EXEEXT) \
              gettext.jar gnu/gettext/*.class

install-exec-local: install-exec-java-@BUILDJAVAEXE@
install-exec-java-yes: all-java-yes
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)
	$(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) gnu.gettext.DumpResource$(EXEEXT) $(DESTDIR)$(pkglibdir)/gnu.gettext.DumpResource$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) gnu.gettext.GetURL$(EXEEXT) $(DESTDIR)$(pkglibdir)/gnu.gettext.GetURL$(EXEEXT)
install-exec-java-no:

install-data-local: install-data-java-@BUILDJAVAEXE@
install-data-java-yes:
install-data-java-no: install-data-java-no-@BUILDJAVA@
install-data-java-no-yes: all-java-no-yes
	$(INSTALL_DATA) gettext.jar $(DESTDIR)$(jardir)/gettext.jar
install-data-java-no-no:

installdirs-local: installdirs-java-@BUILDJAVAEXE@
installdirs-java-yes:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)
installdirs-java-no: installdirs-java-no-@BUILDJAVA@
installdirs-java-no-yes:
	$(MKDIR_P) $(DESTDIR)$(jardir)
installdirs-java-no-no:

uninstall-local: uninstall-java-@BUILDJAVAEXE@
uninstall-java-yes:
	$(RM) $(DESTDIR)$(pkglibdir)/gnu.gettext.DumpResource$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibdir)/gnu.gettext.GetURL$(EXEEXT)
uninstall-java-no: uninstall-java-no-@BUILDJAVA@
uninstall-java-no-yes:
	$(RM) $(DESTDIR)$(jardir)/gettext.jar
uninstall-java-no-no:


# Special rules for C# auxiliary programs.

EXTRA_DIST += msgfmt.cs msgunfmt.cs

CLEANFILES += \
  msgfmt.net.exe msgfmt.net.exe.mdb \
  msgunfmt.net.exe msgunfmt.net.exe.mdb

all-local: all-csharp-@BUILDCSHARP@
all-csharp-yes: msgfmt.net.exe msgunfmt.net.exe
all-csharp-no:

msgfmt.net.exe: msgfmt.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ $(srcdir)/msgfmt.cs

msgunfmt.net.exe: msgunfmt.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ -L ../../gettext-runtime/intl-csharp -l GNU.Gettext $(srcdir)/msgunfmt.cs

install-exec-local: install-exec-csharp-@BUILDCSHARP@
install-exec-csharp-yes: all-csharp-yes
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)
	$(INSTALL_DATA) msgfmt.net.exe $(DESTDIR)$(pkglibdir)/msgfmt.net.exe
	$(INSTALL_DATA) msgunfmt.net.exe $(DESTDIR)$(pkglibdir)/msgunfmt.net.exe
install-exec-csharp-no:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)

installdirs-local: install-csharp
installdirs-csharp:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)

uninstall-local: uninstall-csharp-@BUILDCSHARP@
uninstall-csharp-yes: all-csharp-yes
	$(RM) $(DESTDIR)$(pkglibdir)/msgfmt.net.exe
	$(RM) $(DESTDIR)$(pkglibdir)/msgunfmt.net.exe
uninstall-csharp-no:


# Special rules for Tcl auxiliary program.

EXTRA_DIST += msgunfmt.tcl

install-data-local: install-tcl
install-tcl:
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(srcdir)/msgunfmt.tcl $(DESTDIR)$(pkgdatadir)/msgunfmt.tcl

installdirs-local: installdirs-tcl
installdirs-tcl:
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)

uninstall-local: uninstall-tcl
uninstall-tcl:
	$(RM) $(DESTDIR)$(pkgdatadir)/msgunfmt.tcl


# Support for relocatability.
RELOCATABLE_LIBRARY_PATH = $(libdir)
RELOCATABLE_SRC_DIR = $(top_srcdir)/gnulib-lib
RELOCATABLE_BUILD_DIR = ../gnulib-lib
RELOCATABLE_CONFIG_H_DIR = ..
RELOCATABLE_STRIP = :

# Version information according to Woe32 conventions.
if WOE32
WOE32_LDADD = gettext.res
gettext.res : $(top_srcdir)/../windows/gettext.rc
	$(WINDRES) `$(SHELL) $(top_srcdir)/../windows/windres-options --escape $(VERSION)` -i $(top_srcdir)/../windows/gettext.rc -o gettext.res --output-format=coff
MOSTLYCLEANFILES += gettext.res
else
WOE32_LDADD =
endif
