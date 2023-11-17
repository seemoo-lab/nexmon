ifndef NEXMON_SETUP_ENV
$(error run 'source setup_env.sh' first in the repository\'s root directory)
endif

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
SHELL := /bin/bash
include ../version.mk
include $(FW_PATH)/definitions.mk

LOCAL_SRCS=$(wildcard src/*.c) src/ucode0_compressed.c src/ucode1_compressed.c src/ucode2_compressed.c src/ucode3_compressed.c
COMMON_SRCS=$(NEXMON_ROOT)/patches/common/ucode_compression_code.c $(NEXMON_ROOT)/patches/common/objmem.c $(NEXMON_ROOT)/patches/common/wrapper.c
FW_SRCS=$(wildcard $(FW_PATH)/*.c)

OBJS=$(addprefix obj/,$(notdir $(LOCAL_SRCS:.c=.o)) $(notdir $(COMMON_SRCS:.c=.o)) $(notdir $(FW_SRCS:.c=.o)))

DATE := $(shell date -u +'%b %e %Y')
TIME := $(shell date -u +'%T')

CFLAGS= \
	-fplugin=$(CCPLUGIN) \
	-fplugin-arg-nexmon-objfile=$@ \
	-fplugin-arg-nexmon-prefile=gen/nexmon.pre \
	-fplugin-arg-nexmon-chipver=$(NEXMON_CHIP_NUM) \
	-fplugin-arg-nexmon-fwver=$(NEXMON_FW_VERSION_NUM) \
	-fno-strict-aliasing \
	-DNEXMON_CHIP=$(NEXMON_CHIP) \
	-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
	-DWLC_BMAC_PROCESS_UCODE_SR_BL_HOOK_ADDR=$(WLC_BMAC_PROCESS_UCODE_SR_BL_HOOK_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_0_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_0_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_1_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_1_ADDR) \
	-DWLC_UCODEX_WRITE_BL_HOOK_ADDR=$(WLC_UCODEX_WRITE_BL_HOOK_ADDR) \
	-DRECLAIM_3_END_PTR=$(RECLAIM_3_END_PTR) \
	-DPATCHSTART=$(PATCHSTART) \
	-DUCODE0START_PTR=$(UCODE0START_PTR) \
	-DUCODE0SIZE_PTR=$(UCODE0SIZE_PTR) \
	-DUCODE1START_PTR=$(UCODE1START_PTR) \
	-DUCODE1SIZE_PTR=$(UCODE1SIZE_PTR) \
	-DUCODE2START_PTR=$(UCODE2START_PTR) \
	-DUCODE2SIZE_PTR=$(UCODE2SIZE_PTR) \
	-DUCODE3START_PTR=$(UCODE3START_PTR) \
	-DUCODE3SIZE_PTR=$(UCODE3SIZE_PTR) \
	-DVERSION_PTR_1=$(VERSION_PTR_1) \
	-DVERSION_PTR_2=$(VERSION_PTR_2) \
	-DVERSION_PTR_3=$(VERSION_PTR_3) \
	-DVERSION_PTR_4=$(VERSION_PTR_4) \
	-DDATE_PTR=$(DATE_PTR) \
	-DTIME_PTR=$(TIME_PTR) \
	-DDATE=\""$(DATE)"\" \
	-DTIME=\""$(TIME)"\" \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=$(NEXMON_ARCH) \
	-Wno-unused-function \
	-ffunction-sections -fdata-sections \
	-I$(NEXMON_ROOT)/patches/include \
	-Iinclude \
	-I$(FW_PATH)

all: $(FW_FILE)

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
	$(Q)cat gen/nexmon.pre | awk '{ if ($$3 != "obj/wrapper.o") { print $$0; } }' > tmp
	$(Q)cat gen/nexmon.pre | awk '{ if ($$3 == "obj/wrapper.o") { print $$0; } }' >> tmp
	$(Q)cat tmp | awk '{ if ($$1 ~ /^0x/) { if ($$3 != "obj/wrapper.o") { if (!x[$$1]++) { print $$0; } } else { if (!x[$$1]) { print $$0; } } } else { print $$0; } }' > gen/nexmon2.pre

gen/nexmon.ld: gen/nexmon2.pre $(OBJS)
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)sort gen/nexmon2.pre | gawk -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.ld.awk > $@

gen/nexmon.mk: gen/nexmon2.pre $(OBJS) $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)printf "gen/$(DONGLE_FW_FILE): gen/patch.elf FORCE\n" > $@
	$(Q)sort gen/nexmon2.pre | \
		awk '{if (($$1 ~ /^0x/) && ($$2 == "PATCH")) { if ( (strtonum($$1) >= strtonum($(DONGLE_RAMSTART))) && (strtonum($$1) <= (strtonum($(DONGLE_RAMSTART)) + strtonum($(DONGLE_RAMSIZE))))) { print $$0; } } else { print $$0; }}' | \
		gawk -v src_file=gen/patch.elf -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.1.awk | \
		gawk -v ramstart=$(DONGLE_RAMSTART) -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.2.awk >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > tmp && mv tmp $@

gen/nexmon_pageable.mk: gen/nexmon2.pre $(OBJS) $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING MAKE FILE\033[0m gen/nexmon.pre => %s\n" $@
	$(Q)printf "gen/$(HOST_FW_FILE): gen/patch.elf FORCE\n" > $@
	$(Q)sort gen/nexmon2.pre | \
		awk '($$1 ~ /^0x/) && ($$2 == "PATCH") && (strtonum($$1) >= strtonum($(HOST_RAMSTART))) && (strtonum($$1) <= (strtonum($(HOST_RAMSTART)) + strtonum($(HOST_RAMSIZE))))' | \
		gawk -v src_file=gen/patch.elf -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.1.awk | \
		gawk -v ramstart=$(HOST_RAMSTART) -f $(NEXMON_ROOT)/buildtools/scripts/nexmon.mk.2.awk >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > tmp && mv tmp $@

gen/memory.ld: $(FW_PATH)/definitions.mk
	@printf "\033[0;31m  GENERATING LINKER FILE\033[0m %s\n" $@
	$(Q)printf "ram : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(DONGLE_RAMSTART) $(DONGLE_RAMSIZE) >> $@
	$(Q)printf "ucode : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(UCODE0START) $$(($(PATCHSTART) - $(UCODE0START))) >> $@
	$(Q)printf "patch : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(PATCHSTART) $(PATCHSIZE) >> $@

gen/patch.elf: patch.ld gen/nexmon.ld gen/memory.ld $(OBJS)
	@printf "\033[0;31m  LINKING OBJECTS\033[0m => %s (details: log/linker.log, log/linker.err)\n" $@
	$(Q)$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M >>log/linker.log 2>>log/linker.err

gen/$(DONGLE_FW_FILE): $(FW_PATH)/$(DONGLE_FW_FILE) gen/nexmon.mk
	$(Q)cp $(FW_PATH)/$(DONGLE_FW_FILE) $@
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon.mk => %s (details: log/patches.log)\n" $@
	$(Q)make -f gen/nexmon.mk >>log/patches.log 2>>log/patches.log

gen/$(HOST_FW_FILE): $(FW_PATH)/$(HOST_FW_FILE) gen/nexmon_pageable.mk
	$(Q)cp $(FW_PATH)/$(HOST_FW_FILE) $@
	@printf "\033[0;31m  APPLYING PATCHES\033[0m gen/nexmon_pageable.mk => %s (details: log/patches_pageable.log)\n" $@
	$(Q)make -f gen/nexmon_pageable.mk >>log/patches_pageable.log 2>>log/patches_pageable.log

$(FW_FILE): init gen/patch.elf gen/$(DONGLE_FW_FILE) gen/$(HOST_FW_FILE) $(FW_PATH)/$(FW_FILE)
	$(Q)cp $(FW_PATH)/$(FW_FILE) $@
	@printf "\033[0;31m  MERGE HYBRID FIRMWARE\033[0m %s + %s => %s\n" gen/$(DONGLE_FW_FILE) gen/$(HOST_FW_FILE) $@
	$(Q)dd if=gen/$(DONGLE_FW_FILE) of=$@ bs=1 seek=$$(($(DONGLE_FW_OFFSET))) conv=notrunc 2>/dev/null
	$(Q)dd if=gen/$(HOST_FW_FILE) of=$@ bs=1 seek=$$(($(HOST_FW_OFFSET))) conv=notrunc 2>/dev/null

###################################################################
# ucode compression related
###################################################################

gen/ucode0.bin: $(FW_PATH)/d11ucodex_btcxmu132_2.bin
	@printf "\033[0;31m  COPYING FIRST UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/ucode1.bin: $(FW_PATH)/d11ucode_btcxmu132_2.bin
	@printf "\033[0;31m  COPYING SECOND UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/ucode2.bin: $(FW_PATH)/d11ucodex_mu132_2.bin
	@printf "\033[0;31m  COPYING THIRD UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/ucode3.bin: $(FW_PATH)/d11ucode_mu132_2.bin
	@printf "\033[0;31m  COPYING FORTH UCODE\033[0m %s => %s\n" $< $@
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

gen/ucode3_compressed.bin: gen/ucode3.bin
	@printf "\033[0;31m  COMPRESSING FORTH UCODE\033[0m %s => %s\n" $< $@
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

src/ucode3_compressed.c: gen/ucode3_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

###################################################################

clean-firmware: FORCE
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr $(FW_FILE) obj gen log src/ucode0_compressed.c src/ucode1_compressed.c src/ucode2_compressed.c src/ucode3_compressed.c

clean: clean-firmware
	$(Q)rm -fr BUILD_NUMBER

FORCE:
