GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
USE_VENDOR_CMD := 0

# Build natively with gcc only when running on an Arm host building for itself.
# On an aarch64 host the NDK cross-toolchain (NDK_NATIVE_HOST_ARCH, exported by
# the top-level utilities Makefile / setup_env.sh) means we are cross-compiling
# for Android, so take the ndk-build path just like an x86_64 host -- otherwise
# the Android consumers (e.g. libfakeioctl) would not find local/<abi>/*.a.
NATIVE_BUILD := $(filter $(shell uname -m),armv6l armv7l aarch64)
ifdef NDK_NATIVE_HOST_ARCH
NATIVE_BUILD :=
endif

all: libs/armeabi/libnexio.a

ifeq ($(NATIVE_BUILD),)
libs/armeabi/libnexio.a: Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21
else
libs/armeabi/libnexio.a: libnexio.c
ifeq ($(USE_VENDOR_CMD),1)
	gcc -c libnexio.c -o libnexio.o -DUSE_VENDOR_CMD -DVERSION=\"$(GIT_VERSION)\" -I../../patches/include $(shell pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0)
else
	gcc -std=gnu11 -fcommon -c libnexio.c -o libnexio.o -DBUILD_ON_RPI -DVERSION=\"$(GIT_VERSION)\" -I../../patches/include
endif
	ar rcs libnexio.a libnexio.o
endif

clean:
	rm -Rf libs
	rm -Rf local libnexio.o libnexio.a

install:
	@echo "nothing to install ..."
