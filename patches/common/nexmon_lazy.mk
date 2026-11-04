# Common lazy probes for nexmon Makefiles — included by patches/*/nexmon/Makefile
# Avoids forking git/uname/kernel-symvers on `make clean` and dedups 40 copies.
ifneq ($(filter clean%,$(MAKECMDGOALS)),)
GIT_VERSION :=
KERNEL_VERSION :=
SYMVERS_SUPPLEMENT :=
DRIVER_FOLDER_NAME :=
else
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
KERNEL_VERSION = $(shell uname -r | sed 's/\([0-9]\+\.[0-9]\+\)\..*/\1/')
DRIVER_FOLDER_NAME = $(NEXMON_ROOT)/patches/driver/brcmfmac_$(KERNEL_VERSION).y-nexmon
SYMVERS_SUPPLEMENT = $(shell $(NEXMON_ROOT)/buildtools/scripts/kernel-symvers-supplement.sh /lib/modules/$(shell uname -r)/build 2>/dev/null)
endif
