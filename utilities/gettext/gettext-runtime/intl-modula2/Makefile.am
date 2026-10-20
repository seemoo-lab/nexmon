## Makefile for the gettext-runtime/intl-modula2 subdirectory of GNU gettext
## Copyright (C) 2025 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.10 gnu

EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES = core *.stackdump
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
SUFFIXES =

RM = rm -f

# Cf. <https://lists.nongnu.org/archive/html/gm2/2025-04/msg00015.html>
# and <https://lists.nongnu.org/archive/html/gm2/2025-04/msg00022.html>
m2includedir = $(includedir)/m2

if BUILD_MODULA2
lib_LTLIBRARIES = libintl_m2.la
endif
nodist_libintl_m2_la_SOURCES = Libintl.def
libintl_m2_la_SOURCES        = Libintl.mod LibintlFromC.def
if BUILD_MODULA2
# What object files to include in libintl_m2.la.
libintl_m2_la_LIBADD       = Libintl.lo
libintl_m2_la_DEPENDENCIES = Libintl.lo
if USE_INCLUDED_LIBINTL
libintl_m2_la_LIBADD       += ../intl/libintl.la
endif

nodist_m2include_HEADERS = Libintl.def
BUILT_SOURCES += Libintl.def

# Rules for compiling a .mod file, that work even without a VPATH variable.
AM_M2FLAGS = -I. -I$(srcdir)
Libintl.lo: Libintl.def $(srcdir)/Libintl.mod $(srcdir)/LibintlFromC.def
	$(AM_V_CC)$(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(M2C) $(AM_M2FLAGS) $(M2FLAGS) -c -o $@ $(srcdir)/Libintl.mod

# Libtool's library version information for libintl_m2.
# Before making a gettext release, you must change this according to the
# libtool documentation, section "Library interface versions".
LTV_CURRENT=0
LTV_REVISION=0
LTV_AGE=0

# How to build libintl_m2.la.
# Finding the gm2 libm2pim is hairy:
# - In GCC builds from source, the .la files are present and work.
# - In distros, either libm2pim.so may work, or m2/m2pim/libm2pim.so, depending
#   on the distro's packaging.
libintl_m2_la_LDFLAGS = \
  -no-undefined \
  -version-info $(LTV_CURRENT):$(LTV_REVISION):$(LTV_AGE) \
  -rpath $(libdir) \
  $$(libm2=`$(M2C) -print-file-name=libm2pim.la`; \
     if test "$$libm2" != 'libm2pim.la'; then dir=`dirname "$$libm2"`; echo "-L$$dir" -lm2pim; else \
       libm2=`$(M2C) -print-file-name=libm2pim.$(M2SHLIBEXT)`; \
       if test "$$libm2" != 'libm2pim.$(M2SHLIBEXT)'; then dir=`dirname "$$libm2"`; echo "-L$$dir" -lm2pim; else \
         libm2=`$(M2C) -print-file-name=m2/m2pim/libm2pim.$(M2SHLIBEXT)`; \
         if test "$$libm2" != 'm2/m2pim/libm2pim.$(M2SHLIBEXT)'; then dir=`dirname "$$libm2"`; echo "-L$$dir" -lm2pim; fi; \
       fi; \
     fi)

# Installation of libintl_m2.la.

install-exec-local: install-exec-libintl
install-exec-libintl: libintl_m2.la
	$(mkdir_p) $(DESTDIR)$(libdir)
	$(LIBTOOL) --mode=install \
	  $(INSTALL_DATA) libintl_m2.la $(DESTDIR)$(libdir)/libintl_m2.la; \
	if test "@RELOCATABLE@" = yes; then \
	  dependencies=`sed -n -e 's,^dependency_libs=\(.*\),\1,p' < $(DESTDIR)$(libdir)/libintl_m2.la | sed -e "s,^',," -e "s,'\$$,,"`; \
	  if test -n "$$dependencies"; then \
	    rm -f $(DESTDIR)$(libdir)/libintl_m2.la; \
	  fi; \
	fi

installdirs-local: installdirs-libintl
installdirs-libintl:
	$(mkdir_p) $(DESTDIR)$(libdir)

uninstall-local: uninstall-libintl
uninstall-libintl:
	$(LIBTOOL) --mode=uninstall \
	  rm -f $(DESTDIR)$(libdir)/libintl_m2.la

endif

EXTRA_DIST += HACKING
