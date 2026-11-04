# ndk-build for aircrack-ng 1.7 (reorganized src/ + lib/ layout).
LOCAL_PATH := $(call my-dir)

MY_CFLAGS := -g -O3 -Wall -DANDROID -DOS_ANDROID -DLinux \
	-DHAVE_CONFIG_H -DSTATIC_BUILD -DDYNAMIC=0 \
	-D_FILE_OFFSET_BITS=64 -fPIC -fcommon \
	-Wno-unused-parameter -Wno-unused-variable \
	-Wno-unused-but-set-variable -Wno-array-bounds \
	-Wno-deprecated-declarations \
	-include openssl/evp.h

MY_C_INCLUDES := \
	$(LOCAL_PATH)/android \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/aircrack-ng/osdep \
	$(LOCAL_PATH)/lib \
	$(LOCAL_PATH)/lib/osdep \
	$(LOCAL_PATH)/lib/radiotap \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/aircrack-ng \
	$(LOCAL_PATH)/src/airodump-ng \
	$(LOCAL_PATH)/src/easside-ng \
	$(LOCAL_PATH)/src/airdecloak-ng \
	$(LOCAL_PATH)/src/airventriloquist-ng \
	$(LOCAL_PATH)/../boringssl/include

# ---------------------------------------------------------------------------
# Prebuilts from utilities/libssl and utilities/libcrypto (boringssl)
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libssl
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libssl/local/$(TARGET_ARCH_ABI)/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../boringssl/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcrypto
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libcrypto/local/$(TARGET_ARCH_ABI)/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../boringssl/include
include $(PREBUILT_STATIC_LIBRARY)

# ---------------------------------------------------------------------------
# sqlite3 (AOSP libsqlite Android.mk is not usable with standalone ndk-build)
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libsqlite3_aircrack
LOCAL_SRC_FILES := ../libsqlite/sqlite3.c
LOCAL_CFLAGS += -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION -DNDEBUG \
	-Wno-unused-parameter -Wno-unused-variable
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------------------------------------------
# aircrack-ng internal libraries
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libosdep
LOCAL_SRC_FILES := \
	lib/osdep/network.c \
	lib/osdep/file.c \
	lib/osdep/osdep.c \
	lib/osdep/linux.c \
	lib/osdep/linux_tap.c \
	lib/osdep/common.c \
	lib/radiotap/radiotap.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaccrypto
LOCAL_SRC_FILES := lib/crypto/crypto.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libptw
LOCAL_SRC_FILES := lib/ptw/aircrack-ptw-lib.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaircrack-ce-wep
LOCAL_SRC_FILES := lib/ce-wep/uniqueiv.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaircrack-ce-wpa
LOCAL_SRC_FILES := \
	lib/ce-wpa/crypto_engine.c \
	lib/ce-wpa/memory.c \
	lib/ce-wpa/simd-intrinsics.c \
	lib/ce-wpa/wpapsk.c
LOCAL_CFLAGS += $(MY_CFLAGS) -DSIMD_CORE -DHAS_NEON -D__ARM_NEON__
ifneq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_CFLAGS += -mfpu=neon -mfloat-abi=softfp
endif
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaircrack
LOCAL_SRC_FILES := \
	lib/libac/adt/avl_tree.c \
	lib/libac/adt/circular_buffer.c \
	lib/libac/adt/circular_queue.c \
	lib/libac/cpu/simd_cpuid.c \
	lib/libac/cpu/trampoline_arm.c \
	lib/libac/cpu/cpuset_pthread.c \
	lib/libac/support/fragments.c \
	lib/libac/support/common.c \
	lib/libac/support/communications.c \
	lib/libac/support/crypto_engine_loader.c \
	lib/libac/support/mcs_index_rates.c \
	lib/libac/tui/console.c \
	lib/libac/utf8/verifyssid.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcowpatty
LOCAL_SRC_FILES := lib/cowpatty/cowpatty.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

MY_STATIC_LIBS := \
	libaircrack \
	libaircrack-ce-wpa \
	libaircrack-ce-wep \
	libptw \
	libaccrypto \
	libosdep \
	libssl \
	libcrypto

# ---------------------------------------------------------------------------
# Binaries
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := aircrack-ng
LOCAL_SRC_FILES := \
	src/aircrack-ng/aircrack-ng.c \
	src/aircrack-ng/linecount.cpp \
	src/aircrack-ng/session.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS) libsqlite3_aircrack
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airdecap-ng
LOCAL_SRC_FILES := src/airdecap-ng/airdecap-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := packetforge-ng
LOCAL_SRC_FILES := src/packetforge-ng/packetforge-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := aireplay-ng
LOCAL_SRC_FILES := src/aireplay-ng/aireplay-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airodump-ng
LOCAL_SRC_FILES := \
	src/airodump-ng/airodump-ng.c \
	src/airodump-ng/dump_write.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airserv-ng
LOCAL_SRC_FILES := src/airserv-ng/airserv-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airtun-ng
LOCAL_SRC_FILES := src/airtun-ng/airtun-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := ivstools
LOCAL_SRC_FILES := src/ivstools/ivstools.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := kstats
LOCAL_SRC_FILES := src/kstats/kstats.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := libaircrack
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := wesside-ng
LOCAL_SRC_FILES := src/wesside-ng/wesside-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := easside-ng
LOCAL_SRC_FILES := src/easside-ng/easside-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := buddy-ng
LOCAL_SRC_FILES := src/buddy-ng/buddy-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := besside-ng
LOCAL_SRC_FILES := src/besside-ng/besside-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := makeivs-ng
LOCAL_SRC_FILES := src/makeivs-ng/makeivs-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airolib-ng
LOCAL_SRC_FILES := src/airolib-ng/airolib-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS) -DHAVE_REGEXP
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS) libcowpatty libsqlite3_aircrack
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airbase-ng
LOCAL_SRC_FILES := src/airbase-ng/airbase-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := airdecloak-ng
LOCAL_SRC_FILES := src/airdecloak-ng/airdecloak-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := tkiptun-ng
LOCAL_SRC_FILES := src/tkiptun-ng/tkiptun-ng.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := wpaclean
LOCAL_SRC_FILES := src/wpaclean/wpaclean.c
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBS)
LOCAL_LDLIBS += -lz -lm
include $(BUILD_EXECUTABLE)
