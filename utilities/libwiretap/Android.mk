LOCAL_PATH:= $(call my-dir)
WIRESHARK_SRC_PATH:= $(LOCAL_PATH)/../wireshark

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	$(WIRESHARK_SRC_PATH)/wiretap/5views.c \
	$(WIRESHARK_SRC_PATH)/wiretap/aethra.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ascendtext.c \
	$(WIRESHARK_SRC_PATH)/wiretap/atm.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ber.c \
	$(WIRESHARK_SRC_PATH)/wiretap/btsnoop.c \
	$(WIRESHARK_SRC_PATH)/wiretap/camins.c \
	$(WIRESHARK_SRC_PATH)/wiretap/capsa.c \
	$(WIRESHARK_SRC_PATH)/wiretap/catapult_dct2000.c \
	$(WIRESHARK_SRC_PATH)/wiretap/commview.c \
	$(WIRESHARK_SRC_PATH)/wiretap/cosine.c \
	$(WIRESHARK_SRC_PATH)/wiretap/csids.c \
	$(WIRESHARK_SRC_PATH)/wiretap/daintree-sna.c \
	$(WIRESHARK_SRC_PATH)/wiretap/dbs-etherwatch.c \
	$(WIRESHARK_SRC_PATH)/wiretap/dct3trace.c \
	$(WIRESHARK_SRC_PATH)/wiretap/erf.c \
	$(WIRESHARK_SRC_PATH)/wiretap/eyesdn.c \
	$(WIRESHARK_SRC_PATH)/wiretap/file_access.c \
	$(WIRESHARK_SRC_PATH)/wiretap/file_wrappers.c \
	$(WIRESHARK_SRC_PATH)/wiretap/hcidump.c \
	$(WIRESHARK_SRC_PATH)/wiretap/i4btrace.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ipfix.c \
	$(WIRESHARK_SRC_PATH)/wiretap/iptrace.c \
	$(WIRESHARK_SRC_PATH)/wiretap/iseries.c \
	$(WIRESHARK_SRC_PATH)/wiretap/mime_file.c \
	$(WIRESHARK_SRC_PATH)/wiretap/json.c \
	$(WIRESHARK_SRC_PATH)/wiretap/k12.c \
	$(WIRESHARK_SRC_PATH)/wiretap/lanalyzer.c \
	$(WIRESHARK_SRC_PATH)/wiretap/logcat_text.c \
	$(WIRESHARK_SRC_PATH)/wiretap/logcat.c \
	$(WIRESHARK_SRC_PATH)/wiretap/libpcap.c \
	$(WIRESHARK_SRC_PATH)/wiretap/merge.c \
	$(WIRESHARK_SRC_PATH)/wiretap/mpeg.c \
	$(WIRESHARK_SRC_PATH)/wiretap/mplog.c \
	$(WIRESHARK_SRC_PATH)/wiretap/mp2t.c \
	$(WIRESHARK_SRC_PATH)/wiretap/netmon.c \
	$(WIRESHARK_SRC_PATH)/wiretap/netscaler.c \
	$(WIRESHARK_SRC_PATH)/wiretap/netscreen.c \
	$(WIRESHARK_SRC_PATH)/wiretap/nettl.c \
	$(WIRESHARK_SRC_PATH)/wiretap/nettrace_3gpp_32_423.c \
	$(WIRESHARK_SRC_PATH)/wiretap/network_instruments.c \
	$(WIRESHARK_SRC_PATH)/wiretap/netxray.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ngsniffer.c \
	$(WIRESHARK_SRC_PATH)/wiretap/packetlogger.c \
	$(WIRESHARK_SRC_PATH)/wiretap/pcap-common.c \
	$(WIRESHARK_SRC_PATH)/wiretap/pcapng.c \
	$(WIRESHARK_SRC_PATH)/wiretap/peekclassic.c \
	$(WIRESHARK_SRC_PATH)/wiretap/peektagged.c \
	$(WIRESHARK_SRC_PATH)/wiretap/pppdump.c \
	$(WIRESHARK_SRC_PATH)/wiretap/radcom.c \
	$(WIRESHARK_SRC_PATH)/wiretap/snoop.c \
	$(WIRESHARK_SRC_PATH)/wiretap/stanag4607.c \
	$(WIRESHARK_SRC_PATH)/wiretap/tnef.c \
	$(WIRESHARK_SRC_PATH)/wiretap/toshiba.c \
	$(WIRESHARK_SRC_PATH)/wiretap/visual.c \
	$(WIRESHARK_SRC_PATH)/wiretap/vms.c \
	$(WIRESHARK_SRC_PATH)/wiretap/vwr.c \
	$(WIRESHARK_SRC_PATH)/wiretap/wtap.c \
	$(WIRESHARK_SRC_PATH)/wiretap/wtap_opttypes.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ws_version_info.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ascend.c \
	$(WIRESHARK_SRC_PATH)/wiretap/ascend_scanner.c \
	$(WIRESHARK_SRC_PATH)/wiretap/k12text.c

LOCAL_MODULE := libwiretap

LOCAL_C_INCLUDES := \
	$(WIRESHARK_SRC_PATH) \
	$(WIRESHARK_SRC_PATH)/wiretap \
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
