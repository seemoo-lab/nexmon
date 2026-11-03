SHELL = /bin/bash
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

# Skip heavy shell probes when only cleaning — saves 3 forks and a sha256.
-include $(NEXMON_ROOT)/patches/common/nexmon_lazy.mk
KBUILD_EXTRA_SYMV = $(if $(SYMVERS_SUPPLEMENT),KBUILD_EXTRA_SYMBOLS=$(SYMVERS_SUPPLEMENT))
include ../version.mk
include $(FW_PATH)/definitions.mk

LOCAL_SRCS=$(wildcard src/*.c) src/ucode_compressed.c src/templateram.c
COMMON_SRCS=$(wildcard $(NEXMON_ROOT)/patches/common/*.c)
FW_SRCS=$(wildcard $(FW_PATH)/*.c)

OBJS=$(sort $(addprefix obj/,$(notdir $(LOCAL_SRCS:.c=.o)) $(notdir $(COMMON_SRCS:.c=.o)) $(notdir $(FW_SRCS:.c=.o))))
PREFILES=$(OBJS:.o=.pre)

# Auto-generated header dependencies (-MMD/-MP below). Without these a change to
# a shared header (patches/include, ./include, or the firmware headers) would
# not rebuild the affected objects and stale .o files would be silently reused.
DEPS=$(OBJS:.o=.d)
-include $(DEPS)

CFLAGS= \
	-fplugin=$(CCPLUGIN) \
	-fplugin-arg-nexmon-objfile=obj/$*.o \
	-fplugin-arg-nexmon-prefile=obj/$*.pre.tmp \
	-fplugin-arg-nexmon-chipver=$(NEXMON_CHIP_NUM) \
	-fplugin-arg-nexmon-fwver=$(NEXMON_FW_VERSION_NUM) \
	-DNEXMON_CHIP=$(NEXMON_CHIP) \
	-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
	-DWLC_UCODE_WRITE_BL_HOOK_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_ADDR) \
	-DHNDRTE_RECLAIM_0_END_PTR=$(HNDRTE_RECLAIM_0_END_PTR) \
	-DTEMPLATERAMSTART_PTR=$(TEMPLATERAMSTART_PTR) \
	-DPATCHSTART=$(PATCHSTART) \
	-DUCODESIZE=$(UCODESIZE) \
	-DVERSION_PTR_1=$(VERSION_PTR_1) \
	-DVERSION_PTR_2=$(VERSION_PTR_2) \
	-DVERSION_PTR_3=$(VERSION_PTR_3) \
	-DVERSION_PTR_4=$(VERSION_PTR_4) \
	-DDATE_PTR=$(DATE_PTR) \
	-DTIME_PTR=$(TIME_PTR) \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-std=gnu11 -Wall -Werror -Wno-unused-variable -Wno-unused-function -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -fno-align-functions -march=$(NEXMON_ARCH) \
	-ffunction-sections -fdata-sections \
	-MMD -MP \
	-I$(NEXMON_ROOT)/patches/include \
	-Iinclude \
	-I$(FW_PATH)

all: $(RAM_FILE) brcmfmac.ko

firmware-only: $(RAM_FILE)

init: check-nexmon-setup-env
	$(Q)mkdir -p obj gen log
	$(Q)if ! test -f BUILD_NUMBER; then echo 0 > BUILD_NUMBER; fi
# By default every build increments BUILD_NUMBER and rebuilds version.o. Keep
# this explicit rather than touching a source while make is checking its mtime.
# Set NEXMON_FREEZE_BUILD_NUMBER=1 to keep the number fixed and only rebuild
# version.o when its inputs actually changed.
ifndef NEXMON_FREEZE_BUILD_NUMBER
	$(Q)echo $$(($$(cat BUILD_NUMBER) + 1)) > BUILD_NUMBER
endif
	$(Q)$(MAKE) -s -f $(NEXMON_ROOT)/patches/common/header.mk

ifndef NEXMON_FREEZE_BUILD_NUMBER
obj/version.o obj/version.pre: FORCE
endif

# These can be requested directly or built concurrently with the objects.
gen/memory.ld gen/ucode.asm gen/ucode.bin gen/ucode_compressed.bin \
src/ucode_compressed.c src/templateram.c: | init

brcmfmac.ko: check-nexmon-setup-env
ifneq ("$(wildcard $(DRIVER_FOLDER_NAME))", "")
ifeq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l))
	@printf "\033[0;31m  BUILDING DRIVER for kernel $(KERNEL_VERSION)\033[0m brcmfmac_$(KERNEL_VERSION).y-nexmon/brcmfmac.ko (details: log/driver.log)\n" $@
	$(Q)mkdir -p log
	$(Q)KBUILD_MODPOST_WARN=1 $(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(DRIVER_FOLDER_NAME) $(KBUILD_EXTRA_SYMV) >log/driver.log
else ifeq ($(shell uname -m),$(filter $(shell uname -m), aarch64))
	@printf "\033[0;31m  BUILDING DRIVER for kernel $(KERNEL_VERSION)\033[0m brcmfmac_$(KERNEL_VERSION).y-nexmon/brcmfmac.ko (details: log/driver.log)\n" $@
	$(Q)mkdir -p log
	$(Q)ARCH="arm64" KBUILD_MODPOST_WARN=1 $(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(DRIVER_FOLDER_NAME) $(KBUILD_EXTRA_SYMV) >log/driver.log
else
	$(warning Warning: Driver can not be compiled on this platform, execute the make command on a Raspberry Pi)
endif
else
	$(warning Warning: Driver for kernel $(KERNEL_VERSION) not found)
endif

# A multi-target pattern rule produces the object and its plugin metadata in
# one invocation, including when an old build has objects but no .pre files.
# The plugin appends, so start with an empty private file and publish it only
# after successful compilation. No compiler rewrites another object's metadata.
define compile_object
	@printf "\033[0;31m  COMPILING\033[0m %s => obj/$*.o (details: log/compiler.log)\n" $<
	$(Q): > obj/$*.pre.tmp
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o obj/$*.o >>log/compiler.log && mv obj/$*.pre.tmp obj/$*.pre
endef

obj/%.o obj/%.pre: src/%.c | init
	$(compile_object)

obj/%.o obj/%.pre: $(NEXMON_ROOT)/patches/common/%.c | init
	$(compile_object)

obj/%.o obj/%.pre: $(FW_PATH)/%.c | init
	$(compile_object)

# Merge only the current source set, in a stable order, once all compilers have
# finished. FORCE also handles removed sources; unchanged metadata keeps its
# timestamp so a frozen incremental build need not regenerate linker scripts.
gen/nexmon.pre: $(OBJS) $(PREFILES) FORCE | init
	$(Q)cat $(PREFILES) > $@.tmp
	$(Q)if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

gen/nexmon.ld: gen/nexmon.pre
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon.pre | gawk -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.ld.awk > $@

gen/nexmon.mk: gen/nexmon.pre gen/patch.elf $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)printf "$(RAM_FILE): gen/patch.elf FORCE\n" > $@
	$(Q)sort gen/nexmon.pre | \
		gawk -v src_file=gen/patch.elf -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.1.awk | \
		gawk -v ramstart=$(RAMSTART) -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.2.awk >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > $@.tmp && mv $@.tmp $@

gen/flashpatches.ld: gen/nexmon.pre
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon.pre | \
		gawk -f $(NEXMON_ROOT)/buildtools/scripts/flashpatches.ld.awk > $@

gen/flashpatches.mk: gen/nexmon.pre $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)cat gen/nexmon.pre | gawk \
		-v fp_data_base=$(FP_DATA_BASE) \
		-v fp_config_base=$(FP_CONFIG_BASE) \
		-v fp_data_end_ptr=$(FP_DATA_END_PTR) \
		-v fp_config_base_ptr_1=$(FP_CONFIG_BASE_PTR_1) \
		-v fp_config_end_ptr_1=$(FP_CONFIG_END_PTR_1) \
		-v fp_config_base_ptr_2=$(FP_CONFIG_BASE_PTR_2) \
		-v fp_config_end_ptr_2=$(FP_CONFIG_END_PTR_2) \
		-v ramstart=$(RAMSTART) \
		-v out_file=$(RAM_FILE) \
		-v src_file=gen/patch.elf \
		-f $(NEXMON_ROOT)/buildtools/scripts/flashpatches.mk.awk > $@

gen/memory.ld: $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m %s\n" $@
	$(Q)printf "rom : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(ROMSTART) $(ROMSIZE) > $@
	$(Q)printf "ram : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(RAMSTART) $(RAMSIZE) >> $@
	$(Q)printf "patch : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(PATCHSTART) $(PATCHSIZE) >> $@
	$(Q)printf "ucode : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $$(($(UCODESTART) - 4)) $$(($(FP_CONFIG_BASE) - $(UCODESTART) + 4)) >> $@
	$(Q)printf "fpconfig : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(FP_CONFIG_BASE) $(FP_CONFIG_SIZE) >> $@

gen/patch.elf: patch.ld gen/nexmon.ld gen/flashpatches.ld gen/memory.ld $(OBJS)
	@printf "\033[0;31m  LINKING OBJECTS\033[0m => %s (details: log/linker.log, log/linker.err)\n" $@
	$(Q)$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M >>log/linker.log 2>>log/linker.err

$(RAM_FILE): init gen/patch.elf $(FW_PATH)/$(RAM_FILE) gen/nexmon.mk gen/flashpatches.mk
	$(Q)cp $(FW_PATH)/$(RAM_FILE) $@
	@printf "\033[0;31m  APPLYING FLASHPATCHES\033[0m gen/flashpatches.mk => %s (details: log/flashpatches.log)\n" $@
	$(Q)$(MAKE) -f gen/flashpatches.mk >>log/flashpatches.log 2>>log/flashpatches.log
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon.mk => %s (details: log/patches.log)\n" $@
	$(Q)$(MAKE) -f gen/nexmon.mk >>log/patches.log 2>>log/patches.log

###################################################################
# ucode compression related
###################################################################

gen/ucode.asm: $(FW_PATH)/ucode.bin
	@printf "\033[0;31m  DISASSEMBLING UCODE\033[0m %s => %s\n" $< $@
	$(Q)$(NEXMON_ROOT)/buildtools/b43-unified/disassembler/b43-dasm $< $@ --arch 15 --format raw-le32
	$(Q)$(NEXMON_ROOT)/buildtools/b43-unified/debug/b43-beautifier --asmfile $@ --defs $(NEXMON_ROOT)/buildtools/b43-unified/debug/include > $@.tmp && mv $@.tmp $@

ifneq ($(wildcard src/ucode.asm), )
gen/ucode.bin: src/ucode.asm
	@printf "\033[0;31m  ASSEMBLING UCODE\033[0m %s => %s\n" $< $@

ifneq ($(wildcard $(NEXMON_ROOT)/buildtools/b43-unified/assembler/b43-asm.bin), )
	$(Q)PATH="$(PATH):$(NEXMON_ROOT)/buildtools/b43-unified/assembler" $(NEXMON_ROOT)/buildtools/b43-unified/assembler/b43-asm $< $@ --format raw-le32
else
	$(error Warning: please compile b43-asm.bin first)
endif

else
gen/ucode.bin: $(FW_PATH)/ucode.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@
endif

gen/ucode_compressed.bin: gen/ucode.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)$(ZLIBFLATE) < $< > $@

src/ucode_compressed.c: gen/ucode_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

src/templateram.c: $(FW_PATH)/templateram.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

###################################################################

check-nexmon-setup-env:
ifndef NEXMON_SETUP_ENV
	$(error run 'source setup_env.sh' first in the repository\'s root directory)
endif

backup-firmware:
ifeq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l aarch64))
	cp /lib/firmware/brcm/$(RAM_FILE) $(RAM_FILE).orig
else
	$(warning Warning: Cannot backup the original firmware on this arch.)
endif

install-firmware: $(RAM_FILE) brcmfmac.ko
ifeq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l aarch64))
	@printf "\033[0;31m  COPYING\033[0m %s => /lib/firmware/nexmon/%s\n" $(RAM_FILE) $(RAM_FILE)
	$(Q)mkdir -p /lib/firmware/nexmon
	$(Q)cp $(RAM_FILE) /lib/firmware/nexmon/$(RAM_FILE)
	@printf "\033[0;31m  SETTING UP ALTERNATIVE\033[0m cyfmac43455-sdio.bin to /lib/firmware/nexmon/%s\n" $(RAM_FILE)
	$(Q)update-alternatives --install /lib/firmware/cypress/cyfmac43455-sdio.bin cyfmac43455-sdio.bin /lib/firmware/nexmon/$(RAM_FILE) 30
	$(Q)update-alternatives --set cyfmac43455-sdio.bin /lib/firmware/nexmon/$(RAM_FILE)
ifneq ("$(wildcard $(DRIVER_FOLDER_NAME)/brcmfmac.ko)", "")
	@for module in $$(lsmod | grep "^brcmfmac" | awk '{print $$1}'); do \
		printf "\033[0;31m  UNLOADING\033[0m $$module\n"; \
		rmmod $$module; \
	done
	$(Q)modprobe brcmutil
	@printf "\033[0;31m  RELOADING\033[0m brcmfmac\n"
	$(Q)insmod $(DRIVER_FOLDER_NAME)/brcmfmac.ko
else
	$(warning Warning: Driver not compiled)
endif
else
	$(warning Warning: Cannot install firmware on this arch, $(RAM_FILE) needs to be manually installed on your Raspberry Pi)
endif

clean-firmware: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr $(RAM_FILE) obj gen log src/ucode_compressed.c src/templateram.c

clean: clean-firmware
ifeq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l aarch64))
ifneq ("$(wildcard $(DRIVER_FOLDER_NAME))", "")
	@printf "\033[0;31m  CLEANING DRIVER\033[0m\n" $@
	$(Q)$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(DRIVER_FOLDER_NAME) clean
endif
endif
	$(Q)rm -f BUILD_NUMBER

.PHONY: all firmware-only init check-nexmon-setup-env clean clean-firmware FORCE

FORCE:
