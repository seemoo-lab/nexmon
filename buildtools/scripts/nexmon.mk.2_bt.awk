#####
# Expected Input:
# Parameter:
#
# case REGION:
# ?
#
# case TARGETREGION:
#   $1      $2                      $3                        $4
# < ? > TARGETREGION <obj file which holds the targetregion> < ? >
#
# case PATCH:
#        $1         $2                $3                     $4
# <addr to patch> PATCH <obj file which holds the patch> <patch_name>
#####
BEGIN {
	patch_nr = initial_patch_nr;
}
{
    if(patch_nr == 115){
        patch_nr = 118    
    }
	if ($2 == "PATCH") {
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((" $1 " - " ramstart "))\n\t$(Q)printf \"  PATCH " $4 " @ " $1 "\\n\"";
		# Handling of patches which need to be written to the rom.
        # Since this is not directly possible we need to create rompatch files to be written to the
        # ram (see rampatch mechanism).
        if ($1 <= 0x90000 || ($1 >= 0x260000 && $1 <= 0x268000)) {
            # rampatch mechanism
            # All data written to the file is in little-endian, documenation below is big-endian for
            # a better legibility:
            # 0x08           - 1 byte  - type of this patch entry: rom-patch
            # 0x000f         - 2 bytes - size of rom patch entries is static: 15
            # patch-number   - 1 byte  - alias patch slot-number 
            # target-address - 4 bytes - target address where the patch should be applied
			printf("\t$(Q)echo -en '\\x08\\x0f\\x00\\x%02x\\x%s\\x%s\\x%s\\x%s' > gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 
					patch_nr, substr($1,9,2), substr($1,7,2), substr($1,5,2), substr($1,3,2), 5000 + patch_nr)
            # patch value    - 4 bytes - the new value which will be patched to the target address
			printf("\t$(Q)cat gen/section.bin >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
            # empty bytes    - 2 bytes - two bytes. filler?
            # unknown        - 4 bytes - some bytes with unknown usage
			printf("\t$(Q)echo -en '\\x00\\x00\\x00\\x00\\x00\\x00' >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
			patch_nr += 1
		} else {
            # no rom patch so we can simply create a writeram patch.
			print "\t$(Q)cp gen/section.bin gen/hcd_extracted/20_writeram_" $1 ".bin"
		}
	} else if ($2 == "REGION")
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $1 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((0x" $4 " - " ramstart "))\t$(Q)printf \"  REGION " $1 " @ " $4 "\\n\"";
	else if ($2 == "TARGETREGION" && $4 != "")
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $1 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((0x" $4 " - " ramstart "))\n\t$(Q)printf \"  TARGETREGION " $1 " @ " $4 "\\n\"";
}
