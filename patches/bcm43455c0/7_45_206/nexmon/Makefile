SHELL = /bin/bash
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
include ../version.mk
include $(FW_PATH)/definitions.mk

LOCAL_SRCS=$(wildcard src/*.c) src/ucode_compressed.c
COMMON_SRCS=$(wildcard $(NEXMON_ROOT)/patches/common/*.c)
FW_SRCS=$(wildcard $(FW_PATH)/*.c)

OBJS=$(addprefix obj/,$(notdir $(LOCAL_SRCS:.c=.o)) $(notdir $(COMMON_SRCS:.c=.o)) $(notdir $(FW_SRCS:.c=.o)))

KERNEL_VERSION = $(shell uname -r | sed 's/\([0-9]\+\.[0-9]\+\)\..*/\1/')
DRIVER_FOLDER_NAME = $(NEXMON_ROOT)/patches/driver/brcmfmac_$(KERNEL_VERSION).y-nexmon

CFLAGS= \
	-fplugin=$(CCPLUGIN) \
	-fplugin-arg-nexmon-objfile=$@ \
	-fplugin-arg-nexmon-prefile=gen/nexmon.pre \
	-fplugin-arg-nexmon-chipver=$(NEXMON_CHIP_NUM) \
	-fplugin-arg-nexmon-fwver=$(NEXMON_FW_VERSION_NUM) \
	-DNEXMON_CHIP=$(NEXMON_CHIP) \
	-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
	-DWLC_UCODE_WRITE_BL_HOOK_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_ADDR) \
	-DHNDRTE_RECLAIM_0_END_PTR=$(HNDRTE_RECLAIM_0_END_PTR) \
	-DTEMPLATERAMSTART_PTR=$(TEMPLATERAMSTART_PTR) \
	-DPATCHSTART=$(PATCHSTART) \
	-DUCODESIZE=$(UCODESIZE) \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=$(NEXMON_ARCH) \
	-ffunction-sections -fdata-sections \
	-I$(NEXMON_ROOT)/patches/include \
	-Iinclude \
	-I$(FW_PATH)

all: $(RAM_FILE) brcmfmac.ko

init: FORCE
	$(Q)if ! test -f BUILD_NUMBER; then echo 0 > BUILD_NUMBER; fi
	$(Q)echo $$(($$(cat BUILD_NUMBER) + 1)) > BUILD_NUMBER
	$(Q)touch src/version.c
	$(Q)make -s -f $(NEXMON_ROOT)/patches/common/header.mk
	$(Q)mkdir -p obj gen log

brcmfmac.ko: check-nexmon-setup-env
ifneq ("$(wildcard $(DRIVER_FOLDER_NAME))", "")
ifeq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l))
	@printf "\033[0;31m  BUILDING DRIVER for kernel $(KERNEL_VERSION)\033[0m brcmfmac_$(KERNEL_VERSION).y-nexmon/brcmfmac.ko (details: log/driver.log)\n" $@
	$(Q)mkdir -p log
	$(Q)make -C /lib/modules/$(shell uname -r)/build M=$(DRIVER_FOLDER_NAME) -j4 >log/driver.log
else ifeq ($(shell uname -m),$(filter $(shell uname -m), aarch64))
	@printf "\033[0;31m  BUILDING DRIVER for kernel $(KERNEL_VERSION)\033[0m brcmfmac_$(KERNEL_VERSION).y-nexmon/brcmfmac.ko (details: log/driver.log)\n" $@
	$(Q)mkdir -p log
	$(Q)ARCH="arm64" make -C /lib/modules/$(shell uname -r)/build M=$(DRIVER_FOLDER_NAME) -j4 >log/driver.log
else
	$(warning Warning: Driver can not be compiled on this platform, execute the make command on a raspberry pi)
endif
else
	$(warning Warning: Driver for kernel $(KERNEL_VERSION) not found)
endif

obj/%.o: src/%.c
	@printf "\033[0;31m  COMPILING\033[0m %s => %s (details: log/compiler.log)\n" $< $@
	$(Q)cat gen/nexmon.pre 2>>log/error.log | gawk '{ if ($$3 != "$@") print; }' > tmp && mv tmp gen/nexmon.pre
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o $@ >>log/compiler.log

obj/%.o: $(NEXMON_ROOT)/patches/common/%.c
	@printf "\033[0;31m  COMPILING\033[0m %s => %s (details: log/compiler.log)\n" $< $@
	$(Q)cat gen/nexmon.pre 2>>log/error.log | gawk '{ if ($$3 != "$@") print; }' > tmp && mv tmp gen/nexmon.pre
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o $@ >>log/compiler.log

obj/%.o: $(FW_PATH)/%.c
	@printf "\033[0;31m  COMPILING\033[0m %s => %s (details: log/compiler.log)\n" $< $@
	$(Q)cat gen/nexmon.pre 2>>log/error.log | gawk '{ if ($$3 != "$@") print; }' > tmp && mv tmp gen/nexmon.pre
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o $@ >>log/compiler.log

