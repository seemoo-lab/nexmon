all: libs/armeabi-v7a/iwconfig libs/armeabi-v7a/iwlist libs/armeabi-v7a/iwpriv

libs/armeabi-v7a/iwconfig: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

libs/armeabi-v7a/iwlist: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

libs/armeabi-v7a/iwpriv: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi-v7a/iwconfig libs/armeabi-v7a/iwlist libs/armeabi-v7a/iwpriv
	adb push libs/armeabi-v7a/iwconfig /sdcard/
	adb push libs/armeabi-v7a/iwlist /sdcard/
	adb push libs/armeabi-v7a/iwpriv /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/iwconfig /system/bin/iwconfig"'
	adb shell 'su -c "cp /sdcard/iwlist /system/bin/iwlist"'
	adb shell 'su -c "cp /sdcard/iwpriv /system/bin/iwpriv"'
	adb shell 'su -c "chmod +x /system/bin/iwconfig"'
	adb shell 'su -c "chmod +x /system/bin/iwlist"'
	adb shell 'su -c "chmod +x /system/bin/iwpriv"'

clean:
	rm -Rf libs
	rm -Rf local
