include definitions.mk

all: hcd_extracted/rompatches

#TODO the hcd file is a minimal example and does not continue ROMPATCHES, ignore errors
hcd_extracted/rompatches: hcd_extracted/10_writeram_0x00218000.bin definitions.mk
	@printf "\033[0;31m  EXTRACT HCD FILE ROMPATCHES \033[0m\n"
	-$(NEXMON_ROOT)/buildtools/hcd_extractor/bthcd_rompatches -p $< -o $@ >/dev/null

hcd_extracted/10_writeram_0x00218000.bin: $(HCD_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACT HCD FILE RAMPATCHES \033[0m\n"
	-$(NEXMON_ROOT)/buildtools/hcd_extractor/bthcd -p $< -o hcd_extracted >/dev/null

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -r hcd_extracted patchout_complete.bin
