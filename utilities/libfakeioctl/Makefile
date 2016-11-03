all: libs/armeabi/libfakeioctl.so

libs/armeabi/libfakeioctl.so: Android.mk fakeioctl.c
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/libfakeioctl.so
	adb push $< /sdcard/
	adb shell su -c "mount -o remount,rw /system"
	adb shell su -c "cp /sdcard/libfakeioctl.so /system/lib/libfakeioctl.so"

clean:
	rm -Rf libs
	rm -Rf local
