{
	if ($2 == "PATCH")
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/sec_" $4 ".bin && dd if=gen/sec_" $4 ".bin of=$@ bs=1 conv=notrunc seek=$$((" $1 " - " ramstart ")) && rm -f gen/sec_" $4 ".bin\n\t$(Q)printf \"  PATCH " $4 " @ " $1 "\\n\"";
	else if ($2 == "REGION" && $5 != "") {
		addr = $5; sub(/^0[xX]/, "", addr);
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $4 " $< gen/sec_" $4 ".bin && dd if=gen/sec_" $4 ".bin of=$@ bs=1 conv=notrunc seek=$$((0x" addr " - " ramstart ")) && rm -f gen/sec_" $4 ".bin\n\t$(Q)printf \"  REGION " $4 " (" $1 ") @ " addr "\\n\"";
	}
	else if ($2 == "TARGETREGION" && $4 != "") {
		addr = $4; sub(/^0[xX]/, "", addr);
		print "\t$(Q)$(CC)objcopy -O binary -j .text." $1 " $< gen/sec_" $1 ".bin && dd if=gen/sec_" $1 ".bin of=$@ bs=1 conv=notrunc seek=$$((0x" addr " - " ramstart ")) && rm -f gen/sec_" $1 ".bin\n\t$(Q)printf \"  TARGETREGION " $1 " @ " addr "\\n\"";
	}
}
