## Makefile for the gettext-tools/src subdirectory of GNU gettext
## Copyright (C) 1995-2026 Free Software Foundation, Inc.
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
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnits subdir-objects
EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES = core *.stackdump
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

RM = rm -f

bin_PROGRAMS = \
  msgcmp msgfmt msgmerge msgunfmt xgettext \
  msgattrib msgcat msgcomm msgconv msgen msgexec msgfilter msggrep msginit \
  msgpre msguniq \
  recode-sr-latin
if BUILD_SPIT_IN_C
bin_PROGRAMS += spit
endif

noinst_PROGRAMS = hostname urlget cldr-plurals

noinst_SCRIPTS = spit.py
if !BUILD_SPIT_IN_C
noinst_SCRIPTS += spit
endif

if INSTALL_PRIVATE_LIBRARIES
# Specify that libgettextsrc should be installed in $(libdir).
lib_LTLIBRARIES = libgettextsrc.la
else
# Specify that libgettextsrc should not be installed.
noinst_LTLIBRARIES = libgettextsrc.la
endif

noinst_LIBRARIES = libxgettextx.a $(LIBXGETTEXTTS)

noinst_HEADERS = \
  pos.h message.h po-error.h xerror-handler.h po-xerror.h \
  read-po-internal.h po-charset.h read-po-lex.h \
  open-catalog.h read-catalog-special.h \
  read-catalog-abstract.h read-catalog.h read-catalog-file.h \
  read-po.h read-properties.h read-stringtable.h \
  str-list.h \
  write-catalog.h write-po.h write-properties.h write-stringtable.h \
  dir-list.h file-list.h read-po-gram.h cldr-plural.h \
  cldr-plural-exp.h locating-rules.h its.h search-path.h \
  msgl-charset.h msgl-equal.h msgl-iconv.h msgl-ascii.h msgl-ofn.h msgl-cat.h \
  msgl-header.h msgl-english.h msgl-check.h msgl-fsearch.h msgl-merge.h \
  msgfmt.h msgunfmt.h \
  plural-count.h plural-eval.h plural-distrib.h \
  read-mo.h write-mo.h \
  read-java.h write-java.h \
  read-csharp.h write-csharp.h \
  read-resources.h write-resources.h \
  read-tcl.h write-tcl.h \
  write-qt.h \
  read-desktop.h write-desktop.h \
  write-xml.h \
  po-time.h plural-table.h lang-table.h country-table.h format.h filters.h \
  xgettext.h \
  if-error.h \
  rc-str-list.h xg-pos.h xg-encoding.h xg-mixed-string.h xg-formatstring.h \
  xg-arglist-context.h xg-arglist-callshape.h xg-arglist-parser.h xg-message.h \
  xg-check.h \
  x-po.h x-properties.h x-stringtable.h \
  x-c.h \
  x-python.h \
  x-java.h \
  x-csharp.h \
  x-javascript.h \
  x-typescript.h \
  x-typescriptx.h \
  x-typescript-impl.h \
  x-scheme.h \
  x-lisp.h \
  x-elisp.h \
  x-librep.h \
  x-rust.h \
  x-go.h \
  x-ruby.h \
  x-sh.h \
  x-awk.h \
  x-lua.h \
  x-modula2.h \
  x-d.h html5-entities.h \
  x-ocaml.h \
  x-smalltalk.h \
  x-vala.h \
  x-tcl.h \
  x-perl.h \
  x-php.h \
  x-ycp.h \
  x-rst.h \
  x-desktop.h \
  x-glade.h x-gsettings.h x-appdata.h

EXTRA_DIST += FILES project-id

# The following are already set through configmake.m4 and configure.ac:
#pkgdatadir = $(datadir)/gettext
#pkglibdir = $(libdir)/gettext
#pkglibexecdir = $(libexecdir)/gettext
jardir = $(pkgdatadir)
jardir_c_make = $(pkgdatadir_c_make)
projectsdir = $(pkgdatadir)/projects
projectsdir_c_make = $(pkgdatadir_c_make)\"/projects\"
schemadir = $(pkgdatadir)/schema

