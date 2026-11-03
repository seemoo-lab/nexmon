HOSTUNAME := $(shell uname -s)
PLATFORMUNAME := $(shell uname -m)

ifeq ($(HOSTUNAME),Darwin)
PLUGIN_DIR := gcc-nexmon-plugin-osx
else ifeq ($(HOSTUNAME),Linux)
ifeq ($(PLATFORMUNAME),x86_64)
PLUGIN_DIR := gcc-nexmon-plugin
else ifeq ($(PLATFORMUNAME),aarch64)
ifneq ($(USE_LEGACY_GCC),1)
ifneq ($(wildcard arm-gnu-toolchain-15.3.rel1-aarch64-arm-none-eabi/bin/arm-none-eabi-gcc),)
PLUGIN_DIR := gcc-nexmon-plugin-aarch64
else
PLUGIN_DIR := gcc-nexmon-plugin-arm
endif
else
PLUGIN_DIR := gcc-nexmon-plugin-arm
endif
else ifneq ($(filter armv7l armv6l,$(PLATFORMUNAME)),)
PLUGIN_DIR := gcc-nexmon-plugin-arm
endif
endif

PLUGIN_TARGET := $(if $(PLUGIN_DIR),$(PLUGIN_DIR)/nexmon.so,)

ALL_TOOLS := $(PLUGIN_TARGET) flash_patch_extractor ucode_extractor b43/assembler/b43-asm.bin b43/disassembler/b43-dasm b43-v2/assembler/b43-asm.bin b43-v2/disassembler/b43-dasm b43-v3/assembler/b43-asm.bin b43-v3/disassembler/b43-dasm b43-unified/assembler/b43-asm.bin b43-unified/disassembler/b43-dasm

all: $(ALL_TOOLS)

# Delegate to sub-directory makefiles; sub-makes are incremental, but top-level
# always delegates (phony) so changes inside subdirs are detected. With `make -j`
# all tools build in parallel via the jobserver.

ifneq ($(PLUGIN_TARGET),)
$(PLUGIN_TARGET):
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(PLUGIN_DIR)
endif

b43-unified/assembler/b43-asm.bin:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43-unified/disassembler/b43-dasm:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43-v3/assembler/b43-asm.bin:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43-v3/disassembler/b43-dasm:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43-v2/assembler/b43-asm.bin:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43-v2/disassembler/b43-dasm:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43/assembler/b43-asm.bin:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

b43/disassembler/b43-dasm:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $(dir $@)

flash_patch_extractor:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $@

ucode_extractor:
	@printf "\033[0;31m  BUILDING\033[0m %s\n" $@
	$(Q)$(MAKE) -C $@

clean:
	@printf "\033[0;31m  CLEANING BUILDTOOLS\033[0m\n"
ifneq ($(PLUGIN_DIR),)
	$(Q)$(MAKE) -C $(PLUGIN_DIR) clean
endif
	$(Q)$(MAKE) -C flash_patch_extractor clean
	$(Q)$(MAKE) -C ucode_extractor clean
	$(Q)$(MAKE) -C b43/assembler clean
	$(Q)$(MAKE) -C b43/disassembler clean
	$(Q)$(MAKE) -C b43-v2/assembler clean
	$(Q)$(MAKE) -C b43-v2/disassembler clean
	$(Q)$(MAKE) -C b43-v3/assembler clean
	$(Q)$(MAKE) -C b43-v3/disassembler clean
	$(Q)$(MAKE) -C b43-unified/assembler clean
	$(Q)$(MAKE) -C b43-unified/disassembler clean

.PHONY: all clean $(ALL_TOOLS)
	
