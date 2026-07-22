GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
USE_VENDOR_CMD := 0

all: libs/armeabi/libnexio.a

ifneq ($(shell uname -m),$(filter $(shell uname -m), armv6l armv7l aarch64))
libs/armeabi/libnexio.a: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21
else
libs/armeabi/libnexio.a: libnexio.c
ifeq ($(USE_VENDOR_CMD),1)
	gcc -c libnexio.c -o libnexio.o -DUSE_VENDOR_CMD -DVERSION=\"$(GIT_VERSION)\" -I../../patches/include $(shell pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0)
else
	gcc -c libnexio.c -o libnexio.o -DBUILD_ON_RPI -DVERSION=\"$GIT_VERSION\" -I../../patches/include
endif
	ar rcs libnexio.a libnexio.o
endif

clean:
	rm -Rf libs
	rm -Rf local
