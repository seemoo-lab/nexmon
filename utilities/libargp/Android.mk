LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# gnulib argp from MSYS2 libargp 20260115
# (https://git.savannah.gnu.org/cgit/gnulib.git, commit e93789db).
# bionic getopt is not sufficient (argp needs getopt_int.h / _getopt_internal);
# strchrnul/mempcpy/rawmemchr fill libc gaps at API 21.
LOCAL_SRC_FILES := \
	argp-ba.c \
	argp-eexst.c \
	argp-fmtstream.c \
	argp-fs-xinl.c \
	argp-help.c \
	argp-parse.c \
	argp-pin.c \
	argp-pv.c \
	argp-pvh.c \
	argp-xinl.c \
	basename-lgpl.c \
	getopt.c \
	getopt1.c \
	strchrnul.c \
	mempcpy.c \
	rawmemchr.c

LOCAL_CFLAGS := -std=gnu11 -DHAVE_CONFIG_H=1 -include config.h \
	-Wno-unused-parameter -Wno-sign-compare -Wno-unused-function \
	-Wno-\#warnings
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE := libargp

include $(BUILD_STATIC_LIBRARY)
