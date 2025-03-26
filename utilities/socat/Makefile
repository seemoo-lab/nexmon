GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

libs/armeabi/socat: FORCE
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21 GIT_VERSION=$(GIT_VERSION)

install: libs/armeabi/socat
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/socat /system/bin/socat"'
	adb shell 'su -c "chmod +x /system/bin/socat"'

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
