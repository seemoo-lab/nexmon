VERSION = 015

# The toolchain definitions
CC		= cc
SPARSE		= sparse

V		= @		# Verbose build:  make V=1
C		= 0		# Sparsechecker build:  make C=1
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CC)
ifeq ($(C),1)
QUIET_SPARSE	= $(Q:@=@echo '     SPARSE   '$@;)$(SPARSE)
else
QUIET_SPARSE	= @true
endif

PREFIX ?= /usr/local
CFLAGS		?= -Os -fomit-frame-pointer
CFLAGS		+= -std=c99 -Wall -pedantic -D_BSD_SOURCE
LDFLAGS		?=

SRCS = fwcutter.c md5.c
BIN	= b43-fwcutter

CFLAGS += -DFWCUTTER_VERSION_=$(VERSION)

.SUFFIXES:
.PHONY: all install clean distclean
.DEFAULT_GOAL := all

DEPS = $(sort $(patsubst %.c,dep/%.d,$(1)))
OBJS = $(sort $(patsubst %.c,obj/%.o,$(1)))

# Generate dependencies
$(call DEPS,$(SRCS)): dep/%.d: %.c 
	@mkdir -p $(dir $@)
	$(QUIET_DEPEND) -o $@.tmp -MM -MT "$@ $(patsubst dep/%.d,obj/%.o,$@)" $(CFLAGS) $< && mv -f $@.tmp $@

-include $(call DEPS,$(SRCS))

# Generate object files
$(call OBJS,$(SRCS)): obj/%.o:
	@mkdir -p $(dir $@)
	$(QUIET_SPARSE) $(SPARSEFLAGS) $<
	$(QUIET_CC) -o $@ -c $(CFLAGS) $<

all: $(BIN)

$(BIN): $(call OBJS,$(SRCS))
	$(QUIET_CC) $(CFLAGS) -o $(BIN) $(call OBJS,$(SRCS)) $(LDFLAGS)

install: all
	install -d -o 0 -g 0 -m 755 $(PREFIX)/bin/
	install -o 0 -g 0 -m 755 $(BIN) $(PREFIX)/bin/
	install -d -o 0 -g 0 -m 755 $(PREFIX)/man/man1/
	install -o 0 -g 0 -m 644 $(BIN).1 $(PREFIX)/man/man1/

clean:
	-rm -Rf obj dep *.orig *.rej *~

distclean: clean
	-rm -f *.fw $(BIN)
