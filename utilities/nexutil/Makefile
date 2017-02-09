GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

all: libs/armeabi/nexutil

ifneq ($(shell uname -m),armv7l)
libs/armeabi/nexutil: Android.mk nexutil.c ../libargp/local/armeabi/libargp.a ../libnexio/local/armeabi/libnexio.a FORCE
	touch nexutil.c
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21 GIT_VERSION=$(GIT_VERSION)
else
libs/armeabi/nexutil: nexutil.c FORCE
	gcc -static -o nexutil nexutil.c -DBUILD_ON_RPI -DVERSION=\"$GIT_VERSION\" -I../../patches/include -L../libnexio/ -lnexio
endif

../libargp/local/armeabi/libargp.a: FORCE
	cd ../libargp && make

../libnexio/local/armeabi/libnexio.a: FORCE
	cd ../libnexio && make

ifneq ($(shell uname -m),armv7l)
install: libs/armeabi/nexutil
	adb push $< /sdcard/
	adb shell su -c "mount -o remount,rw /system"
	adb shell su -c "cp /sdcard/nexutil /system/bin/nexutil"
	adb shell su -c "chmod +x /system/bin/nexutil"
else
install: nexutil
	cp $< /usr/bin/
endif

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
	
