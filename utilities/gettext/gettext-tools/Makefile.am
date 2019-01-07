## Makefile for the gettext-tools directory of GNU gettext
## Copyright (C) 1995-2007, 2010, 2015-2016 Free Software Foundation, Inc.
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
ACLOCAL_AMFLAGS = -I m4 -I ../gettext-runtime/m4 -I ../m4 -I gnulib-m4 -I libgrep/gnulib-m4 -I libgettextpo/gnulib-m4

SUBDIRS = doc intl gnulib-lib libgrep src libgettextpo po projects styles misc man m4 tests gnulib-tests examples its

EXTRA_DIST = misc/DISCLAIM
MOSTLYCLEANFILES = core *.stackdump


# Files installed for use by gettextize.

gettextsrcdir = $(datadir)/gettext
gettextsrc_SCRIPTS = ../build-aux/config.rpath


# Files installed for the examples.

examplesbuildauxdir = $(docdir)/examples/build-aux
examplesbuildaux_DATA = \
  ../build-aux/javacomp.sh.in ../build-aux/javaexec.sh.in \
  ../build-aux/csharpcomp.sh.in ../build-aux/csharpexec.sh.in


# Files copied from other directories.

all-local: $(srcdir)/ABOUT-NLS
$(srcdir)/ABOUT-NLS: $(srcdir)/../gettext-runtime/ABOUT-NLS
	cp $(srcdir)/../gettext-runtime/ABOUT-NLS $(srcdir)/ABOUT-NLS


# Allow users to use "gnulib-tool --update".
EXTRA_DIST += gnulib-m4/gnulib-cache.m4


# Woe32 support.

EXTRA_DIST += README.woe32


# Cygwin support.

EXTRA_DIST += woe32dll/export.h

maintainer-update-po: $(top_builddir)/config.status
	$(SHELL) ./config.status --recheck
	$(SHELL) ./config.status po/Makefile.in po-directories
	cd po && $(MAKE) update-po
	cd examples && $(MAKE) maintainer-update-po
