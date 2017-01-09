LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	rawproxy.c

LOCAL_MODULE := rawproxy
LOCAL_CFLAGS += -D_GL_INLINE_HEADER_BEGIN=
LOCAL_CFLAGS += -D_GL_INLINE_HEADER_END=
LOCAL_CFLAGS += -DARGP_EI=inline

LOCAL_CFLAGS += -DVERSION=\"$(GIT_VERSION)\"

LOCAL_CFLGGS += -DUSE_LIBPCAP=1

LOCAL_STATIC_LIBRARIES := libpcap libargp
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := libargp
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libargp/local/armeabi/libargp.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libargp
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libpcap
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libpcap/local/armeabi/libpcap.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libpcap
include $(PREBUILT_STATIC_LIBRARY)
