all: libs/armeabi-v7a/tcpdump

libs/armeabi-v7a/tcpdump: Android.mk ../libssl/local/armeabi-v7a/libssl.a ../libcrypto/local/armeabi-v7a/libcrypto.a ../libpcap/local/armeabi-v7a/libpcap.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libpcap/local/armeabi-v7a/libpcap.a: FORCE
	cd ../libpcap && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

../libssl/local/armeabi-v7a/libssl.a: FORCE
	cd ../libssl && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

../libcrypto/local/armeabi-v7a/libcrypto.a: FORCE
	cd ../libcrypto && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

install: libs/armeabi-v7a/tcpdump
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/tcpdump /system/bin/tcpdump"'
	adb shell 'su -c "chmod +x /system/bin/tcpdump"'

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
