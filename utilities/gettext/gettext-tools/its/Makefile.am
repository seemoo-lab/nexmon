## Makefile for the gettext-tools/its subdirectory of GNU gettext
## Copyright (C) 2015-2024 Free Software Foundation, Inc.
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

pkgdatadir = $(datadir)/gettext
itsdir = $(pkgdatadir)$(PACKAGE_SUFFIX)/its

ITS_FILES = \
  glade1.its glade2.its gtkbuilder.its \
  gsettings.its \
  metainfo.its \
  docbook4.its docbook5.its

LOC_FILES = \
  glade.loc \
  gsettings.loc \
  metainfo.loc \
  docbook.loc

dist_its_DATA = $(ITS_FILES) $(LOC_FILES)

# Checks the XML files against their schemas.
# Work around <https://gitlab.gnome.org/GNOME/libxml2/-/issues/809>.
check-local:
	@for file in $(ITS_FILES); do \
	  echo "Checking $$file..."; \
	  echo "$(top_builddir)/build-aux/xml-validate-10 $(top_srcdir)/src/its.xsd10 $(srcdir)/$$file"; \
	  $(top_builddir)/build-aux/xml-validate-10 $(top_srcdir)/src/its.xsd10 $(srcdir)/$$file || exit 1; \
	  echo "$(top_builddir)/build-aux/xml-validate-11 $(top_srcdir)/src/its.xsd11 $(srcdir)/$$file"; \
	  $(top_builddir)/build-aux/xml-validate-11 $(top_srcdir)/src/its.xsd11 $(srcdir)/$$file || exit 1; \
	done
	@for file in $(LOC_FILES); do \
	  echo "Checking $$file..."; \
	  echo "$(top_builddir)/build-aux/xml-validate-10 $(top_srcdir)/src/locating-rules.xsd10 $(srcdir)/$$file"; \
	  $(top_builddir)/build-aux/xml-validate-10 $(top_srcdir)/src/locating-rules.xsd10 $(srcdir)/$$file || exit 1; \
	  echo "$(top_builddir)/build-aux/xml-validate-11 $(top_srcdir)/src/locating-rules.xsd11 $(srcdir)/$$file"; \
	  $(top_builddir)/build-aux/xml-validate-11 $(top_srcdir)/src/locating-rules.xsd11 $(srcdir)/$$file || exit 1; \
	done
