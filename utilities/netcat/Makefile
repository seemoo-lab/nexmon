all: libs/armeabi/nc

libs/armeabi/nc: netcat.c Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/nc
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/nc /system/bin/nc"'
	adb shell 'su -c "chmod +x /system/bin/nc"'

clean:
	rm -Rf libs
	rm -Rf local
