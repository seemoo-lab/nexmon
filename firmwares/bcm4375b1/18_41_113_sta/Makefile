include definitions.mk

all: ucode1.bin ucode2.bin templateram0.bin templateram1.bin templateram2.bin flashpatches.c

ucode1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE1\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE1START)-$(RAMSTART))) count=$$(($(UCODE1SIZE))) 2>/dev/null

ucode2.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE2\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE2START)-$(RAMSTART))) count=$$(($(UCODE2SIZE))) 2>/dev/null

templateram0.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING TEMPLATERAM 0\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAMSTART0)-$(RAMSTART))) count=$$(($(TEMPLATERAMSIZE0))) 2>/dev/null

templateram1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING TEMPLATERAM 1\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAMSTART1)-$(RAMSTART))) count=$$(($(TEMPLATERAMSIZE1))) 2>/dev/null

templateram2.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING TEMPLATERAM 2\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAMSTART2)-$(RAMSTART))) count=$$(($(TEMPLATERAMSIZE2))) 2>/dev/null

flashpatches.c: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES\033[0m\n"
	$(Q)printf "#include <patcher.h>\n\n" > flashpatches.c
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -x -r $< -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) >> $@

rom.bin: ../rom.bin
	@printf "\033[0;31m  APPLYING FLASHPATCHES TO CLEAN ROM\033[0m\n"
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -y -r $(RAM_FILE) -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) -i $< -o $@ -t $(ROMSTART) > /dev/null

complete.bin: $(RAM_FILE) rom.bin
	@printf "\033[0;31m  MERGING %s and rom.bin into %s\033[0m\n" $< $@
	$(Q)dd if=rom.bin of=$@ bs=1 seek=$$(($(ROMSTART))) conv=notrunc 2>/dev/null
	$(Q)dd if=$< of=$@ bs=1 seek=$$(($(RAMSTART))) conv=notrunc 2>/dev/null

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f ucode1.bin ucode2.bin templateram0.bin templateram1.bin templateram2.bin flashpatches.c rom.bin
