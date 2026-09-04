# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# List of files which contain translatable strings.
POTFILES = \
  Hello.java

# Usually the message domain is the same as the package name.
DOMAIN = $(PACKAGE)

# These options get passed to xgettext.
XGETTEXT_OPTIONS =

# This is the copyright holder that gets inserted into the header of the
# $(DOMAIN).pot file.  Set this to the copyright holder of the surrounding
# package.  (Note that the msgid strings, extracted from the package's
# sources, belong to the copyright holder of the package.)  Translators are
# expected to transfer the copyright for their translations to this person
# or entity, or to disclaim their copyright.  The empty string stands for
# the public domain; in this case the translators are expected to disclaim
# their copyright.
COPYRIGHT_HOLDER = Yoyodyne, Inc.

# This tells whether or not to prepend "GNU " prefix to the package
# name that gets inserted into the header of the $(DOMAIN).pot file.
# Possible values are "yes", "no", or empty.  If it is empty, try to
# detect it automatically by scanning the files in $(top_srcdir) for
# "GNU packagename" string.
PACKAGE_GNU = no

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
MSGID_BUGS_ADDRESS = bug-gettext@gnu.org

# These options get passed to msgmerge.
# Useful options are in particular:
#   --previous            to keep previous msgids of translated messages
MSGMERGE_OPTIONS =

# These options get passed to msginit.
# If you want to disable line wrapping when writing PO files, add
# --no-wrap to MSGMERGE_OPTIONS, XGETTEXT_OPTIONS, and
# MSGINIT_OPTIONS.
MSGINIT_OPTIONS =

MSGMERGE = @MSGMERGE@
MSGMERGE_FOR_MSGFMT_OPTION = @MSGMERGE_FOR_MSGFMT_OPTION@
MSGCAT = msgcat
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

# This is computed as $(foreach file, $(POTFILES), $(top_srcdir)/$(file))
POTFILES_DEPS != for file in $(POTFILES); do echo $(top_srcdir)/$$file; done

# The set of available translations.
ALL_LINGUAS != if test -f $(srcdir)/LINGUAS; then \
                 sed -e '/^\#/d' < $(srcdir)/LINGUAS; \
               else \
                 echo $(LINGUAS); \
               fi
# This is computed as $(foreach lang, $(ALL_LINGUAS), $(srcdir)/$(lang).po)
POFILES != for lang in $(ALL_LINGUAS); do echo $(srcdir)/$$lang.po; done
# This is computed as $(foreach lang, $(ALL_LINGUAS), $(lang).nop)
DUMMYPOFILES != for lang in $(ALL_LINGUAS); do echo $$lang.nop; done
# This is computed as $(foreach lang, $(ALL_LINGUAS), $(srcdir)/$(DOMAIN)_$(lang).properties)
PROPERTIESFILES != for lang in $(ALL_LINGUAS); do echo $(srcdir)/$(DOMAIN)_$$lang.properties; done
# This is computed as $(foreach lang, $(ALL_LINGUAS), $(srcdir)/$(DOMAIN)_$(lang).class)
CLASSFILES != for lang in $(ALL_LINGUAS); do echo $(srcdir)/$(DOMAIN)_$$lang.class; done

# The set of desired translations, as specified by the installer or distributor.
DESIRED_LINGUAS = @DESIRED_LINGUAS@
# The set of translations to install. This is computed based on $(ALL_LINGUAS)
# and $(DESIRED_LINGUAS). It is a subset of $(ALL_LINGUAS).
# We use the presentlang catalog if desiredlang is
#   a. equal to presentlang, or
#   b. a variant of presentlang (because in this case, presentlang can be used
#      as a fallback for messages which are not translated in the desiredlang
#      catalog).
INST_LINGUAS != for presentlang in $(ALL_LINGUAS); do \
                  useit=false; \
                  for desiredlang in $(DESIRED_LINGUAS); do \
                    case "$$desiredlang" in \
                      "$$presentlang" | "$$presentlang"_* | "$$presentlang".* | "$$presentlang"@*) \
                        useit=true ;; \
                    esac; \
                  done; \
                  if $$useit; then echo $$presentlang; fi; \
                done
