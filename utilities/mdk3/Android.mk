LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE		:= mdk3
LOCAL_SRC_FILES:=\
	src/mdk3.c \
	src/debug.c \
	src/helpers.c \
	src/mac_addr.c \
	src/linkedlist.c \
	src/greylist.c \
	src/dumpfile.c \
	src/packet.c \
	src/brute.c \
	src/osdep.c \
	src/channelhopper.c \
	src/ghosting.c \
	src/fragmenting.c

LOCAL_CFLAGS:=-g -O3 -Wall -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter \
	-I $(LOCAL_PATH)/src -I $(LOCAL_PATH)/src/osdep -I $(LOCAL_PATH)/src/osdep/radiotap \
	-D__int8_t_defined -fPIC -mandroid -DANDROID -DOS_ANDROID -DLinux

LOCAL_C_INCLUDES +=$(LOCAL_PATH)/../libpcap
LOCAL_STATIC_LIBRARIES := libpcap libosdep libattacks
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := libosdep
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libosdep/local/armeabi/libosdep.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libosdep/src
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE		:= libattacks
LOCAL_SRC_FILES		:=\
	src/attacks/attacks.c \
	src/attacks/auth_dos.c \
	src/attacks/beacon_flood.c \
	src/attacks/countermeasures.c \
	src/attacks/deauth.c \
	src/attacks/dummy.c \
	src/attacks/eapol.c \
	src/attacks/fuzzer.c \
	src/attacks/ieee80211s.c \
	src/attacks/probing.c \
	src/attacks/wids.c

LOCAL_CFLAGS:=-g -O3 -Wall -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter \
	-D__int8_t_defined -fPIC -mandroid -DANDROID -DOS_ANDROID -DLinux

LOCAL_STATIC_LIBRARIES := libosdep
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src/attacks
include $(BUILD_STATIC_LIBRARY)