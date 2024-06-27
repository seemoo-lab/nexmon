include definitions.mk

all: d11ucode_mu129.bin d11ucodex_mu129.bin d11ucode_mu129_1.bin d11ucodex_mu129_1.bin d11ucode_btcxmu129_1.bin d11ucodex_btcxmu129_1.bin d11ucode_mu129_2.bin d11ucodex_mu129_2.bin d11ucode_btcxmu129_2.bin d11ucodex_btcxmu129_2.bin d11smc1.bin

d11ucode_mu129.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FIRST UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE0START)-$(RAMSTART))) count=$$(($(UCODE0SIZE))) 2>/dev/null

d11ucodex_mu129.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING FIRST UCODE X\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEX0START)-$(RAMSTART))) count=$$(($(UCODEX0SIZE))) 2>/dev/null

d11ucode_mu129_1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE1START)-$(RAMSTART))) count=$$(($(UCODE1SIZE))) 2>/dev/null

d11ucodex_mu129_1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND UCODE X\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEX1START)-$(RAMSTART))) count=$$(($(UCODEX1SIZE))) 2>/dev/null

d11ucode_btcxmu129_1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND UCODE BTCX\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEBT1START)-$(RAMSTART))) count=$$(($(UCODEBT1SIZE))) 2>/dev/null

d11ucodex_btcxmu129_1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SECOND UCODE X BTCX\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEXBT1START)-$(RAMSTART))) count=$$(($(UCODEXBT1SIZE))) 2>/dev/null

d11ucode_mu129_2.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD UCODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODE2START)-$(RAMSTART))) count=$$(($(UCODE2SIZE))) 2>/dev/null

d11ucodex_mu129_2.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD UCODE X\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEX2START)-$(RAMSTART))) count=$$(($(UCODEX2SIZE))) 2>/dev/null

d11ucode_btcxmu129_2.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD UCODE BTCX\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEBT2START)-$(RAMSTART))) count=$$(($(UCODEBT2SIZE))) 2>/dev/null

d11ucodex_btcxmu129_2.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING THIRD UCODE X BTCX\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(UCODEXBT2START)-$(RAMSTART))) count=$$(($(UCODEXBT2SIZE))) 2>/dev/null

d11smc1.bin: $(RAM_FILE) definitions.mk
	@printf "\033[0;31m  EXTRACTING SMC CODE\033[0m\n"
	$(Q)dd if=$< of=$@ bs=1 skip=$$(($(D11SMC1START)-$(RAMSTART))) count=$$(($(D11SMC1SIZE))) 2>/dev/null

clean:
	@printf "\033[0;31m  CLEANING\033[0m\n"
	$(Q)rm -f d11ucode_mu129.bin d11ucodex_mu129.bin d11ucode_mu129_1.bin d11ucodex_mu129_1.bin d11ucode_btcxmu129_1.bin d11ucodex_btcxmu129_1.bin d11ucode_mu129_2.bin d11ucodex_mu129_2.bin d11ucode_btcxmu129_2.bin d11ucodex_btcxmu129_2.bin d11smc1.bin
