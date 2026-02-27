## Makefile for the gettext-tools/examples/po subdirectory of GNU gettext
## Copyright (C) 1995-1998, 2000-2007, 2009, 2016 Free Software Foundation,
## Inc.
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

AUTOMAKE_OPTIONS = 1.5 gnits

# List of files which contain translatable strings.
POTFILES = \
  hello-c/hello.c \
  hello-c-gnome/hello.c \
  hello-c-gnome3/hello.c \
  hello-c-gnome3/hello.desktop.in.in \
  hello-c-gnome3/hello.gschema.xml \
  hello-c-gnome3/hello.ui \
  hello-c++/hello.cc \
  hello-c++-qt/hello.cc \
  hello-c++-kde/hello.cc \
  hello-c++-kde/hellowindow.h \
  hello-c++-kde/hellowindow.cc \
  hello-c++-gnome/hello.cc \
  hello-c++-wxwidgets/hello.cc \
  hello-objc/hello.m \
  hello-objc-gnustep/main.m \
  hello-objc-gnustep/AppController.h \
  hello-objc-gnustep/AppController.m \
  hello-objc-gnustep/Hello.h \
  hello-objc-gnustep/Hello.m \
  hello-objc-gnome/hello.m \
  hello-sh/hello.sh \
  hello-python/hello.py.in \
  hello-clisp/hello.lisp.in \
  hello-librep/hello.jl.in \
  hello-guile/hello.scm \
  hello-smalltalk/hello.st.in \
  hello-java/Hello.java \
  hello-java-awt/Hello.java \
  hello-java-swing/Hello.java \
  hello-java-qtjambi/Hello.java \
  hello-csharp/hello.cs \
  hello-csharp-forms/hello.cs \
  hello-gawk/hello.awk \
  hello-pascal/hello.pas \
  hello-ycp/hello.ycp \
  hello-tcl/hello.tcl \
  hello-tcl-tk/hello.tcl \
  hello-perl/hello-1.pl.in \
  hello-perl/hello-2.pl.in \
  hello-php/hello.php

# List of small POT files.
SMALLPOTS = \
  hello-c.pot \
  hello-c-gnome.pot \
  hello-c-gnome3.pot \
  hello-c++.pot \
  hello-c++-qt.pot \
  hello-c++-kde.pot \
  hello-c++-gnome.pot \
  hello-c++-wxwidgets.pot \
  hello-objc.pot \
  hello-objc-gnustep.pot \
  hello-objc-gnome.pot \
  hello-sh.pot \
  hello-python.pot \
  hello-clisp.pot \
  hello-librep.pot \
  hello-guile.pot \
  hello-smalltalk.pot \
  hello-java.pot \
  hello-java-awt.pot \
  hello-java-swing.pot \
  hello-java-qtjambi.pot \
  hello-csharp.pot \
  hello-csharp-forms.pot \
  hello-gawk.pot \
  hello-pascal.pot \
  hello-ycp.pot \
  hello-tcl.pot \
  hello-tcl-tk.pot \
  hello-perl.pot \
  hello-php.pot

# This is a synthetic domain name.
DOMAIN = gettext-examples

# This is the copyright holder that gets inserted into the header of the
# $(DOMAIN).pot file.  Set this to the copyright holder of the surrounding
# package.  (Note that the msgstr strings, extracted from the package's
# sources, belong to the copyright holder of the package.)  Translators are
# expected to transfer the copyright for their translations to this person
# or entity, or to disclaim their copyright.  The empty string stands for
# the public domain; in this case the translators are expected to disclaim
# their copyright.
COPYRIGHT_HOLDER = Yoyodyne, Inc.

