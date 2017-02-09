#SRCS=$(wildcard *.c) ucode_compressed.c
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
Q=@

NEXMON_CHIP=CHIP_VER_BCM43438
NEXMON_FW_VERSION=FW_VER_7_45_41_26_r640327
RAMSTART=0x0
FP_DATA_END_PTR=0x38E3C
FP_CONFIG_BASE_PTR_1=0x3AC2C
FP_CONFIG_END_PTR_1=0x3AC28
FP_CONFIG_BASE_PTR_2=0x3A9B4
FP_CONFIG_END_PTR_2=0x3A9B0
FP_CONFIG_BASE=0x5A3A8
FP_DATA_BASE=0x1000
FW_FILE_NAME=brcmfmac43430-sdio.bin

ORIG_FW=../../bootimg_src/firmware/brcmfmac43430-sdio.orig.bin

all: brcmfmac/brcmfmac.ko brcmfmac43430-sdio.bin

rpi3: brcmfmac/brcmfmac.ko

#include ucode_compression_code.mk
ifneq ($(shell uname -m),armv7l)
CC=../../buildtools/gcc-arm-none-eabi-5_4-2016q2_x86/bin/arm-none-eabi-
else
CC=../../buildtools/gcc-arm-none-eabi-5_4-2016q2_arm/bin/arm-none-eabi-
endif

brcmfmac/brcmfmac.ko: check-nexmon-setup-env
	make -C ../../buildtools/gcc-nexmon-plugin/
ifneq ($(shell uname -m),armv7l)
	echo "Building on x86"
	make -C ../../kernel/ bcm2709_defconfig
	make -C ../../kernel/ -j4
	make -C ../../kernel/ M=$$PWD/brcmfmac -j2
else
	echo "Building on RPI3"
	make -C /lib/modules/$(shell uname -r)/build M=$$PWD/brcmfmac  -j2
endif


