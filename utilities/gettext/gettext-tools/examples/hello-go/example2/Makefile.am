# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# The list of programs that are built.
bin_PROGRAMS = hello2sl

# The source files of the 'hello2sl' program.
nodist_hello2sl_SOURCES = hello2sl.go

EXTRA_DIST = go.mod.in

CLEANFILES = go.sum

# Rules for compiling the programs.

go.sum:
	$(GO) mod download github.com/gosexy/gettext

hello2sl$(EXEEXT): $(nodist_hello2sl_SOURCES) go.sum
	$(SHELL) ../gocomp.sh $(nodist_hello2sl_SOURCES)
