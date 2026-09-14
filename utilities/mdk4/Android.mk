LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE		:= mdk4
LOCAL_SRC_FILES:=\
	src/mdk4.c \
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
	-D__int8_t_defined -fPIC -DANDROID -DOS_ANDROID -DLinux -fcommon \
	-DCONFIG_LIBNL20 -D_REVISION=nexmon

LOCAL_C_INCLUDES +=$(LOCAL_PATH)/../libpcap
LOCAL_STATIC_LIBRARIES := libpcap libosdep libattacks libnl
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := libosdep
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libosdep/local/$(TARGET_ARCH_ABI)/libosdep.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libosdep/src
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libpcap
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libpcap/local/$(TARGET_ARCH_ABI)/libpcap.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libpcap
include $(PREBUILT_STATIC_LIBRARY)


# libnl 3.12.0 core (libnl-3) + generic netlink (libnl-genl-3).
# channelhopper.c uses nl80211 for channel enumeration/switching. The
# linux-private include dir is kept internal to this module so it does not
# shadow the toolchain's <linux/*> headers pulled in by libpcap.
include $(CLEAR_VARS)
LOCAL_MODULE		:= libnl
LOCAL_SRC_FILES		:=\
	../libnl/lib/addr.c \
	../libnl/lib/attr.c \
	../libnl/lib/cache.c \
	../libnl/lib/cache_mngr.c \
	../libnl/lib/cache_mngt.c \
	../libnl/lib/data.c \
	../libnl/lib/error.c \
	../libnl/lib/handlers.c \
	../libnl/lib/hash.c \
	../libnl/lib/hashtable.c \
	../libnl/lib/mpls.c \
	../libnl/lib/msg.c \
	../libnl/lib/nl.c \
	../libnl/lib/object.c \
	../libnl/lib/socket.c \
	../libnl/lib/utils.c \
	../libnl/lib/version.c \
	../libnl/lib/genl/ctrl.c \
	../libnl/lib/genl/family.c \
	../libnl/lib/genl/genl.c \
	../libnl/lib/genl/mngt.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../libnl/include \
	$(LOCAL_PATH)/../libnl/third_party/c-list/src \
	$(LOCAL_PATH)/../libnl/lib
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libnl/include
# linux-private carries libnl's private kernel-header copies (linux/in.h,
# linux/types.h, ...) which shadow bionic's and break <netinet/in.h>. Add it
# with -idirafter so it is only consulted for headers the NDK sysroot lacks.
LOCAL_CFLAGS := -DCONFIG_LIBNL20 -Wno-unused-parameter -Wno-sign-compare -Wno-format \
	-idirafter $(LOCAL_PATH)/../libnl/include/linux-private
include $(BUILD_STATIC_LIBRARY)


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
	src/attacks/poc.c \
	src/attacks/probing.c \
	src/attacks/wids.c

LOCAL_CFLAGS:=-g -O3 -Wall -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter \
	-D__int8_t_defined -fPIC -DANDROID -DOS_ANDROID -DLinux -fcommon \
	-D_REVISION=nexmon

LOCAL_STATIC_LIBRARIES := libosdep
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src/attacks
include $(BUILD_STATIC_LIBRARY)
