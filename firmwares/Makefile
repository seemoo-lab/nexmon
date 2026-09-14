SUBDIRS = $(dir $(wildcard */Makefile))

all: $(SUBDIRS)

$(SUBDIRS): FORCE
	@printf "\033[0;31m  EXECUTING MAKE FOR CHIP VERSION\033[0m %s\n" $@
	$(Q)make -C $@

clean: FORCE
	@printf "\033[0;31m  CLEANING CHIP VERSIONS\033[0m\n"
	$(Q)for dir in $(SUBDIRS); do [ -f $$dir/Makefile ] && make -C $$dir clean || true; done

FORCE:
