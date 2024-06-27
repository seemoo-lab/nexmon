SUBDIRS = $(dir $(wildcard */definitions.mk))

all: $(SUBDIRS)

$(SUBDIRS): FORCE
ifndef (,$(wildcard $(addprefix $@, Makefile)))
	@printf "\033[0;31m  EXECUTING MAKE FOR FIRMWARE VERSION\033[0m %s\n" $@
	$(Q)make -C $@
endif

rom.idc: $(RAM_FILE)
	@printf "\033[0;31m  GENERATING IDC FILE\033[0m %s\n" $@
	$(Q)cat rom.map | gawk 'BEGIN { printf "#import <idc.idc>\n\nstatic main() {\n" } { if (!seen[$$1]++) { printf "\tMakeName(0x%s,\"%s\");\n",$$1,$$3 }; } END { printf "}\n" }' > $@

FORCE:
