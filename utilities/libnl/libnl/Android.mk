LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  lib/cache.c \
	lib/data.c \
	lib/nl.c \
	lib/doc.c \
	lib/cache_mngr.c \
	lib/addr.c \
	lib/socket.c \
	lib/fib_lookup/lookup.c \
	lib/fib_lookup/request.c \
	lib/msg.c \
	lib/object.c \
	lib/attr.c \
	lib/utils.c \
	lib/cache_mngt.c \
	lib/handlers.c \
	lib/genl/ctrl.c \
	lib/genl/mngt.c \
	lib/genl/family.c \
	lib/genl/genl.c \
	lib/route/rtnl.c \
	lib/route/route_utils.c \
	lib/netfilter/nfnl.c \
	lib/error.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_MODULE := libnl

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := libnl
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_MODULE := libnl

include $(BUILD_SHARED_LIBRARY)
