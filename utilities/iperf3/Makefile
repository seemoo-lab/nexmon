all: libs/armeabi/iperf3

libs/armeabi/iperf3: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/iperf3
	adb push $< /sdcard/
	adb shell 'su -c "mount -o remount,rw /system"'
	adb shell 'su -c "cp /sdcard/iperf3 /system/bin/iperf3"'
	adb shell 'su -c "chmod +x /system/bin/iperf3"'

clean:
	rm -Rf libs
	rm -Rf local