# Note that Automake's $(DEFAULT_INCLUDES) already contains
# -I. -I$(srcdir) -I$(top_builddir).
# The option -I$(top_srcdir) is needed so that woe32dll/export.h is found.
AM_CPPFLAGS = \
  -I$(top_srcdir) \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/include \
  -I../libgrep -I$(top_srcdir)/libgrep \
  -I../gnulib-lib -I$(top_srcdir)/gnulib-lib \
  -I../../gettext-runtime/intl -I$(top_srcdir)/../gettext-runtime/intl
DEFS = \
  -DLOCALEDIR=$(localedir_c_make) \
  -DUSEJAVA=$(USEJAVA) \
  -DGETTEXTJAR=$(jardir_c_make)\"/gettext.jar\" \
  -DBINDIR=$(bindir_c_make) \
  -DLIBDIR=$(libdir_c_make) \
  -DLIBEXECDIR=$(libexecdir_c_make) \
  -DGETTEXTDATADIR=$(pkgdatadir_c_make) \
  -DPROJECTSDIR=$(projectsdir_c_make) \
  -DEXEEXT=\"@EXEEXT@\" @DEFS@

# Enable more warning options in this directory.
AM_CFLAGS = @WARN_CFLAGS@

# Ensure that <stdint.h> defines SIZE_MAX in C++ mode, like it does in C mode.
AM_CXXFLAGS = -D__STDC_LIMIT_MACROS

LDADD = ../gnulib-lib/libgettextlib.la $(LTLIBUNISTRING) @LTLIBINTL@ $(WOE32_LDADD)
OTHERPROGDEPENDENCIES = ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)

SED = sed
BISON = @TOOLS_BISON@
JAR = @JAR@
JAVACOMP = $(SHELL) ../javacomp.sh
CSHARPCOMP = $(SHELL) ../csharpcomp.sh
CSHARPCOMPFLAGS = @CSHARPCOMPFLAGS@


# All programs deal with message lists.
# All programs must read PO files. (msgunfmt also, for read-java.c,
# read-csharp.c and read-resources.c.)
# message.c -> str-list.c.
# (read-catalog-abstract.c <--> read-po-gram.y <--> read-po-lex.c) -> str-list.c.
# (read-catalog-abstract.c <--> read-po-gram.y <--> read-po-lex.c) -> po-charset.c.
# (read-catalog-abstract.c <--> read-po-gram.y <--> read-po-lex.c) -> message.c -> str-list.c.
# read-catalog-file.c -> open-catalog.c -> dir-list.c -> str-list.c.
COMMON_SOURCE = \
  message.c \
  pos.c \
  po-error.c
if !WOE32DLL
COMMON_SOURCE += xerror-handler.c
else
COMMON_SOURCE += ../woe32dll/c++xerror-handler.cc
endif
COMMON_SOURCE += \
  read-catalog-abstract.c \
  read-po-lex.c \
  read-po-gram.y \
  po-charset.c \
  read-po.c \
  read-properties.c \
  read-stringtable.c \
  open-catalog.c \
  dir-list.c \
  str-list.c

# xgettext and msgfmt deal with format strings.
if !WOE32DLL
FORMAT_SOURCE = format.c
else
FORMAT_SOURCE = ../woe32dll/c++format.cc
endif
FORMAT_SOURCE += \
  format-invalid.h \
  format-c.c format-c-parse.h \
  format-c++-brace.c \
  format-python.c \
  format-python-brace.c \
  format-java.c \
  format-java-printf.c \
  format-csharp.c \
  format-javascript.c \
  format-scheme.c \
  format-lisp.c \
  format-elisp.c \
  format-librep.c \
  format-rust.c \
  format-go.c \
  format-ruby.c \
  format-sh.c \
  format-sh-printf.c \
  format-awk.c \
  format-lua.c \
  format-pascal.c \
  format-modula2.c \
  format-d.c \
  format-ocaml.c \
  format-smalltalk.c \
  format-qt.c \
  format-qt-plural.c \
  format-kde.c \
  format-kde-kuit.c \
  format-boost.c \
  format-tcl.c \
  format-perl.c \
  format-perl-brace.c \
  format-php.c \
  format-gcc-internal.c \
  format-gfc-internal.c

