LOCAL_PATH := $(call my-dir)
UTILITIES_PATH := $(LOCAL_PATH)/../../../../../../../utilities
MDK3_PATH := $(UTILITIES_PATH)/mdk3


include $(CLEAR_VARS)
LOCAL_MODULE		:= beaconflood
LOCAL_SRC_FILES:=\
	src/infomessage.c\
    src/attackbeaconflood.c \
    src/beaconfloodstop.c \
    src/attacks/attacks_beaconflood.c \
    src/attacks/mybeacon_flood.c \
	$(MDK3_PATH)/src/debug.c \
	$(MDK3_PATH)/src/helpers.c \
	$(MDK3_PATH)/src/mac_addr.c \
	$(MDK3_PATH)/src/linkedlist.c \
	$(MDK3_PATH)/src/greylist.c \
	$(MDK3_PATH)/src/dumpfile.c \
	$(MDK3_PATH)/src/packet.c \
	$(MDK3_PATH)/src/brute.c \
	$(MDK3_PATH)/src/osdep.c \
	$(MDK3_PATH)/src/channelhopper.c \
	$(MDK3_PATH)/src/ghosting.c \
	$(MDK3_PATH)/src/fragmenting.c

LOCAL_CFLAGS := -g -O3 -Wall -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter \
	-lpcap -D__int8_t_defined -fPIC -mandroid -DANDROID -DOS_ANDROID -DLinux

LOCAL_EXPORT_C_INCLUDES := $(MDK3_PATH)/src/attacks
LOCAL_C_INCLUDES += $(MDK3_PATH)/src $(LOCAL_PATH)/src
LOCAL_STATIC_LIBRARIES := libosdep libpcap libsender
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libosdep
LOCAL_SRC_FILES := $(MDK3_PATH)/../libosdep/local/armeabi/libosdep.a
LOCAL_EXPORT_C_INCLUDES := $(MDK3_PATH)/../libosdep/src
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libpcap
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libpcap/local/armeabi/libpcap.a
LOCAL_EXPORT_C_INCLUDES := $(UTILITIES_PATH)/libpcap
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libsender
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libsender/local/armeabi/libsender.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libsender/src
include $(PREBUILT_STATIC_LIBRARY)
