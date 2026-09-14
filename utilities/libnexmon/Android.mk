LOCAL_PATH:= $(call my-dir)
PATCHES_PATH := $(LOCAL_PATH)/../../patches

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	nexmon.c

LOCAL_MODULE := libnexmon
LOCAL_CFLAGS := -std=c99
LOCAL_LDLIBS := -ldl
LOCAL_C_INCLUDES += $(PATCHES_PATH)/include
LOCAL_STATIC_LIBRARIES  += libnexio

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libnexio
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libnexio/local/$(TARGET_ARCH_ABI)/libnexio.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libnexio
include $(PREBUILT_STATIC_LIBRARY)