# This is computed as $(foreach lang, $(INST_LINGUAS), $(DOMAIN)_$(lang).properties)
CATALOGS != for lang in $(INST_LINGUAS); do echo $(DOMAIN)_$$lang.properties; done

SUFFIXES = .po .gpo .nop .po-create

# Which files appear in release tarballs?
#
# The .properties or .class files appear in release tarballs, because the GNU
# Coding Standards say in
# <https://www.gnu.org/prep/standards/html_node/Utilities-in-Makefiles.html>:
#   "The configure script and the Makefile rules for building and installation
#    should not use any utilities directly except these:
#      awk ... true"
# and 'msgfmt' is not in this list.  In other words, when a user - who does not
# have the GNU gettext tools installed - does "make && make install", it should
# install the .properties or .class files.  The only way to achieve this is to
# include them in the release tarballs.
#
# The .pot file appears in release tarballs, because it is needed when a
# translator runs "msginit" to start working on a PO file or when a web-based
# PO editor prepares the initial PO file for a translator.  Translators and
# translation projects should not have to run 'xgettext' on some source code;
# that's not their business, and they would not be in a good position to deal
# with errors.
#
# The .po files appear in release tarballs, because
#   - They are part of the source code of the .properties or .class files, and
#     it would be a bad idea to include the (generated) .properties or .class
#     files without their corresponding source code. Even if the .po files can
#     be found on some translation project's site or in some separate git
#     repository, users and distributors should not have to go that far, to hunt
#     them down.
#   - The .po file is needed when a translator runs "msginit" to continue
#     working on a PO file (in packages that do not use a web-based translation
#     project).
# These .po files are those that the translators submitted or committed in
# version control, or a translation project's daemon committed on behalf of the
# translators.
# Up until 2025, we included the *merged* .po files in the release tarballs.
# These merged .po files, that we now name .gpo files in this Makefile,
# are neither source code nor entirely generated files: They are a mix of
# source code (from the translators) and generated parts (from the .pot file).
# This caused two problems:
#   * Regarding the tarballs: They are "non-source" files in the sense of
#     <https://www.gnu.org/prep/standards/html_node/Releases.html>.
#     Distributors who want to make sure that they rebuild everything from
#     source had a problem here.
#     Also, small changes in the package caused massive line number changes
#     in the .po files.
#   * Regarding version control:
#     Packages that put the .po files under version control had changes in
#     all .po files nearly each time "make dist" was invoked. Leading to
#       - Lots of modified files shown by "git status",
#       - Frequent merge conflicts when merging between branches,
#       - A voluminous version control history.
#     Whereas packages that do not put the .po files under version control
#     had to fetch them from somewhere (usually a translation project),
#     meaning that an 'autopull.sh' step was needed before the 'autogen.sh'
#     step of the package's build system.
# Instead, now, we include the original .po files in the release tarballs.
#
# The .pot file, stamp-po, .po files, and .properties or .class files appear
# in release tarballs. The GNU Coding Standards say in
# <https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html>:
#   "GNU distributions usually contain some files which are not source files
#    ... . Since these files normally appear in the source directory, they
#    should always appear in the source directory, not in the build directory.
#    So Makefile rules to update them should put the updated files in the
#    source directory."
# Therefore we put these files in the source directory, not the build directory.

# This target fetches the PO files, produced by translators, from a
# translations project's site on the internet, and generates the associated
# LINGUAS file (for packages that use such a translations project and where
# this translation project does not already commit the PO files and the LINGUAS
# file into the version control repository of the package automatically).
.PHONY: fetch-po
fetch-po:
	cd $(srcdir) && @SHELL@ ./fetch-po

# This target creates a merged PO file (mostly for debugging purposes).
.po.gpo:
	@lang=`echo $* | sed -e 's,.*/,,'`; \
	echo "rm -f $${lang}.gpo && $(MSGMERGE) $(MSGMERGE_FOR_MSGFMT_OPTION) $(MSGMERGE_OPTIONS) -o $${lang}.gpo $(srcdir)/$${lang}.po $(srcdir)/$(DOMAIN).pot"; \
	rm -f $${lang}.gpo && \
	$(MSGMERGE) $(MSGMERGE_FOR_MSGFMT_OPTION) $(MSGMERGE_OPTIONS) -o $${lang}.gpo $(srcdir)/$${lang}.po $(srcdir)/$(DOMAIN).pot


