all: libs/armeabi/libnexmon.so

libs/armeabi/libnexmon.so: Android.mk nexmon.c ../libnexio/local/armeabi/libnexio.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libnexio/local/armeabi/libnexio.a: FORCE
	cd ../libnexio && make

install: libs/armeabi/libnexmon.so libs/arm64-v8a/libnexmon.so
	adb shell 'su -c "mount -o rw,remount /system"'
	adb push libs/armeabi/libnexmon.so /sdcard/
	adb shell 'su -c "cp /sdcard/libnexmon.so /system/lib/libnexmon.so"'
	adb push libs/arm64-v8a/libnexmon.so /sdcard/
	adb shell 'su -c "cp /sdcard/libnexmon.so /system/lib64/libnexmon.so"'

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
