include definitions.mk

all: ucode.bin templateram.bin flashpatches.c

ucode.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)$(NEXMON_ROOT)/buildtools/ucode_extractor/ucodeext -r $< -b $(UCODESTART) -l $(UCODESIZE) -o $@

templateram.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING TEMPLATERAM\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAMSTART)-$(RAMSTART))) count=$$(($(TEMPLATERAMSIZE))) 2>/dev/null

flashpatches.c: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES\033[0m\n"
	$(Q)printf "#include <patcher.h>\n\n" > flashpatches.c
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -r $< -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) >> $@

rom.bin: ../rom.bin
	@printf "\033[0;31m  APPLYING FLASHPATCHES TO CLEAN ROM\033[0m\n"
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -r $(RAM_FILE) -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) -i $< -o $@ -t $(ROMSTART) > /dev/null

complete.bin: brcmfmac43430-sdio.bin rom.bin
	@printf "\033[0;31m  CONCATENATING RAM AND ROM\033[0m\n"
	$(Q)dd if=rom.bin of=$@ bs=1 status=none conv=notrunc seek=$$(($(ROMSTART)))
	$(Q)dd if=$< of=$@ bs=1 status=none conv=notrunc seek=$$(($(RAMSTART)))

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f ucode.bin flashpatches.c templateram.bin
