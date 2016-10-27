include definitions.mk

all: ucode.bin flashpatches.c

ucode.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODESTART)-$(RAMSTART))) count=$$(($(UCODESIZE))) 2>/dev/null

flashpatches.c: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES\033[0m\n"
	$(Q)printf "#include <patcher.h>\n\n" > flashpatches.c
	$(Q)$(NEXMON_ROOT)/buildtools/flash_patch_extractor/fpext -r $< -b $(FP_CONFIG_ORIGBASE) -e $(FP_CONFIG_ORIGEND) >> $@

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f ucode.bin flashpatches.c
