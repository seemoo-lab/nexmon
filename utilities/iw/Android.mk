LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
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
	../libnl/lib/genl/mngt.c \
	iw.c genl.c event.c info.c phy.c \
	interface.c ibss.c station.c survey.c util.c ocb.c \
	mesh.c mpath.c mpp.c scan.c reg.c \
	reason.c status.c connect.c link.c offch.c ps.c cqm.c \
	bitrate.c wowlan.c coalesce.c roc.c p2p.c vendor.c \
	sections.c ap.c bloom.c ftm.c keys.c measurements.c mgmt.c \
	nan.c sar.c sha256.c version.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libnl/include \
	$(LOCAL_PATH)/../libnl/third_party/c-list/src \
	$(LOCAL_PATH)/../libnl/lib

LOCAL_CFLAGS += -DCONFIG_LIBNL20

# linux-private carries libnl's private kernel-header copies (linux/in.h,
# linux/types.h, ...) which shadow bionic's and break <netinet/in.h>. Add it
# with -idirafter so it is only consulted for headers the NDK sysroot lacks.
LOCAL_CFLAGS += -idirafter $(LOCAL_PATH)/../libnl/include/linux-private

# Silence some warnings for now. Needs to be fixed upstream. b/26105799
LOCAL_CFLAGS += -Wno-unused-parameter \
                -Wno-sign-compare \
                -Wno-format
LOCAL_CLANG_CFLAGS += -Wno-enum-conversion

LOCAL_LDFLAGS := -Wl,--no-gc-sections
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := iw

LOCAL_MODULE_CLASS := EXECUTABLES

include $(BUILD_EXECUTABLE)
