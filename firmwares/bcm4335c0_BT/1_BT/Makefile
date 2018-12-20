include definitions.mk

all: hcd_extracted/rompatches

hcd_extracted/rompatches: hcd_extracted/10_writeram_0x00218000.bin definitions.mk
	@printf "\033[0;31m EXTRACT HCD FILE \033[0m\n"
	$(Q)$(NEXMON_ROOT)/buildtools/hcd_extractor/bthcd_rompatches -p $< -o $@

hcd_extracted/10_writeram_0x00218000.bin: $(HCD_FILE) definitions.mk
	@printf "\033[0;31m EXTRACT HCD FILE \033[0m\n"
	$(Q)$(NEXMON_ROOT)/buildtools/hcd_extractor/bthcd -p $< -o hcd_extracted

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -r hcd_extracted
