all: libs/armeabi/iperf

libs/armeabi/iperf: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/iperf
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/iperf /system/bin/iperf"'
	adb shell 'su -c "chmod +x /system/bin/iperf"'

clean:
	rm -Rf libs
	rm -Rf local