# This is the email address or URL to which the translators shall report
# bugs in the untranslated strings:
# - Strings which are not entire sentences, see the maintainer guidelines
#   in the GNU gettext documentation, section 'Preparing Strings'.
# - Strings which use unclear terms or require additional context to be
#   understood.
# - Strings which make invalid assumptions about notation of date, time or
#   money.
# - Pluralisation problems.
# - Incorrect English spelling.
# - Incorrect formatting.
# It can be your email address, or a mailing list address where translators
# can write to without being subscribed, or the URL of a web page through
# which the translators can contact you.
MSGID_BUGS_ADDRESS = bug-gnu-gettext@gnu.org

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

# These options get passed to msgmerge.
# Useful options are in particular:
#   --previous            to keep previous msgids of translated messages,
#   --quiet               to reduce the verbosity.
MSGMERGE_OPTIONS = --quiet

MSGMERGE = msgmerge
MSGMERGE_UPDATE = @MSGMERGE@ --update
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

# This is computed as $(foreach file, $(POTFILES), $(top_srcdir)/$(file))
POTFILES_DEPS = @POTFILES_DEPS@

# This is computed as $(foreach lang, $(LINGUAS), $(srcdir)/$(lang).po)
POFILES = @POFILES@
# This is computed as $(foreach lang, $(LINGUAS), $(lang).po-update)
UPDATEPOFILES = @UPDATEPOFILES@
# This is computed as $(foreach lang, $(LINGUAS), $(lang).nop)
DUMMYPOFILES = @DUMMYPOFILES@

SUFFIXES = .po .sed .sin .nop .po-update

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


all-local: all-local-@USE_NLS@

all-local-yes: stamp-po
all-local-no:

# $(srcdir)/$(DOMAIN).pot is only created when needed. When xgettext finds no
# internationalized messages, no $(srcdir)/$(DOMAIN).pot is created (because
# we don't want to bother translators with empty POT files). We assume that
# LINGUAS is empty in this case, i.e. $(POFILES) is empty. In this case,
# stamp-po is a nop (i.e. a phony target).

# stamp-po is a timestamp denoting the last time at which the CATALOGS have
# been loosely updated. Its purpose is that when a developer or translator
# checks out the package via CVS, and the $(DOMAIN).pot file is not in CVS,
# "make" will update the $(DOMAIN).pot and the $(CATALOGS), but subsequent
# invocations of "make" will do nothing. This timestamp would not be necessary
# if updating the $(CATALOGS) would always touch them; however, the rule for
# $(POFILES) has been designed to not touch files that don't need to be
# changed.
stamp-po: $(srcdir)/$(DOMAIN).pot
	@test ! -f $(srcdir)/$(DOMAIN).pot || { \
	  echo "touch stamp-po" && \
	  echo timestamp > stamp-poT && \
	  mv stamp-poT stamp-po; \
	}

