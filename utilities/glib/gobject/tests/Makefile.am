include $(top_srcdir)/glib-tap.mk

LDADD = ../libgobject-2.0.la $(top_builddir)/glib/libglib-2.0.la
AM_CPPFLAGS = -g $(gobject_INCLUDES) $(GLIB_DEBUG_FLAGS)
DEFS = -DG_LOG_DOMAIN=\"GLib-GObject\"
AM_CFLAGS = $(GLIB_WARN_CFLAGS)

# -----------------------------------------------------------------------------

test_programs = \
	qdata				\
	boxed				\
	enums				\
	param				\
	threadtests			\
	dynamictests			\
	binding				\
	properties			\
	reference			\
	value				\
	type				\
	private				\
	closure				\
	object				\
	signal-handler			\
	$(NULL)

# -----------------------------------------------------------------------------

test_programs += ifaceproperties
ifaceproperties_SOURCES = ifaceproperties.c testcommon.h

# -----------------------------------------------------------------------------

# The marshalers test requires running a binary, so we cannot build it when
# cross-compiling

if !CROSS_COMPILING
glib_genmarshal=$(top_builddir)/gobject/glib-genmarshal

test_programs += signals
signals_SOURCES = signals.c
nodist_signals_SOURCES = marshalers.c marshalers.h
signals.o: marshalers.h
CLEANFILES += marshalers.h marshalers.c
EXTRA_DIST += marshalers.list

marshalers.h: Makefile.am marshalers.list
	$(AM_V_GEN) $(glib_genmarshal) --prefix=test $(srcdir)/marshalers.list --header --valist-marshallers > marshalers.h

marshalers.c: Makefile.am marshalers.h marshalers.list
	$(AM_V_GEN) (echo "#include \"marshalers.h\""; $(glib_genmarshal) --prefix=test $(srcdir)/marshalers.list --body --valist-marshallers) > $@.tmp && mv $@.tmp $@

endif # !CROSS_COMPILING
