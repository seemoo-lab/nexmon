LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/sources.mk
include $(LOCAL_PATH)/sources.mk

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH) $(LOCAL_PATH)/gen
LOCAL_CFLAGS += -DBORINGSSL_ANDROID_SYSTEM -DOPENSSL_NO_ASM -Wno-unused-parameter
LOCAL_CPPFLAGS += -std=gnu++17 -fno-exceptions -fno-rtti
LOCAL_SRC_FILES += $(ssl_sources)
