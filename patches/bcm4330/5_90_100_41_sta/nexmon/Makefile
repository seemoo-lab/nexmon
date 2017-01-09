GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
FW_PATH=$(NEXMON_ROOT)/firmwares/bcm4330/5_90_100_41_sta
include $(FW_PATH)/definitions.mk

HEADER_SRCS=$(wildcard $(NEXMON_ROOT)/patches/include/*.h) $(wildcard $(FW_PATH)/.h)
LOCAL_SRCS=$(wildcard src/*.c) src/ucode_compressed.c
COMMON_SRCS=$(wildcard $(NEXMON_ROOT)/patches/common/*.c)
FW_SRCS=$(wildcard $(FW_PATH)/*.c)

OBJS=$(addprefix obj/,$(notdir $(LOCAL_SRCS:.c=.o)) $(notdir $(COMMON_SRCS:.c=.o)) $(notdir $(FW_SRCS:.c=.o)))

CFLAGS= \
	-fplugin=$(CCPLUGIN) \
	-fplugin-arg-nexmon-objfile=$@ \
	-fplugin-arg-nexmon-prefile=gen/nexmon.pre \
	-fplugin-arg-nexmon-chipver=$(NEXMON_CHIP_NUM) \
	-fplugin-arg-nexmon-fwver=$(NEXMON_FW_VERSION_NUM) \
	-DNEXMON_CHIP=$(NEXMON_CHIP) \
	-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=$(NEXMON_ARCH) \
	-ffunction-sections -fdata-sections \
	-I$(NEXMON_ROOT)/patches/include \
	-I$(FW_PATH)

all: $(RAM_FILE)

init: FORCE
	$(Q)if ! test -f BUILD_NUMBER; then echo 0 > BUILD_NUMBER; fi
	$(Q)echo $$(($$(cat BUILD_NUMBER) + 1)) > BUILD_NUMBER
	$(Q)touch src/version.c
	$(Q)make -s -f $(NEXMON_ROOT)/patches/common/header.mk
	$(Q)mkdir -p obj gen log

obj/%.o: src/%.c $(HEADER_SRCS)
	@printf "\033[0;31m  COMPILING\033[0m %s => %s (details: log/compiler.log)\n" $< $@
	$(Q)cat gen/nexmon.pre 2>>log/error.log | gawk '{ if ($$3 != "$@") print; }' > tmp && mv tmp gen/nexmon.pre
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o $@ >>log/compiler.log

obj/%.o: $(NEXMON_ROOT)/patches/common/%.c $(HEADER_SRCS)
	@printf "\033[0;31m  COMPILING\033[0m %s => %s (details: log/compiler.log)\n" $< $@
	$(Q)cat gen/nexmon.pre 2>>log/error.log | gawk '{ if ($$3 != "$@") print; }' > tmp && mv tmp gen/nexmon.pre
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o $@ >>log/compiler.log

obj/%.o: $(FW_PATH)/%.c $(HEADER_SRCS)
	@printf "\033[0;31m  COMPILING\033[0m %s => %s (details: log/compiler.log)\n" $< $@
	$(Q)cat gen/nexmon.pre 2>>log/error.log | gawk '{ if ($$3 != "$@") print; }' > tmp && mv tmp gen/nexmon.pre
	$(Q)$(CC)gcc $(CFLAGS) -c $< -o $@ >>log/compiler.log

gen/nexmon.ld: $(OBJS)
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon.pre | gawk -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.ld.awk > $@

gen/nexmon.mk: $(OBJS) $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)printf "%s: gen/patch.elf FORCE\n" $(RAM_FILE) > $@
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
	$(Q)printf "ucode : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(UCODESTART) $$(($(UCODESIZE) - $(FP_CONFIG_SIZE))) >> $@
	$(Q)printf "fpconfig : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(FP_CONFIG_BASE) $(FP_CONFIG_SIZE) >> $@

gen/patch.elf: patch.ld gen/nexmon.ld gen/flashpatches.ld gen/memory.ld $(OBJS)
	@printf "\033[0;31m  LINKING OBJECTS\033[0m => %s (details: log/linker.log)\n" $@
	$(Q)$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M >>log/linker.log 2>>log/linker.log

$(RAM_FILE): init gen/patch.elf $(FW_PATH)/$(RAM_FILE) gen/nexmon.mk gen/flashpatches.mk
	$(Q)cp $(FW_PATH)/$(RAM_FILE) $@
	@printf "\033[0;31m  APPLYING FLASHPATCHES\033[0m gen/flashpatches.mk => %s (details: log/flashpatches.log)\n" $@
	$(Q)make -f gen/flashpatches.mk >>log/flashpatches.log 2>>log/flashpatches.log
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon.mk => %s (details: log/patches.log)\n" $@
	$(Q)make -f gen/nexmon.mk >>log/patches.log 2>>log/flashpatches.log

###################################################################
# ucode compression related
###################################################################

ucode_compressed.bin: $(FW_PATH)/ucode.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/ucode_compressed.c: ucode_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)xxd -i $< >> $@

###################################################################

check-nexmon-setup-env:
ifndef NEXMON_SETUP_ENV
	$(error run 'source setup_env.sh' first in the repository\'s root directory)
endif

install-firmware: $(RAM_FILE)
	@printf "\033[0;31m  REMOUNTING /system\033[0m\n"
	$(Q)adb shell 'su -c "mount -o rw,remount /system"'
	@printf "\033[0;31m  COPYING TO PHONE\033[0m %s => /sdcard/%s\n" $< $<
	$(Q)adb push $< /sdcard/ >> log/adb.log 2>> log/adb.log
	@printf "\033[0;31m  COPYING\033[0m /sdcard/bcmdhd_sta.bin => /system/etc/wifi/bcmdhd_sta.bin\n"
	$(Q)adb shell 'su -c "cp /sdcard/bcmdhd_sta.bin /system/etc/wifi/bcmdhd_sta.bin"'
	@printf "\033[0;31m  REBOOTING TO RELOAD FIRMWARE\033[0m\n"
	$(Q)adb shell 'reboot'

backup-firmware: FORCE
	adb shell 'su -c "cp /vendor/firmware/fw_bcmdhd.bin /sdcard/fw_bcmdhd.orig.bin"'
	adb pull /sdcard/fw_bcmdhd.orig.bin

install-backup: fw_bcmdhd.orig.bin
	adb shell 'su -c "mount -o rw,remount /system"' && \
	adb push $< /sdcard/ && \
	adb shell 'su -c "cp /sdcard/fw_bcmdhd.bin /vendor/firmware/fw_bcmdhd.bin"'
	adb shell 'su -c "ifconfig wlan0 down && ifconfig wlan0 up"'

clean-firmware: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr $(RAM_FILE) obj gen log src/ucode_compressed.c ucode_compressed.bin

clean: clean-firmware
	$(Q)rm -f BUILD_NUMBER

FORCE:
