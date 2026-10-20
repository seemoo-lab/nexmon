## Makefile for the gettext-tools/wizard subdirectory of GNU gettext
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

AUTOMAKE_OPTIONS = 1.2 gnits
EXTRA_DIST =
MOSTLYCLEANFILES =
CLEANFILES =
DISTCLEANFILES =


# Scripts for maintainers.

bin_SCRIPTS = gettextize

DISTCLEANFILES += gettextize


# gettextize copies these files into packages.

pkgdatadir = $(datadir)/gettext
podir = $(pkgdatadir)/po
po_DATA = \
  po-templates/traditional/Makefile.in.in \
  po-templates/traditional/remove-potcdate.sed \
  po-templates/traditional/Makevars.template \
  po-templates/traditional/Rules-quot \
  po-templates/traditional/quot.sed \
  po-templates/traditional/boldquot.sed \
  po-templates/traditional/en@quot.header \
  po-templates/traditional/en@boldquot.header \
  po-templates/traditional/insert-header.sed
po_SCRIPTS = \
  po-templates/traditional/fetch-po

EXTRA_DIST += $(po_DATA) $(po_SCRIPTS)


# We don't install the source code in unpacked form any more.
# We don't install the Makevars file any more either.
install-data-local: uninstall-sources
uninstall-local: uninstall-sources
uninstall-sources:
	rm -rf $(DESTDIR)$(pkgdatadir)/intl \
	       $(DESTDIR)$(podir)/Makevars
