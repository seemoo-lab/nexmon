include definitions.mk

all: d11ucode_p2p_D11s.bin d11ucode_p2p_D11a.bin d11ucode_p2p_D11b.bin sr_source_code_88_main.bin sr_source_code_88_aux.bin sr_source_code_88_scan.bin sr_source_code_88_dig.bin flashpatches.c

d11ucode_p2p_D11s.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FIRST UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE0START)-$(RAMSTART))) count=$$(($(UCODE0SIZE))) 2>/dev/null

d11ucode_p2p_D11a.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE1START)-$(RAMSTART))) count=$$(($(UCODE1SIZE))) 2>/dev/null

d11ucode_p2p_D11b.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE2START)-$(RAMSTART))) count=$$(($(UCODE2SIZE))) 2>/dev/null

sr_source_code_88_main.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FIRST TEMPLATERAM\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAM0START)-$(RAMSTART))) count=$$(($(TEMPLATERAM0SIZE))) 2>/dev/null

sr_source_code_88_aux.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND TEMPLATERAM\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAM1START)-$(RAMSTART))) count=$$(($(TEMPLATERAM1SIZE))) 2>/dev/null

sr_source_code_88_scan.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD TEMPLATERAM\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAM2START)-$(RAMSTART))) count=$$(($(TEMPLATERAM2SIZE))) 2>/dev/null

sr_source_code_88_dig.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FOURTH TEMPLATERAM\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(TEMPLATERAM3START)-$(RAMSTART))) count=$$(($(TEMPLATERAM3SIZE))) 2>/dev/null

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
	$(Q)rm -f d11ucode_p2p_D11s.bin d11ucode_p2p_D11a.bin d11ucode_p2p_D11b.bin sr_source_code_88_main.bin sr_source_code_88_aux.bin sr_source_code_88_scan.bin sr_source_code_88_dig.bin flashpatches.c
