AUTOMAKE_OPTIONS = nostdinc

noinst_PROGRAMS = extract_interface
TESTS =
if HAVE_PYTHON
  TESTS += isl_test_python.py
  isl_test_python.py: isl.py
endif
TEST_EXTENSIONS = .py
AM_TESTS_ENVIRONMENT = \
	export PYTHONPATH=.; \
	export LD_LIBRARY_PATH=../.libs;
PY_LOG_COMPILER = $(PYTHON)

includes = -I$(top_builddir) -I$(top_srcdir) \
	-I$(top_builddir)/include -I$(top_srcdir)/include

extract_interface_CPPFLAGS = $(includes)
extract_interface_CXXFLAGS = $(CLANG_CXXFLAGS)
extract_interface_SOURCES = \
	generator.h \
	generator.cc \
	python.h \
	python.cc \
	extract_interface.h \
	extract_interface.cc
extract_interface_LDFLAGS = $(CLANG_LDFLAGS)
extract_interface_LDADD = \
	-lclangFrontend -lclangSerialization -lclangParse -lclangSema \
	$(LIB_CLANG_EDIT) \
	-lclangAnalysis -lclangAST -lclangLex -lclangBasic -lclangDriver \
	$(CLANG_LIBS) $(CLANG_LDFLAGS)

CLEANFILES = isl.py

# dummy library that captures the dependencies on all headers
# that are relevant for the bindings
noinst_LIBRARIES = libdep.a
libdep_a_CPPFLAGS = $(includes)
libdep_a_SOURCES = all.c

isl.py: extract_interface libdep.a isl.py.top
	(cat $(srcdir)/isl.py.top; \
		./extract_interface$(EXEEXT) --language=python $(includes) \
			$(srcdir)/all.h) \
			> isl.py

dist-hook: isl.py
	cp isl.py $(distdir)/
