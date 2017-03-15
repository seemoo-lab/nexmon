all:

install-iperf: iperf
	adb push $< /sdcard/
	adb shell 'su -c "mount -o remount,rw /system"'
	adb shell 'su -c "cp /sdcard/iperf /system/bin/iperf"'
	adb shell 'su -c "chmod +x /system/bin/iperf"'

install-iperf3: iperf3
	adb push $< /sdcard/
	adb shell 'su -c "mount -o remount,rw /system"'
	adb shell 'su -c "cp /sdcard/iperf3 /system/bin/iperf3"'
	adb shell 'su -c "chmod +x /system/bin/iperf3"'

install: install-iperf3 install-iperf
