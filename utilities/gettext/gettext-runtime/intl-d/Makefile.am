## Makefile for the gettext-runtime/intl-d subdirectory of GNU gettext
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

RM = rm -f

dincludedir = $(includedir)/d

if BUILD_D
lib_LIBRARIES = libintl_d.a
endif
libintl_d_a_SOURCES = \
  gnu/libintl/package.d \
  gnu/libintl/libintl.d \
  gnu/libintl/internal/low.d
if BUILD_D
libintl_d_a_LIBADD = package.o libintl.o low.o
libintl_d_a_DEPENDENCIES = package.o libintl.o low.o

# Rules for compiling a .d file, that work even without a VPATH variable.
package.o: $(srcdir)/gnu/libintl/package.d
	$(SHELL) ../dcomp.sh -I$(srcdir) -c $(srcdir)/gnu/libintl/package.d
libintl.o: $(srcdir)/gnu/libintl/libintl.d
	$(SHELL) ../dcomp.sh -I$(srcdir) -c $(srcdir)/gnu/libintl/libintl.d
low.o: $(srcdir)/gnu/libintl/internal/low.d
	$(SHELL) ../dcomp.sh -I$(srcdir) -c $(srcdir)/gnu/libintl/internal/low.d

install-data-local: install-d-sources
installdirs-local: installdirs-d-sources
uninstall-local: uninstall-d-sources

install-d-sources:
	$(MKDIR_P) $(DESTDIR)$(dincludedir)
	$(MKDIR_P) $(DESTDIR)$(dincludedir)/gnu/libintl/internal
	@for f in $(libintl_d_a_SOURCES); do \
	  echo "$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(dincludedir)/$$f"; \
	  $(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(dincludedir)/$$f; \
	done

installdirs-d-sources:
	$(MKDIR_P) $(DESTDIR)$(dincludedir)
	$(MKDIR_P) $(DESTDIR)$(dincludedir)/gnu/libintl/internal

uninstall-d-sources:
	@for f in $(libintl_d_a_SOURCES); do \
	  echo "$(RM) $(DESTDIR)$(dincludedir)/$$f"; \
	  $(RM) $(DESTDIR)$(dincludedir)/$$f; \
	done

endif
