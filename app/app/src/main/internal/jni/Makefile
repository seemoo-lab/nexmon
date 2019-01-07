all: ../libs/armeabi/libnexmonpentestsuite.so
	cp ../libs/armeabi/libnexmonpentestsuite.so ../../libs/armeabi

../libs/armeabi/libnexmonpentestsuite.so: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

clean:
	rm -rf local