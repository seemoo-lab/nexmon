NDK_ROOT ?= $(shell if [ -n "$$ANDROID_NDK_ROOT" ]; then echo "$$ANDROID_NDK_ROOT"; elif [ -n "$$ANDROID_NDK_HOME" ]; then echo "$$ANDROID_NDK_HOME"; elif [ -n "$$ANDROID_NDK" ]; then echo "$$ANDROID_NDK"; elif [ -d "/opt/android-sdk/ndk/30.0.16248370" ]; then echo "/opt/android-sdk/ndk/30.0.16248370"; elif [ -d "/opt/android-sdk/ndk" ]; then find /opt/android-sdk/ndk -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1; elif command -v ndk-build >/dev/null 2>&1; then dirname "$$(command -v ndk-build)"; fi)
export NDK_ROOT

# The NDK ships no linux-aarch64 host toolchain: build/tools/ndk_bin_common.sh
# maps Linux/aarch64 straight to x86_64, so on an ARM64 host every compiler run
# is emulated by qemu-x86_64 -- slow, and prone to aborting mid-build with
# "libc++abi: Pure virtual function called!" / "qemu: uncaught target signal 6".
# buildtools/ndk-native-aarch64.sh builds a native host toolchain next to the
# x86_64 one; when it is present, steer ndk-build at it (see ndk-make-native
# for why this goes through GNUMAKE rather than a plain exported HOST_ARCH).
NDK_HOST_ARCH := $(shell uname -m)
QEMU_NEEDS_SERIAL := 0
ifeq ($(NDK_HOST_ARCH),aarch64)
ifneq ($(wildcard $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-aarch64/bin/clang),)
    export NDK_NATIVE_HOST_ARCH := aarch64
    export GNUMAKE := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../buildtools/ndk-make-native)
else
    $(info NOTE: building through qemu-x86_64; run buildtools/ndk-native-aarch64.sh for a native toolchain)
    QEMU_NEEDS_SERIAL := 1
endif
endif

# Only disable jobserver/parallelism when forced through qemu-x86_64 emulation.
# With a native aarch64 toolchain or on x86_64, `make -j$(nproc)` safely
# parallelizes 30+ NDK builds and cuts utilities build from minutes to seconds.
ifeq ($(QEMU_NEEDS_SERIAL),1)
unexport MAKEFLAGS
.NOTPARALLEL:
endif

ARCH ?= all

SUPPORTED_ARCHES := all arm armv7 armeabi armeabi-v7a arm64 aarch64 arm64-v8a x86 x86_64 x64

ifeq ($(ARCH),all)
    NDK_APP_ABI := all
else ifneq ($(filter $(ARCH),arm armv7 armeabi armeabi-v7a),)
    NDK_APP_ABI := armeabi-v7a
else ifneq ($(filter $(ARCH),arm64 aarch64 arm64-v8a),)
    NDK_APP_ABI := arm64-v8a
else ifeq ($(ARCH),x86)
    NDK_APP_ABI := x86
else ifneq ($(filter $(ARCH),x86_64 x64),)
    NDK_APP_ABI := x86_64
else
    $(error Unsupported ARCH=$(ARCH). Supported: $(SUPPORTED_ARCHES))
endif

NDK_ABI_ARG := APP_ABI=$(NDK_APP_ABI)
ARCH_TARGETS := arm armv7 armeabi armeabi-v7a arm64 aarch64 arm64-v8a x86 x86_64 x64

SUBDIRS := $(wildcard */.)

all: $(SUBDIRS)

$(ARCH_TARGETS):
	$(MAKE) ARCH=$@ all

$(SUBDIRS):
	$(MAKE) -C $@ $(NDK_ABI_ARG)

# These link prebuilt BoringSSL, libnexio, libargp, libpcap, libglib-2.0, etc.
# Explicit DAG prerequisites ensure parallel `make -j` builds dependencies first
# without races, file truncations, or compiler conflicts.
aircrack-ng/. tcpdump/. socat/.: libcrypto/. libssl/.
libfakeioctl/. libnexmon/. nexutil/.: libnexio/.
nexutil/. rawproxy/. rawproxyreverse/.: libargp/.
rawproxy/. rawproxyreverse/. mdk4/. tcpdump/.: libpcap/.
mdk4/.: libosdep/. libnl/.
iw/.: libnl/.
wireshark/. libwiretap/. libwireshark/. libwsutil/.: libglib-2.0/.

install:
	@for dir in $(SUBDIRS); do \
		if $(MAKE) -C $$dir -n install >/dev/null 2>&1; then \
			$(MAKE) -C $$dir install $(NDK_ABI_ARG) || exit $$?; \
		fi; \
	done

update:
	python3 $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../buildtools/scripts/check_updates.py) --category utilities

clean:
	@for dir in $(SUBDIRS); do \
		if $(MAKE) -C $$dir -n clean >/dev/null 2>&1; then \
			$(MAKE) -C $$dir clean $(NDK_ABI_ARG) || exit $$?; \
		fi; \
	done

.PHONY: all $(SUBDIRS) $(ARCH_TARGETS) install update clean
