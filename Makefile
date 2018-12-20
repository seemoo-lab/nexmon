all: buildtools firmwares

firmwares: buildtools FORCE
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES AND UCODE\033[0m\n"
	$(Q)make -C $@

buildtools: FORCE
	@printf "\033[0;31m  BUILDING BUILDTOOLS\033[0m\n"
	$(Q)make -C $@

clean-all: clean-firmwares clean-buildtools

clean-firmwares:
	@printf "\033[0;31m CLEANING FIRMWARE BUILD FILES\033[0m\n"
	$(Q)make -C firmwares clean

clean-buildtools:
	@printf "\033[0;31m CLEANING BUILDTOOLS BUILD FILES\033[0m\n"
	$(Q)make -C buildtools clean

FORCE:


