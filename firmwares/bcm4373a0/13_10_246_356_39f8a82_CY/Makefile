include definitions.mk

all: $(FW_FILE) $(RAM_FILE) ucode.bin templateram.bin flashpatches.c

$(FW_FILE):
	@printf "\033[0;31m  DOWNLOADING FIRMWARE\033[0m\n"
	$(Q)wget -nv --show-progress -O - https://github.com/Ezurio/Connectivity_Stack_Release_Packages/releases/download/LRD-REL-13.24.0.20/summit-lwb5plus-usb-sa-firmware-13.24.0.20.tar.bz2 | tar -xjv --strip-components 3 lib/firmware/brcm/$@

$(RAM_FILE): $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FIRMWARE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(RAM_FILE_OFFSET))) count=$$(($(RAM_FILE_LEN))) 2>/dev/null

ucode.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODESTART)-$(RAMSTART))) count=$$(($(UCODESIZE))) 2>/dev/null

templateram.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING TEMPLATERAM\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAMSTART)-$(RAMSTART))) count=$$(($(TEMPLATERAMSIZE))) 2>/dev/null

flashpatches.c: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES\033[0m\n"
	$(Q)printf "#include <patcher.h>\n\n" > flashpatches.c
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -y -r $< -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) >> $@

rom.bin: ../rom.bin
	@printf "\033[0;31m  APPLYING FLASHPATCHES TO CLEAN ROM\033[0m\n"
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -y -r $(RAM_FILE) -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) -i $< -o $@ -t $(ROMSTART) > /dev/null

complete.bin: $(RAM_FILE) rom.bin
	@printf "\033[0;31m  MERGING %s and rom.bin into %s\033[0m\n" $< $@
	$(Q)dd if=rom.bin of=$@ bs=1 seek=$$(($(ROMSTART))) count=$$(($(ROMSIZE))) conv=notrunc 2>/dev/null
	$(Q)dd if=$< of=$@ bs=1 seek=$$(($(RAMSTART))) conv=notrunc 2>/dev/null

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f ucode.bin templateram.bin flashpatches.c rom.bin $(RAM_FILE) $(FW_FILE)
