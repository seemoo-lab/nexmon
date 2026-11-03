PYTHON ?= python3

# Self-bootstrap the build environment. Historically you had to
# `source setup_env.sh` before running make; instead, if the environment has
# not been set up yet (NEXMON_SETUP_ENV is unset), source it here once and
# re-exec make inside the resulting environment so a bare `make` just works.
# setup_env.sh stays the single source of truth for platform detection,
# toolchain selection and LD_LIBRARY_PATH; we do not duplicate that logic.
ifndef NEXMON_SETUP_ENV

SETUP_ENV := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))setup_env.sh

__nexmon_bootstrap:
	@bash -c 'source "$$1" && shift && exec "$$1" --no-print-directory "$$@"' bash "$(SETUP_ENV)" "$(MAKE)" $(MAKECMDGOALS)

ifeq ($(MAKECMDGOALS),)
.DEFAULT_GOAL := __nexmon_bootstrap
else
# Route every requested goal through a single bootstrap+re-exec (the empty
# recipe below just satisfies make; the real work happens in the re-exec).
$(MAKECMDGOALS): __nexmon_bootstrap
	@:
endif
.PHONY: __nexmon_bootstrap $(MAKECMDGOALS)

else

all: buildtools firmwares

ARCH_TARGETS := arm armv7 armeabi armeabi-v7a arm64 aarch64 arm64-v8a x86 x86_64 x64

firmwares: buildtools
	@printf "\033[0;31m  EXTRACTING FLASHPATCHES AND UCODE\033[0m\n"
	$(Q)$(MAKE) -C $@

buildtools:
	@printf "\033[0;31m  BUILDING BUILDTOOLS\033[0m\n"
	$(Q)$(MAKE) -C $@

utilities:
	@printf "\033[0;31m  BUILDING UTILITIES\033[0m\n"
	$(Q)$(MAKE) -C $@

$(ARCH_TARGETS:%=utilities-%): utilities-%:
	@printf "\033[0;31m  BUILDING UTILITIES ($*)\033[0m\n"
	$(Q)$(MAKE) -C utilities ARCH=$*

$(ARCH_TARGETS:%=all-%): all-%: buildtools firmwares utilities-%

update:
	@printf "\033[0;31m  CHECKING UTILITIES AND LIBRARIES VERSIONS\033[0m\n"
	$(Q)$(PYTHON) buildtools/scripts/check_updates.py

test: check

check:
	@printf "\033[0;32m  RUNNING UNIT AND INTEGRATION TESTS\033[0m\n"
	$(Q)$(PYTHON) -m unittest discover -s tests -v
	$(Q)$(PYTHON) patches/bcm43455c0/7_45_265-28bca26-CY/nexmon/validate_build.py

clean:
	@printf "\033[0;31m  CLEANING FIRMWARES, BUILDTOOLS AND UTILITIES\033[0m\n"
	$(Q)$(MAKE) -C firmwares clean
	$(Q)$(MAKE) -C buildtools clean
	$(Q)$(MAKE) -C utilities clean

.PHONY: all $(ARCH_TARGETS:%=all-%) $(ARCH_TARGETS:%=utilities-%) firmwares buildtools utilities update clean check test FORCE

# Back-compat: some sub-makes still use FORCE as order-only
FORCE:
	@:

endif
