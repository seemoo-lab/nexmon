LOCAL_PATH:= $(call my-dir)

libpcap_cflags := \
  -Wno-unused-parameter \
  -D_U_="__attribute__((unused))" \
  -Werror \

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  pcap-linux.c pcap-usb-linux.c pcap-can-linux.c pcap-netfilter-linux.c pcap-netfilter-linux-android.c \
  fad-gifc.c \
  pcap.c inet.c gencode.c optimize.c nametoaddr.c etherent.c \
  savefile.c sf-pcap.c sf-pcap-ng.c pcap-common.c \
  bpf/net/bpf_filter.c bpf_image.c bpf_dump.c \
  version.c \

# Generated on the host with `configure && make` and copied across.
LOCAL_SRC_FILES += grammar.c
LOCAL_SRC_FILES += scanner.c

LOCAL_CFLAGS += $(libpcap_cflags)
LOCAL_CFLAGS += -Wno-sign-compare
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -include strings.h # For ffs(3).

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE:= libpcap

include $(BUILD_STATIC_LIBRARY)
