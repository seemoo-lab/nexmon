# Note that some host libraries have the same module name as the target
# libraries. This is currently needed to build, for example, adb. But it's
# probably something that should be changed.

THIS_LOCAL_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)/../boringssl

# Target static library
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libcrypto
# Modern BoringSSL (0.20260813.0) exports its headers from include/ (was src/include).
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(THIS_LOCAL_PATH)/Android.mk $(LOCAL_PATH)/crypto-sources.mk
LOCAL_CFLAGS += -fvisibility=hidden -DBORINGSSL_IMPLEMENTATION -DOPENSSL_SMALL -DOPENSSL_NO_ASM -Wno-unused-parameter
include $(LOCAL_PATH)/crypto-sources.mk
include $(BUILD_STATIC_LIBRARY)