# libgettextsrc contains all code that is needed by at least two programs
# and that does not use OpenMP.
libgettextsrc_la_SOURCES = \
  $(COMMON_SOURCE) \
  read-catalog-special.c \
  read-catalog.c read-catalog-file.c \
  write-catalog.c write-properties.c write-stringtable.c write-po.c \
  msgl-ascii.c \
  msgl-ofn.c \
  msgl-iconv.c \
  msgl-equal.c \
  msgl-cat.c \
  msgl-header.c \
  msgl-english.c \
  msgl-check.c \
  file-list.c \
  msgl-charset.c \
  msgl-fsearch.c \
  lang-table.c \
  po-time.c \
  plural-exp.c plural-eval.c plural-table.c \
  plural-count.c \
  quote.h \
  sentence.h sentence.c \
  $(FORMAT_SOURCE) \
  read-desktop.c \
  locating-rules.c its.c \
  search-path.c

# xgettext has some tree-sitter based backends.
LIBXGETTEXTTS = libxgettextts2.a libxgettextts3.a libxgettextts4.a libxgettextts5.a libxgettextts1.a
libxgettextts1_a_SOURCES = \
  ../tree-sitter-$(TREE_SITTER_VERSION)/lib/src/lib.c
libxgettextts1_a_CPPFLAGS = \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/include \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/src
libxgettextts1_a_CFLAGS = $(AM_CFLAGS) @FOREIGN_WARN_CFLAGS@ @NO_EXPENSIVE_WARN_CFLAGS@
libxgettextts2_a_SOURCES = \
  ../tree-sitter-rust-$(TREE_SITTER_RUST_VERSION)/src/rust-parser.c \
  ../tree-sitter-rust-$(TREE_SITTER_RUST_VERSION)/src/rust-scanner.c \
  ../tree-sitter-go-$(TREE_SITTER_GO_VERSION)/src/go-parser.c \
  ../tree-sitter-d-$(TREE_SITTER_D_VERSION)/src/d-parser.c \
  ../tree-sitter-d-$(TREE_SITTER_D_VERSION)/src/d-scanner.c
libxgettextts2_a_CPPFLAGS = \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/include
libxgettextts2_a_CFLAGS = $(AM_CFLAGS) @FOREIGN_WARN_CFLAGS@ @NO_EXPENSIVE_WARN_CFLAGS@
libxgettextts3_a_SOURCES = \
  ../tree-sitter-typescript-$(TREE_SITTER_TYPESCRIPT_VERSION)/typescript/src/ts-parser.c \
  ../tree-sitter-typescript-$(TREE_SITTER_TYPESCRIPT_VERSION)/typescript/src/ts-scanner.c
libxgettextts3_a_CPPFLAGS = \
  -I$(top_srcdir)/tree-sitter-typescript-$(TREE_SITTER_TYPESCRIPT_VERSION)/typescript/src \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/include
libxgettextts3_a_CFLAGS = $(AM_CFLAGS) @FOREIGN_WARN_CFLAGS@ @NO_EXPENSIVE_WARN_CFLAGS@
libxgettextts4_a_SOURCES = \
  ../tree-sitter-typescript-$(TREE_SITTER_TYPESCRIPT_VERSION)/tsx/src/tsx-parser.c \
  ../tree-sitter-typescript-$(TREE_SITTER_TYPESCRIPT_VERSION)/tsx/src/tsx-scanner.c
libxgettextts4_a_CPPFLAGS = \
  -I$(top_srcdir)/tree-sitter-typescript-$(TREE_SITTER_TYPESCRIPT_VERSION)/tsx/src \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/include
libxgettextts4_a_CFLAGS = $(AM_CFLAGS) @FOREIGN_WARN_CFLAGS@ @NO_EXPENSIVE_WARN_CFLAGS@
libxgettextts5_a_SOURCES = \
  ../tree-sitter-ocaml-$(TREE_SITTER_OCAML_VERSION)/grammars/ocaml/src/ocaml-parser.c \
  ../tree-sitter-ocaml-$(TREE_SITTER_OCAML_VERSION)/grammars/ocaml/src/ocaml-scanner.c
libxgettextts5_a_CPPFLAGS = \
  -I$(top_srcdir)/tree-sitter-ocaml-$(TREE_SITTER_OCAML_VERSION)/grammars/ocaml/src \
  -I$(top_srcdir)/tree-sitter-$(TREE_SITTER_VERSION)/lib/include
libxgettextts5_a_CFLAGS = $(AM_CFLAGS) @FOREIGN_WARN_CFLAGS@ @NO_EXPENSIVE_WARN_CFLAGS@

