all: libs/armeabi/nexutil

ifneq ($(shell uname -m),armv7l)
libs/armeabi/nexutil: Android.mk nexutil.c ../libargp/local/armeabi/libargp.a ../libnexio/local/armeabi/libnexio.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21
else
libs/armeabi/nexutil: nexutil.c
	gcc -o nexutil nexutil.c -DBUILD_ON_RPI
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
	