all: libs/armeabi/iwconfig libs/armeabi/iwlist libs/armeabi/iwpriv

libs/armeabi/iwconfig: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

libs/armeabi/iwlist: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

libs/armeabi/iwpriv: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install: libs/armeabi/iwconfig libs/armeabi/iwlist libs/armeabi/iwpriv
	adb push libs/armeabi/iwconfig /sdcard/
	adb push libs/armeabi/iwlist /sdcard/
	adb push libs/armeabi/iwpriv /sdcard/
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
