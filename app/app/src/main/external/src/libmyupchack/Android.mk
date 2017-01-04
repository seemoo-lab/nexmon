LOCAL_PATH := $(call my-dir)
UTILITIES_PATH := $(LOCAL_PATH)/../../../../../../../utilities


include $(CLEAR_VARS)
LOCAL_MODULE		:= myupchack

LOCAL_SRC_FILES		:= src/upc_keys.c

LOCAL_CFLAGS		+= \
	-g -W -Wall -O3 -DANDROID -D_REVISION=0 -Wno-unused-but-set-variable -Wno-array-bounds \
	-Wno-unused-parameter -Wno-unused-variable -D__int8_t_defined -fPIC -mandroid -DANDROID \
	-DOS_ANDROID -DLinux -DHAVE_SQLITE

LOCAL_STATIC_LIBRARIES	:= libcrypto 
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libcrypto
LOCAL_SRC_FILES := $(UTILITIES_PATH)/libcrypto/local/armeabi/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(UTILITIES_PATH)/boringssl/src/include
include $(PREBUILT_STATIC_LIBRARY)
