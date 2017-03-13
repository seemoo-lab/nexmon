LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fPIE
LOCAL_LDFLAGS += -fPIE -pie

LOCAL_MODULE := iperf3
LOCAL_SRC_FILES := \
	src/cjson.c \
	src/iperf_api.c \
	src/iperf_error.c \
	src/iperf_client_api.c \
	src/iperf_locale.c \
	src/iperf_server_api.c \
	src/iperf_tcp.c \
	src/iperf_udp.c \
	src/iperf_sctp.c \
	src/iperf_util.c \
	src/main.c \
	src/net.c \
	src/tcp_info.c \
	src/tcp_window_size.c \
	src/timer.c \
	src/units.c

LOCAL_C_INCLUDES += src

include $(BUILD_EXECUTABLE)