# Note: Target 'all' must not depend on target '$(DOMAIN).pot-update',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
$(DOMAIN).pot-update: $(POTFILES_DEPS) $(SMALLPOTS) remove-potcdate.sed
	if test -n '$(MSGID_BUGS_ADDRESS)'; then \
	  msgid_bugs_address='$(MSGID_BUGS_ADDRESS)'; \
	else \
	  msgid_bugs_address='$(PACKAGE_BUGREPORT)'; \
	fi; \
	$(XGETTEXT) --default-domain=$(DOMAIN) \
	  --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS) \
	  --copyright-holder='$(COPYRIGHT_HOLDER)' \
	  --msgid-bugs-address="$$msgid_bugs_address" \
	  --directory='$(srcdir)' \
	  $(SMALLPOTS)
	test ! -f $(DOMAIN).po || { \
	  if test -f $(srcdir)/$(DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
	    if cmp $(DOMAIN).1po $(DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).po; \
	    else \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(srcdir)/$(DOMAIN).pot && \
	      mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	  fi; \
	}


hello-c.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c

hello-c-gnome.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c-gnome

hello-c-gnome3.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c-gnome3

hello-c++.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c++

hello-c++-qt.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c++-qt

hello-c++-kde.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c++-kde

hello-c++-gnome.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c++-gnome

hello-c++-wxwidgets.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-c++-wxwidgets

hello-objc.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-objc

hello-objc-gnustep.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-objc-gnustep

hello-objc-gnome.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-objc-gnome

hello-sh.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-sh

hello-python.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-python

hello-clisp.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-clisp

hello-librep.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-librep

hello-guile.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-guile

hello-smalltalk.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-smalltalk

hello-java.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-java

hello-java-awt.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-java-awt

hello-java-swing.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-java-swing

hello-java-qtjambi.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-java-qtjambi

hello-csharp.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-csharp

hello-csharp-forms.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-csharp-forms

hello-gawk.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-gawk

hello-pascal.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-pascal

hello-ycp.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-ycp

hello-tcl.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-tcl

hello-tcl-tk.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-tcl-tk

hello-perl.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-perl

hello-php.pot : $(POTFILES_DEPS)
	$(SHELL) '$(srcdir)/xsmallpot.sh' '$(srcdir)' hello-php


# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(srcdir)/$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target rebuilds a PO file if $(DOMAIN).pot has changed.
# Note that a PO file is not touched if it doesn't need to be changed.
$(POFILES): $(srcdir)/$(DOMAIN).pot
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) $${lang}.po $(DOMAIN).pot"; \
	cd $(srcdir) && $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) $${lang}.po $(DOMAIN).pot


html ID:

MOSTLYCLEANFILES =
MOSTLYCLEANFILES += remove-potcdate.sed
MOSTLYCLEANFILES += stamp-poT
MOSTLYCLEANFILES += core core.* *.stackdump $(DOMAIN).po $(DOMAIN).1po $(DOMAIN).2po *.new.po
MOSTLYCLEANFILES += *.o

MAINTAINERCLEANFILES = stamp-po $(SMALLPOTS)

EXTRA_DIST = remove-potcdate.sin xsmallpot.sh mmsmallpo.sh LINGUAS \
  $(POFILES)

# Hidden from automake, but really activated. Works around an automake-1.5 bug.
#distdir: distdir1
distdir1:
	if test -f $(srcdir)/$(DOMAIN).pot; then \
	  for file in $(DOMAIN).pot stamp-po $(SMALLPOTS); do \
	    if test -f $$file; then d=.; else d=$(srcdir); fi; \
	    cp -p $$d/$$file $(distdir)/$$file || exit 1; \
	  done; \
	fi

update-po: Makefile
	$(MAKE) $(DOMAIN).pot-update
	test -z "$(UPDATEPOFILES)" || $(MAKE) $(UPDATEPOFILES)

# General rule for updating PO files.

.nop.po-update:
	@lang=`echo $@ | sed -e 's/\.po-update$$//'`; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE) $(MSGMERGE_OPTIONS) $$lang.po $(DOMAIN).pot -o $$lang.new.po"; \
	cd $(srcdir); \
	if $(MSGMERGE) $(MSGMERGE_OPTIONS) $$lang.po $(DOMAIN).pot -o $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	  $(MAKE) $(SMALLPOFILES_FOR_lang) LL=$$lang; \
	else \
	  echo "msgmerge for $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

$(DUMMYPOFILES):


# Rules for updating small PO files in the other directories.
# Depend on LL being set.

