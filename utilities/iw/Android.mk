LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../libnl/lib/cache.c \
	../libnl/lib/data.c \
	../libnl/lib/nl.c \
	../libnl/lib/doc.c \
	../libnl/lib/cache_mngr.c \
	../libnl/lib/addr.c \
	../libnl/lib/socket.c \
	../libnl/lib/fib_lookup/lookup.c \
	../libnl/lib/fib_lookup/request.c \
	../libnl/lib/msg.c \
	../libnl/lib/object.c \
	../libnl/lib/attr.c \
	../libnl/lib/utils.c \
	../libnl/lib/cache_mngt.c \
	../libnl/lib/handlers.c \
	../libnl/lib/genl/ctrl.c \
	../libnl/lib/genl/mngt.c \
	../libnl/lib/genl/family.c \
	../libnl/lib/genl/genl.c \
	../libnl/lib/route/rtnl.c \
	../libnl/lib/route/route_utils.c \
	../libnl/lib/netfilter/nfnl.c \
	../libnl/lib/error.c \
	iw.c genl.c event.c info.c phy.c \
	interface.c ibss.c station.c survey.c util.c ocb.c \
	mesh.c mpath.c mpp.c scan.c reg.c \
	reason.c status.c connect.c link.c offch.c ps.c cqm.c \
	bitrate.c wowlan.c coalesce.c roc.c p2p.c vendor.c \
	sections.c version.c

LOCAL_C_INCLUDES += ../libnl/include

LOCAL_CFLAGS += -DCONFIG_LIBNL20

# Silence some warnings for now. Needs to be fixed upstream. b/26105799
LOCAL_CFLAGS += -Wno-unused-parameter \
                -Wno-sign-compare \
                -Wno-format
LOCAL_CLANG_CFLAGS += -Wno-enum-conversion

LOCAL_LDFLAGS := -Wl,--no-gc-sections
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libnl
LOCAL_MODULE := iw

LOCAL_MODULE_CLASS := EXECUTABLES

include $(BUILD_EXECUTABLE)
