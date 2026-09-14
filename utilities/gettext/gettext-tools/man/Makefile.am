## Makefile for the gettext-tools/man subdirectory of GNU gettext
## Copyright (C) 2001-2025 Free Software Foundation, Inc.
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

VERSION = @VERSION@

AUTOMAKE_OPTIONS = 1.2 gnits
EXTRA_DIST =

# A manual page for each of the bin_PROGRAMS in src/Makefile.am
# and for each of the bin_SCRIPTS in wizard/Makefile.am, autotools/Makefile.am.

man_aux = \
msgcmp.x msgfmt.x msgmerge.x msgunfmt.x xgettext.x \
msgattrib.x msgcat.x msgcomm.x msgconv.x msgen.x msgexec.x msgfilter.x \
msggrep.x msginit.x msgpre.x msguniq.x \
recode-sr-latin.x \
spit.x \
gettextize.x autopoint.x po-fetch.x

# Likewise.

man_MAN1SRC = \
msgcmp.1 msgfmt.1 msgmerge.1 msgunfmt.1 xgettext.1 \
msgattrib.1 msgcat.1 msgcomm.1 msgconv.1 msgen.1 msgexec.1 msgfilter.1 \
msggrep.1 msginit.1 msgpre.1 msguniq.1 \
recode-sr-latin.1 \
spit.1
man_MAN1WIZARD = \
gettextize.1
man_MAN1AUTOTOOLS = \
autopoint.1
man_MAN1MISC = \
po-fetch.1
man_MAN1 = $(man_MAN1SRC) $(man_MAN1WIZARD) $(man_MAN1AUTOTOOLS) $(man_MAN1MISC)
man_MANS = $(man_MAN1)

man_HTML = \
msgcmp.1.html msgfmt.1.html msgmerge.1.html msgunfmt.1.html xgettext.1.html \
msgattrib.1.html msgcat.1.html msgcomm.1.html msgconv.1.html msgen.1.html \
msgexec.1.html msgfilter.1.html msggrep.1.html msginit.1.html msgpre.1.html \
msguniq.1.html \
recode-sr-latin.1.html \
spit.1.html \
gettextize.1.html autopoint.1.html po-fetch.1.html

EXTRA_DIST += help2man $(man_aux) po-fetch.1.diff $(man_MANS) $(man_HTML)
MAINTAINERCLEANFILES = $(man_MANS) $(man_HTML)

PERL = @PERL@
RM = rm -f

# help2man 1.24 or newer.
HELP2MAN = $(PERL) -w -- $(srcdir)/help2man

# groff 1.17 or newer.
MAN2HTML = groff -mandoc -Thtml
# Some versions of groff -Thtml map '\-' to '&minus;' = U+2212, which yields
# the wrong result when copy&pasting command-line options and such.
# Also remove 'CreationDate:' lines, for reproducible builds.
MAN2HTML_POSTPROCESS = sed -e 's/\&minus;/-/g' -e '/CreationDate:/d'

UPDATEMODE =


gt_man2html = \
	if test -f $${srcdir}$$m; then \
	  $(MAN2HTML) $${srcdir}$$m > t-$@ && \
	  $(MAN2HTML_POSTPROCESS) < t-$@ > t2-$@ && \
	  mv t2-$@ $@; \
	  rm -f t-$@ t2-$@; \
	fi


# We distribute both the man pages and their HTML equivalent.
# The user can generate the parts, via
#   make man
#   make html

all-local: html-local
install-data-local: install-html
installdirs-local: installdirs-html
uninstall-local: uninstall-html


# Man pages.

# The progname.x files contain some extra information not found in the
# "progname --help" output.  progname.1 depends on the source, not the
# binary; we don't need to regenerate the binary when any source file
# changes, only the main one.