SMALLPOFILES_FOR_lang = \
  ../hello-c/po/$$lang.po \
  ../hello-c-gnome/po/$$lang.po \
  ../hello-c-gnome3/po/$$lang.po \
  ../hello-c++/po/$$lang.po \
  ../hello-c++-qt/po/$$lang.po \
  ../hello-c++-kde/po/$$lang.po \
  ../hello-c++-gnome/po/$$lang.po \
  ../hello-c++-wxwidgets/po/$$lang.po \
  ../hello-objc/po/$$lang.po \
  ../hello-objc-gnustep/po/$$lang.po \
  ../hello-objc-gnome/po/$$lang.po \
  ../hello-sh/po/$$lang.po \
  ../hello-python/po/$$lang.po \
  ../hello-clisp/po/$$lang.po \
  ../hello-librep/po/$$lang.po \
  ../hello-guile/po/$$lang.po \
  ../hello-smalltalk/po/$$lang.po \
  ../hello-java/po/$$lang.po \
  ../hello-java-awt/po/$$lang.po \
  ../hello-java-swing/po/$$lang.po \
  ../hello-java-qtjambi/po/$$lang.po \
  ../hello-csharp/po/$$lang.po \
  ../hello-csharp-forms/po/$$lang.po \
  ../hello-gawk/po/$$lang.po \
  ../hello-pascal/po/$$lang.po \
  ../hello-ycp/po/$$lang.po \
  ../hello-tcl/po/$$lang.po \
  ../hello-tcl-tk/po/$$lang.po \
  ../hello-perl/po/$$lang.po \
  ../hello-php/po/$$lang.po

../hello-c/po/$(LL).po: hello-c.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c $(LL)

../hello-c-gnome/po/$(LL).po: hello-c-gnome.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c-gnome $(LL)

../hello-c-gnome3/po/$(LL).po: hello-c-gnome3.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c-gnome3 $(LL)

../hello-c++/po/$(LL).po: hello-c++.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c++ $(LL)

../hello-c++-qt/po/$(LL).po: hello-c++-qt.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c++-qt $(LL)

../hello-c++-kde/po/$(LL).po: hello-c++-kde.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c++-kde $(LL)

../hello-c++-gnome/po/$(LL).po: hello-c++-gnome.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c++-gnome $(LL)

../hello-c++-wxwidgets/po/$(LL).po: hello-c++-wxwidgets.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-c++-wxwidgets $(LL)

../hello-objc/po/$(LL).po: hello-objc.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-objc $(LL)

../hello-objc-gnustep/po/$(LL).po: hello-objc-gnustep.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-objc-gnustep $(LL)

../hello-objc-gnome/po/$(LL).po: hello-objc-gnome.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-objc-gnome $(LL)

../hello-sh/po/$(LL).po: hello-sh.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-sh $(LL)

../hello-python/po/$(LL).po: hello-python.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-python $(LL)

../hello-clisp/po/$(LL).po: hello-clisp.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-clisp $(LL)

../hello-librep/po/$(LL).po: hello-librep.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-librep $(LL)

../hello-guile/po/$(LL).po: hello-guile.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-guile $(LL)

../hello-smalltalk/po/$(LL).po: hello-smalltalk.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-smalltalk $(LL)

../hello-java/po/$(LL).po: hello-java.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-java $(LL)

../hello-java-awt/po/$(LL).po: hello-java-awt.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-java-awt $(LL)

../hello-java-swing/po/$(LL).po: hello-java-swing.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-java-swing $(LL)

../hello-java-qtjambi/po/$(LL).po: hello-java-qtjambi.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-java-qtjambi $(LL)

../hello-csharp/po/$(LL).po: hello-csharp.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-csharp $(LL)

../hello-csharp-forms/po/$(LL).po: hello-csharp-forms.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-csharp-forms $(LL)

../hello-gawk/po/$(LL).po: hello-gawk.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-gawk $(LL)

../hello-pascal/po/$(LL).po: hello-pascal.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-pascal $(LL)

../hello-ycp/po/$(LL).po: hello-ycp.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-ycp $(LL)

../hello-tcl/po/$(LL).po: hello-tcl.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-tcl $(LL)

../hello-tcl-tk/po/$(LL).po: hello-tcl-tk.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-tcl-tk $(LL)

../hello-perl/po/$(LL).po: hello-perl.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-perl $(LL)

../hello-php/po/$(LL).po: hello-php.pot $(LL).po
	$(SHELL) mmsmallpo.sh hello-php $(LL)