gen/nexmon.ld: $(OBJS)
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon.pre | gawk -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.ld.awk > $@

gen/nexmon.mk: $(OBJS) $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)printf "$(RAM_FILE): gen/patch.elf FORCE\n" > $@
	$(Q)sort gen/nexmon.pre | \
		gawk -v src_file=gen/patch.elf -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.1.awk | \
		gawk -v ramstart=$(RAMSTART) -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.2.awk >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > tmp && mv tmp $@

gen/flashpatches.ld: $(OBJS)
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon.pre | \
		gawk -f $(NEXMON_ROOT)/buildtools/scripts/flashpatches.ld.awk > $@

gen/flashpatches.mk: $(OBJS) $(FW_PATH)/definitions.mk
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
	$(Q)printf "ucode : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(UCODESTART) $$(($(UCODESIZE) - $(PATCHSIZE))) >> $@
	$(Q)printf "fpconfig : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(FP_CONFIG_BASE) $(FP_CONFIG_SIZE) >> $@

gen/patch.elf: patch.ld gen/nexmon.ld gen/flashpatches.ld gen/memory.ld $(OBJS)
	@printf "\033[0;31m  LINKING OBJECTS\033[0m => %s (details: log/linker.log, log/linker.err)\n" $@
	$(Q)$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M >>log/linker.log 2>>log/linker.err

$(RAM_FILE): init gen/patch.elf $(FW_PATH)/$(RAM_FILE) gen/nexmon.mk gen/flashpatches.mk
	$(Q)cp $(FW_PATH)/$(RAM_FILE) $@
	@printf "\033[0;31m  APPLYING FLASHPATCHES\033[0m gen/flashpatches.mk => %s (details: log/flashpatches.log)\n" $@
	$(Q)make -f gen/flashpatches.mk >>log/flashpatches.log 2>>log/flashpatches.log
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon.mk => %s (details: log/patches.log)\n" $@
	$(Q)make -f gen/nexmon.mk >>log/patches.log 2>>log/flashpatches.log

gen/dummy.idc: $(RAM_FILE)
	@printf "\033[0;31m  GENERATING IDC FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)cat gen/nexmon.pre | gawk 'BEGIN { printf "#import <idc.idc>\n\nstatic main() {\n" } { if (!seen[$$0]++ && $$2 == "DUMMY") { printf "\tMakeName(%s,\"%s\");\n",$$1,$$4 }; } END { printf "}\n" }' > $@

###################################################################
# ucode compression related
###################################################################

gen/ucode.asm: $(FW_PATH)/ucode.bin
	@printf "\033[0;31m  DISASSEMBLING UCODE\033[0m %s => %s\n" $< $@
	$(Q)$(NEXMON_ROOT)/buildtools/b43/disassembler/b43-dasm $< $@ --arch 15 --format raw-le32
	$(Q)$(NEXMON_ROOT)/buildtools/b43/debug/b43-beautifier --asmfile $@ --defs $(NEXMON_ROOT)/buildtools/b43/debug/include > tmp && mv tmp $@

ifneq ($(wildcard src/ucode.asm), )
gen/ucode.bin: src/ucode.asm
	@printf "\033[0;31m  ASSEMBLING UCODE\033[0m %s => %s\n" $< $@

ifneq ($(wildcard $(NEXMON_ROOT)/buildtools/b43/assembler/b43-asm.bin), )
	$(Q)PATH=$(PATH):$(NEXMON_ROOT)/buildtools/b43/assembler $(NEXMON_ROOT)/buildtools/b43/assembler/b43-asm $< $@ --format raw-le32
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
	$(Q)cat $< | $(ZLIBFLATE) > $@

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
	@printf "\033[0;31m  COPYING\033[0m $(RAM_FILE) => /lib/firmware/brcm/$(RAM_FILE)\n"
	$(Q)cp $(RAM_FILE) /lib/firmware/brcm/$(RAM_FILE)
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
	$(warning Warning: Cannot install firmware on this arch, $(RAM_FILE) needs to be copied manually into /lib/firmware/brcm/ on your RPI3)
endif

clean-firmware: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr $(RAM_FILE) obj gen log src/ucode_compressed.c src/templateram.c ucode_compressed.bin

clean: clean-firmware
ifeq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l aarch64))
ifneq ("$(wildcard $(DRIVER_FOLDER_NAME))", "")
	@printf "\033[0;31m  CLEANING DRIVER\033[0m\n" $@
	$(Q)make -C /lib/modules/$(shell uname -r)/build M=$(DRIVER_FOLDER_NAME) clean
endif
endif
	$(Q)rm -f BUILD_NUMBER

FORCE:
