LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# Source list tracks iperf 2.2.1 src/Makefile.am (iperf_SOURCES) and
# compat/Makefile.am (libcompat_a_SOURCES). check*.c/.cpp are standalone test
# tools with their own main() and are intentionally excluded.
LOCAL_SRC_FILES:= \
	compat/Thread.c \
	compat/error.c \
	compat/delay.c \
	compat/gettimeofday.c \
	compat/gettcpinfo.c \
	compat/inet_ntop.c \
	compat/inet_pton.c \
	compat/signal.c \
	compat/snprintf.c \
	compat/string.c \
	src/Client.cpp \
	src/Extractor.c \
	src/isochronous.cpp \
	src/Launch.cpp \
	src/active_hosts.cpp \
	src/Listener.cpp \
	src/Locale.c \
	src/PerfSocket.cpp \
	src/Reporter.c \
	src/Reports.c \
	src/ReportOutputs.c \
	src/Server.cpp \
	src/Settings.cpp \
	src/SocketAddr.c \
	src/gnu_getopt.c \
	src/gnu_getopt_long.c \
	src/histogram.c \
	src/main.cpp \
	src/service.c \
	src/socket_io.c \
	src/stdio.c \
	src/packet_ring.c \
	src/tcp_window_size.c \
	src/pdfs.c \
	src/dscp.c \
	src/iperf_formattime.c \
	src/iperf_multicast_api.c \
	src/checksums.c \
	src/prague_cc.cpp


LOCAL_CFLAGS:=-O2 -g -DHAVE_CONFIG_H -fcommon
#LOCAL_CFLAGS+=-DLINUX

LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES := include

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=iperf

# gold in binutils 2.22 will warn about the usage of mktemp
LOCAL_LDFLAGS += -Wl,--no-fatal-warnings

include $(BUILD_EXECUTABLE)
