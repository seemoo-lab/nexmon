## Makefile for the gettext-runtime/libasprintf subdirectory of GNU gettext
## Copyright (C) 2002-2007, 2009-2010, 2015-2016 Free Software Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation; either version 2.1 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies
ACLOCAL_AMFLAGS = -I ../../m4 -I ../m4 -I gnulib-m4
EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES =

RM = rm -f

DEFS = -DIN_LIBASPRINTF @DEFS@


# Library include file.

include_HEADERS = autosprintf.h

all-local $(libasprintf_la_OBJECTS): autosprintf.h
autosprintf.h: autosprintf.in.h
	cp $(srcdir)/autosprintf.in.h autosprintf.h
MOSTLYCLEANFILES += autosprintf.h
EXTRA_DIST += autosprintf.in.h

dist-hook:
	rm -f $(distdir)/autosprintf.h


# Library code.

lib_LTLIBRARIES = libasprintf.la

noinst_LTLIBRARIES =

libasprintf_la_SOURCES = \
  xsize.h xsize.c \
  lib-asprintf.h lib-asprintf.c \
  autosprintf.h autosprintf.cc

# Sources used only on platforms lacking vasprintf().
lib_asprintf_EXTRASOURCES = \
  verify.h \
  printf-args.h printf-args.c \
  printf-parse.h printf-parse.c \
  vasnprintf.h vasnprintf.c asnprintf.c \
  vasprintf.h vasprintf.c asprintf.c
lib-asprintf.lo: $(lib_asprintf_EXTRASOURCES)
EXTRA_DIST += $(lib_asprintf_EXTRASOURCES)


# How to build libasprintf.
# With libtool 1.5.14, on some platforms, like BeOS, "libtool --tag=CXX" fails
# to create a shared library, however "libtool --tag=CC" succeeds.
libasprintf_la_LDFLAGS = @LTNOUNDEF@
libasprintf.la: $(libasprintf_la_OBJECTS) $(libasprintf_la_DEPENDENCIES)
	$(AM_V_GEN)$(CXXLINK) -rpath $(libdir) $(libasprintf_la_LDFLAGS) $(libasprintf_la_OBJECTS) $(libasprintf_la_LIBADD) $(LIBS) || \
	$(LINK) -rpath $(libdir) $(libasprintf_la_LDFLAGS) $(libasprintf_la_OBJECTS) $(libasprintf_la_LIBADD) $(LIBS)


# A small set of gnulib modules is needed here as well.
include Makefile.gnulib

# Allow users to use "gnulib-tool --update".
EXTRA_DIST += gnulib-m4/gnulib-cache.m4


# Clean up after Solaris cc.
clean-local:
	rm -rf SunWS_cache


# Documentation.

MAKEINFO = env LANG= LC_MESSAGES= LC_ALL= LANGUAGE= @MAKEINFO@
MAKEINFOFLAGS = --no-split

info_TEXINFOS = autosprintf.texi

# We distribute only the HTML documentation.
# The user can generate the others, via
#   make autosprintf.ps
#   make autosprintf.pdf

all-local: html-local
install-data-local: install-html
installdirs-local: installdirs-html
uninstall-local: uninstall-html

# CLEANFILES: autosprintf.{dvi,ps,pdf,html} are already known to automake.


# Documentation in DVI format.

# The install-dvi target is already defined by automake.

installdirs-dvi:
	$(MKDIR_P) $(DESTDIR)$(dvidir)

uninstall-dvi:
	$(RM) $(DESTDIR)$(dvidir)/autosprintf.dvi

# Temporary index files. automake removes only the predefined ones by itself.
MOSTLYCLEANFILES +=


# Documentation in Postscript format.

# Override of automake's definition:
#DVIPS = @DVIPS@
DVIPS = @DVIPS@ -D600

autosprintf.ps: autosprintf.dvi
	$(DVIPS) -o $@ `if test -f autosprintf.dvi; then echo autosprintf.dvi; else echo $(srcdir)/autosprintf.dvi; fi`

# The install-ps target is already defined by automake.

installdirs-ps:
	$(MKDIR_P) $(DESTDIR)$(psdir)

uninstall-ps:
	$(RM) $(DESTDIR)$(psdir)/autosprintf.ps


# Documentation in Portable Document Format.

# The install-pdf target is already defined by automake.

installdirs-pdf:
	$(MKDIR_P) $(DESTDIR)$(pdfdir)

uninstall-pdf:
	$(RM) $(DESTDIR)$(pdfdir)/autosprintf.pdf


# Documentation in HTML format.

TEXI2HTML = @PERL@ $(srcdir)/texi2html

HTMLS = autosprintf_all.html

html-local: $(HTMLS)

# Override of automake's definition.
# We want to use texi2html, not makeinfo --html.
autosprintf_all.html: autosprintf.texi
	$(TEXI2HTML) -number -monolithic `if test -f autosprintf.texi; then echo autosprintf.texi; else echo $(srcdir)/autosprintf.texi; fi`
	mv autosprintf.html autosprintf_all.html

# The install-html target is already defined by automake.

installdirs-html:
	$(MKDIR_P) $(DESTDIR)$(htmldir)

uninstall-html:
	$(RM) $(DESTDIR)$(htmldir)/autosprintf.html

EXTRA_DIST += texi2html autosprintf_all.html


# Woe32 support.

EXTRA_DIST += README.woe32