$(man_MAN1SRC): help2man $(top_srcdir)/../.version
	progname=`echo $@ | sed -e 's/\.in$$//' -e 's/\.1$$//'`; \
	IN_HELP2MAN=1 $(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" \
	  ../src/$${progname}$(EXEEXT) $(srcdir)/$${progname}.x $@

msgcmp.1: msgcmp.x ../src/msgcmp.c
msgfmt.1: msgfmt.x ../src/msgfmt.c
msgmerge.1: msgmerge.x ../src/msgmerge.c
msgunfmt.1: msgunfmt.x ../src/msgunfmt.c
xgettext.1: xgettext.x ../src/xgettext.c
msgattrib.1: msgattrib.x ../src/msgattrib.c
msgcat.1: msgcat.x ../src/msgcat.c
msgcomm.1: msgcomm.x ../src/msgcomm.c
msgconv.1: msgconv.x ../src/msgconv.c
msgen.1: msgen.x ../src/msgen.c
msgexec.1: msgexec.x ../src/msgexec.c
msgfilter.1: msgfilter.x ../src/msgfilter.c
msggrep.1: msggrep.x ../src/msggrep.c
msginit.1: msginit.x ../src/msginit.c
msgpre.1: msgpre.x ../src/msgpre.c
msguniq.1: msguniq.x ../src/msguniq.c
recode-sr-latin.1: recode-sr-latin.x ../src/recode-sr-latin.c
spit.1: spit.x ../src/spit.c

$(man_MAN1WIZARD): help2man $(top_srcdir)/../.version
	progname=`echo $@ | sed -e 's/\.in$$//' -e 's/\.1$$//'`; \
	IN_HELP2MAN=1 $(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" \
	  ../wizard/$${progname} $(srcdir)/$${progname}.x $@

gettextize.1: gettextize.x ../wizard/gettextize.in

$(man_MAN1AUTOTOOLS): help2man $(top_srcdir)/../.version
	progname=`echo $@ | sed -e 's/\.in$$//' -e 's/\.1$$//'`; \
	IN_HELP2MAN=1 $(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN) --version-option=--version-without-config" \
	  ../autotools/$${progname} $(srcdir)/$${progname}.x $@

autopoint.1: autopoint.x ../autotools/autopoint.in

$(man_MAN1MISC): help2man $(top_srcdir)/../.version
	progname=`echo $@ | sed -e 's/\.in$$//' -e 's/\.1$$//'`; \
	IN_HELP2MAN=1 $(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" \
	  ../misc/$${progname} $(srcdir)/$${progname}.x $@
	patch $@ < $(srcdir)/po-fetch.1.diff

po-fetch.1: po-fetch.x ../misc/po-fetch.in po-fetch.1.diff


# Man pages in HTML format.

html-local: $(man_HTML)

$(man_HTML):
	srcdir=''; \
	progname=`echo $@ | sed -e 's/\.1\.html$$//'`; \
	m=$${progname}.1; \
	test -f ./$$m || srcdir=$(srcdir)/; \
	$(gt_man2html)

msgcmp.1.html: msgcmp.1
msgfmt.1.html: msgfmt.1
msgmerge.1.html: msgmerge.1
msgunfmt.1.html: msgunfmt.1
xgettext.1.html: xgettext.1
msgattrib.1.html: msgattrib.1
msgcat.1.html: msgcat.1
msgcomm.1.html: msgcomm.1
msgconv.1.html: msgconv.1
msgen.1.html: msgen.1
msgexec.1.html: msgexec.1
msgfilter.1.html: msgfilter.1
msggrep.1.html: msggrep.1
msginit.1.html: msginit.1
msgpre.1.html: msgpre.1
msguniq.1.html: msguniq.1
recode-sr-latin.1.html: recode-sr-latin.1
spit.1.html: spit.1
gettextize.1.html: gettextize.1
autopoint.1.html: autopoint.1
po-fetch.1.html: po-fetch.1

install-html-local:
	$(MKDIR_P) $(DESTDIR)$(htmldir)
	for file in $(man_HTML); do \
	  if test -f $$file; then dir=.; else dir=$(srcdir); fi; \
	  $(INSTALL_DATA) $$dir/$$file $(DESTDIR)$(htmldir)/$$file; \
	done

installdirs-html:
	$(MKDIR_P) $(DESTDIR)$(htmldir)

uninstall-html:
	for file in $(man_HTML); do \
	  $(RM) $(DESTDIR)$(htmldir)/$$file; \
	done