# msggrep needs pattern matching.
LIBGREP = ../libgrep/libgrep.a ../libgrep/gnulib-lib/libgnu.a

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
msgmerge_SOURCES += msgl-merge.c
msgunfmt_SOURCES = msgunfmt.c
msgunfmt_SOURCES += \
  read-mo.c read-java.c read-csharp.c read-resources.c read-tcl.c \
  ../../gettext-runtime/intl/hash-string.c
if !WOE32DLL
xgettext_SOURCES = xgettext.c
else
xgettext_SOURCES = ../woe32dll/c++xgettext.cc
endif
xgettext_SOURCES += \
  if-error.c \
  xg-pos.c \
  xg-encoding.c \
  xg-mixed-string.c \
  xg-formatstring.c \
  xg-arglist-context.c xg-arglist-callshape.c xg-arglist-parser.c \
  xg-message.c \
  xg-check.c \
  x-po.c \
  x-c.c \
  x-python.c \
  x-java.c \
  x-csharp.c \
  x-javascript.c \
  x-typescript.c \
  x-typescriptx.c \
  x-scheme.c \
  x-lisp.c \
  x-elisp.c \
  x-librep.c \
  x-rust.c \
  x-ruby.c \
  x-sh.c ../../gettext-runtime/src/escapes.h \
  x-awk.c \
  x-lua.c \
  x-modula2.c \
  x-d.c \
  x-ocaml.c \
  x-smalltalk.c \
  x-vala.c \
  x-tcl.c \
  x-perl.c \
  x-php.c \
  x-ycp.c \
  x-rst.c \
  x-desktop.c
# x-go.c cannot be compiled by GCC 11 with '-fanalyzer' in 1 GB of RAM.
# Therefore use special CFLAGS for this compilation unit.
libxgettextx_a_SOURCES = \
  x-go.c
libxgettextx_a_CPPFLAGS = $(xgettext_CPPFLAGS)
libxgettextx_a_CFLAGS = $(AM_CFLAGS) @NO_EXPENSIVE_WARN_CFLAGS@
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
msginit_SOURCES += msgl-merge.c
msginit_SOURCES += ../../gettext-runtime/intl/localealias.c
msgpre_SOURCES = msgpre.c
if !WOE32DLL
msguniq_SOURCES = msguniq.c
else
msguniq_SOURCES = ../woe32dll/c++msguniq.cc
endif
recode_sr_latin_SOURCES = recode-sr-latin.c filter-sr-latin.c
if BUILD_SPIT_IN_C
spit_SOURCES = spit.c country-table.c
spit_CFLAGS = $(AM_CFLAGS) $(INCJSON_C) $(INCCURL)
endif
hostname_SOURCES = hostname.c
urlget_SOURCES = urlget.c
cldr_plurals_SOURCES = cldr-plural.y cldr-plural-exp.c cldr-plurals.c
cldr_plurals_CFLAGS = $(AM_CFLAGS) $(INCXML)
cldr_plurals_LDADD = libgettextsrc.la $(LDADD)

if USE_INSTALLED_LIBTEXTSTYLE
LT_LIBTEXTSTYLE = @LTLIBTEXTSTYLE@
else
# How to get the include files of libtextstyle.
textstyle.h textstyle/version.h textstyle/woe32dll.h:
	here=`pwd`; \
	cd ../../libtextstyle/lib && \
	  $(MAKE) install-nobase_includeHEADERS install-nobase_nodist_includeHEADERS DESTDIR= includedir="$$here"
BUILT_SOURCES    += textstyle.h textstyle/version.h textstyle/woe32dll.h
MOSTLYCLEANFILES += textstyle.h textstyle/version.h textstyle/woe32dll.h
# Where to find the built libtextstyle library.
LT_LIBTEXTSTYLE = ../../libtextstyle/lib/libtextstyle.la
endif

