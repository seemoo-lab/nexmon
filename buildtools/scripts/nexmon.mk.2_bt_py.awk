{
	if ($2 == "PATCH") {
		#FIXME calling objcopy directly might break on other systems
		# is in buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86/bin/arm-none-eabi-objdump
		# and $NEXMON_ROOT is not found within a syscall like this...
		system("/usr/bin/arm-none-eabi-objcopy -O binary -j .text." $4 " gen/patch.elf gen/section.bin");
		
		# Check if we need Patchram or normal RAM
		if ($1 <= rom_area_below || ($1 >= rom_area_start && $1 <= rom_area_end)) {   
			# InternalBlue already does the 4 byte alignment for us within ROM
			printf("internalblue.patchRom(0x%08x, '", $1);
		} else {
			# Just write into RAM
			printf("internalblue.writeMem(0x%08x, '", $1);
		}
		system("hexdump -v -e '/1 \"\\\\x%02x\"'  gen/section.bin"); #FIXME directly calling hexdump utility might also not work everywhere
		printf("')\n");
	} else if (($2 == "REGION") || ($2 == "TARGETREGION" && $4 != "")) {
		system("/usr/bin/arm-none-eabi-objcopy -O binary -j .text." $1 " gen/patch.elf gen/section.bin");
		printf("internalblue.writeMem(0x%08x, '", $1);
		system("hexdump -v -e '/1 \"\\\\x%02x\"'  gen/section.bin");
		printf("')\n");
	}
}
