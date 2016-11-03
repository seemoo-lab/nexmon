all: libs/armeabi/nexutil

libs/armeabi/nexutil: Android.mk nexutil.c
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/nexutil
	adb push $< /sdcard/
	adb shell su -c "mount -o remount,rw /system"
	adb shell su -c "cp /sdcard/nexutil /system/bin/nexutil"

clean:
	rm -Rf libs
	rm -Rf local
