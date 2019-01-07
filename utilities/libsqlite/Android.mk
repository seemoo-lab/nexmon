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

include $(BUILD_STATIC_LIBRARY)
