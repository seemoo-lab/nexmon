LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# argp-standalone 1.5.0 (https://github.com/argp-standalone/argp-standalone).
# It ships its own option scanner, so no getopt sources are needed; the only
# libc gap fillers required on bionic are strchrnul() and mempcpy().
LOCAL_SRC_FILES := \
	argp-ba.c \
	argp-eexst.c \
	argp-fmtstream.c \
	argp-help.c \
	argp-parse.c \
	argp-pv.c \
	argp-pvh.c \
	strchrnul.c \
	mempcpy.c

# HAVE_CONFIG_H pulls in the hand-maintained config.h next to these sources;
# -I$(LOCAL_PATH) lets the <config.h> / <argp.h> angle-bracket includes resolve.
LOCAL_CFLAGS := -std=gnu99 -DHAVE_CONFIG_H=1 -Wno-unused-parameter -Wno-sign-compare
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE := libargp

include $(BUILD_STATIC_LIBRARY)
