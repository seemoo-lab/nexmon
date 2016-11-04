PROGS = airbase-ng airdecap-ng aireplay-ng airolib-ng airtun-ng buddy-ng ivstools tkiptun-ng wpaclean aircrack-ng airdecloak-ng airodump-ng airserv-ng besside-ng easside-ng kstats makeivs-ng packetforge-ng wesside-ng

all: libs/armeabi/aircrack-ng

libs/armeabi/aircrack-ng: Android.mk SYSTEM_LIBS ../libosdep/local/armeabi/libosdep.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libosdep/local/armeabi/libosdep.a: FORCE
	cd ../libosdep && make

SYSTEM_LIBS: FORCE
	cd ../shared_system_libs && make

install: libs/armeabi/aircrack-ng
	adb push libs/armeabi/aireplay-ng /sdcard/
	adb shell su -c "mount -o remount,rw /system"
	adb shell su -c "cp /sdcard/aireplay-ng /system/bin/aireplay-ng"
	
FORCE:

clean:
	rm -Rf libs
	rm -Rf local
