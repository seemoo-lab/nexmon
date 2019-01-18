GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
include ../version.mk
include $(FW_PATH)/definitions.mk

LOCAL_SRCS=$(wildcard src/*.c)
FW_SRCS=$(wildcard $(FW_PATH)/*.c)

OBJS=$(addprefix obj/,$(notdir $(LOCAL_SRCS:.c=.o)) $(notdir $(FW_SRCS:.c=.o)))

CFLAGS= \
	-fplugin=$(CCPLUGIN) \
	-fplugin-arg-nexmon-objfile=$@ \
	-fplugin-arg-nexmon-prefile=gen/nexmon.pre \
	-fplugin-arg-nexmon-chipver=$(NEXMON_CHIP_NUM) \
	-fplugin-arg-nexmon-fwver=$(NEXMON_FW_VERSION_NUM) \
	-fno-strict-aliasing \
	-DNEXMON_CHIP=$(NEXMON_CHIP) \
	-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
	-DPATCHSTART=$(PATCHSTART) \
	-DUCODESIZE=$(UCODESIZE) \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=$(NEXMON_ARCH) \
	-Wno-unused-label \
	-ffunction-sections -fdata-sections \
	-I$(NEXMON_ROOT)/patches/include \
	-Iinclude \
	-I$(FW_PATH)

all: bcm4335c0.hcd

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
	$(Q)rm -r gen/hcd_extracted || true
	$(Q)cp -r $(FW_PATH)/hcd_extracted gen/
	$(Q)rm gen/hcd_extracted/10_writeram_0x00218000.bin
	# Printing the content of the nexmon.mk file
	$(Q)printf "bcm4335c0.hcd: bcm4335c0.bin\n" > $@
	$(Q)printf "\t$(Q)cat gen/hcd_extracted/rompatches/* > gen/hcd_extracted/00_writeram_0x00218000.bin\n" >> $@
	$(Q)printf "\t$(Q)$(NEXMON_ROOT)/buildtools/hcd_generator/bthcd_generate -i gen/hcd_extracted -o bcm4335c0.hcd\n\n" >> $@
	$(Q)printf "bcm4335c0.bin: gen/patch.elf FORCE\n" >> $@
	$(Q)sort gen/nexmon2.pre | \
		gawk -v src_file=gen/patch.elf -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.1.awk | \
		gawk -v ramstart=0x0 -v initial_patch_nr=$(INITIAL_PATCH_NR) --non-decimal-data -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.2_bt.awk >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > tmp && mv tmp $@

gen/memory.ld: $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m %s\n" $@
	$(Q)printf "patch : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(PATCHSTART) $(PATCHSIZE) >> $@

gen/patch.elf: patch.ld gen/nexmon.ld gen/memory.ld $(OBJS)
	@printf "\033[0;31m  LINKING OBJECTS\033[0m => %s (details: log/linker.log, log/linker.err)\n" $@
	$(Q)$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M >>log/linker.log 2>>log/linker.err

bcm4335c0.hcd: init gen/patch.elf $(FW_PATH)/$(BIN_FILE) gen/nexmon.mk
	$(Q)cp $(FW_PATH)/$(BIN_FILE) $@
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon.mk => %s (details: log/patches.log)\n" $@
	$(Q)make -f gen/nexmon.mk >>log/patches.log 2>>log/flashpatches.log


check-nexmon-setup-env:
ifndef NEXMON_SETUP_ENV
	$(error run 'source setup_env.sh' first in the repository\'s root directory)
endif

backup-hcd: FORCE
	adb shell 'su -c "cp /vendor/firmware/bcm4335c0.hcd /sdcard/bcm4335c0.orig.hcd"'
	adb pull /sdcard/bcm4335c0.orig.hcd
	ls -l bcm4335c0.orig.hcd

install-patch: bcm4335c0.hcd
	@printf "\033[0;31m  INSTALLING PATCHES\033[0m %s => device (details: log/patches.log)\n" $<
	@if [ -f bcm4335c0.orig.hcd ]; then echo "Copying patched HCD file to device..."; else printf "\033[0;31m ERROR: NO HCD BACKUP FOUND. PLEASE RUN make backup-hcd FIRST!\033[0m \n"; false; fi;
	adb shell 'su -c "mount -o rw,remount /system"' && \
	adb push $< /sdcard/ && \
	adb shell 'su -c "cp /sdcard/bcm4335c0.hcd /vendor/firmware/bcm4335c0.hcd"'
	adb shell 'su -c "service call bluetooth_manager 8"'
	adb shell 'su -c "service call bluetooth_manager 6"'

install-backup: bcm4335c0.orig.hcd
	@printf "\033[0;31m  INSTALLING HCD BACKUP\033[0m %s => device (details: log/patches.log)\n" $<
	adb shell 'su -c "mount -o rw,remount /system"' && \
	adb push $< /sdcard/ && \
	adb shell 'su -c "cp /sdcard/bcm4335c0.orig.hcd /vendor/firmware/bcm4335c0.hcd"'
	adb shell 'su -c "service call bluetooth_manager 8"'
	adb shell 'su -c "service call bluetooth_manager 6"'

debug-install:
	@printf "\033[0;31m DEBUG INSTALLING PATCHES\033[0m %s => device (details: log/patches.log)\n" $<
	cat gen/hcd_extracted/rompatches/* > gen/hcd_extracted/00_writeram_0x00218000.bin
	/home/dennis/Downloads/nexmon/buildtools/hcd_generator/bthcd_generate -i gen/hcd_extracted -o bcm4335c0.hcd
	@if [ -f bcm4335c0.orig.hcd ]; then echo "Copying patched HCD file to device..."; else printf "\033[0;31m ERROR: NO HCD BACKUP FOUND. PLEASE RUN make backup-hcd FIRST!\033[0m \n"; false; fi;
	adb shell 'su -c "mount -o rw,remount /system"' && \
	adb push bcm4335c0.hcd /sdcard/ && \
	adb shell 'su -c "cp /sdcard/bcm4335c0.hcd /vendor/firmware/bcm4335c0.hcd"'
	adb shell 'su -c "service call bluetooth_manager 8"'
	adb shell 'su -c "service call bluetooth_manager 6"'

clean-firmware: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr bcm4335c0.hcd bcm4335c0.bin obj gen log

clean: clean-firmware
	$(Q)rm -f BUILD_NUMBER

FORCE:
