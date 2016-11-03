all: buildtools firmwares

firmwares: buildtools FORCE
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES AND UCODE\033[0m\n"
	$(Q)make -C $@

buildtools: FORCE
	@printf "\033[0;31m  BUILDING BUILDTOOLS\033[0m\n"
	$(Q)make -C $@

FORCE:
