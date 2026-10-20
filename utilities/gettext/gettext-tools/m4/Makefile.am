## Makefile for the gettext-tools/m4 directory of GNU gettext
## Copyright (C) 2003-2025 Free Software Foundation, Inc.
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

# Files installed for use by gettextize.
# Note: We do *not* install them into $(datadir)/share/aclocal/ any more,
# because there, 'aclocal --install' (possibly invoked from 'autoreconf')
# would see the files and copy them into the user's package, destroying
# the consistency between po.m4 and po/Makefile.in.in.

pkgdatadir = $(datadir)/gettext
macrosdir = $(pkgdatadir)/m4
macros_DATA = \
  ../../gettext-runtime/gnulib-m4/host-cpu-c-abi.m4 \
  ../../gettext-runtime/gnulib-m4/iconv.m4 \
  ../../gettext-runtime/gnulib-m4/intlmacosx.m4 \
  ../../gettext-runtime/gnulib-m4/lib-ld.m4 \
  ../../gettext-runtime/gnulib-m4/lib-link.m4 \
  ../../gettext-runtime/gnulib-m4/lib-prefix.m4 \
  ../../gettext-runtime/m4/build-to-host.m4 \
  ../../gettext-runtime/m4/gettext.m4 \
  ../../gettext-runtime/m4/nls.m4 \
  ../../gettext-runtime/m4/po.m4 \
  ../../gettext-runtime/m4/progtest.m4

# Files installed for the examples.

examplesbuildauxdir = $(docdir)/examples/build-aux
examplesbuildaux_DATA = \
  ../gnulib-m4/javacomp.m4 ../gnulib-m4/javaexec.m4 \
  ../gnulib-m4/csharp.m4 ../gnulib-m4/csharpcomp.m4 ../gnulib-m4/csharpexec.m4 csharpexec-test.exe \
  ../gnulib-m4/gocomp.m4 \
  ../gnulib-m4/dcomp.m4 \
  ../gnulib-m4/modula2comp.m4

# Generate this list with
# find . -type f -name '*.m4' -printf '  %f\n' | sort | tr '\012' @ | sed 's/@$/%/;s/@/ \\@/g' | tr @% '\012\012'
EXTRA_DIST = \
  README \
  csharpexec-test.cs csharpexec-test.exe \
  exported.m4 \
  hostname.m4 \
  locale-de.m4


# Files installed for use by the old intltool.m4.
# Listing nls.m4 is OK here, since it has essentially not changed over 20 years
# and probably won't change in the future either.

aclocaldir = @aclocaldir@
aclocal_DATA = \
  ../../gettext-runtime/m4/nls.m4


# List of all file names that we ever stored in $(aclocaldir),
# minus those that we still store in $(aclocaldir).
OLD_M4_FILES = \
  build-to-host.m4 \
  codeset.m4 \
  extern-inline.m4 \
  fcntl-o.m4 \
  gettext.m4 \
  glibc2.m4 \
  glibc21.m4 \
  host-cpu-c-abi.m4 \
  iconv.m4 \
  intdiv0.m4 \
  intl.m4 \
  intldir.m4 \
  intlmacosx.m4 \
  intmax.m4 \
  inttypes.m4 \
  inttypes-h.m4 \
  inttypes-pri.m4 \
  inttypes_h.m4 \
  isc-posix.m4 \
  lcmessage.m4 \
  lib-ld.m4 \
  lib-link.m4 \
  lib-prefix.m4 \
  lock.m4 \
  longdouble.m4 \
  longlong.m4 \
  po.m4 \
  printf-posix.m4 \
  progtest.m4 \
  signed.m4 \
  size_max.m4 \
  stdint_h.m4 \
  threadlib.m4 \
  uintmax_t.m4 \
  ulonglong.m4 \
  visibility.m4 \
  wchar_t.m4 \
  wint_t.m4 \
  xsize.m4

# We don't install the *.m4 files in $(aclocaldir) any more.
install-data-local: uninstall-macros
uninstall-local: uninstall-macros
uninstall-macros:
	for f in $(OLD_M4_FILES); do \
	  rm -f '$(DESTDIR)@aclocaldir@'/$$f; \
	done