# Version information according to Woe32 conventions.
EXTRA_DIST += libgettextsrc.rc
if WOE32
WOE32_LIBADD = libgettextsrc.res.lo
# This rule is executed only on Woe32 systems.
# Use $(RC) with libtool, $(WINDRES) when not using libtool.
# The following sed expressions come from the windres-options script. They are
# inlined here, so that they can be written in a Makefile without requiring a
# temporary file. They must contain literal newlines rather than semicolons,
# so that they work with the sed-3.02 that is shipped with MSYS.
libgettextsrc.res.lo: $(srcdir)/libgettextsrc.rc
	nlinit=`echo 'nl="'; echo '"'`; eval "$$nlinit"; \
	sed_extract_major='/^[0-9]/{'$${nl}'s/^\([0-9]*\).*/\1/p'$${nl}q$${nl}'}'$${nl}'c\'$${nl}0$${nl}q; \
	sed_extract_minor='/^[0-9][0-9]*[.][0-9]/{'$${nl}'s/^[0-9]*[.]\([0-9]*\).*/\1/p'$${nl}q$${nl}'}'$${nl}'c\'$${nl}0$${nl}q; \
	sed_extract_subminor='/^[0-9][0-9]*[.][0-9][0-9]*[.][0-9]/{'$${nl}'s/^[0-9]*[.][0-9]*[.]\([0-9]*\).*/\1/p'$${nl}q$${nl}'}'$${nl}'c\'$${nl}0$${nl}q; \
	$(LIBTOOL) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) \
	  "-DPACKAGE_VERSION_STRING=\\\"$(VERSION)\\\"" \
	  "-DPACKAGE_VERSION_MAJOR="`echo '$(VERSION)' | sed -n -e "$$sed_extract_major"` \
	  "-DPACKAGE_VERSION_MINOR="`echo '$(VERSION)' | sed -n -e "$$sed_extract_minor"` \
	  "-DPACKAGE_VERSION_SUBMINOR="`echo '$(VERSION)' | sed -n -e "$$sed_extract_subminor"` \
	  -i $(srcdir)/libgettextsrc.rc -o libgettextsrc.res.lo --output-format=coff
MOSTLYCLEANFILES += libgettextsrc.res.lo
else
WOE32_LIBADD =
endif
libgettextsrc_la_LIBADD       = $(WOE32_LIBADD)
libgettextsrc_la_DEPENDENCIES = $(WOE32_LIBADD)

# How to build libgettextsrc.la.
# Need ../gnulib-lib/libgettextlib.la.
# Need $(LTLIBUNISTRING) because ulc_width_linebreaks, uc_width, etc. may be
# taken from libunistring, when the configure option --with-libunistring-prefix
# was given.
# Need @LTLIBINTL@ because many source files use gettext().
# Need @LTLIBICONV@ because po-charset.c, read-po-lex.c, msgl-iconv.c,
# write-po.c use iconv().
libgettextsrc_la_LDFLAGS = \
  -release @VERSION@ \
  ../gnulib-lib/libgettextlib.la $(LTLIBUNISTRING) $(LT_LIBTEXTSTYLE) @LTLIBINTL@ @LTLIBICONV@ -lc -no-undefined

# OS/2 does not support a DLL name longer than 8 characters.
if OS2
libgettextsrc_la_LDFLAGS += -os2dllname gtsrc
endif

libgettextsrc_la_CPPFLAGS = $(AM_CPPFLAGS) $(INCXML)

# Needed for the expansion of LIBGETTEXTSRC_DLL_VARIABLE on MSVC.
libgettextsrc_la_CPPFLAGS += -DIN_LIBGETTEXTSRC

# Tell the mingw or Cygwin linker which symbols to export.
if WOE32DLL
if !WINDOWS_MSVC
libgettextsrc_la_LDFLAGS += -Wl,--export-all-symbols
endif
endif

