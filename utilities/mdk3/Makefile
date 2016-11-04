all: libs/armeabi/mdk3

libs/armeabi/mdk3: Android.mk ../libosdep/local/armeabi/libosdep.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libosdep/local/armeabi/libosdep.a: FORCE
	cd ../libosdep && make

clean:
	rm -Rf libs
	rm -Rf local

FORCE:

