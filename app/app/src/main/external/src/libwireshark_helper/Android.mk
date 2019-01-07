LOCAL_PATH:= $(call my-dir)
UTILITIES_PATH := $(LOCAL_PATH)/../../../../../../../utilities

include $(CLEAR_VARS)
LOCAL_MODULE := libwireshark_helper
LOCAL_SRC_FILES:= wireshark_helper.c
LOCAL_CFLAGS := -DHAVE_LIBPCAP
LOCAL_C_INCLUDES += $(UTILITIES_PATH)
LOCAL_STATIC_LIBRARIES := libtshark libwireshark libwiretap libwsutil libpcap libgmodule-2.0 libglib-2.0 libiconv libintl
LOCAL_LDLIBS := -llog -lz
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libglib-2.0.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/glib-2.0
LOCAL_MODULE := libglib-2.0
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libgmodule-2.0.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/glib-2.0
LOCAL_MODULE := libgmodule-2.0
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libiconv.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/glib-2.0
LOCAL_MODULE := libiconv
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libintl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/glib-2.0
LOCAL_MODULE := libintl
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libpcap
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libpcap/local/armeabi/libpcap.a
LOCAL_EXPORT_C_INCLUDES := $(UTILITIES_PATH)/libpcap
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libtshark.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/wireshark
LOCAL_MODULE := libtshark
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libwireshark.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/wireshark
LOCAL_MODULE := libwireshark
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libwiretap.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/wireshark
LOCAL_MODULE := libwiretap
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(LOCAL_PATH)/prebuildlibs/libwsutil.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/wireshark
LOCAL_MODULE := libwsutil
include $(PREBUILT_STATIC_LIBRARY)
