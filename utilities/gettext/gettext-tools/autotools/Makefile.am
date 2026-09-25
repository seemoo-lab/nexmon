## Makefile for the gettext-tools/autotools subdirectory of GNU gettext
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

bin_SCRIPTS = autopoint

DISTCLEANFILES += autopoint convert-archive

CLEANFILES += \
  archive.dir.tar.gz \
  archive.dir.tar.bz2 \
  archive.dir.tar.xz \
  archive.cvs.tar.gz \
  archive.git.tar.gz

EXTRA_DIST += add-to-archive cvsuser.c archive.dir.tar

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = $(ARCHIVE_@ARCHIVE_FORMAT@)
ARCHIVE_dirgz = archive.dir.tar.gz
ARCHIVE_dirbz2 = archive.dir.tar.bz2
ARCHIVE_dirxz = archive.dir.tar.xz
ARCHIVE_git = archive.git.tar.gz
ARCHIVE_cvs = archive.cvs.tar.gz

# The archive.dir.tar file appears in release tarballs.
# The GNU Coding Standards say in
# <https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html>:
#   "GNU distributions usually contain some files which are not source files
#    ... . Since these files normally appear in the source directory, they
#    should always appear in the source directory, not in the build directory.
#    So Makefile rules to update them should put the updated files in the
#    source directory."
# Therefore we put this file in the source directory, not the build directory.

# The archive.dir.tar.gz is generated from archive.dir.tar.
archive.dir.tar.gz: $(srcdir)/archive.dir.tar
	gzip -c -9 < '$(srcdir)'/archive.dir.tar > $@-t && mv $@-t $@
MOSTLYCLEANFILES += archive.dir.tar.gz-t

# The archive.dir.tar.bz2 is generated from archive.dir.tar.
archive.dir.tar.bz2: $(srcdir)/archive.dir.tar
	bzip2 -c -9 < '$(srcdir)'/archive.dir.tar > $@-t && mv $@-t $@
MOSTLYCLEANFILES += archive.dir.tar.bz2-t

# The archive.dir.tar.xz is generated from archive.dir.tar.
archive.dir.tar.xz: $(srcdir)/archive.dir.tar
	xz -c -5 < '$(srcdir)'/archive.dir.tar > $@-t && mv $@-t $@
MOSTLYCLEANFILES += archive.dir.tar.xz-t

# The archive.git.tar.gz is generated from archive.dir.tar.
archive.git.tar.gz: $(srcdir)/archive.dir.tar
	./convert-archive dir git '$(srcdir)'/archive.dir.tar $@

# The archive.cvs.tar.gz is generated from archive.dir.tar.
archive.cvs.tar.gz: $(srcdir)/archive.dir.tar
	./convert-archive dir cvs '$(srcdir)'/archive.dir.tar $@
