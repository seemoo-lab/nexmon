LOCAL_PATH:= $(call my-dir)
UTILITIES_PATH := $(LOCAL_PATH)/../../../../../../../utilities

libpcap_cflags := \
  -Wno-unused-parameter \
  -D_U_="__attribute__((unused))" \
  -Werror \

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  $(UTILITIES_PATH)/libpcap/pcap-linux.c \
  $(UTILITIES_PATH)/libpcap/pcap-usb-linux.c \
  $(UTILITIES_PATH)/libpcap/pcap-can-linux.c \
  $(UTILITIES_PATH)/libpcap/pcap-netfilter-linux.c \
  $(UTILITIES_PATH)/libpcap/pcap-netfilter-linux-android.c \
  $(UTILITIES_PATH)/libpcap/fad-gifc.c \
  $(UTILITIES_PATH)/libpcap/pcap.c \
  $(UTILITIES_PATH)/libpcap/inet.c \
  $(UTILITIES_PATH)/libpcap/gencode.c \
  $(UTILITIES_PATH)/libpcap/optimize.c \
  $(UTILITIES_PATH)/libpcap/nametoaddr.c \
  $(UTILITIES_PATH)/libpcap/etherent.c \
  $(UTILITIES_PATH)/libpcap/savefile.c \
  $(UTILITIES_PATH)/libpcap/sf-pcap.c \
  $(UTILITIES_PATH)/libpcap/sf-pcap-ng.c \
  $(UTILITIES_PATH)/libpcap/pcap-common.c \
  $(UTILITIES_PATH)/libpcap/bpf/net/bpf_filter.c \
  $(UTILITIES_PATH)/libpcap/bpf_image.c \
  $(UTILITIES_PATH)/libpcap/bpf_dump.c \
  $(UTILITIES_PATH)/libpcap/version.c \
  mysniffer.c \
  myinjecter.c

# Generated on the host with `configure && make` and copied across.
LOCAL_SRC_FILES += $(UTILITIES_PATH)/libpcap/grammar.c
LOCAL_SRC_FILES += $(UTILITIES_PATH)/libpcap/scanner.c

LOCAL_CFLAGS += $(libpcap_cflags)
LOCAL_CFLAGS += -Wno-sign-compare
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -include strings.h # For ffs(3).

LOCAL_C_INCLUDES := $(UTILITIES_PATH)/libpcap
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_MODULE := libpcap

include $(BUILD_STATIC_LIBRARY)
