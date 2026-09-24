# The toolchain definitions
CC		= gcc
SPARSE		= sparse

V		= @		# Verbose build:  make V=1
C		= 0		# Sparsechecker build:  make C=1
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CC)
ifeq ($(C),1)
QUIET_SPARSE	= $(Q:@=@echo '     SPARSE   '$@;)$(SPARSE)
else
QUIET_SPARSE	= @/bin/true
endif

PREFIX		?= /usr/local
CFLAGS		?= -O2 -fomit-frame-pointer -pipe
CFLAGS		+= -std=gnu99 -D_GNU_SOURCE \
		   -Wall -Wextra -Wformat=2 -Wundef -Wno-unused-parameter \
		   -Wshadow -Wpointer-arith
SPARSEFLAGS	= $(CFLAGS) -D__transparent_union__=__unused__ -D_STRING_ARCH_unaligned=1
LDFLAGS		?=

BIN = b43-dasm
SRCS = main.c util.c args.c

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
	install -m 755 $(BIN) $(PREFIX)/bin/
	install -m 755 b43-ivaldump $(PREFIX)/bin/
	install -m 755 brcm80211-ivaldump $(PREFIX)/bin/
	install -m 755 brcm80211-fwconv $(PREFIX)/bin/

clean:
	-rm -Rf obj dep *~ *.orig *.rej b43-dasm

distclean: clean
	-rm -f $(BIN)
