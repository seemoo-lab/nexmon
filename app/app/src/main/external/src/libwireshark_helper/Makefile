all: Android.mk ../libpcap/local/armeabi/libpcap.a prebuildlibs/libwireshark.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-23
	cp libs/armeabi/*.so ../../../libs/armeabi

prebuildlibs/libwireshark.a: prebuildlibs/libwireshark.tar.gz
	cd prebuildlibs && tar xfvz libwireshark.tar.gz

../libpcap/local/armeabi/libpcap.a: FORCE
	cd ../libpcap && make

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
