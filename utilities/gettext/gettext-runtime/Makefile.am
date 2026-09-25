## Makefile for the gettext-runtime directory of GNU gettext
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

AUTOMAKE_OPTIONS = 1.5 gnu
ACLOCAL_AMFLAGS = -I m4 -I ../m4 -I gnulib-m4

# The list of subdirectories depends on whether --disable-libasprintf was
# specified.
if ENABLE_LIBASPRINTF
SUBDIR_libasprintf = libasprintf
else
SUBDIR_libasprintf =
endif
SUBDIRS = doc intl intl-java intl-csharp intl-d intl-modula2 gnulib-lib $(SUBDIR_libasprintf) src po man m4 tests install-tests

EXTRA_DIST = BUGS


# Files installed for the user and for use by gettextize.

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = ABOUT-NLS


# Allow users to use "gnulib-tool --update".
EXTRA_DIST += gnulib-m4/gnulib-cache.m4


# Windows support.

EXTRA_DIST += INSTALL.windows


# Support for "make dist" without prior "make".

# Ensure that the manual pages are up-to-date when "make dist" runs.
# Hidden from automake, but really activated. Works around an automake bug.
#distdir: distdir1
.PHONY: distdir1
distdir1: man/gettext.1 man/ngettext.1 man/printf_gettext.1 man/printf_ngettext.1 man/envsubst.1
man/gettext.1 man/ngettext.1 man/printf_gettext.1 man/printf_ngettext.1 man/envsubst.1: gen-man1
.PHONY: gen-man1
gen-man1: src/gettext.c man/gettext.x \
          src/ngettext.c man/ngettext.x \
          src/printf_gettext.c man/printf_gettext.x \
          src/printf_ngettext.c man/printf_ngettext.x \
          src/envsubst.c man/envsubst.x
	cd gnulib-lib && $(MAKE) $(AM_MAKEFLAGS)
	cd src && $(MAKE) $(AM_MAKEFLAGS) gettext$(EXEEXT) ngettext$(EXEEXT) printf_gettext$(EXEEXT) printf_ngettext$(EXEEXT) envsubst$(EXEEXT)
	cd man && $(MAKE) $(AM_MAKEFLAGS) gettext.1 ngettext.1 printf_gettext.1 printf_ngettext.1 envsubst.1


maintainer-update-po: $(top_builddir)/config.status
	$(SHELL) ./config.status --recheck
	$(SHELL) ./config.status po/Makefile.in po-directories
	cd po && $(MAKE) update-po