all-local: $(srcdir)/stamp-po

# $(srcdir)/$(DOMAIN).pot is only created when needed. When xgettext finds no
# internationalized messages, no $(srcdir)/$(DOMAIN).pot is created (because
# we don't want to bother translators with empty POT files). We assume that
# LINGUAS is empty in this case, i.e. $(POFILES) and $(PROPERTIESFILES) are
# empty. In this case, $(srcdir)/stamp-po is a nop (i.e. a phony target).

# $(srcdir)/stamp-po is a timestamp denoting the last time at which the CATALOGS
# have been loosely updated. Its purpose is that when a developer or translator
# checks out the package from a version control system, and the $(DOMAIN).pot
# file is not under version control, "make" will update the $(DOMAIN).pot and
# the $(CATALOGS), but subsequent invocations of "make" will do nothing. This
# timestamp would not be necessary if updating the $(CATALOGS) would always
# touch them; however, the rule for $(DOMAIN).pot-update has been designed to
# not touch $(srcdir)/$(DOMAIN).pot if it doesn't need to be changed.
#
# Note: The name 'stamp-po' is a bit misleading. It does *not* designate a
# timestamp for the *.po files. Rather it is a timestamp for all generated
# files in this directory.
$(srcdir)/stamp-po: $(srcdir)/$(DOMAIN).pot
	test ! -f $(srcdir)/$(DOMAIN).pot || \
	  $(MAKE) update-properties
	@test ! -f $(srcdir)/$(DOMAIN).pot || { \
	  echo "touch $(srcdir)/stamp-po" && \
	  echo timestamp > $(srcdir)/stamp-poT && \
	  mv $(srcdir)/stamp-poT $(srcdir)/stamp-po; \
	}

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
# The determination of whether the package xyz is a GNU one is based on the
# heuristic whether some file in the top level directory mentions "GNU xyz".
# If GNU 'find' is available, we avoid grepping through monster files.
$(DOMAIN).pot-update: $(POTFILES_DEPS)
	package_gnu="$(PACKAGE_GNU)"; \
	test -n "$$package_gnu" || { \
	  if { if (LC_ALL=C find --version) 2>/dev/null | grep GNU >/dev/null; then \
	         LC_ALL=C find -L $(top_srcdir) -maxdepth 1 -type f -size -10000000c -exec grep -i 'GNU $(PACKAGE)' /dev/null '{}' ';' 2>/dev/null; \
	       else \
	         LC_ALL=C grep -i 'GNU $(PACKAGE)' $(top_srcdir)/* 2>/dev/null; \
	       fi; \
	     } | grep -v 'libtool:' >/dev/null; then \
	     package_gnu=yes; \
	   else \
	     package_gnu=no; \
	   fi; \
	}; \
	if test "$$package_gnu" = "yes"; then \
	  package_prefix='GNU '; \
	else \
	  package_prefix=''; \
	fi; \
	if test -n '$(MSGID_BUGS_ADDRESS)'; then \
	  msgid_bugs_address='$(MSGID_BUGS_ADDRESS)'; \
	else \
	  msgid_bugs_address='$(PACKAGE_BUGREPORT)'; \
	fi; \
	case `$(XGETTEXT) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	  '' | 0.[0-9] | 0.[0-9].* | 0.1[0-5] | 0.1[0-5].* | 0.16 | 0.16.[0-1]*) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	      $(XGETTEXT_OPTIONS) @XGETTEXT_EXTRA_OPTIONS@ \
	      $(POTFILES) \
	    ;; \
	  *) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --package-name="$${package_prefix}$(PACKAGE)" \
	      --package-version='$(VERSION)' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	      $(XGETTEXT_OPTIONS) @XGETTEXT_EXTRA_OPTIONS@ \
	      $(POTFILES) \
	    ;; \
	esac
	test ! -f $(DOMAIN).po || { \
	  if test -f $(srcdir)/$(DOMAIN).pot-header; then \
	    sed -e '1,/^#$$/d' < $(DOMAIN).po > $(DOMAIN).1po && \
	    cat $(srcdir)/$(DOMAIN).pot-header $(DOMAIN).1po > $(DOMAIN).po && \
	    rm -f $(DOMAIN).1po \
	    || exit 1; \
	  fi; \
	  if test -f $(srcdir)/$(DOMAIN).pot; then \
	    sed -f $(srcdir)/remove-potcdate.sed < $(srcdir)/$(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f $(srcdir)/remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
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

# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(srcdir)/$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target attempts to build a PO file.
$(POFILES):
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	if test -f "$(srcdir)/$${lang}.po"; then \
	  : ; \
	else \
	  $(MAKE) $${lang}.po-create; \
	fi


html ID:

MOSTLYCLEANFILES =
MOSTLYCLEANFILES += stamp-poT
MOSTLYCLEANFILES += core core.* *.stackdump $(DOMAIN).po $(DOMAIN).1po $(DOMAIN).2po *.gpo *.new.po

MAINTAINERCLEANFILES = \
  $(DOMAIN).pot stamp-po \
  $(DOMAIN).properties $(PROPERTIESFILES) $(CLASSFILES)

EXTRA_DIST = remove-potcdate.sed LINGUAS $(POFILES) $(DOMAIN).properties $(PROPERTIESFILES)

# Hidden from automake, but really activated. Works around an automake-1.5 bug.
#distdir: distdir1
distdir1:
	$(MAKE) update-po
	$(MAKE) $(srcdir)/stamp-po
	@for file in fetch-po; do \
	  if test -f $$file; then d=.; else d=$(srcdir); fi; \
	  cp -p $$d/$$file $(distdir)/$$file || exit 1; \
	done
	@if test -f $(srcdir)/$(DOMAIN).pot; then \
	  for file in $(DOMAIN).pot stamp-po; do \
	    if test -f $$file; then d=.; else d=$(srcdir); fi; \
	    cp -p $$d/$$file $(distdir)/$$file || exit 1; \
	  done; \
	else \
	  case $(XGETTEXT) in \
	    :) echo "Warning: Creating a tarball without '$(DOMAIN).pot', because a suitable 'xgettext' program was not found in PATH." 1>&2;; \
	    *) echo "Warning: Creating a tarball without '$(DOMAIN).pot', because 'xgettext' found no strings to extract. Check the POTFILES and the XGETTEXT_OPTIONS in the Makefile.am file." 1>&2;; \
	  esac; \
	fi

# This rule updates all generated files in this directory.
# Note: The name 'update-po' is a bit misleading. It does *not* modify the *.po
# files. But we can't rename it easily, because it's a documented target.
update-po: Makefile
	$(MAKE) $(DOMAIN).pot-update
	$(MAKE) update-properties

# General rule for creating PO files.

.nop.po-create:
	@lang=`echo $@ | sed -e 's/\.po-create$$//'`; \
	echo "File $$lang.po does not exist. If you are a translator, you can create it through 'msginit'." 1>&2; \
	exit 1

$(DUMMYPOFILES):

# During .po -> .properties or .class conversion, take into account the most
# recent changes to the .pot file. This eliminates the need to update the .po
# files when the .pot file has changed (see above).

update-properties: Makefile $(srcdir)/$(DOMAIN).pot $(POFILES)
	@echo "$(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en --properties-output -o $(srcdir)/$(DOMAIN).properties"; \
	$(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en --properties-output -o $(DOMAIN).properties || { rm -f $(DOMAIN).properties; exit 1; }
# Move $(DOMAIN).properties into $(srcdir). But don't provoke a gratuitous error
# in a VPATH build with read-only $(srcdir).
	@if test '$(srcdir)' != .; then \
	  if test -f $(srcdir)/$(DOMAIN).properties && cmp $(DOMAIN).properties $(srcdir)/$(DOMAIN).properties >/dev/null; then \
	    rm -f $(DOMAIN).properties; \
	  else \
	    mv -f $(DOMAIN).properties $(srcdir)/$(DOMAIN).properties; \
	  fi; \
	fi
	@for f in $(POFILES); do \
	  lang=`echo $$f | sed -e 's,.*/,,' -e 's,\.po$$,,'`; \
	  echo "$(MSGMERGE) $(MSGMERGE_FOR_MSGFMT_OPTION) -o $${lang}.gpo $(srcdir)/$${lang}.po $(srcdir)/$(DOMAIN).pot"; \
	  $(MSGMERGE) $(MSGMERGE_FOR_MSGFMT_OPTION) -o $${lang}.gpo $(srcdir)/$${lang}.po $(srcdir)/$(DOMAIN).pot || exit 1; \
	  echo "$(GMSGFMT) -c --statistics --verbose -o /dev/null $${lang}.gpo"; \
	  $(GMSGFMT) -c --statistics --verbose -o /dev/null $${lang}.gpo || exit 1; \
	  echo "$(MSGCAT) --properties-output -o $(srcdir)/$(DOMAIN)_$$lang.properties $${lang}.gpo"; \
	  $(MSGCAT) --properties-output -o $(DOMAIN)_$$lang.properties.tmp $${lang}.gpo || exit 1; \
	  if test '$(srcdir)' = .; then \
	    mv -f $(DOMAIN)_$$lang.properties.tmp $(DOMAIN)_$$lang.properties || exit 1; \
	  else \
	    if test -f $(srcdir)/$(DOMAIN)_$$lang.properties && cmp $(DOMAIN)_$$lang.properties.tmp $(srcdir)/$(DOMAIN)_$$lang.properties >/dev/null; then \
	      rm -f $(DOMAIN)_$$lang.properties.tmp; \
	    else \
	      mv -f $(DOMAIN)_$$lang.properties.tmp $(srcdir)/$(DOMAIN)_$$lang.properties || exit 1; \
	    fi; \
	  fi; \
	  rm -f $${lang}.gpo; \
	done

# Alternatively, we could create classes instead of properties files.
update-classes: Makefile $(srcdir)/$(DOMAIN).pot $(POFILES)
	@echo "$(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en --properties-output -o $(srcdir)/$(DOMAIN).properties"; \
	$(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en --properties-output -o $(DOMAIN).properties || { rm -f $(DOMAIN).properties; exit 1; }
# Move $(DOMAIN).properties into $(srcdir). But don't provoke a gratuitous error
# in a VPATH build with read-only $(srcdir).
	@if test '$(srcdir)' != .; then \
	  if test -f $(srcdir)/$(DOMAIN).properties && cmp $(DOMAIN).properties $(srcdir)/$(DOMAIN).properties >/dev/null; then \
	    rm -f $(DOMAIN).properties; \
	  else \
	    mv -f $(DOMAIN).properties $(srcdir)/$(DOMAIN).properties; \
	  fi; \
	fi
	@for f in $(POFILES); do \
	  lang=`echo $$f | sed -e 's,.*/,,' -e 's,\.po$$,,'`; \
	  echo "$(MSGMERGE) $(MSGMERGE_FOR_MSGFMT_OPTION) -o $${lang}.gpo $(srcdir)/$${lang}.po $(srcdir)/$(DOMAIN).pot"; \
	  $(MSGMERGE) $(MSGMERGE_FOR_MSGFMT_OPTION) -o $${lang}.gpo $(srcdir)/$${lang}.po $(srcdir)/$(DOMAIN).pot || exit 1; \
	  echo "$(GMSGFMT) -c --statistics --verbose -j -d $(srcdir) -r $(DOMAIN) -l $${lang} $${lang}.gpo"; \
	  $(GMSGFMT) -c --statistics --verbose -j -d . -r $(DOMAIN) -l $${lang} $${lang}.gpo || exit 1; \
	  if test '$(srcdir)' != .; then \
	    if test -f $(srcdir)/$(DOMAIN)_$$lang.class && cmp $(DOMAIN)_$$lang.class $(srcdir)/$(DOMAIN)_$$lang.class >/dev/null; then \
	      rm -f $(DOMAIN)_$$lang.class; \
	    else \
	      mv -f $(DOMAIN)_$$lang.class $(srcdir)/$(DOMAIN)_$$lang.class || exit 1; \
	    fi; \
	  fi; \
	  rm -f $${lang}.gpo; \
	done

echo-catalogs:
# When packaging the catalogs for installation, include the fallback catalog always.
	@echo $(DOMAIN).properties $(CATALOGS)
