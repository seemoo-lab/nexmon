#!/bin/sh
echo 0=$0 1=$1 2=$2 ${0}
if [ $# -lt 1 ]
then
	echo Usage: $0 firmware.bin
	exit 0
fi
if [ "$1" != "parseFW" ]
then 
	cat /opt/nexmon/patches/common/wrapper.c|grep "_98_97" |sed 's/).*\/\* \([a-z0-9_]*\)/, \1,/;s/\*\///'|tr -d ' '|cut -d, -f4,3,5 --output-delimiter=',' > /tmp/search.list
	for line in `cat /tmp/search.list`;do
		oldref=`echo $line|cut -d, -f1`
		func=`echo $line|cut -d, -f2`
		hook=`echo $line|cut -d, -f3`
		found=`/opt/bgrep/bgrep "$hook" $1|cut -d: -f2`
		printf "%0.8x " $oldref
		echo $found $func
	done
else
echo $*
fi

exit 0
# strings /opt/nexmon/firmwares/bcm43430a1/7_45_98_94/brcmfmac43430-sdio.bin |sed -n '/Version:/s/.*Version: \(.*\) (.*/\1/p' -> FW
# grep -a -b --only-matching $'\x90\xf8\x76\x30\x10\xb5' cyfmac43430-sdio.bin|cut -d: -f1
sed 's/.\{2\}/\\\x&/g'