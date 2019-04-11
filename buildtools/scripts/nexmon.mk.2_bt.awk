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
	# The right hand parameter are exported as variables to the shell environment.
	# Their definition can be found in the firmwares definition.mk file
	# e.g. firmwares/bcm4335c0_BT/1_BT/definitions.mk
	patch_nr = initial_patch_nr;
}
function printPatchPrefix(address)
{
    byte_0 = rshift(and(address, 0xff000000), 24);
    byte_1 = rshift(and(address, 0x00ff0000), 16);
    byte_2 = rshift(and(address, 0x0000ff00), 8);
    byte_3 = and(address,        0x000000ff);
    printf("\t$(Q)/bin/echo -en '\\x08\\x0f\\x00\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x' > gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n",
            patch_nr, byte_3, byte_2, byte_1, byte_0, 5000 + patch_nr);
}
function printPatchSuffix()
{
    printf("\t$(Q)/bin/echo -en '\\x00\\x00\\x00\\x00\\x00\\x00' >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
}
{
	if ($2 == "PATCH") {
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/section.bin && dd if=gen/section.bin of=$@ bs=1 conv=notrunc seek=$$((" $1 " - " ramstart "))\n\t$(Q)printf \"  PATCH " $4 " @ " $1 "\\n\"";
		# Handling of patches which need to be written to the rom.
		# Since this is not directly possible we need to create rompatch files to be written to the
		# ram (see rampatch mechanism).
		if ($1 <= rom_area_below) {   
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
				# get original ROM contents from firmware (2x4 bytes)
				printf("\t$(Q)dd if=$(FW_PATH)/$(FW_BIN) skip=%d bs=1 count=8 of=gen/aligned_rom\n", $1)
				# position our 4 byte BL patch within that
				printf("\t$(Q)dd if=gen/section.bin of=gen/aligned_rom bs=1 seek=%d conv=notrunc\n", alignment)
				# generate two patches (with only the 4 byte BL patch used)
				printPatchPrefix(aligned)
				printf("\t$(Q)dd if=gen/aligned_rom bs=1 count=4 of=gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr);
				printPatchSuffix()
				patch_nr += 1
				printPatchPrefix(aligned_next)
				printf("\t$(Q)dd if=gen/aligned_rom bs=1 skip=4 count=4 of=gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr);
			printPatchSuffix()
			} else {
				printPatchPrefix($1)
				printf("\t$(Q)cat gen/section.bin >> gen/hcd_extracted/rompatches/rompatch_nr%04d_0x08.bin\n", 5000 + patch_nr)
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
