LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  src/osdep/network.c \
  src/osdep/file.c \
  src/osdep/osdep.c \
  src/osdep/linux.c \
  src/osdep/linux_tap.c \
  src/osdep/common.c \
  src/osdep/radiotap/radiotap.c

LOCAL_CFLAGS += -g -W -Wall -O3 -DANDROID -D_REVISION=0 -D_FILE_OFFSET_BITS=64 \
	-Wno-unused-but-set-variable -Wno-array-bounds -Wno-unused-parameter -Wno-unused-variable \
	-I $(LOCAL_PATH)/src -I $(LOCAL_PATH)/src/radiotap \
	-D__int8_t_defined -fPIC -mandroid -DANDROID -DOS_ANDROID -DLinux -DHAVE_SQLITE

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE:= libosdep

include $(BUILD_STATIC_LIBRARY)
