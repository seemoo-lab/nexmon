GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
SHELL := /bin/bash
include ../version.mk
include $(FW_PATH)/definitions.mk

ifndef NEXMON_SETUP_ENV
$(error run 'source setup_env.sh' first in the repository\'s root directory)
endif

LOCAL_SRCS=$(wildcard src/*.c) \
src/d11ucode_mu129_compressed.c \
src/d11ucodex_mu129_compressed.c \
src/d11ucode_mu129_1_compressed.c \
src/d11ucodex_mu129_1_compressed.c \
src/d11ucode_btcxmu129_1_compressed.c \
src/d11ucodex_btcxmu129_1_compressed.c \
src/d11ucode_mu129_2_compressed.c \
src/d11ucodex_mu129_2_compressed.c \
src/d11ucode_btcxmu129_2_compressed.c \
src/d11ucodex_btcxmu129_2_compressed.c

COMMON_SRCS=$(wildcard $(NEXMON_ROOT)/patches/common/*.c)
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
	-DRECLAIM_END_PTR=$(RECLAIM_END_PTR) \
	-DPATCHSTART=$(PATCHSTART) \
	-DWLC_BMAC_PROCESS_UCODE_SR_BL_HOOK_ADDR=$(WLC_BMAC_PROCESS_UCODE_SR_BL_HOOK_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_0_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_0_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_1_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_1_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_2_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_2_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_3_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_3_ADDR) \
	-DWLC_UCODE_WRITE_BL_HOOK_4_ADDR=$(WLC_UCODE_WRITE_BL_HOOK_4_ADDR) \
	-DWLC_UCODEX_WRITE_BL_HOOK_ADDR=$(WLC_UCODEX_WRITE_BL_HOOK_ADDR) \
	-DUCODE0START_PTR=$(UCODE0START_PTR) \
	-DUCODE0SIZE_PTR=$(UCODE0SIZE_PTR) \
	-DUCODEX0START_PTR=$(UCODEX0START_PTR) \
	-DUCODEX0SIZE_PTR=$(UCODEX0SIZE_PTR) \
	-DUCODE1START_PTR=$(UCODE1START_PTR) \
	-DUCODE1SIZE_PTR=$(UCODE1SIZE_PTR) \
	-DUCODEX1START_PTR=$(UCODEX1START_PTR) \
	-DUCODEX1SIZE_PTR=$(UCODEX1SIZE_PTR) \
	-DUCODEBT1START_PTR=$(UCODEBT1START_PTR) \
	-DUCODEBT1SIZE_PTR=$(UCODEBT1SIZE_PTR) \
	-DUCODEXBT1START_PTR=$(UCODEXBT1START_PTR) \
	-DUCODEXBT1SIZE_PTR=$(UCODEXBT1SIZE_PTR) \
	-DUCODE2START_PTR=$(UCODE2START_PTR) \
	-DUCODE2SIZE_PTR=$(UCODE2SIZE_PTR) \
	-DUCODEX2START_PTR=$(UCODEX2START_PTR) \
	-DUCODEX2SIZE_PTR=$(UCODEX2SIZE_PTR) \
	-DUCODEBT2START_PTR=$(UCODEBT2START_PTR) \
	-DUCODEBT2SIZE_PTR=$(UCODEBT2SIZE_PTR) \
	-DUCODEXBT2START_PTR=$(UCODEXBT2START_PTR) \
	-DUCODEXBT2SIZE_PTR=$(UCODEXBT2SIZE_PTR) \
	-DVERSION_PTR_1=$(VERSION_PTR_1) \
	-DVERSION_PTR_2=$(VERSION_PTR_2) \
	-DVERSION_PTR_3=$(VERSION_PTR_3) \
	-DVERSION_PTR_4=$(VERSION_PTR_4) \
	-DDATE_PTR=$(DATE_PTR) \
	-DDATE=\""$(DATE)"\" \
	-DTIME_PTR=$(TIME_PTR) \
	-DTIME=\""$(TIME)"\" \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DBUILD_NUMBER=\"$$(cat BUILD_NUMBER)\" \
	-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=$(NEXMON_ARCH) \
	-Wno-unused-function \
	-ffunction-sections -fdata-sections \
	-I$(NEXMON_ROOT)/patches/include \
	-Iinclude \
	-I$(FW_PATH)

.PHONY: all init copy-firmware load-firmware clean-firmware clean

all: $(RAM_FILE)

init:
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
	$(Q)cat tmp | awk '{ if ($$1 ~ /^0x/) { if ($$3 != "obj/wrapper.o") { if (!x[$$1]++) { print $$0; } } else { if (!x[$$1]) { print $$0; } } } else { print$$0; } }' > gen/nexmon2.pre

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
	$(Q)printf "ram : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(RAMSTART) $(RAMSIZE) >> $@
	$(Q)printf "ucode : ORIGIN = 0x%08x, LENGTH = 0x%08x\n" $(UCODEBT1START) $$(($(PATCHSTART) - $(UCODEBT1START))) >> $@
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

gen/d11ucode_btcxmu129_1.bin: $(FW_PATH)/d11ucode_btcxmu129_1.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucode_btcxmu129_1_compressed.bin: gen/d11ucode_btcxmu129_1.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucode_btcxmu129_1_compressed.c: gen/d11ucode_btcxmu129_1_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucode_btcxmu129_2.bin: $(FW_PATH)/d11ucode_btcxmu129_2.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucode_btcxmu129_2_compressed.bin: gen/d11ucode_btcxmu129_2.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucode_btcxmu129_2_compressed.c: gen/d11ucode_btcxmu129_2_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucode_mu129_1.bin: $(FW_PATH)/d11ucode_mu129_1.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucode_mu129_1_compressed.bin: gen/d11ucode_mu129_1.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucode_mu129_1_compressed.c: gen/d11ucode_mu129_1_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucode_mu129_2.bin: $(FW_PATH)/d11ucode_mu129_2.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucode_mu129_2_compressed.bin: gen/d11ucode_mu129_2.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucode_mu129_2_compressed.c: gen/d11ucode_mu129_2_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucode_mu129.bin: $(FW_PATH)/d11ucode_mu129.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucode_mu129_compressed.bin: gen/d11ucode_mu129.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucode_mu129_compressed.c: gen/d11ucode_mu129_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucodex_btcxmu129_1.bin: $(FW_PATH)/d11ucodex_btcxmu129_1.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucodex_btcxmu129_1_compressed.bin: gen/d11ucodex_btcxmu129_1.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucodex_btcxmu129_1_compressed.c: gen/d11ucodex_btcxmu129_1_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucodex_btcxmu129_2.bin: $(FW_PATH)/d11ucodex_btcxmu129_2.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucodex_btcxmu129_2_compressed.bin: gen/d11ucodex_btcxmu129_2.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucodex_btcxmu129_2_compressed.c: gen/d11ucodex_btcxmu129_2_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucodex_mu129_1.bin: $(FW_PATH)/d11ucodex_mu129_1.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucodex_mu129_1_compressed.bin: gen/d11ucodex_mu129_1.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucodex_mu129_1_compressed.c: gen/d11ucodex_mu129_1_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucodex_mu129_2.bin: $(FW_PATH)/d11ucodex_mu129_2.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucodex_mu129_2_compressed.bin: gen/d11ucodex_mu129_2.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucodex_mu129_2_compressed.c: gen/d11ucodex_mu129_2_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

gen/d11ucodex_mu129.bin: $(FW_PATH)/d11ucodex_mu129.bin
	@printf "\033[0;31m  COPYING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cp $< $@

gen/d11ucodex_mu129_compressed.bin: gen/d11ucodex_mu129.bin
	@printf "\033[0;31m  COMPRESSING UCODE\033[0m %s => %s\n" $< $@
	$(Q)cat $< | $(ZLIBFLATE) > $@

src/d11ucodex_mu129_compressed.c: gen/d11ucodex_mu129_compressed.bin
	@printf "\033[0;31m  GENERATING C FILE\033[0m %s => %s\n" $< $@
	$(Q)printf "#pragma NEXMON targetregion \"ucode\"\n\n" > $@
	$(Q)cd $(dir $<) && xxd -i $(notdir $<) >> $(shell pwd)/$@

###################################################################

copy-firmware: $(RAM_FILE)
ifneq ($(strip $(SSHPASS)),)
ifneq ($(strip $(SSHUSER)),)
ifneq ($(strip $(SSHADDRESS)),)
	@printf "\033[0;31m  COPYING\033[0m %s to %s:/jffs/%s\n" $(RAM_FILE) $(SSHADDRESS) $(RAM_FILE)
	@SSHPASS=$(SSHPASS) sshpass -e ssh $(SSHUSER)@$(SSHADDRESS) '/usr/sbin/socat -u TCP-LISTEN:65535,reuseaddr OPEN:/jffs/$(RAM_FILE),creat'& sleep 1; socat -u FILE:$(RAM_FILE) TCP:$(SSHADDRESS):65535
else
	$(error No SSH address given. Set with SSHADDRESS=<address> when calling make)
endif
else
	$(error No SSH user given. Set with SSHUSER=<ssh username> when calling make)
endif
else
	$(error No SSH password given. Set with SSHPASS=<ssh password> when calling make)
endif

load-firmware:
ifneq ($(strip $(SSHPASS)),)
ifneq ($(strip $(SSHUSER)),)
ifneq ($(strip $(SSHADDRESS)),)
	@printf "\033[0;31m  CHECKING FILE EXISTENCE\033[0m %s:/jffs/%s\n" $(SSHADDRESS) $(RAM_FILE)
	@SSHPASS=$(SSHPASS) sshpass -e ssh $(SSHUSER)@$(SSHADDRESS) '[ -s /jffs/$(RAM_FILE) ]' || (echo Error: $(SSHADDRESS):/jffs/$(RAM_FILE) does not exist. Run \'make copy-firmware\' first.; exit 1)
	@printf "\033[0;31m  UNLOADING\033[0m dhd module\n"
	@SSHPASS=$(SSHPASS) sshpass -e ssh $(SSHUSER)@$(SSHADDRESS) '/sbin/rmmod dhd; /bin/usleep 200000'
	@printf "\033[0;31m  LOADING\033[0m dhd module\n"
	@SSHPASS=$(SSHPASS) sshpass -e ssh $(SSHUSER)@$(SSHADDRESS) '/sbin/insmod $$(/usr/bin/find /lib/modules -name dhd.ko) firmware_path=/jffs/$(RAM_FILE) instance_base=1 dhd_msg_level=0x400001'
	@printf "\033[0;31m  RECONFIGURING\033[0m interfaces\n"
	@SSHPASS=$(SSHPASS) sshpass -e ssh $(SSHUSER)@$(SSHADDRESS) '/sbin/restart_wireless >/dev/null 2>/dev/null'
	@SSHPASS=$(SSHPASS) sshpass -e ssh $(SSHUSER)@$(SSHADDRESS) '/bin/brctl delif br0 eth7'
else
	$(error No SSH address given. Set with SSHADDRESS=<address> when calling make)
endif
else
	$(error No SSH user given. Set with SSHUSER=<ssh username> when calling make)
endif
else
	$(error No SSH password given. Set with SSHPASS=<ssh password> when calling make)
endif

clean-firmware:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -fr $(RAM_FILE) obj gen log src/d11ucode_mu129_compressed.c src/d11ucodex_mu129_compressed.c src/d11ucode_mu129_1_compressed.c src/d11ucodex_mu129_1_compressed.c src/d11ucode_btcxmu129_1_compressed.c src/d11ucodex_btcxmu129_1_compressed.c src/d11ucode_mu129_2_compressed.c src/d11ucodex_mu129_2_compressed.c src/d11ucode_btcxmu129_2_compressed.c src/d11ucodex_btcxmu129_2_compressed.c

clean: clean-firmware
	$(Q)rm -fr BUILD_NUMBER
