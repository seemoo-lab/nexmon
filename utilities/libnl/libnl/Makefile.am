# -*- Makefile -*-

ACLOCAL_AMFLAGS = -I m4

OPT_DIRS =

if ENABLE_CLI
OPT_DIRS += src
endif

SUBDIRS = include lib doc $(OPT_DIRS)

pkgconfig_DATA = libnl-2.0.pc

sysconfdir = @sysconfdir@/libnl
sysconf_DATA = etc/pktloc

.PHONY: cscope
cscope:
	cscope -b -q -R -Iinclude -slib -ssrc;
