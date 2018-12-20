all: flash_patch_extractor ucode_extractor b43/assembler/b43-asm.bin b43/disassembler/b43-dasm hcd_generator hcd_extractor

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

hcd_generator: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@ 
	$(Q)make -C $@

hcd_extractor: FORCE
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@ 
	$(Q)make -C $@

FORCE:
	
