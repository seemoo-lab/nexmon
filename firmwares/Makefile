SUBDIRS = $(dir $(wildcard */Makefile))

all: $(SUBDIRS)

$(SUBDIRS): FORCE
	@printf "\033[0;31m  EXECUTING MAKE FOR CHIP VERSION\033[0m %s\n" $@
	$(Q)make -C $@

FORCE:
