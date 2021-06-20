PROGS = airbase-ng airdecap-ng aireplay-ng airolib-ng airtun-ng buddy-ng ivstools tkiptun-ng wpaclean aircrack-ng airdecloak-ng airodump-ng airserv-ng besside-ng easside-ng kstats makeivs-ng packetforge-ng wesside-ng

all: libs/armeabi/aircrack-ng

libs/armeabi/aircrack-ng: Android.mk ../libsqlite/local/armeabi/libsqlite.a ../libcrypto/local/armeabi/libcrypto.a ../libssl/local/armeabi/libssl.a ../libosdep/local/armeabi/libosdep.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libosdep/local/armeabi/libosdep.a: FORCE
	cd ../libosdep && make

../libsqlite/local/armeabi/libsqlite.a: FORCE
	cd ../libsqlite && make

../libssl/local/armeabi/libssl.a: FORCE
	cd ../libssl && make

../libcrypto/local/armeabi/libcrypto.a: FORCE
	cd ../libcrypto && make

install: libs/armeabi/aircrack-ng
	adb push libs/armeabi/aireplay-ng /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/aireplay-ng /system/bin/aireplay-ng"'
	adb shell 'su -c "chmod +x /system/bin/aireplay-ng"'

FORCE:

clean:
	rm -Rf libs
	rm -Rf local
