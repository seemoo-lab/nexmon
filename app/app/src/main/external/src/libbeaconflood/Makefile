all: Android.mk ../libsender/local/armeabi/libsender.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21
	cp libs/armeabi/*.so ../../../libs/armeabi

../libsender/local/armeabi/libsender.a: FORCE
	cd ../libsender && make

clean:
	rm -Rf libs
	rm -Rf local

FORCE:

