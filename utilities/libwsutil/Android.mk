LOCAL_PATH:= $(call my-dir)
WIRESHARK_SRC_PATH:= $(LOCAL_PATH)/../wireshark

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	$(WIRESHARK_SRC_PATH)/wsutil/adler32.c \
	$(WIRESHARK_SRC_PATH)/wsutil/aes.c \
	$(WIRESHARK_SRC_PATH)/wsutil/airpdcap_wep.c \
	$(WIRESHARK_SRC_PATH)/wsutil/base64.c \
	$(WIRESHARK_SRC_PATH)/wsutil/bitswap.c \
	$(WIRESHARK_SRC_PATH)/wsutil/buffer.c \
	$(WIRESHARK_SRC_PATH)/wsutil/clopts_common.c \
	$(WIRESHARK_SRC_PATH)/wsutil/cmdarg_err.c \
	$(WIRESHARK_SRC_PATH)/wsutil/copyright_info.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crash_info.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc6.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc7.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc8.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc10.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc11.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc16.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc16-plain.c \
	$(WIRESHARK_SRC_PATH)/wsutil/crc32.c \
	$(WIRESHARK_SRC_PATH)/wsutil/des.c \
	$(WIRESHARK_SRC_PATH)/wsutil/eax.c \
	$(WIRESHARK_SRC_PATH)/wsutil/filesystem.c \
	$(WIRESHARK_SRC_PATH)/wsutil/frequency-utils.c \
	$(WIRESHARK_SRC_PATH)/wsutil/g711.c \
	$(WIRESHARK_SRC_PATH)/wsutil/inet_addr.c \
	$(WIRESHARK_SRC_PATH)/wsutil/interface.c \
	$(WIRESHARK_SRC_PATH)/wsutil/jsmn.c \
	$(WIRESHARK_SRC_PATH)/wsutil/md4.c \
	$(WIRESHARK_SRC_PATH)/wsutil/md5.c \
	$(WIRESHARK_SRC_PATH)/wsutil/mpeg-audio.c \
	$(WIRESHARK_SRC_PATH)/wsutil/nstime.c \
	$(WIRESHARK_SRC_PATH)/wsutil/os_version_info.c \
	$(WIRESHARK_SRC_PATH)/wsutil/plugins.c \
	$(WIRESHARK_SRC_PATH)/wsutil/privileges.c \
	$(WIRESHARK_SRC_PATH)/wsutil/rc4.c \
	$(WIRESHARK_SRC_PATH)/wsutil/report_err.c \
	$(WIRESHARK_SRC_PATH)/wsutil/sha1.c \
	$(WIRESHARK_SRC_PATH)/wsutil/sha2.c \
	$(WIRESHARK_SRC_PATH)/wsutil/sober128.c \
	$(WIRESHARK_SRC_PATH)/wsutil/str_util.c \
	$(WIRESHARK_SRC_PATH)/wsutil/strnatcmp.c \
	$(WIRESHARK_SRC_PATH)/wsutil/tempfile.c \
	$(WIRESHARK_SRC_PATH)/wsutil/time_util.c \
	$(WIRESHARK_SRC_PATH)/wsutil/type_util.c \
	$(WIRESHARK_SRC_PATH)/wsutil/unicode-utils.c \
	$(WIRESHARK_SRC_PATH)/wsutil/ws_mempbrk.c \
	$(WIRESHARK_SRC_PATH)/wsutil/inet_pton.c \
	$(WIRESHARK_SRC_PATH)/wsutil/popcount.c

LOCAL_MODULE := libwsutil

LOCAL_C_INCLUDES := \
	$(WIRESHARK_SRC_PATH) \
	$(WIRESHARK_SRC_PATH)/wsutil \
	$(LOCAL_PATH)/../libglib-2.0/glib-2.0


LOCAL_CFLAGS := -std=gnu99 -DHAVE_CONFIG_H -I. -I.. -I.. -D_FORTIFY_SOURCE=2 -DWS_BUILD_DLL \
	-DG_DISABLE_SINGLE_INCLUDES -DG_DISABLE_DEPRECATED -pthread \
	-Wall -Wextra -Wendif-labels -Wpointer-arith -Wformat-security -fwrapv -fno-strict-overflow \
	-fno-delete-null-pointer-checks -Wvla -Waddress -Wattributes -Wdiv-by-zero -Wignored-qualifiers \
	-Wpragmas -Wno-overlength-strings -Wno-long-long -Wc++-compat -Wdeclaration-after-statement \
	-Wshadow -Wno-pointer-sign -Wold-style-definition -Wstrict-prototypes -Wlogical-op \
	-Wjump-misses-init -fexcess-precision=fast -fvisibility=hidden \
	-DTOP_SRCDIR=\"$(WIRESHARK_SRC_PATH)\" \
	-DDATAFILE_DIR=\"/sdcard/wireshark\" \
	-DEXTCAP_DIR=\"/sdcard/wireshark/extcap\"

include $(BUILD_STATIC_LIBRARY)
