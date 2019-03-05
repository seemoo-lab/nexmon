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
        lower_rom_area = rom_area_below;
        rom_area_lower_boundary = rom_area_start;
        rom_area_upper_boundary = rom_area_end;
}
function printPatchPrefix(address)
{
    byte_0 = rshift(and(address, 0xff000000), 24);
    byte_1 = rshift(and(address, 0x00ff0000), 16);
    byte_2 = rshift(and(address, 0x0000ff00), 8);
    byte_3 = and(address,        0x000000ff);
    printf("\t$(Q)echo -en '\\x08\\x0f\\x00\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x' > gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n",
            patch_nr, byte_3, byte_2, byte_1, byte_0, 5000 + patch_nr);
}
function printPatchSuffix()
{
    printf("\t$(Q)echo -en '\\x00\\x00\\x00\\x00\\x00\\x00' >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
}
function printPatchCat(input)
{
    printf("\t$(Q)cat " patch_input " >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 
           patch_input, 5000 + patch_nr)
    
}
{
	if ($2 == "PATCH") {
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((" $1 " - " ramstart "))\n\t$(Q)printf \"  PATCH " $4 " @ " $1 "\\n\"";
		# Handling of patches which need to be written to the rom.
        # Since this is not directly possible we need to create rompatch files to be written to the
        # ram (see rampatch mechanism).
        if ($1 <= lower_rom_area || ($1 >= rom_area_lower_boundary && $1 <= rom_area_upper_boundary)) {   
            # rampatch mechanism
            # All data written to the file is in little-endian, documenation below is big-endian for
            # a better legibility:
            # 0x08           - 1 byte  - type of this patch entry: rom-patch
            # 0x000f         - 2 bytes - size of rom patch entries is static: 15
            # patch-number   - 1 byte  - alias patch slot-number 
            # target-address - 4 bytes - target address where the patch should be applied
            # patch value    - 4 bytes - the new value which will be patched to the target address
            # empty bytes    - 2 bytes - two bytes. filler?
            # unknown        - 4 bytes - some bytes with unknown usage
			
            alignment = $1 % 4;
            if(alignment > 0){
                aligned = $1 - alignment
                aligned_next = aligned + 4
                printf("\tpython $(NEXMON_ROOT)/buildtools/scripts/bt_patch_alignment.py %#08x gen/section.bin gen/aligned_patch\n", $1)
                printPatchPrefix(aligned)
                #printPatchCat("gen/aligned_patch_one")
                printf("\t$(Q)cat gen/aligned_patch_one >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr);
                printPatchSuffix()
                patch_nr += 1
                printPatchPrefix(aligned_next)
                #printPatchCat("gen/aligned_patch_two")
                printf("\t$(Q)cat gen/aligned_patch_two >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr);
                printPatchSuffix()
                
            } else {
                #printf("\t$(Q)echo -en '\\x08\\x0f\\x00\\x%02x\\x%s\\x%s\\x%s\\x%s' > gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 
	    		#		patch_nr, substr($1,9,2), substr($1,7,2), substr($1,5,2), substr($1,3,2), 5000 + patch_nr)
                printPatchPrefix($1)
			    printf("\t$(Q)cat gen/section.bin >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
			    #printf("\t$(Q)echo -en '\\x00\\x00\\x00\\x00\\x00\\x00' >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
                printPatchSuffix()
            }
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
