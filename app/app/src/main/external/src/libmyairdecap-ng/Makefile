UTILITIES_PATH=../../../../../../../utilities/

all: libs/armeabi/libmyairdecap-ng.so
	cd libs/armeabi/ && cp *.so ../../../../../libs/armeabi

libs/armeabi/libmyairdecap-ng.so: Android.mk $(UTILITIES_PATH)/libsqlite/local/armeabi/libsqlite.a $(UTILITIES_PATH)/libcrypto/local/armeabi/libcrypto.a $(UTILITIES_PATH)/libssl/local/armeabi/libssl.a ../../../../../../../utilities/libosdep/local/armeabi/libosdep.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

$(UTILITIES_PATH)/libosdep/local/armeabi/libosdep.a: FORCE
	cd $(UTILITIES_PATH)/libosdep && make

$(UTILITIES_PATH)/libsqlite/local/armeabi/libsqlite.a: FORCE
	cd $(UTILITIES_PATH)/libsqlite && make

$(UTILITIES_PATH)/libssl/local/armeabi/libssl.a: FORCE
	cd $(UTILITIES_PATH)/libssl && make

$(UTILITIES_PATH)/libcrypto/local/armeabi/libcrypto.a: FORCE
	cd $(UTILITIES_PATH)/libcrypto && make
	
FORCE:

clean:
	rm -Rf libs
	rm -Rf local
