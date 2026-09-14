all: flash_patch_extractor ucode_extractor b43/assembler/b43-asm.bin b43/disassembler/b43-dasm b43-v2/assembler/b43-asm.bin b43-v2/disassembler/b43-dasm b43-v3/assembler/b43-asm.bin b43-v3/disassembler/b43-dasm b43-unified/assembler/b43-asm.bin b43-unified/disassembler/b43-dasm

b43-unified/assembler/b43-asm.bin: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43-unified/disassembler/b43-dasm: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43-v3/assembler/b43-asm.bin: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43-v3/disassembler/b43-dasm: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43-v2/assembler/b43-asm.bin: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43-v2/disassembler/b43-dasm: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43/assembler/b43-asm.bin: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

b43/disassembler/b43-dasm: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $(dir $@)

flash_patch_extractor: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $@

ucode_extractor: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)make -C $@

clean: FORCE
	@printf "\033[0;31m  CLEANING BUILDTOOLS\033[0m\n"
	$(Q)make -C flash_patch_extractor clean
	$(Q)make -C ucode_extractor clean
	$(Q)make -C b43/assembler clean
	$(Q)make -C b43/disassembler clean
	$(Q)make -C b43-v2/assembler clean
	$(Q)make -C b43-v2/disassembler clean
	$(Q)make -C b43-v3/assembler clean
	$(Q)make -C b43-v3/disassembler clean
	$(Q)make -C b43-unified/assembler clean
	$(Q)make -C b43-unified/disassembler clean

FORCE:
	
