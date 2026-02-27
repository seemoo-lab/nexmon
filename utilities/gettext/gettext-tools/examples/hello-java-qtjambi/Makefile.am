# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# General automake options.
AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The list of subdirectories containing Makefiles.
SUBDIRS = m4 po

# The list of programs that are built.
bin_JAVAPROGRAMS = hello

# The source files of the 'hello' program.
hello_SOURCES = Hello.java
hello_CLASSES = Hello.class

# The entry point of the 'hello' program.
hello_MAINCLASS = Hello

# The link dependencies of the 'hello' program.
hello_JAVALIBS = @LIBINTL_JAR@

# The resources of the 'hello' program, excluding message catalogs, but
# including the fallback message catalog.
hello_RESOURCES = hello-java-qtjambi.properties

# Resources that are generated from PO files.
MAINTAINERCLEANFILES = hello-java-qtjambi*.properties

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh


# ----------------- General rules for compiling Java programs -----------------

jardir = $(datadir)/$(PACKAGE)
pkgdatadir = $(datadir)/$(PACKAGE)
pkglibdir = $(libdir)/$(PACKAGE)

GCJ = @GCJ@
GCJFLAGS = @GCJFLAGS@
JAR = @JAR@
JAVACOMP = $(SHELL) javacomp.sh
AR = ar
RANLIB = @RANLIB@

EXTRA_DIST += $(hello_SOURCES)
CLEANFILES =
DISTCLEANFILES = javacomp.sh javaexec.sh


if USEJEXE


# Rules for compiling Java programs as native code.

all-local: $(hello_MAINCLASS)$(EXEEXT) hello-resources.jar hello.sh

# Does not work yet with GCC 3.3.
#$(hello_MAINCLASS)$(EXEEXT): $(srcdir)/Hello.java
#	CLASSPATH=.@CLASSPATH_SEPARATOR@$(hello_JAVALIBS)@CLASSPATH_SEPARATOR@$$CLASSPATH $(GCJ) $(GCJFLAGS) $(srcdir)/Hello.java $(hello_JAVALIBS) --main=$(hello_MAINCLASS) -o $@

$(hello_MAINCLASS)$(EXEEXT): Hello.$(OBJEXT) libintl.a
	$(GCJ) $(GCJFLAGS) Hello.$(OBJEXT) libintl.a --main=$(hello_MAINCLASS) -o $@

Hello.$(OBJEXT): $(srcdir)/Hello.java
	CLASSPATH=.@CLASSPATH_SEPARATOR@$(hello_JAVALIBS)@CLASSPATH_SEPARATOR@$$CLASSPATH $(GCJ) $(GCJFLAGS) -c $(srcdir)/Hello.java -o $@

libintl.a:
	rm -rf tmpdir
	mkdir tmpdir
	cd tmpdir && $(JAR) xf @LIBINTL_JAR@ && \
	for f in `find . -name '*.class' -print`; do \
	  $(GCJ) $(GCJFLAGS) -c $$f -o `echo $$f | sed -e 's,^\./,,' -e 's,\.class$$,,' -e 's,/,.,g'`.$(OBJEXT) || exit 1; \
	done && \
	rm -f ../libintl.a && \
	ar cru ../libintl.a `find . -name '*.$(OBJEXT)' -print`
	rm -rf tmpdir
	$(RANLIB) $@

hello-resources.jar:
	catalogs=`MAKEFLAGS= $(MAKE) -s -C po echo-catalogs`; \
	$(JAR) cf $@ $(hello_RESOURCES) $$catalogs

hello.sh:
	{ echo '#!/bin/sh'; \
	  echo "CLASSPATH='$(jardir)/hello-resources.jar'\$${CLASSPATH+\"@CLASSPATH_SEPARATOR@\$$CLASSPATH\"}"; \
	  echo "export CLASSPATH"; \
	  echo "exec '$(pkglibdir)/$(hello_MAINCLASS)$(EXEEXT)' \"\$$@\""; \
	} > $@

install-exec-local: all-local
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) hello.sh $(DESTDIR)$(bindir)/hello
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)
	$(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) $(hello_MAINCLASS)$(EXEEXT) $(DESTDIR)$(pkglibdir)/$(hello_MAINCLASS)$(EXEEXT)

install-data-local: all-local
	$(MKDIR_P) $(DESTDIR)$(jardir)
	$(INSTALL_DATA) hello-resources.jar $(DESTDIR)$(jardir)/hello-resources.jar

installdirs-local:
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)
	$(MKDIR_P) $(DESTDIR)$(jardir)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/hello
	rm -f $(DESTDIR)$(pkglibdir)/$(hello_MAINCLASS)$(EXEEXT)
	rm -f $(DESTDIR)$(jardir)/hello-resources.jar

CLEANFILES += $(hello_MAINCLASS)$(EXEEXT) *.$(OBJEXT) *.a tmpdir hello-resources.jar hello.sh


else


# Rules for compiling Java programs as jar libraries.
# This is the preferred mode during development, because you can easily test
# the program without installing it, simply by doing "java -jar hello.jar".

all-local: hello.jar hello.sh

hello.jar: $(hello_CLASSES)
	{ echo "Manifest-Version: 1.0"; echo "Main-Class: $(hello_MAINCLASS)"; echo 'Class-Path: @LIBINTL_JAR@'; } > Manifest.mf
	catalogs=`MAKEFLAGS= $(MAKE) -s -C po echo-catalogs`; \
	$(JAR) cfm $@ Manifest.mf Hello*.class $(hello_RESOURCES) $$catalogs
	rm -f Manifest.mf

Hello.class: $(srcdir)/Hello.java
	CLASSPATH=.@CLASSPATH_SEPARATOR@$(hello_JAVALIBS)@CLASSPATH_SEPARATOR@$$CLASSPATH $(JAVACOMP) -d . $(srcdir)/Hello.java

hello.sh:
	{ echo '#!/bin/sh'; \
	  echo "CLASSPATH='$(jardir)/hello.jar@CLASSPATH_SEPARATOR@$(hello_JAVALIBS)'\$${CLASSPATH+\"@CLASSPATH_SEPARATOR@\$$CLASSPATH\"}"; \
	  echo "export CLASSPATH"; \
	  echo "exec /bin/sh '$(pkgdatadir)/javaexec.sh' $(hello_MAINCLASS) \"\$$@\""; \
	} > $@

install-exec-local: all-local
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) hello.sh $(DESTDIR)$(bindir)/hello

install-data-local: all-local
	$(MKDIR_P) $(DESTDIR)$(jardir)
	$(INSTALL_DATA) hello.jar $(DESTDIR)$(jardir)/hello.jar
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) javaexec.sh $(DESTDIR)$(pkgdatadir)/javaexec.sh

installdirs-local:
	$(MKDIR_P) $(DESTDIR)$(jardir)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/hello
	rm -f $(DESTDIR)$(jardir)/hello.jar
	rm -f $(DESTDIR)$(pkgdatadir)/javaexec.sh

CLEANFILES += hello.jar Hello*.class Manifest.mf hello.sh


endif
