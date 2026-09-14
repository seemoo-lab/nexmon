GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

all: libs/armeabi-v7a/rawproxy

libs/armeabi-v7a/rawproxy: Android.mk rawproxy.c ../libargp/local/armeabi-v7a/libargp.a ../libpcap/local/armeabi-v7a/libpcap.a FORCE
	touch rawproxy.c
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21 GIT_VERSION=$(GIT_VERSION)

../libargp/local/armeabi-v7a/libargp.a: FORCE
	cd ../libargp && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

../libpcap/local/armeabi-v7a/libpcap.a: FORCE
	cd ../libpcap && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

clean:
	rm -Rf libs
	rm -Rf local

install: libs/armeabi-v7a/rawproxy
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/rawproxy /system/bin/rawproxy"'

FORCE:
