all: libs/armeabi/dhdutil

libs/armeabi/dhdutil: *.c *.h Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/dhdutil
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/dhdutil /system/bin/dhdutil"'
	adb shell 'su -c "chmod +x /system/bin/dhdutil"'

clean:
	rm -Rf libs
	rm -Rf local
