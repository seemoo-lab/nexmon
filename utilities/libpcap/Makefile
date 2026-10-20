#all: ../../bin/mdk3

#../../bin/mdk3: libs/armeabi/mdk3
#	cp libs/armeabi/* ../../bin

all: libs/armeabi/libpcap.a

libs/armeabi/libpcap.a: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

install:
	@echo "nothing to install ..."

clean:
	rm -Rf libs
	rm -Rf local