# Specify installation directory, for --enable-relocatable.
if RELOCATABLE_VIA_LD
# This is needed, because libgettextsrc depends on libgettextlib and libintl.
libgettextsrc_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(libdir)`
endif

# Compile-time flags for particular source files.
msgmerge_CFLAGS = $(AM_CFLAGS) $(OPENMP_CFLAGS)
msgmerge_CXXFLAGS = $(AM_CXXFLAGS) $(OPENMP_CFLAGS)
msginit_CFLAGS = $(AM_CFLAGS) $(OPENMP_CFLAGS)
msginit_CXXFLAGS = $(AM_CXXFLAGS) $(OPENMP_CFLAGS)
# On mingw, the compiler option '-fno-threadsafe-statics' avoids requiring
# the symbols __cxa_guard_acquire and __cxa_guard_release, which in turn
# avoids a dependency towards libstdc++.
if WOE32DLL
if WINDOWS_NATIVE
if !WINDOWS_MSVC
xgettext_CXXFLAGS = $(AM_CXXFLAGS) -fno-threadsafe-statics
endif
endif
endif

# Link dependencies.
# INTL_MACOSX_LIBS is needed because the programs depend on libintl.la
# but libtool doesn't put -Wl,-framework options into .la files.
# For msginit, it is also needed because of localename.c.
msgcmp_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ @MSGMERGE_LIBM@ $(WOE32_LDADD)
msgfmt_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgmerge_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ @MSGMERGE_LIBM@ $(WOE32_LDADD) $(OPENMP_CFLAGS)
msgunfmt_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
xgettext_LDADD = libxgettextx.a $(LIBXGETTEXTTS) libgettextsrc.la @INTL_MACOSX_LIBS@ @LTLIBICONV@ $(WOE32_LDADD)
msgattrib_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgcat_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgcomm_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgconv_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgen_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgexec_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgfilter_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msggrep_LDADD = $(LIBGREP) libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msginit_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msgpre_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
msguniq_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(WOE32_LDADD)
if BUILD_SPIT_IN_C
spit_LDADD = libgettextsrc.la @INTL_MACOSX_LIBS@ $(LIBJSON_C) $(LIBCURL) $(WOE32_LDADD)
endif
hostname_LDADD = $(LDADD) $(GETADDRINFO_LIB)

# Specify when to relink the programs.
msgcmp_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgfmt_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgmerge_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgunfmt_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
xgettext_DEPENDENCIES = libxgettextx.a $(LIBXGETTEXTTS) libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgattrib_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgcat_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgcomm_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgconv_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgen_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgexec_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgfilter_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msggrep_DEPENDENCIES = $(LIBGREP) libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msginit_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msgpre_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
msguniq_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
recode_sr_latin_DEPENDENCIES = $(OTHERPROGDEPENDENCIES)
if BUILD_SPIT_IN_C
spit_DEPENDENCIES = libgettextsrc.la ../gnulib-lib/libgettextlib.la $(WOE32_LDADD)
endif
hostname_DEPENDENCIES = $(OTHERPROGDEPENDENCIES)
urlget_DEPENDENCIES = $(OTHERPROGDEPENDENCIES)

# Specify installation directory, for --enable-relocatable.
msgcmp_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgfmt_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgmerge_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgunfmt_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
xgettext_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgattrib_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgcat_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgcomm_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgconv_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgen_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgexec_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgfilter_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msggrep_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msginit_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msgpre_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
msguniq_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
recode_sr_latin_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
if BUILD_SPIT_IN_C
spit_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(bindir_c_make)
endif
hostname_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(pkglibexecdir_c_make)
urlget_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(pkglibexecdir_c_make)
cldr_plurals_CPPFLAGS = $(AM_CPPFLAGS) -DINSTALLDIR=$(pkglibexecdir_c_make)
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
msgpre_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
msguniq_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
recode_sr_latin_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
if BUILD_SPIT_IN_C
spit_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(bindir)`
endif
hostname_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibexecdir)`
urlget_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibexecdir)`
cldr_plurals_LDFLAGS = `$(RELOCATABLE_LDFLAGS) $(pkglibexecdir)`
else
xgettext_LDFLAGS =
endif

# Link-time flags for particular programs.
# With MSVC 14/x86_64, the default stack size of 1 MB is sufficient only for
# a recursion depth in "xgettext -L Rust" invocations of 501. To guarantee a
# recursion depth of 1000, we need to increase the stack size. Cf.
# <https://learn.microsoft.com/en-us/cpp/build/reference/f-set-stack-size>.
if PLATFORM_IS_MSVC
xgettext_LDFLAGS += -Wl,-F4000000
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

# The Automake generated .y.c rule is broken: When executed in a VPATH build,
#   - The .c file gets generated in the build directory. But since it requires
#     special tools to rebuild it, we need to distribute it in the tarballs,
#     and by the GNU Coding Standards
#     <https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html>
#     the file should be generated in the source directory.
#   - The #line numbers in the .c file refer to a nonexistent file once it
#     has been moved from the build directory to the source directory. This
#     leads to error if 'lcov' is used later.
# Therefore we override this rule.
#
# Also, the ylwrap script is not usable when both a .c and a .h file are to be
# generated from the .y file. The reason is that this script does nothing in a
# situation where the .h file is older than the .y file and the .y file is older
# than the .c file. (This is intentional, see the comment "Do not overwrite
# unchanged header files to avoid useless recompilations.") The effect is that
# during "make dist", a tarball is created where the .h file is older than the
# .y file. This has two negative consequences:
#   - For a user who builds an unmodified tarball, bison will be invoked.
#   - During "make distcheck" the rule
#       FILE.c FILE.h: FILE.y
#     always fires and, since $(srcdir) is read-only, the commands which update
#     (or at least touch) $(srcdir)/FILE.c and $(srcdir)/FILE.h fail.
# Therefore we don't use ylwrap.
#
# Note: There is no point in using Bison's --output option, since we need to
# postprocess the generated files and we don't want that unpostprocessed files
# remain in place if the user interrupts the build through Ctrl-C.

