# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# General automake options.
AUTOMAKE_OPTIONS = foreign no-dependencies
ACLOCAL_AMFLAGS = -I m4

# The list of subdirectories containing Makefiles.
SUBDIRS = m4 po

# The list of programs that are built.
bin_PROGRAMS = hello hello2

# The source files of the 'hello' program.
hello_SOURCES = hello.c hello-resources.c
# The source files of the 'hello2' program.
hello2_SOURCES = hello2.c hello2-resources.c

# Define a C macro LOCALEDIR indicating where catalogs will be installed
# and a C macro PKGDATADIR indicating a package-specific location.
DEFS = \
  -DLOCALEDIR=$(localedir_c_make) \
  -DPKGDATADIR=$(pkgdatadir_c_make) \
  @DEFS@

# Make sure the gnome.h include file is found.
AM_CPPFLAGS = $(GTK_CFLAGS)

# Link time dependencies.
LDADD = $(GTK_LIBS) @LIBINTL@

# Compile GSettings schema.
gschemas.compiled: hello2.gschema.xml
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --targetdir=. $(srcdir)

# Compile assets into a C source and link it with the application.
hello-resources.c: hello.gresource.xml hello.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ \
		--sourcedir=$(srcdir) --generate-source
hello2-resources.c: hello2.gresource.xml hello2.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ \
		--sourcedir=$(srcdir) --generate-source

# Install the compiled GSettings schema in a package-specific location
# (so that "make install" works with a --prefix other than /usr).
pkgdata_DATA = gschemas.compiled

desktopdir = $(datadir)/applications
desktop_DATA = hello.desktop hello2.desktop

# Merge translations back into a Desktop Entry file.

# Note that the resulting file should be included in EXTRA_DIST and
# processed earlier than the variable substitution below.  Otherwise,
# the 'msgfmt' command will be required at compile-time.
hello.desktop.in: hello.desktop.in.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
hello2.desktop.in: hello2.desktop.in.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

# Substitute variables in a Desktop Entry file.
hello.desktop: hello.desktop.in
	$(AM_V_GEN) sed -e 's|@bindir[@]|$(bindir)|g' $< > $@
hello2.desktop: hello2.desktop.in
	$(AM_V_GEN) sed -e 's|@bindir[@]|$(bindir)|g' $< > $@

BUILT_SOURCES        = gschemas.compiled hello-resources.c hello2-resources.c hello.desktop hello2.desktop
CLEANFILES           = gschemas.compiled                                      hello.desktop hello2.desktop
DISTCLEANFILES       =
MAINTAINERCLEANFILES =                   hello-resources.c hello2-resources.c hello.desktop.in hello2.desktop.in

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh \
	hello.desktop.in.in hello.desktop.in \
	hello.gresource.xml hello.ui \
	hello2.desktop.in.in hello2.desktop.in \
	hello2.gresource.xml hello2.ui \
	hello2.gschema.xml