%.o: %.c ../include/*.h
	$(CC)gcc \
		-fplugin=../../buildtools/gcc-nexmon-plugin/nexmon.so \
		-fplugin-arg-nexmon-fwfile=brcmfmac43430-sdio.bin \
		-fplugin-arg-nexmon-ramstart=$(RAMSTART) \
		-fplugin-arg-nexmon-objfile=$@ \
		-fplugin-arg-nexmon-prefile=nexmon.generated.pre \
		-fplugin-arg-nexmon-chipver=`printf "#include \"../include/firmware_version.h\"\n%s\n" $(NEXMON_CHIP) | $(CC)gcc -E -x c - | tail -n 1` \
		-fplugin-arg-nexmon-fwver=`printf "#include \"../include/firmware_version.h\"\n%s\n" $(NEXMON_FW_VERSION) | $(CC)gcc -E -x c - | tail -n 1` \
		-DNEXMON_CHIP=$(NEXMON_CHIP) \
		-DNEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
		-Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -march=armv7-r -ffunction-sections -fdata-sections -I../include -c $< -o $@

nexmon.generated.ld: $(OBJS) FORCE
	$(Q)sort nexmon.generated.pre | \
		gawk '{ \
			if ($$2 == "PATCH") \
				print ".text." $$4 " " $$1 " : { KEEP(" $$3 " (.*." $$4 ")) }"; \
			else if ($$2 == "DUMMY") \
				print ".text.dummy." $$4 " " $$1 " : { " $$3 " (.*." $$4 ") }"; \
			else if ($$2 == "REGION") \
				print ".text." $$4 " : { KEEP(" $$3 " (.*." $$4 ")) } >" $$1; \
			else if ($$2 == "TARGETREGION") \
				print ".text." $$1 " : { " $$3 " (.text .text.* .data .data.* .bss .bss.* .rodata .rodata.*) } >"  $$1; \
		}' > $@

nexmon.generated.mk: $(OBJS) FORCE
	$(Q)printf "$(FW_FILE_NAME): patch.elf FORCE\n" > $@
	$(Q)sort nexmon.generated.pre | \
		gawk '{ if ($$2 == "TARGETREGION") { "./getsectionaddr.sh .text." $$1 " patch.elf" | getline result; print $$0 " " result} else {print; }; }' | \
		gawk '{ \
			if ($$2 == "PATCH") \
				print "\t$$(Q)$$(CC)objcopy -O binary -j .text." $$4 " $$< section.generated.bin && dd if=section.generated.bin of=$$@ bs=1 conv=notrunc status=none seek=$$$$((" $$1 " - $(RAMSTART)))\n\t$$(Q)printf \"  PATCH " $$4 " @ " $$1 "\\n\""; \
			else if ($$2 == "REGION") \
				print "\t$$(Q)$$(CC)objcopy -O binary -j .text." $$1 " $$< section.generated.bin && dd if=section.generated.bin of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x" $$4 " - $(RAMSTART)))\t$$(Q)printf \"  REGION " $$1 " @ " $$4 "\\n\""; \
			else if ($$2 == "TARGETREGION" && $$4 != "") \
				print "\t$$(Q)$$(CC)objcopy -O binary -j .text." $$1 " $$< section.generated.bin && dd if=section.generated.bin of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x" $$4 " - $(RAMSTART)))\n\t$$(Q)printf \"  TARGETREGION " $$1 " @ " $$4 "\\n\""; \
		}' >> $@
	$(Q)printf "\nFORCE:\n" >> $@
	$(Q)gawk '!a[$$0]++' $@ > tmp && mv tmp $@

flashpatches.generated.ld: $(OBJS) FORCE
	$(Q)sort nexmon.generated.pre | \
		gawk '{ \
			if ($$2 == "FLASHPATCH") \
				print ".text." $$4 " " $$1 " : { KEEP(" $$3 " (.*." $$4 ")) }"; \
		}' > $@

flashpatches.generated.mk: $(OBJS) FORCE
	cat nexmon.generated.pre | gawk ' \
		function htonl(a) { return rshift(and(a, 0xff000000), 24) + rshift(and(a, 0xff0000), 8) + lshift(and(a, 0xff00), 8) + lshift(and(a, 0xff), 24); } \
		BEGIN { \
			fp_data_base = $(FP_DATA_BASE); \
			fp_data_end = fp_data_base; \
			fp_config_base = $(FP_CONFIG_BASE); \
			fp_config_end = fp_config_base; \
			printf "$(FW_FILE_NAME): patch.elf FORCE\n"; \
		} \
		{ \
			if ($$2 == "FLASHPATCH") { \
				printf "\t$$(Q)$$(CC)objcopy -O binary -j .text." $$4 " $$< section.generated.bin && dd if=section.generated.bin of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", fp_data_end, $(RAMSTART); \
				printf "\t$$(Q)printf %08x%08x%08x | xxd -r -p | dd of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", htonl(strtonum($$1)), htonl(4), htonl(fp_data_end), fp_config_end, $(RAMSTART); \
				printf "\t$$(Q)printf \"  FLASHPATCH %s @ %s\\n\"\n", $$4, $$1; \
				fp_data_end = fp_data_end + 8; \
				fp_config_end = fp_config_end + 12; \
			} \
		} \
		END { \
			printf "\t$$(Q)printf %08x | xxd -r -p | dd of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", htonl(fp_data_end), $(FP_DATA_END_PTR), $(RAMSTART); \
			printf "\t$$(Q)printf \"  PATCH fp_data_end @ 0x%08x\\n\"\n", $(FP_DATA_END_PTR); \
			printf "\t$$(Q)printf %08x | xxd -r -p | dd of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", htonl(fp_config_base), $(FP_CONFIG_BASE_PTR_1), $(RAMSTART); \
			printf "\t$$(Q)printf \"  PATCH fp_config_base @ 0x%08x\\n\"\n", $(FP_CONFIG_BASE_PTR_1); \
			printf "\t$$(Q)printf %08x | xxd -r -p | dd of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", htonl(fp_config_end), $(FP_CONFIG_END_PTR_1), $(RAMSTART); \
			printf "\t$$(Q)printf \"  PATCH fp_config_end @ 0x%08x\\n\"\n", $(FP_CONFIG_END_PTR_1); \
			printf "\t$$(Q)printf %08x | xxd -r -p | dd of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", htonl(fp_config_base), $(FP_CONFIG_BASE_PTR_2), $(RAMSTART); \
			printf "\t$$(Q)printf \"  PATCH fp_config_base @ 0x%08x\\n\"\n", $(FP_CONFIG_BASE_PTR_2); \
			printf "\t$$(Q)printf %08x | xxd -r -p | dd of=$$@ bs=1 conv=notrunc status=none seek=$$$$((0x%08x - 0x%08x))\n", htonl(fp_config_end), $(FP_CONFIG_END_PTR_2), $(RAMSTART); \
			printf "\t$$(Q)printf \"  PATCH fp_config_end @ 0x%08x\\n\"\n", $(FP_CONFIG_END_PTR_2); \
			printf "\n\nFORCE:\n" \
		}' > flashpatches.generated.mk

wrapper.o: ../include/wrapper.c
	make -C ../wrapper \
		NEXMON_CHIP=$(NEXMON_CHIP) \
		NEXMON_FW_VERSION=$(NEXMON_FW_VERSION) \
		WRAPPER_PRE_FILE=$(NEXMON_FIRMWARE_PATCHING)/nexmon/nexmon.generated.pre \
		WRAPPER_OBJ_FILE=$(NEXMON_FIRMWARE_PATCHING)/nexmon/$@ \
		Q=$(Q)

patch.elf: patch.ld patch.o wrapper.o nexmon.generated.ld flashpatches.generated.ld $(OBJS)
	$(CC)ld -T $< -o $@ --gc-sections --print-gc-sections -M

brcmfmac43430-sdio.bin: patch.elf $(ORIG_FW) nexmon.generated.mk flashpatches.generated.mk
	cp $(ORIG_FW) $@
	$(Q)make -f flashpatches.generated.mk Q=$(Q)
	$(Q)make -f nexmon.generated.mk Q=$(Q)


check-nexmon-setup-env:
ifndef NEXMON_SETUP_ENV
	$(error run 'source setup_env.sh' first)
endif


clean-firmware: FORCE
	rm -f brcmfmac43430-sdio.bin *.o ucode_compressed.c ucode_compressed.bin *.generated.* wrapper.ld
	make -C ../wrapper clean

clean: clean-firmware
ifneq ($(shell uname -m),armv7l)
	make -C ../../kernel/ M=$$PWD/brcmfmac clean
else
	make -C /lib/modules/$(shell uname -r)/build M=$$PWD/brcmfmac clean
endif

FORCE:
