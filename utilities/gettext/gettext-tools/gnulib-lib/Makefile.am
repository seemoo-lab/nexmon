## Makefile for the gettext-tools/gnulib-lib subdirectory of GNU gettext
## Copyright (C) 1995-2025 Free Software Foundation, Inc.
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
MOSTLYCLEANDIRS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
SUFFIXES =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

# Extra files to be installed.

# Where to install javaversion.class.
pkgdatadir = $(datadir)/gettext
pkgdata_DATA =

# Note that Automake's $(DEFAULT_INCLUDES) already contains
# -I. -I$(srcdir) -I$(top_builddir).
AM_CPPFLAGS =

# Needed for the gnulib source code that does '#include "gettext.h"'.
AM_CPPFLAGS += -I../../gettext-runtime/intl
# Needed so that woe32dll/export.h is found.
AM_CPPFLAGS += -I$(top_srcdir)

# Needed for the expansion of DLL_VARIABLE on MSVC.
AM_CPPFLAGS += -DIN_LIBGETTEXTLIB

# Parametrization of the 'relocatable' module.
AM_CPPFLAGS += -DDEPENDS_ON_LIBICONV=1 -DDEPENDS_ON_LIBINTL=1

# Parametrization of the 'libxml' module:
# When building a shared library, don't export the variables
# xmlMalloc, xmlMallocAtomic, xmlRealloc, xmlFree, xmlMemStrdup.
if INCLUDED_LIBXML
AM_CPPFLAGS += -DLIBXML_STATIC
endif

# Enable more warning options in this directory.
AM_CFLAGS = @WARN_CFLAGS@

# Rules generated and collected by gnulib-tool.
include Makefile.gnulib

# Version information according to Woe32 conventions.
EXTRA_DIST += libgettextlib.rc
if WOE32
WOE32_LIBADD = libgettextlib.res.lo
# This rule is executed only on Woe32 systems.
# Use $(RC) with libtool, $(WINDRES) when not using libtool.
# The following sed expressions come from the windres-options script. They are
# inlined here, so that they can be written in a Makefile without requiring a
# temporary file. They must contain literal newlines rather than semicolons,
# so that they work with the sed-3.02 that is shipped with MSYS.
libgettextlib.res.lo: $(srcdir)/libgettextlib.rc
	nlinit=`echo 'nl="'; echo '"'`; eval "$$nlinit"; \
	sed_extract_major='/^[0-9]/{'$${nl}'s/^\([0-9]*\).*/\1/p'$${nl}q$${nl}'}'$${nl}'c\'$${nl}0$${nl}q; \
	sed_extract_minor='/^[0-9][0-9]*[.][0-9]/{'$${nl}'s/^[0-9]*[.]\([0-9]*\).*/\1/p'$${nl}q$${nl}'}'$${nl}'c\'$${nl}0$${nl}q; \
	sed_extract_subminor='/^[0-9][0-9]*[.][0-9][0-9]*[.][0-9]/{'$${nl}'s/^[0-9]*[.][0-9]*[.]\([0-9]*\).*/\1/p'$${nl}q$${nl}'}'$${nl}'c\'$${nl}0$${nl}q; \
	$(LIBTOOL) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) \
	  "-DPACKAGE_VERSION_STRING=\\\"$(VERSION)\\\"" \
	  "-DPACKAGE_VERSION_MAJOR="`echo '$(VERSION)' | sed -n -e "$$sed_extract_major"` \
	  "-DPACKAGE_VERSION_MINOR="`echo '$(VERSION)' | sed -n -e "$$sed_extract_minor"` \
	  "-DPACKAGE_VERSION_SUBMINOR="`echo '$(VERSION)' | sed -n -e "$$sed_extract_subminor"` \
	  -i $(srcdir)/libgettextlib.rc -o libgettextlib.res.lo --output-format=coff
MOSTLYCLEANFILES += libgettextlib.res.lo
else
WOE32_LIBADD =
endif
libgettextlib_la_LIBADD       += $(WOE32_LIBADD)
libgettextlib_la_DEPENDENCIES += $(WOE32_LIBADD)

# OS/2 does not support a DLL name longer than 8 characters.
if OS2
libgettextlib_la_LDFLAGS += -os2dllname gtlib
endif

# Specify installation directory, for --enable-relocatable.
if RELOCATABLE_VIA_LD
# This is needed, because libgettextlib depends on libintl.
libgettextlib_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(libdir)`
endif

# Support for relocatability.
RELOCATABLE_LIBRARY_PATH = $(libdir)

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
# Use the same definition of CXXLD as Automake generates in other Makefiles.
CXXLD = $(CXX)
endif

