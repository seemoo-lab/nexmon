#!/bin/sh
# Requires bgrep from ... git clone https://github.com/tmbinc/bgrep
if [ $# -lt 1 ]
then
	echo Usage: $0 firmware.bin
	exit 0
fi
VER=`strings $1 |sed -n 's/\./_/g;/Version:/s/^\([0-9a-Z]*\)-.*Version: \(.*\) (.*/CHIP_VER_BCM\1, FW_VER_\2/p'`
echo Firmware ID: $VER

echo "R_98_97  R_Fware  Function"
echo "=================================================================="
cat /opt/nexmon/patches/common/wrapper.c|grep "_98_97" |sed 's/).*\/\* \([a-zA-Z0-9_]*\)/, \1,/;s/\*\///'|tr -d ' '|cut -d, -f4,3,5 --output-delimiter=',' > /tmp/search.list
for line in `cat /tmp/search.list`;do
	oldref=`echo $line|cut -d, -f1`
	func=`echo $line|cut -d, -f2`
	hook=`echo $line|cut -d, -f3`
	found=`/opt/bgrep/bgrep "$hook" $1|cut -d: -f2`
	printf "%0.8x " $oldref
	echo $found $func
#	echo "AT($VER, 0x$found)"
	#	AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x57770)
done

exit 0
# strings /opt/nexmon/firmwares/bcm43430a1/7_45_98_94/brcmfmac43430-sdio.bin |sed -n '/Version:/s/.*Version: \(.*\) (.*/\1/p' -> FW
# grep -a -b --only-matching $'\x90\xf8\x76\x30\x10\xb5' cyfmac43430-sdio.bin|cut -d: -f1
sed 's/.\{2\}/\\\x&/g'