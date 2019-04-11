{
	if ($2 == "PATCH") {
		system(objcopy " -O binary -j .text." $4 " gen/patch.elf gen/section.bin");
		
		# Check if we need Patchram or normal RAM
		if ($1 <= rom_area_below) {   
			# InternalBlue already does the 4 byte alignment for us within ROM
			printf("internalblue.patchRom(0x%08x, '", $1);
		} else {
			# Just write into RAM
			printf("internalblue.writeMem(0x%08x, '", $1);
		}
		system("hexdump -v -e '/1 \"\\\\x%02x\"'  gen/section.bin"); #FIXME directly calling hexdump utility might not work everywhere
		printf("')\n");
	} else if (($2 == "REGION") || ($2 == "TARGETREGION" && $4 != "")) {
		system(objcopy " -O binary -j .text." $1 " gen/patch.elf gen/section.bin");
		printf("internalblue.writeMem(0x%08x, '", $1);
		system("hexdump -v -e '/1 \"\\\\x%02x\"'  gen/section.bin");
		printf("')\n");
	}
}
