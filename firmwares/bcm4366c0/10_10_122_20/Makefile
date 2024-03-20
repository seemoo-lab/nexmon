include definitions.mk

all: $(RAM_FILE) muucode.bin muucodex.bin nonmuucode.bin nonmuucodex.bin flashpatches.c

$(RAM_FILE): dhd.ko
	dd bs=1 if=$< of=$@ skip=$$((0x04e798)) count=$$((0x10f4cf)) status=none

muucode.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(MUUCODESTART)-$(RAMSTART))) count=$$(($(MUUCODESIZE))) 2>/dev/null

muucodex.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(MUUCODEXSTART)-$(RAMSTART))) count=$$(($(MUUCODEXSIZE))) 2>/dev/null

nonmuucode.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(NONMUUCODESTART)-$(RAMSTART))) count=$$(($(NONMUUCODESIZE))) 2>/dev/null

nonmuucodex.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(NONMUUCODEXSTART)-$(RAMSTART))) count=$$(($(NONMUUCODEXSIZE))) 2>/dev/null

flashpatches.c: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES\033[0m\n"
	$(Q)printf "#include <patcher.h>\n\n" > flashpatches.c
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -y -r $< -s $(RAMSTART) -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) >> $@

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f *ucodex.bin *ucode.bin templateram.bin flashpatches.c $(RAM_FILE)
