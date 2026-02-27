# Copyright 2013 The Android Open Source Project

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := rsa_test
LOCAL_SRC_FILES := rsa_test.c
LOCAL_STATIC_LIBRARIES := libmincrypt
include $(BUILD_HOST_NATIVE_TEST)

include $(CLEAR_VARS)
LOCAL_MODULE := ecdsa_test
LOCAL_SRC_FILES := ecdsa_test.c
LOCAL_STATIC_LIBRARIES := libmincrypt
include $(BUILD_HOST_NATIVE_TEST)
