LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# libnl 3.12.0 core (libnl-3) + generic netlink (libnl-genl-3) sources.
# Only the core and genl subsystems are built; the route/netfilter/xfrm/cli
# subsystems are not needed by the nexmon utilities (iw/nl80211).
LOCAL_SRC_FILES := \
	lib/addr.c \
	lib/attr.c \
	lib/cache.c \
	lib/cache_mngr.c \
	lib/cache_mngt.c \
	lib/data.c \
	lib/error.c \
	lib/handlers.c \
	lib/hash.c \
	lib/hashtable.c \
	lib/mpls.c \
	lib/msg.c \
	lib/nl.c \
	lib/object.c \
	lib/socket.c \
	lib/utils.c \
	lib/version.c \
	lib/genl/ctrl.c \
	lib/genl/family.c \
	lib/genl/genl.c \
	lib/genl/mngt.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/linux-private \
	$(LOCAL_PATH)/third_party/c-list/src \
	$(LOCAL_PATH)/lib
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_MODULE := libnl

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := libnl
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_MODULE := libnl

include $(BUILD_SHARED_LIBRARY)
