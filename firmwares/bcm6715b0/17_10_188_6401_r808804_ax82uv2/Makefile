include definitions.mk

all: d11ucodex_btcxmu132_2.bin d11ucode_btcxmu132_2.bin d11ucodex_mu132_2.bin d11ucode_mu132_2.bin $(DONGLE_FW_FILE) $(HOST_FW_FILE)

d11ucodex_btcxmu132_2.bin: $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FIRST UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(DONGLE_FW_OFFSET)+$(UCODE3START)-$(DONGLE_RAMSTART))) count=$$(($(UCODE3SIZE))) 2>/dev/null

d11ucode_btcxmu132_2.bin: $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(DONGLE_FW_OFFSET)+$(UCODE2START)-$(DONGLE_RAMSTART))) count=$$(($(UCODE2SIZE))) 2>/dev/null

d11ucodex_mu132_2.bin: $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(DONGLE_FW_OFFSET)+$(UCODE1START)-$(DONGLE_RAMSTART))) count=$$(($(UCODE1SIZE))) 2>/dev/null

d11ucode_mu132_2.bin: $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FORTH UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(DONGLE_FW_OFFSET)+$(UCODE0START)-$(DONGLE_RAMSTART))) count=$$(($(UCODE0SIZE))) 2>/dev/null

$(DONGLE_FW_FILE): $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING DONGLE FIRMWARE IMAGE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(DONGLE_FW_OFFSET))) count=$$(($(DONGLE_FW_SIZE))) 2>/dev/null

$(HOST_FW_FILE): $(FW_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING HOST FIRMWARE IMAGE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(HOST_FW_OFFSET))) count=$$(($(HOST_FW_SIZE))) 2>/dev/null

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f d11ucodex_btcxmu132_2.bin d11ucode_btcxmu132_2.bin d11ucodex_mu132_2.bin d11ucode_mu132_2.bin $(DONGLE_FW_FILE) $(HOST_FW_FILE)
