SUBDIRS = $(dir $(wildcard */definitions.mk))

all: $(SUBDIRS)

$(SUBDIRS): FORCE
ifndef (,$(wildcard $(addprefix $@, Makefile)))
	@printf "\033[0;31m  EXECUTING MAKE FOR FIRMWARE VERSION\033[0m %s\n" $@
	$(Q)make -C $@
endif

clean: FORCE
	@printf "\033[0;31m  CLEANING FIRMWARE VERSIONS\033[0m\n"
	$(Q)for dir in $(SUBDIRS); do [ -f $$dir/Makefile ] && make -C $$dir clean || true; done

FORCE:
