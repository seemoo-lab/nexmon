include definitions.mk

all: $(RAM_FILE) clm.bin ucode.bin templateram.bin flashpatches.c

$(RAM_FILE): SHELL:=$(shell which bash)
$(RAM_FILE): w43439A0_7_95_49_00_combined.h
	@printf "\033[0;31m  EXTRACTING FIRMWARE BLOB\033[0m\n"
	@$(Q)HEX_COMBINED=$$(head -n-4 $< | tail -n+2 | sed -r 's/((0x)|[, ])//g' | tr -d '\n');FW_LEN=$$(grep CYW43_WIFI_FW_LEN $< | sed -r 's/.*\(([0-9]+)\).*/\1/g');FW_LEN_HEX=$$(($$FW_LEN << 1));printf "%s" "$${HEX_COMBINED::$$FW_LEN_HEX}" | xxd -r -p > "$@"

clm.bin: SHELL:=$(shell which bash)
clm.bin: w43439A0_7_95_49_00_combined.h
	@printf "\033[0;31m  EXTRACTING CLM BLOB\033[0m\n"
	@$(Q)HEX_COMBINED=$$(head -n-4 $< | tail -n+2 | sed -r 's/((0x)|[, ])//g' | tr -d '\n');FW_LEN=$$(grep CYW43_WIFI_FW_LEN $< | sed -r 's/.*\(([0-9]+)\).*/\1/g');CLM_OFF_HEX=$$(((($$FW_LEN + 511) & ~511) << 1));CLM_LEN=$$(grep CYW43_CLM_LEN $< | sed -r 's/.*\(([0-9]+)\).*/\1/g');CLM_LEN_HEX=$$(($$CLM_LEN << 1));printf "%s" "$${HEX_COMBINED:$$CLM_OFF_HEX:$$CLM_LEN_HEX}" | xxd -r -p > "$@"

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

complete.bin: $(RAM_FILE) rom.bin
	@printf "\033[0;31m  CONCATENATING RAM AND ROM\033[0m\n"
	$(Q)dd if=rom.bin of=$@ bs=1 status=none conv=notrunc seek=$$(($(ROMSTART)))
	$(Q)dd if=$< of=$@ bs=1 status=none conv=notrunc seek=$$(($(RAMSTART)))

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f $(RAM_FILE) clm.bin ucode.bin flashpatches.c templateram.bin
