LOCAL_PATH:= $(call my-dir)

libpcap_cflags := \
    -DHAVE_CONFIG_H \
    -Dlint \
    -Wall \
    -Werror \
    -Wno-sign-compare \
    -Wno-unused-parameter

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  pcap-linux.c pcap-netfilter-linux-android.c \
  fad-gifc.c \
  pcap.c gencode.c optimize.c nametoaddr.c etherent.c fmtutils.c \
  savefile.c sf-pcap.c sf-pcapng.c pcap-common.c \
  bpf_filter.c bpf_image.c bpf_dump.c \

# Generated on the host with `configure && make` and copied across.
LOCAL_SRC_FILES += grammar.c
LOCAL_SRC_FILES += scanner.c

LOCAL_CFLAGS += $(libpcap_cflags)

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE:= libpcap

include $(BUILD_STATIC_LIBRARY)
