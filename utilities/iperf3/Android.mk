LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# Source list tracks iperf 3.22 src/Makefile.am (libiperf_la_SOURCES +
# iperf3_SOURCES). t_*.c are standalone test tools with their own main()
# and are intentionally excluded.
LOCAL_SRC_FILES:= \
	src/cjson.c \
	src/dscp.c \
	src/iperf_api.c \
	src/iperf_auth.c \
	src/iperf_client_api.c \
	src/iperf_error.c \
	src/iperf_locale.c \
	src/iperf_pthread.c \
	src/iperf_sctp.c \
	src/iperf_server_api.c \
	src/iperf_tcp.c \
	src/iperf_time.c \
	src/iperf_udp.c \
	src/iperf_util.c \
	src/main.c \
	src/net.c \
	src/tcp_info.c \
	src/timer.c \
	src/units.c

LOCAL_CFLAGS:=-O2 -g -fcommon -D_GNU_SOURCE
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_LDLIBS := -lm

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE:=iperf3

LOCAL_LDFLAGS += -Wl,--no-fatal-warnings

include $(BUILD_EXECUTABLE)
