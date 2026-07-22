LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	mempcpy.c \
	strchrnul.c \
	rawmemchr.c \
	basename-lgpl.c \
	argp-parse.c \
	argp-help.c \
	argp-pvh.c \
	argp-fmtstream.c \
	argp-eexst.c \
	getopt1.c \
	getopt.c

LOCAL_CFLAGS := -std=c99
LOCAL_CFLAGS += -D_GL_INLINE_HEADER_BEGIN=
LOCAL_CFLAGS += -D_GL_INLINE_HEADER_END=
LOCAL_CFLAGS += -DARGP_EI=inline
LOCAL_CFLAGS += -D_GL_INLINE=inline
LOCAL_CFLAGS += -D_GL_ATTRIBUTE_PURE=
LOCAL_CFLAGS += -Dfwrite_unlocked=fwrite
LOCAL_CFLAGS += -Dfputs_unlocked=fputs
LOCAL_CFLAGS += -D__getopt_argv_const=
LOCAL_CFLAGS += -D_GL_UNUSED=
ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -mabi=aapcs-linux
endif

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE:= libargp

include $(BUILD_STATIC_LIBRARY)
