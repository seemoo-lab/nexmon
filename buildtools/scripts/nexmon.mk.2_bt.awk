BEGIN {
	patch_nr = initial_patch_nr;
}
{
	if ($2 == "PATCH") {
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((" $1 " - " ramstart "))\n\t$(Q)printf \"  PATCH " $4 " @ " $1 "\\n\"";
		if ($1 <= 0x90000 || ($1 >= 0x260000 && $1 <= 0x268000)) {
			printf("\t$(Q)echo -en '\\x08\\x0f\\x00\\x%02x\\x%s\\x%s\\x%s\\x%s' > gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 
					patch_nr, substr($1,9,2), substr($1,7,2), substr($1,5,2), substr($1,3,2), 5000 + patch_nr)
			printf("\t$(Q)cat gen/section.bin >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
			printf("\t$(Q)echo -en '\\x00\\x00\\x00\\x00\\x00\\x00' >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
			patch_nr += 1
		} else {
			print "\t$(Q)cp gen/section.bin gen/hcd_extracted/20_writeram_" $1 ".bin"
		}
	} else if ($2 == "REGION")
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $1 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((0x" $4 " - " ramstart "))\t$(Q)printf \"  REGION " $1 " @ " $4 "\\n\"";
	else if ($2 == "TARGETREGION" && $4 != "")
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $1 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((0x" $4 " - " ramstart "))\n\t$(Q)printf \"  TARGETREGION " $1 " @ " $4 "\\n\"";
}