read-po-gram.c read-po-gram.h: read-po-gram.y
	$(AM_V_YACC)$(BISON) -d $(YFLAGS) $(AM_YFLAGS) $(srcdir)/read-po-gram.y \
	&& test ':' = '$(BISON)' || { \
	  sed -e 's|".*/read-po-gram\.y"|"read-po-gram.y"|' \
	      -e 's|"read-po-gram\.tab\.c"|"read-po-gram.c"|' \
	      -e 's|"read-po-gram\.tab\.h"|"read-po-gram.h"|' \
	      < read-po-gram.tab.c > read-po-gram.c-tmp \
	  && sed -e 's|".*/read-po-gram\.y"|"read-po-gram.y"|' \
	         -e 's|"read-po-gram\.tab\.h"|"read-po-gram.h"|' \
	         < read-po-gram.tab.h > read-po-gram.h-tmp \
	  && rm -f read-po-gram.tab.c read-po-gram.tab.h \
	  && mv read-po-gram.c-tmp $(srcdir)/read-po-gram.c \
	  && mv read-po-gram.h-tmp $(srcdir)/read-po-gram.h; \
	}
BUILT_SOURCES += read-po-gram.c read-po-gram.h
MOSTLYCLEANFILES += read-po-gram.tab.c read-po-gram.tab.h read-po-gram.c-tmp read-po-gram.h-tmp
MAINTAINERCLEANFILES += read-po-gram.c read-po-gram.h
EXTRA_DIST += read-po-gram.c read-po-gram.h


cldr-plural.c cldr-plural.h: cldr-plural.y
	$(AM_V_YACC)$(BISON) -d $(YFLAGS) $(AM_YFLAGS) $(srcdir)/cldr-plural.y \
	&& test ':' = '$(BISON)' || { \
	  sed -e 's|".*/cldr-plural\.y"|"cldr-plural.y"|' \
	      -e 's|"cldr-plural\.tab\.c"|"cldr-plural.c"|' \
	      -e 's|"cldr-plural\.tab\.h"|"cldr-plural.h"|' \
	      < cldr-plural.tab.c > cldr-plural.c-tmp \
	  && sed -e 's|".*/cldr-plural\.y"|"cldr-plural.y"|' \
	         -e 's|"cldr-plural\.tab\.h"|"cldr-plural.h"|' \
	         < cldr-plural.tab.h > cldr-plural.h-tmp \
	  && rm -f cldr-plural.tab.c cldr-plural.tab.h \
	  && mv cldr-plural.c-tmp $(srcdir)/cldr-plural.c \
	  && mv cldr-plural.h-tmp $(srcdir)/cldr-plural.h; \
	}
BUILT_SOURCES += cldr-plural.c cldr-plural.h
MOSTLYCLEANFILES += cldr-plural.tab.c cldr-plural.tab.h cldr-plural.c-tmp cldr-plural.h-tmp
MAINTAINERCLEANFILES += cldr-plural.c cldr-plural.h
EXTRA_DIST += cldr-plural.c cldr-plural.h


if !BUILD_SPIT_IN_C
# Ensure that the program 'spit' is available as a script,
# so that its manual page can be generated through 'help2man'.
spit: spit.py
	cp spit.py spit
BUILT_SOURCES += spit
MOSTLYCLEANFILES += spit
endif


# This is implicit in "make all", but is needed explicitly before other target
# invocations.
built-sources: $(BUILT_SOURCES)


# Special rules for installation of 'spit' and auxiliary programs.

