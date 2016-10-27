function htonl(a) { 
	return rshift(and(a, 0xff000000), 24) + rshift(and(a, 0xff0000), 8) + lshift(and(a, 0xff00), 8) + lshift(and(a, 0xff), 24); 
}
BEGIN {
	fp_data_base = strtonum(fp_data_base);
	fp_config_base = strtonum(fp_config_base); 
	fp_data_end_ptr = strtonum(fp_data_end_ptr);
	fp_config_base_ptr_1 = strtonum(fp_config_base_ptr_1);
	fp_config_end_ptr_1 = strtonum(fp_config_end_ptr_1);
	fp_config_base_ptr_2 = strtonum(fp_config_base_ptr_2);
	fp_config_end_ptr_2 = strtonum(fp_config_end_ptr_2);
	ramstart = strtonum(ramstart);

	fp_data_end = fp_data_base;
	fp_config_end = fp_config_base;
	
	printf "%s: %s FORCE\n", out_file, src_file;
}
{
	if ($2 == "FLASHPATCH") {
		printf "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", fp_data_end, ramstart;
		printf "\t$(Q)printf %08x%08x%08x | xxd -r -p | dd of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", htonl(strtonum($1)), htonl(4), htonl(fp_data_end), fp_config_end, ramstart;
		printf "\t$(Q)printf \"  FLASHPATCH %s @ %s\\n\"\n", $4, $1;
		fp_data_end = fp_data_end + 8;
		fp_config_end = fp_config_end + 12;
	}
}
END {
	printf "\t$(Q)printf %08x | xxd -r -p | dd of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", htonl(fp_data_end), fp_data_end_ptr, ramstart;
	printf "\t$(Q)printf \"  PATCH fp_data_end @ 0x%08x\\n\"\n", fp_data_end_ptr;
	printf "\t$(Q)printf %08x | xxd -r -p | dd of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", htonl(fp_config_base), fp_config_base_ptr_1, ramstart;
	printf "\t$(Q)printf \"  PATCH fp_config_base @ 0x%08x\\n\"\n", fp_config_base_ptr_1;
	printf "\t$(Q)printf %08x | xxd -r -p | dd of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", htonl(fp_config_end), fp_config_end_ptr_1, ramstart;
	printf "\t$(Q)printf \"  PATCH fp_config_end @ 0x%08x\\n\"\n", fp_config_end_ptr_1;
	printf "\t$(Q)printf %08x | xxd -r -p | dd of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", htonl(fp_config_base), fp_config_base_ptr_2, ramstart;
	printf "\t$(Q)printf \"  PATCH fp_config_base @ 0x%08x\\n\"\n", fp_config_base_ptr_2;
	printf "\t$(Q)printf %08x | xxd -r -p | dd of=$@ bs=1 conv=notrunc seek=$$((0x%08x - 0x%08x))\n", htonl(fp_config_end), fp_config_end_ptr_2, ramstart;
	printf "\t$(Q)printf \"  PATCH fp_config_end @ 0x%08x\\n\"\n", fp_config_end_ptr_2;
	printf "\n\nFORCE:\n"
}
