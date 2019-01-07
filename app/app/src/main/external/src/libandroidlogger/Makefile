all: libs/armeabi/libandroidlogger
	cd libs/armeabi && cp *.so ../../../../../libs/armeabi

libs/armeabi/libandroidlogger: Android.mk android-logger.c
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

clean:
	rm -Rf libs
	rm -Rf local
