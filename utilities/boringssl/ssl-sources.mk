LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/sources.mk
include $(LOCAL_PATH)/sources.mk

LOCAL_CFLAGS += -I$(LOCAL_PATH)/src/include -Wno-unused-parameter -DBORINGSSL_ANDROID_SYSTEM
LOCAL_SRC_FILES += $(ssl_sources)
