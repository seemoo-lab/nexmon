## Makefile for the gettext-tools/libgettextpo subdirectory of GNU gettext
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
BUILT_SOURCES =
MOSTLYCLEANFILES = core *.stackdump
MOSTLYCLEANDIRS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
SUFFIXES =

lib_LTLIBRARIES = libgettextpo.la

nodist_include_HEADERS = gettext-po.h

noinst_LTLIBRARIES =

nodist_noinst_HEADERS = config.h

AM_CPPFLAGS = \
  -I. -I$(srcdir) \
  -I.. -I$(top_srcdir) \
  -I../src -I$(top_srcdir)/src \
  -I../intl -I$(top_srcdir)/../gettext-runtime/intl

DEFS = -DIN_LIBGETTEXTPO=1 @DEFS@

# libgettextpo contains the public API for PO files.
libgettextpo_la_SOURCES = \
  gettext-po.c \
  $(libgettextpo_la_AUXSOURCES)
# These are auxiliary sources whose symbols should not be exported.
libgettextpo_la_AUXSOURCES = \
  ../src/str-list.c \
  ../src/dir-list.c \
  ../src/message.c \
  ../src/msgl-ascii.c \
  ../src/po-error.c \
  ../src/po-xerror.c \
  ../src/write-catalog.c \
  ../src/write-po.c \
  ../src/open-catalog.c \
  ../src/po-charset.c \
  ../src/po-lex.c \
  ../src/po-gram-gen.c \
  ../src/read-po.c \
  ../src/read-catalog-abstract.c \
  ../src/read-catalog.c \
  ../src/plural-table.c \
  ../src/format-c.c \
  ../src/format-sh.c \
  ../src/format-python.c \
  ../src/format-python-brace.c \
  ../src/format-lisp.c \
  ../src/format-elisp.c \
  ../src/format-librep.c \
  ../src/format-scheme.c \
  ../src/format-java.c \
  ../src/format-javascript.c \
  ../src/format-csharp.c \
  ../src/format-awk.c \
  ../src/format-pascal.c \
  ../src/format-ycp.c \
  ../src/format-tcl.c \
  ../src/format-perl.c \
  ../src/format-perl-brace.c \
  ../src/format-php.c \
  ../src/format-gcc-internal.c \
  ../src/format-gfc-internal.c \
  ../src/format-qt.c \
  ../src/format-qt-plural.c \
  ../src/format-kde.c \
  ../src/format-kde-kuit.c \
  ../src/format-boost.c \
  ../src/format-lua.c \
  ../src/format.c \
  ../src/plural-exp.c \
  ../src/plural-eval.c \
  ../src/msgl-check.c \
  ../src/sentence.c

# Libtool's library version information for libgettextpo.
# See the libtool documentation, section "Library interface versions".
LTV_CURRENT=5
LTV_REVISION=4
LTV_AGE=5

# How to build libgettextpo.la.
# Don't make it depend on libgettextsrc.la or libgettextlib.la, since they
# define an uncontrolled amount of symbols.
libgettextpo_la_LIBADD = libgnu.la $(WOE32_LIBADD) $(LTLIBUNISTRING)
libgettextpo_la_LDFLAGS = \
  -version-info $(LTV_CURRENT):$(LTV_REVISION):$(LTV_AGE) \
  -rpath $(libdir) \
  @LTLIBINTL@ @LTLIBICONV@ -lc -no-undefined

# Tell the mingw or Cygwin linker which symbols to export.
if WOE32DLL
libgettextpo_la_SOURCES += ../woe32dll/gettextpo-exports.c
libgettextpo_la_LDFLAGS += -Wl,--export-all-symbols
endif

# OS/2 does not support a DLL name longer than 8 characters.
if OS2
libgettextpo_la_LDFLAGS += -os2dllname gtpo
endif

# Version information according to Woe32 conventions.
EXTRA_DIST += libgettextpo.rc
if WOE32
WOE32_LIBADD = libgettextpo.res.lo
libgettextpo.res.lo : $(srcdir)/libgettextpo.rc
	$(LIBTOOL) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) `$(SHELL) $(top_srcdir)/../windows/windres-options --escape $(VERSION)` -i $(srcdir)/libgettextpo.rc -o libgettextpo.res.lo --output-format=coff
MOSTLYCLEANFILES += libgettextpo.res.lo
else
WOE32_LIBADD =
endif

# Hide symbols that are defined by libgettextpo_la_AUXSOURCES or libgnu.la
# from the global namespace, by prefixing them with "libgettextpo_".
all check install: config.h
config.h: $(BUILT_SOURCES)
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  : "Avoid double inclusion, to avoid a warning about redefinition of DLL_VARIABLE."; \
	  echo '#ifndef GTPO_CONFIG_H'; \
	  echo '#define GTPO_CONFIG_H'; \
	  echo; \
	  echo '#include "../config.h"'; \
	  : "Turn all gettext() calls into dgettext() calls."; \
	  echo '#define DEFAULT_TEXT_DOMAIN "gettext-tools"'; \
	  : "All code is collected in a single library,"; \
	  : "No references to variables in other libraries exist."; \
	  echo '#undef DLL_VARIABLE'; \
	  echo '#define DLL_VARIABLE'; \
	  echo; \
	  echo '#endif /* GTPO_CONFIG_H */'; \
	} > config.h && \
	if test -n "$(HAVE_GLOBAL_SYMBOL_PIPE)"; then \
	  { \
	    for f in $(libgettextpo_la_AUXSOURCES) $(libgnu_la_SOURCES) $(libgnu_la_LIBADD); do \
	      case $$f in \
	        *.c | *.$(OBJEXT) | *.lo ) \
	          sf=`echo "$$f" | sed -e 's,\\.[^.]*$$,,'`.c; \
	          test -f $$sf || sf=$(srcdir)/$$sf; \
	          of=`echo "$$f" | sed -e 's,^.*/,,' -e 's,\\.[^.]*$$,,'`.$(OBJEXT); \
	          $(COMPILE) $(DEFS) -c $$sf || { rm -f config.h; exit 1; }; \
	          sh ./exported.sh $$of 1>&5; \
	          rm -f $$of `echo "$$of" | sed -e 's,\\.$(OBJEXT)$$,.lo,'`; \
	          ;; \
	      esac; \
	    done; \
	  } 5>&1 \
	    | sed -e 's,.* ,,' | LC_ALL=C sort | LC_ALL=C uniq \
	    | sed -e 's,^obstack_free$$,__obstack_free,' \
	    | sed -e 's,^\(.*\)$$,#define \1 libgettextpo_\1,' > config.h-t && \
	  if test -f config.h; then \
	    cat config.h-t >> config.h; \
	    rm -f config.h-t; \
	  else \
	    rm -f config.h-t; \
	    exit 1; \
	  fi \
	fi
MOSTLYCLEANFILES += config.h config.h-t

# Special rules for installable include files that export variables.

EXTRA_DIST += gettext-po.in.h
BUILT_SOURCES += gettext-po.h
CLEANFILES += gettext-po.h
gettext-po.h: gettext-po.in.h
if WOE32DLL
	sed -e 's/extern \([^()]*\);/extern __declspec (dllimport) \1;/' $(srcdir)/gettext-po.in.h > gettext-po.h-tmp
else
	cp $(srcdir)/gettext-po.in.h gettext-po.h-tmp
endif
	mv gettext-po.h-tmp gettext-po.h

# Allow users to use "gnulib-tool --update".
EXTRA_DIST += gnulib-m4/gnulib-cache.m4

# A small set of gnulib modules is needed here as well.
include Makefile.gnulib
