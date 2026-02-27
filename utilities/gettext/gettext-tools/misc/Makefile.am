## Makefile for the gettext-tools/misc subdirectory of GNU gettext
## Copyright (C) 1995-1997, 2000-2003, 2005-2007, 2009-2010, 2015-2016 Free
## Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.2 gnits
EXTRA_DIST =
MOSTLYCLEANFILES =
CLEANFILES =
DISTCLEANFILES =


# Scripts for maintainers.

bin_SCRIPTS = gettextize autopoint

DISTCLEANFILES += gettextize autopoint convert-archive

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

# The archive.dir.tar is not in the CVS. Alpha-testers must use the one from
# an already installed release.
archive.dir.tar:
	if test -f $(gettextsrcdir)/archive.dir.tar.gz; then \
	  gzip -d -c < $(gettextsrcdir)/archive.dir.tar.gz > $@-t && mv $@-t $@; \
	else \
	  if test -f $(gettextsrcdir)/archive.dir.tar.bz2; then \
	    bzip2 -d -c < $(gettextsrcdir)/archive.dir.tar.bz2 > $@-t && mv $@-t $@; \
	  else \
	    if test -f $(gettextsrcdir)/archive.dir.tar.xz; then \
	      xz -d -c < $(gettextsrcdir)/archive.dir.tar.xz > $@-t && mv $@-t $@; \
	    else \
	      echo "archive.dir.tar is not under version control. Please copy the archive.dir.tar.{gz,bz2,xz} from an already installed release to gettext-tools/misc/ and decompress it there." 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	fi
MOSTLYCLEANFILES += archive.dir.tar-t

# The archive.dir.tar.gz is generated from archive.dir.tar.
archive.dir.tar.gz: archive.dir.tar
	if test -f archive.dir.tar; then \
	  inputfile=archive.dir.tar; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar; \
	fi; \
	gzip -c -9 < "$$inputfile" > $@-t && mv $@-t $@
MOSTLYCLEANFILES += archive.dir.tar.gz-t

# The archive.dir.tar.bz2 is generated from archive.dir.tar.
archive.dir.tar.bz2: archive.dir.tar
	if test -f archive.dir.tar; then \
	  inputfile=archive.dir.tar; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar; \
	fi; \
	bzip2 -c -9 < "$$inputfile" > $@-t && mv $@-t $@
MOSTLYCLEANFILES += archive.dir.tar.bz2-t

# The archive.dir.tar.xz is generated from archive.dir.tar.
archive.dir.tar.xz: archive.dir.tar
	if test -f archive.dir.tar; then \
	  inputfile=archive.dir.tar; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar; \
	fi; \
	xz -c -5 < "$$inputfile" > $@-t && mv $@-t $@
MOSTLYCLEANFILES += archive.dir.tar.xz-t

# The archive.git.tar.gz is generated from archive.dir.tar.
archive.git.tar.gz: archive.dir.tar
	if test -f archive.dir.tar; then \
	  inputfile=archive.dir.tar; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar; \
	fi; \
	./convert-archive dir git "$$inputfile" $@

# The archive.cvs.tar.gz is generated from archive.dir.tar.
archive.cvs.tar.gz: archive.dir.tar
	if test -f archive.dir.tar; then \
	  inputfile=archive.dir.tar; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar; \
	fi; \
	./convert-archive dir cvs "$$inputfile" $@

# Emacs Lisp code.

lisp_LISP = start-po.el po-mode.el po-compat.el

EXTRA_DIST += $(lisp_LISP)
