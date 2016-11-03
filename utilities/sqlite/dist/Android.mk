##
##
## Build the library
##
##

LOCAL_PATH:= $(call my-dir)

# NOTE the following flags,
#   SQLITE_TEMP_STORE=3 causes all TEMP files to go into RAM. and thats the behavior we want
#   SQLITE_ENABLE_FTS3   enables usage of FTS3 - NOT FTS1 or 2.
#   SQLITE_DEFAULT_AUTOVACUUM=1  causes the databases to be subject to auto-vacuum
minimal_sqlite_flags := \
	-DNDEBUG=1 \
	-DHAVE_USLEEP=1 \
	-DSQLITE_HAVE_ISNAN \
	-DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
	-DSQLITE_THREADSAFE=2 \
	-DSQLITE_TEMP_STORE=3 \
	-DSQLITE_POWERSAFE_OVERWRITE=1 \
	-DSQLITE_DEFAULT_FILE_FORMAT=4 \
	-DSQLITE_DEFAULT_AUTOVACUUM=1 \
	-DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
	-DSQLITE_ENABLE_FTS3 \
	-DSQLITE_ENABLE_FTS3_BACKWARDS \
	-DSQLITE_ENABLE_FTS4 \
	-DSQLITE_OMIT_BUILTIN_TEST \
	-DSQLITE_OMIT_COMPILEOPTION_DIAGS \
	-DSQLITE_OMIT_LOAD_EXTENSION \
	-DSQLITE_DEFAULT_FILE_PERMISSIONS=0600 \
	-DSQLITE_SECURE_DELETE

minimal_linux_flags := \
    -DHAVE_POSIX_FALLOCATE=1 \

device_sqlite_flags := $(minimal_sqlite_flags) \
    -DSQLITE_ENABLE_ICU \
    -DUSE_PREAD64 \
    -Dfdatasync=fdatasync \
    -DHAVE_MALLOC_H=1 \
    -DHAVE_MALLOC_USABLE_SIZE

common_src_files := sqlite3.c

# the device library
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_src_files)

LOCAL_CFLAGS += $(device_sqlite_flags)
LOCAL_CFLAGS_linux += $(minimal_linux_flags)

LOCAL_SHARED_LIBRARIES := libdl

LOCAL_MODULE:= libsqlite

LOCAL_C_INCLUDES += $(call include-path-for, system-core)/cutils

LOCAL_SHARED_LIBRARIES += liblog \
            libicuuc \
            libicui18n \
            libutils \
            liblog

# include android specific methods
LOCAL_WHOLE_STATIC_LIBRARIES := libsqlite3_android

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_src_files)
LOCAL_LDLIBS += -lpthread -ldl
LOCAL_CFLAGS += $(minimal_sqlite_flags)
LOCAL_CFLAGS_linux += $(minimal_linux_flags)
LOCAL_MODULE:= libsqlite
LOCAL_SHARED_LIBRARIES += libicuuc-host libicui18n-host
LOCAL_STATIC_LIBRARIES := liblog libutils libcutils

# include android specific methods
LOCAL_WHOLE_STATIC_LIBRARIES := libsqlite3_android
include $(BUILD_HOST_SHARED_LIBRARY)

##
##
## Build the device command line tool sqlite3
##
##
ifneq ($(SDK_ONLY),true)  # SDK doesn't need device version of sqlite3

include $(CLEAR_VARS)

LOCAL_SRC_FILES := shell.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../android \
    $(call include-path-for, system-core)/cutils

LOCAL_SHARED_LIBRARIES := libsqlite \
            libicuuc \
            libicui18n \
            liblog \
            libutils

LOCAL_STATIC_LIBRARIES := libicuandroid_utils

LOCAL_CFLAGS += $(device_sqlite_flags)
LOCAL_CFLAGS_linux += $(minimal_linux_flags)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE_TAGS := debug

LOCAL_MODULE := sqlite3

include $(BUILD_EXECUTABLE)

endif # !SDK_ONLY


##
##
## Build the host command line tool sqlite3
##
##

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_src_files) shell.c
LOCAL_CFLAGS += $(minimal_sqlite_flags) \
    -DNO_ANDROID_FUNCS=1
LOCAL_CFLAGS_linux += $(minimal_linux_flags)

# sqlite3MemsysAlarm uses LOG()
LOCAL_STATIC_LIBRARIES += liblog

LOCAL_LDLIBS_darwin += -lpthread -ldl
LOCAL_LDLIBS_linux += -lpthread -ldl

LOCAL_MODULE_HOST_OS := darwin linux windows

LOCAL_MODULE := sqlite3

include $(BUILD_HOST_EXECUTABLE)

# Build a minimal version of sqlite3 without any android specific
# features against the NDK. This is used by libcore's JDBC related
# unit tests.
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_src_files)
LOCAL_CFLAGS += $(minimal_sqlite_flags)
LOCAL_CFLAGS_linux += $(minimal_linux_flags)
LOCAL_MODULE:= libsqlite_static_minimal
LOCAL_SDK_VERSION := 23
include $(BUILD_STATIC_LIBRARY)

# Same as libsqlite_static_minimal, except that this is for the host.
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_src_files)
LOCAL_CFLAGS += $(minimal_sqlite_flags)
LOCAL_CFLAGS_linux += $(minimal_linux_flags)
LOCAL_MODULE:= libsqlite_static_minimal
include $(BUILD_HOST_STATIC_LIBRARY)
