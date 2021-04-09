all: buildtools firmwares

firmwares: buildtools FORCE
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES AND UCODE\033[0m\n"
	$(Q)make -C $@

buildtools: FORCE
	@printf "\033[0;31m  BUILDING BUILDTOOLS\033[0m\n"
	$(Q)make -C $@

clean: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)find . -iname "templateram*.bin" -delete
	$(Q)find . -iname "vasip.bin" -delete
	$(Q)find . -iname "*ucode*.bin" -delete
	$(Q)find . -iname "flashpatches.c" -delete

FORCE:
