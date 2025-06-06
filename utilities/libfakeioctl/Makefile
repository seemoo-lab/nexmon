all: libs/armeabi/libfakeioctl.so

libs/armeabi/libfakeioctl.so: Android.mk fakeioctl.c ../libnexio/local/armeabi/libnexio.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libnexio/local/armeabi/libnexio.a: FORCE
	cd ../libnexio && make

install: libs/armeabi/libfakeioctl.so libs/arm64-v8a/libfakeioctl.so
	adb shell 'su -c "mount -o rw,remount /system"'
	adb push libs/armeabi/libfakeioctl.so /sdcard/
	adb shell 'su -c "cp /sdcard/libfakeioctl.so /system/lib/libfakeioctl.so"'
	adb push libs/arm64-v8a/libfakeioctl.so /sdcard/
	adb shell 'su -c "cp /sdcard/libfakeioctl.so /system/lib64/libfakeioctl.so"'

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