install-exec-local:
if !BUILD_SPIT_IN_C
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) spit.py $(DESTDIR)$(bindir)/spit
endif
	$(MKDIR_P) $(DESTDIR)$(pkglibexecdir)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) hostname$(EXEEXT) $(DESTDIR)$(pkglibexecdir)/hostname$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) urlget$(EXEEXT) $(DESTDIR)$(pkglibexecdir)/urlget$(EXEEXT)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) cldr-plurals$(EXEEXT) $(DESTDIR)$(pkglibexecdir)/cldr-plurals$(EXEEXT)
	$(INSTALL_SCRIPT) user-email $(DESTDIR)$(pkglibexecdir)/user-email
	$(INSTALL_SCRIPT) $(srcdir)/project-id $(DESTDIR)$(pkglibexecdir)/project-id

installdirs-local:
if !BUILD_SPIT_IN_C
	$(MKDIR_P) $(DESTDIR)$(bindir)
endif
	$(MKDIR_P) $(DESTDIR)$(pkglibexecdir)

uninstall-local:
if !BUILD_SPIT_IN_C
	$(RM) $(DESTDIR)$(bindir)/spit
endif
	$(RM) $(DESTDIR)$(pkglibexecdir)/hostname$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibexecdir)/urlget$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibexecdir)/cldr-plurals$(EXEEXT)
	$(RM) $(DESTDIR)$(pkglibexecdir)/user-email
	$(RM) $(DESTDIR)$(pkglibexecdir)/project-id

DISTCLEANFILES += spit.py user-email


# Special rules for Java compilation.

USEJAVA = $(USEJAVA_@BUILDJAVA@)
USEJAVA_yes = 1
USEJAVA_no = 0

all-local: all-java-@BUILDJAVA@
all-java-yes: gettext.jar
all-java-no:

gnu/gettext/DumpResource.class: $(srcdir)/gnu/gettext/DumpResource.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/DumpResource.java

gnu/gettext/GetURL.class: $(srcdir)/gnu/gettext/GetURL.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/GetURL.java

gettext.jar: gnu/gettext/DumpResource.class gnu/gettext/GetURL.class
	$(top_srcdir)/../build-aux/jar-cf '$(JAR)' '$(top_srcdir)/..' $@ gnu/gettext/DumpResource*.class gnu/gettext/GetURL*.class

EXTRA_DIST += gnu/gettext/DumpResource.java gnu/gettext/GetURL.java

CLEANFILES += gettext.jar gnu/gettext/*.class

install-data-local: install-data-java-@BUILDJAVA@
install-data-java-yes: all-java-yes
	$(INSTALL_DATA) gettext.jar $(DESTDIR)$(jardir)/gettext.jar
install-data-java-no:

installdirs-local: installdirs-java-@BUILDJAVA@
installdirs-java-yes:
	$(MKDIR_P) $(DESTDIR)$(jardir)
installdirs-java-no:

uninstall-local: uninstall-java-@BUILDJAVA@
uninstall-java-yes:
	$(RM) $(DESTDIR)$(jardir)/gettext.jar
uninstall-java-no:


# Special rules for C# auxiliary programs.

EXTRA_DIST += msgfmt.cs msgunfmt.cs

CLEANFILES += \
  msgfmt.net.exe msgfmt.net.exe.mdb msgfmt.net.pdb \
  msgunfmt.net.exe msgunfmt.net.exe.mdb msgunfmt.net.pdb

all-local: all-csharp-@BUILDCSHARP@
all-csharp-yes: msgfmt.net.exe msgunfmt.net.exe
all-csharp-no:

msgfmt.net.exe: msgfmt.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ $(srcdir)/msgfmt.cs

if USE_INSTALLED_CSHARP_DLL
CSHARP_DLL_DIR = $(libdir)
else
CSHARP_DLL_DIR = ../../gettext-runtime/intl-csharp
endif
msgunfmt.net.exe: msgunfmt.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ -L '$(CSHARP_DLL_DIR)' -l GNU.Gettext $(srcdir)/msgunfmt.cs

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


# Special rules for XML schemas.

schema_DATA = \
  its.xsd10 its.xsd11 its-extensions.xsd \
  locating-rules.xsd10 locating-rules.xsd11
EXTRA_DIST += $(schema_DATA)


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


# This Makefile contains rules which don't work with parallel make,
# namely those which build textstyle.h, read-po-gram.c, cldr-plural.c.
# See <https://lists.gnu.org/archive/html/bug-make/2019-05/msg00011.html>.
# So, turn off parallel execution in this Makefile.
.NOTPARALLEL:
