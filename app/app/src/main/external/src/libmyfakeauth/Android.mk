# KrisWebDev[n01ce] Android.mk to build aircrack-ng suite for Android
# Android.mk version: 1.1

LOCAL_PATH := $(call my-dir)
AIRCRACK_NG_PATH := $(LOCAL_PATH)/../../../../../../../utilities/aircrack-ng


include $(CLEAR_VARS)
LOCAL_MODULE		:= myfakeauth

LOCAL_SRC_FILES		:= \
	src/myfakeauth.c \
	$(AIRCRACK_NG_PATH)/src/common.c \
	$(AIRCRACK_NG_PATH)/src/crypto.c

LOCAL_CFLAGS		+= \
	-g -W -Wall -O3 -DANDROID -D_REVISION=0 -D_FILE_OFFSET_BITS=64 \
	-Wno-unused-but-set-variable -Wno-array-bounds -Wno-unused-parameter -Wno-unused-variable \
	-I $(LOCAL_PATH)/src -I $(LOCAL_PATH)/src/include \
	-I $(AIRCRACK_NG_PATH)/src -I $(AIRCRACK_NG_PATH)/src/include \
	-D__int8_t_defined \
	-fPIC -mandroid -DANDROID -DOS_ANDROID -DLinux -DHAVE_SQLITE

LOCAL_SHARED_LIBRARIES	:= libandroidlogger
LOCAL_STATIC_LIBRARIES	:= libssl libcrypto libosdep
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libssl
LOCAL_SRC_FILES := $(AIRCRACK_NG_PATH)/../libssl/local/armeabi/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(AIRCRACK_NG_PATH)/../boringssl/src/include
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libcrypto
LOCAL_SRC_FILES := $(AIRCRACK_NG_PATH)/../libcrypto/local/armeabi/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(AIRCRACK_NG_PATH)/../boringssl/src/include
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libsqlite
LOCAL_SRC_FILES := $(AIRCRACK_NG_PATH)/../libsqlite/local/armeabi/libsqlite.a
LOCAL_EXPORT_C_INCLUDES := $(AIRCRACK_NG_PATH)/../libsqlite
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libosdep
LOCAL_SRC_FILES := $(AIRCRACK_NG_PATH)/../libosdep/local/armeabi/libosdep.a
LOCAL_EXPORT_C_INCLUDES := $(AIRCRACK_NG_PATH)/../libosdep/src
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libandroidlogger
LOCAL_SRC_FILES := ../libandroidlogger/libs/armeabi/libandroidlogger.so
LOCAL_C_INCLUDES := ../libandroidlogger
include $(PREBUILT_SHARED_LIBRARY)
