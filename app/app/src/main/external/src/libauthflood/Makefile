all: Android.mk ../libpcap/local/armeabi/libpcap.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21
	cp libs/armeabi/*.so ../../../libs/armeabi

../libpcap/local/armeabi/libpcap.a: FORCE
	cd ../libpcap && make

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
