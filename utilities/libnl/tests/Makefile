#
# src/Makefile
#
# 	This library is free software; you can redistribute it and/or
#	modify it under the terms of the GNU Lesser General Public
#	License as published by the Free Software Foundation version 2.1
#	of the License.
#
# Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
#

ifeq ($(shell [ ! -r ../Makefile.opts ] && echo 1),)
    include ../Makefile.opts
endif

LDFLAGS	+= -L../lib -lnl ../src/utils.o
CIN 	:= $(wildcard test-*.c)
TOOLS	:= $(CIN:%.c=%)

all: $(TOOLS)

$(TOOLS): ../src/utils.o

test-%: test-%.c
	@echo "  LD $@"; \
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lnl-genl -lnl-route

clean:
	@echo "  CLEAN src"; \
	rm -f $(TOOLS)

distclean: clean

install:
	@true

include ../Makefile.rules
