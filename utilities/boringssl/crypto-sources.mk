LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/sources.mk
include $(LOCAL_PATH)/sources.mk

# Modern BoringSSL (0.20260813.0) is a C++ codebase.  Headers live in include/,
# internal headers are found relative to the tree root, and generated files
# (err_data.cc) live under gen/.  Built without assembly (-DOPENSSL_NO_ASM).
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH) $(LOCAL_PATH)/gen
LOCAL_CFLAGS += -DBORINGSSL_ANDROID_SYSTEM -DOPENSSL_NO_ASM -Wno-unused-parameter
LOCAL_CPPFLAGS += -std=gnu++17 -fno-exceptions -fno-rtti
LOCAL_SRC_FILES += $(crypto_sources)
