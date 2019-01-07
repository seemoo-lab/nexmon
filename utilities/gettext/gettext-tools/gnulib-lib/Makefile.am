## Makefile for the gettext-tools/gnulib-lib subdirectory of GNU gettext
## Copyright (C) 1995-1998, 2000-2006, 2009, 2015-2016 Free Software
## Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.5 gnits subdir-objects no-dependencies
EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES = core *.stackdump
MOSTLYCLEANDIRS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
SUFFIXES =

noinst_LTLIBRARIES =

# Extra files to be installed.

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = gettext.h

# Where to install javaversion.class.
pkgdatadir = $(datadir)/gettext
pkgdata_DATA =

AM_CPPFLAGS =

# Hmm... is this still necessary?
AM_CPPFLAGS += -I$(top_builddir)/intl -I$(top_srcdir)/intl
# Needed so that config.h and woe32dll/export.h are found.
AM_CPPFLAGS += -I$(top_builddir) -I$(top_srcdir)

# Parametrization of the 'relocatable' module.
AM_CPPFLAGS += -DDEPENDS_ON_LIBICONV=1 -DDEPENDS_ON_LIBINTL=1

# Parametrization of the 'libxml' module:
# When building a shared library, don't export the variables
# xmlMalloc, xmlMallocAtomic, xmlRealloc, xmlFree, xmlMemStrdup.
if INCLUDED_LIBXML
AM_CPPFLAGS += -DLIBXML_STATIC
endif

# Rules generated and collected by gnulib-tool.
include Makefile.gnulib

# Which classes to export from the shared library.
MOOPPFLAGS += --dllexport=styled_ostream

# OS/2 does not support a DLL name longer than 8 characters.
if OS2
libgettextlib_la_LDFLAGS += -os2dllname gtlib
endif

# Linking with C++ libraries is needed _only_ on mingw and Cygwin.
if !WOE32DLL
libgettextlib_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(libgettextlib_la_LDFLAGS) $(LDFLAGS) \
	-o $@
else
libgettextlib_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CXX \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(libgettextlib_la_LDFLAGS) $(LDFLAGS) \
	-o $@
endif

