# Note that some host libraries have the same module name as the target
# libraries. This is currently needed to build, for example, adb. But it's
# probably something that should be changed.

THIS_LOCAL_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)/../boringssl

# Target static library
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libcrypto
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/src/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(THIS_LOCAL_PATH)/Android.mk $(LOCAL_PATH)/crypto-sources.mk
LOCAL_SDK_VERSION := 9
LOCAL_CFLAGS += -fvisibility=hidden -DBORINGSSL_SHARED_LIBRARY -DBORINGSSL_IMPLEMENTATION -DOPENSSL_SMALL -DOPENSSL_NO_ASM -Wno-unused-parameter
# sha256-armv4.S does not compile with clang.
LOCAL_CLANG_ASFLAGS_arm += -no-integrated-as
LOCAL_CLANG_ASFLAGS_arm64 += -march=armv8-a+crypto
include $(LOCAL_PATH)/crypto-sources.mk
include $(BUILD_STATIC_LIBRARY)
