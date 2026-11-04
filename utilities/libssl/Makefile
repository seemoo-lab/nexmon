# Same overlap as libcrypto: callers rebuild this library with -j while
# another ndk-build is already compiling it. See utilities/libcrypto/Makefile.
.PHONY: all libs/armeabi/libssl.a

all libs/armeabi/libssl.a &:
	mkdir -p local
	flock local $(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21 $(if $(APP_ABI),APP_ABI=$(APP_ABI))
	touch local/.built.stamp

clean:
	rm -Rf libs
	rm -Rf local

install:
	@echo "nothing to install ..."

FORCE:
	