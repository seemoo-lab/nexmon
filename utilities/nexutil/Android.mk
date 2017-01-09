LOCAL_PATH := $(call my-dir)
PATCHES_PATH := $(LOCAL_PATH)/../../patches

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	nexutil.c

LOCAL_MODULE := nexutil

LOCAL_STATIC_LIBRARIES  += libnexio
LOCAL_STATIC_LIBRARIES  += libargp

LOCAL_CFLAGS += -DVERSION=\"$(GIT_VERSION)\"

LOCAL_C_INCLUDES += $(PATCHES_PATH)/include

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := libnexio
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libnexio/local/armeabi/libnexio.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libnexio
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libargp
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libargp/local/armeabi/libargp.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libargp
include $(PREBUILT_STATIC_LIBRARY)
