GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
SHELL := /bin/bash
include ../version.mk
include $(FW_PATH)/definitions.mk

LOCAL_SRCS=$(wildcard src/*.c) src/ucode0_compressed.c src/ucode1_compressed.c src/ucode2_compressed.c
COMMON_SRCS=$(NEXMON_ROOT)/patches/common/ucode_compression_code.c $(NEXMON_ROOT)/patches/common/objmem.c $(NEXMON_ROOT)/patches/common/argprintf.c $(NEXMON_ROOT)/patches/common/wrapper.c
FW_SRCS=$(wildcard $(FW_PATH)/*.c)

OBJS=$(addprefix obj/,$(notdir $(LOCAL_SRCS:.c=.o)) $(notdir $(COMMON_SRCS:.c=.o)) $(notdir $(FW_SRCS:.c=.o)))

DATETIME := $(shell date -u +'%b %e %Y %T')

CFLAGS= \
	-fplugin=$(CCPLUGIN) \
	-fplugin-arg-nexmon-objfile=$@ \
	-fplugin-arg-nexmon-prefile=gen/nexmon.pre \
	-fplugin-arg-nexmon-chipver=$(NEXMON_CHIP_NUM) \
	-fplugin-arg-nexmon-fwver=$(NEXMON_FW_VERSION_NUM) \
	-fno-strict-aliasing \
	-DNEXMON_CHIP=$(NEXMON_CHIP) \
	-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
	-DWLC_UCODE_WRITE_BL_HOOK_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_ADDR) \
	-DRECLAIM_3_END_PTR_0=$(RECLAIM_3_END_PTR_0) \
	-DRECLAIM_3_END_PTR_1=$(RECLAIM_3_END_PTR_1) \
	-DRECLAIM_3_END_PTR_2=$(RECLAIM_3_END_PTR_2) \
	-DFP_CONFIG_ORIGEND=$(FP_CONFIG_ORIGEND) \
	-DFP_CONFIG_ORIGBASE=$(FP_CONFIG_ORIGBASE) \
	-DPATCHSTART=$(PATCHSTART) \
	-DTINFLATE_OUTPUT_SIZE=$(TINFLATE_OUTPUT_SIZE) \
	-DUCODE0START_PTR=$(UCODE0START_PTR) \
	-DUCODE0SIZE_PTR=$(UCODE0SIZE_PTR) \
	-DUCODE1START_PTR=$(UCODE1START_PTR) \
	-DUCODE1SIZE_PTR=$(UCODE1SIZE_PTR) \
	-DUCODE2START_PTR=$(UCODE2START_PTR) \
	-DUCODE2SIZE_PTR=$(UCODE2SIZE_PTR) \
	-DVERSION_PTR_1=$(VERSION_PTR_1) \
	-DVERSION_PTR_2=$(VERSION_PTR_2) \
	-DVERSION_PTR_3=$(VERSION_PTR_3) \
	-DVERSION_PTR_4=$(VERSION_PTR_4) \
	-DDATETIME_PTR=$(DATETIME_PTR) \
	-DDATETIME=\""$(DATETIME)"\" \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=$(NEXMON_ARCH) \
	-Wno-unused-function \
	-ffunction-sections -fdata-sections \
	-I$(NEXMON_ROOT)/patches/include \
	-Iinclude \
	-I$(FW_PATH)

all: nexmon-magisk.zip

nexmon-magisk/system/vendor/firmware:
	@printf "\033[0;31m  CREATING\033[0m magisk module structure %s\n" $@
	$(Q)mkdir -p $@

nexmon-magisk/system/bin:
	@printf "\033[0;31m  CREATING\033[0m magisk module structure %s\n" $@
	$(Q)mkdir -p $@

nexmon-magisk/system/bin/nexutil: $(NEXMON_ROOT)/utilities/nexutil nexmon-magisk/system/bin
	@printf "\033[0;31m  BUILDING\033[0m nexutil %s\n" $@
	$(Q)make APP_ABI=arm64-v8a -C $<
	$(Q)cp $(NEXMON_ROOT)/utilities/nexutil/libs/arm64-v8a/nexutil $@

nexmon-magisk.zip: $(RAM_FILE) nexmon-magisk nexmon-magisk/system/bin/nexutil nexmon-magisk/system/vendor/firmware
	@printf "\033[0;31m  BUILDING\033[0m magisk module %s (details: log/magisk.log)\n" $@
	$(Q)cp $< nexmon-magisk/system/vendor/firmware/
	$(Q)cd nexmon-magisk && zip -r -Z deflate ../$@ * 2>&1 > ../log/magisk.log && cd ..

init: FORCE
	$(Q)if ! test -f BUILD_NUMBER; then echo 0 > BUILD_NUMBER; fi
	$(Q)echo $$(($$(cat BUILD_NUMBER) + 1)) > BUILD_NUMBER
	$(Q)touch src/version.c
	$(Q)make -s -f $(NEXMON_ROOT)/patches/common/header.mk
	$(Q)mkdir -p obj gen log

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

gen/nexmon2.pre: $(OBJS)
	@printf "\033[0;31m  PREPARING\033[0m %s => %s\n" "gen/nexmon.pre" $@
	$(Q)cat gen/nexmon.pre | awk '{ if ($$3 != "obj/flashpatches.o" && $$3 != "obj/wrapper.o") { print $$0; } }' > tmp
	$(Q)cat gen/nexmon.pre | awk '{ if ($$3 == "obj/flashpatches.o" || $$3 == "obj/wrapper.o") { print $$0; } }' >> tmp
	$(Q)cat tmp | awk '{ if ($$1 ~ /^0x/) { if ($$3 != "obj/flashpatches.o" && $$3 != "obj/wrapper.o") { if (!x[$$1]++) { print $$0; } } else { if (!x[$$1]) { print $$0; } } } else { print $$0; } }' > gen/nexmon2.pre

gen/nexmon.ld: gen/nexmon2.pre $(OBJS)
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon2.pre | gawk -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.ld.awk > $@

gen/nexmon.mk: gen/nexmon2.pre $(OBJS) $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)printf "$(RAM_FILE): gen/patch.elf FORCE\n" > $@
	$(Q)sort gen/nexmon2.pre | \
		gawk -v src_file=gen/patch.elf -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.1.awk | \
		gawk -v ramstart=$(RAMSTART) -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.2.awk >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > tmp && mv tmp $@

gen/memory.ld: $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m %s\n" $@
	$(Q)printf "rom : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(ROMSTART) $(ROMSIZE) > $@
	$(Q)printf "ram : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(RAMSTART) $(RAMSIZE) >> $@
	$(Q)printf "ucode : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $$(($(UCODE0START) - 4)) $$(($(PATCHSTART) - $(UCODE0START) + 4)) >> $@
	$(Q)printf "patch : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(PATCHSTART) $(PATCHSIZE) >> $@

gen/patch.elf: patch.ld gen/nexmon.ld gen/memory.ld $(OBJS)
	@printf "\033[0;31m  LINKING OBJECTS\033[0m => %s (details: log/linker.log, log/linker.err)\n" $@
	$(Q)$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M >>log/linker.log 2>>log/linker.err

$(RAM_FILE): init gen/patch.elf $(FW_PATH)/$(RAM_FILE) gen/nexmon.mk
	$(Q)cp $(FW_PATH)/$(RAM_FILE) $@
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon.mk => %s (details: log/patches.log)\n" $@
	$(Q)make -f gen/nexmon.mk >>log/patches.log 2>>log/patches.log

###################################################################
# ucode compression related
###################################################################

gen/ucode0.bin: $(FW_PATH)/d11ucode_p2p_D11s.bin
	@printf "\033[0;31m  COPYING FIRST UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/ucode1.bin: $(FW_PATH)/d11ucode_p2p_D11a.bin
	@printf "\033[0;31m  COPYING SECOND UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/ucode2.bin: $(FW_PATH)/d11ucode_p2p_D11b.bin
	@printf "\033[0;31m  COPYING THIRD UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/ucode0_compressed.bin: gen/ucode0.bin
	@printf "\033[0;31m  COMPRESSING FIRST UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

gen/ucode1_compressed.bin: gen/ucode1.bin
	@printf "\033[0;31m  COMPRESSING SECOND UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

gen/ucode2_compressed.bin: gen/ucode2.bin
	@printf "\033[0;31m  COMPRESSING THIRD UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/ucode0_compressed.c: gen/ucode0_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

src/ucode1_compressed.c: gen/ucode1_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

src/ucode2_compressed.c: gen/ucode2_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

###################################################################

check-nexmon-setup-env:
ifndef NEXMON_SETUP_ENV
	$(error run 'source setup_env.sh' first in the repository\'s root directory)
endif

dump-rom: FORCE
	@printf "\033[0;31m  DUMPING ROM TO\033[0m /sdcard/rom.bin\n"
	$(Q)adb shell "su -c 'touch /sdcard/rom.bin'" && for n in {0..2340}; do adb shell "su -c 'nexutil -g0x602 -l1024 -i -v$$(($$n*1024+0x10000)) -r >> /sdcard/rom.bin'"; done
	@printf "\033[0;31m  COPY \033[0m %s bytes of /sdcard/rom.bin to %s/../rom.bin\n" $(ROMSIZE) $(FW_PATCH)
	$(Q)adb pull /sdcard/rom.bin tmp.bin
	$(Q)dd if=tmp.bin of=$(FW_PATH)/../rom.bin bs=1 count=$$(($(ROMSIZE))) conv=notrunc status=none
	$(Q)rm tmp.bin

backup-firmware:
	@printf "\033[0;31m  BACKING UP\033[0m /vendor/firmware/%s => %s.backup\n" $(RAM_FILE) $(RAM_FILE).backup
	$(Q)adb shell 'su -c "cp /vendor/firmware/$(RAM_FILE) /sdcard/$(RAM_FILE).backup"' 2>&1 >> log/adb.log
	$(Q)adb pull "/sdcard/$(RAM_FILE).backup" "$(RAM_FILE).backup" 2>&1 >> log/adb.log

install-firmware: nexmon-magisk.zip
	@printf "\033[0;31m  COPYING TO PHONE\033[0m %s => /sdcard/%s\n" $< $<
	$(Q)adb push $< /sdcard/ >> log/adb.log 2>> log/adb.log
	@printf "\033[0;31m  INSTALL\033[0m /sdcard/%s using Magisk\033[0m\n" $<
	$(Q)adb shell 'su -c "magisk --install-module /sdcard/$<"' 2>&1 >> log/adb.log
	@printf "\033[0;31m  REBOOT REQUIRED\033[0m manually run: adb reboot\033[0m\n" $<

clean-firmware: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr $(RAM_FILE) obj gen log src/ucode0_compressed.c src/ucode1_compressed.c src/ucode2_compressed.c nexmon-magisk.zip nexmon-magisk/system

clean: clean-firmware
	$(Q)rm -fr BUILD_NUMBER

FORCE:
