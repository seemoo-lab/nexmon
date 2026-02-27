LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libsender
LOCAL_SRC_FILES := src/sender.c
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/src
include $(BUILD_STATIC_LIBRARY)
