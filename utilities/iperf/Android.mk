LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	compat/Thread.c \
	compat/error.c \
	compat/delay.c \
	compat/gettimeofday.c \
	compat/inet_ntop.c \
	compat/inet_pton.c \
	compat/signal.c \
	compat/snprintf.c \
	compat/string.c \
	src/Client.cpp \
	src/Extractor.c \
	src/Launch.cpp \
	src/List.cpp \
	src/Listener.cpp \
	src/Locale.c \
	src/PerfSocket.cpp \
	src/ReportCSV.c \
	src/ReportDefault.c \
	src/Reporter.c \
	src/Server.cpp \
	src/Settings.cpp \
	src/SocketAddr.c \
	src/gnu_getopt.c \
	src/gnu_getopt_long.c \
	src/main.cpp \
	src/service.c \
	src/sockets.c \
	src/stdio.c \
	src/tcp_window_size.c


LOCAL_CFLAGS:=-O2 -g -DHAVE_CONFIG_H
#LOCAL_CFLAGS+=-DLINUX

LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES := include

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=iperf

# gold in binutils 2.22 will warn about the usage of mktemp
LOCAL_LDFLAGS += -Wl,--no-fatal-warnings

include $(BUILD_EXECUTABLE)
