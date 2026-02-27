nope:

all: libs/armeabi/libglib-2.0.a

libs/armeabi/libglib-2.0.a: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-23

clean:
	rm -Rf libs
	rm -Rf local
