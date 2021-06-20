GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

ifneq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l))
all: libs/armeabi/nexutil
else
all: nexutil
endif

ifneq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l))
libs/armeabi/nexutil: Android.mk nexutil.c ../libargp/local/armeabi/libargp.a ../libnexio/local/armeabi/libnexio.a FORCE
	touch nexutil.c
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21 GIT_VERSION=$(GIT_VERSION)
else
nexutil: nexutil.c ../libnexio/libnexio.a FORCE
	gcc -static -o nexutil nexutil.c bcmwifi_channels.c b64-encode.c b64-decode.c -DBUILD_ON_RPI -DVERSION=\"$GIT_VERSION\" -DUSE_NETLINK -I. -I../../patches/include -I../libnexio -L../libnexio/ -lnexio -I../libargp
endif

../libargp/local/armeabi/libargp.a: FORCE
	cd ../libargp && make

../libnexio/local/armeabi/libnexio.a: FORCE
	cd ../libnexio && make

../libnexio/libnexio.a: FORCE
	cd ../libnexio && make

ifneq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l))
install: libs/armeabi/nexutil
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/nexutil /system/bin/nexutil"'
	adb shell 'su -c "chmod +x /system/bin/nexutil"'

install64: libs/arm64-v8a/nexutil
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/nexutil /system/bin/nexutil"'
	adb shell 'su -c "chmod +x /system/bin/nexutil"'
else
install: nexutil
	cp $< /usr/bin/
endif

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
	
