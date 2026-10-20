all: libs/armeabi-v7a/mdk4

libs/armeabi-v7a/mdk4: Android.mk ../libosdep/local/armeabi-v7a/libosdep.a ../libpcap/local/armeabi-v7a/libpcap.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libosdep/local/armeabi-v7a/libosdep.a: FORCE
	cd ../libosdep && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

../libpcap/local/armeabi-v7a/libpcap.a: FORCE
	cd ../libpcap && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

clean:
	rm -Rf libs
	rm -Rf local

install: libs/armeabi-v7a/mdk4
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/mdk4 /system/bin/mdk4"'
	adb shell 'su -c "chmod +x /system/bin/mdk4"'

FORCE:
