LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/sources.mk
include $(LOCAL_PATH)/sources.mk

LOCAL_CFLAGS += -I$(LOCAL_PATH)/src/include -I$(LOCAL_PATH)/src/crypto -Wno-unused-parameter -DBORINGSSL_ANDROID_SYSTEM
LOCAL_ASFLAGS += -I$(LOCAL_PATH)/src/include -I$(LOCAL_PATH)/src/crypto -Wno-unused-parameter
# Do not add in the architecture-specific files if we don't want to build assembly
ifeq ($(LOCAL_IS_HOST_MODULE),true)
LOCAL_SRC_FILES_linux_x86 := $(linux_x86_sources)
LOCAL_SRC_FILES_linux_x86_64 := $(linux_x86_64_sources)
else
LOCAL_SRC_FILES_x86 := $(linux_x86_sources)
LOCAL_SRC_FILES_x86_64 := $(linux_x86_64_sources)
LOCAL_SRC_FILES_arm := $(linux_arm_sources)
LOCAL_SRC_FILES_arm64 := $(linux_aarch64_sources)
endif
LOCAL_SRC_FILES += $(crypto_sources)
