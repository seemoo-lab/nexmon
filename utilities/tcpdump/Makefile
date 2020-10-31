all: libs/armeabi/tcpdump

libs/armeabi/tcpdump: Android.mk ../libssl/local/armeabi/libssl.a ../libcrypto/local/armeabi/libcrypto.a ../libpcap/local/armeabi/libpcap.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libpcap/local/armeabi/libpcap.a: FORCE
	cd ../libpcap && make

../libssl/local/armeabi/libssl.a: FORCE
	cd ../libssl && make

../libcrypto/local/armeabi/libcrypto.a: FORCE
	cd ../libcrypto && make

install: libs/armeabi/tcpdump
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/tcpdump /system/bin/tcpdump"'
	adb shell 'su -c "chmod +x /system/bin/tcpdump"'

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
