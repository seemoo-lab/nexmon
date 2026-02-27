# The toolchain definitions
CC		= gcc
LEX		= flex
YACC		= bison
SPARSE		= sparse

V		= @		# Verbose build:  make V=1
C		= 0		# Sparsechecker build:  make C=1
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CC)
QUIET_LEX	= $(Q:@=@echo '     LEX      '$@;)$(LEX)
QUIET_YACC	= $(Q:@=@echo '     YACC     '$@;)$(YACC)
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
SPARSEFLAGS	= $(CFLAGS) -D__transparent_union__=__unused__ -D_STRING_ARCH_unaligned=1 \
		  -D__builtin_stpcpy=stpcpy
LDFLAGS		+= -lfl

BIN		= b43-asm.bin
SRCS		= parser.c scanner.c main.c initvals.c util.c args.c

# YACC related CFLAGS
CFLAGS		+= -DYYSTYPE="void *" -DYYERROR_VERBOSE -DYYDEBUG -DYYENABLE_NLS=0 -DYYLTYPE_IS_TRIVIAL=0 -Wno-unused

.SUFFIXES:
.PHONY: all install clean distclean
.DEFAULT_GOAL := all

DEPS = $(sort $(patsubst %.c,dep/%.d,$(1)))
OBJS = $(sort $(patsubst %.c,obj/%.o,$(1)))

# Generate dependencies
$(call DEPS,$(SRCS)): dep/%.d: %.c 
	@mkdir -p $(dir $@)
	$(QUIET_DEPEND) -o $@.tmp -MM -MG -MT "$@ $(patsubst dep/%.d,obj/%.o,$@)" $(CFLAGS) $< && mv -f $@.tmp $@

-include $(call DEPS,$(SRCS))

# Generate object files
$(call OBJS,$(SRCS)): obj/%.o:
	@mkdir -p $(dir $@)
	$(QUIET_SPARSE) $(SPARSEFLAGS) $<
	$(QUIET_CC) -o $@ -c $(CFLAGS) $<

all: $(BIN)

scanner.c: scanner.l parser.h main.h util.h
	$(QUIET_LEX) -o scanner.c --header-file=scanner.h scanner.l

scanner.h: scanner.c

scanner.o: scanner.c
	$(QUIET_CC) $(CFLAGS) -c -o scanner.o scanner.c

parser.c: parser.y main.h util.h
	$(QUIET_YACC) --defines -o parser.c parser.y

parser.h: parser.c

parser.o: parser.c
	$(QUIET_CC) $(CFLAGS) -c -o parser.o parser.c

$(BIN): $(call OBJS,$(SRCS))
	$(QUIET_CC) $(CFLAGS) -o $(BIN) $(call OBJS,$(SRCS)) $(LDFLAGS)

install: all
	install -m 755 $(BIN) $(PREFIX)/bin/
	cp b43-asm b43-asm.inst
	sed -i -e 's/installed=0/installed=1/' b43-asm.inst
	install -m 755 b43-asm.inst $(PREFIX)/bin/b43-asm
	rm -f b43-asm.inst

clean:
	-rm -Rf dep obj *~ *.orig *.rej scanner.c scanner.h parser.c parser.h b43-asm.bin

distclean: clean
	-rm -f $(BIN)
