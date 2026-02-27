LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	socat.c \
	xioinitialize.c \
	xiohelp.c \
	xioparam.c \
	xiodiag.c \
	xioopen.c \
	xioopts.c \
	xiosignal.c \
	xiosigchld.c \
	xioread.c \
	xiowrite.c \
	xiotransfer.c \
	xioengine.c \
	xiolayer.c \
	xioshutdown.c \
	xioclose.c \
	xioexit.c \
	xiosocketpair.c \
	xio-process.c \
	xio-fd.c \
	xio-fdnum.c \
	xio-stdio.c \
	xio-pipe.c \
	xio-gopen.c \
	xio-creat.c \
	xio-file.c \
	xio-named.c \
	xio-socket.c \
	xio-interface.c \
	xio-listen.c \
	xio-unix.c \
	xio-ip.c \
	xio-ip4.c \
	xio-ip6.c \
	xio-ipapp.c \
	xio-tcp.c \
	xio-sctp.c \
	xio-rawip.c \
	xio-socks.c \
	xio-socks5.c \
	xio-proxy.c \
	xio-udp.c \
	xio-progcall.c \
	xio-exec.c \
	xio-system.c \
	xio-termios.c \
	xio-readline.c \
	xio-pty.c \
	xio-openssl.c \
	xio-streams.c \
	xio-ascii.c \
	xiolockfile.c \
	xio-tcpwrap.c \
	xio-ext2.c \
	xio-tun.c \
	xio-nop.c \
	xio-test.c \
	error.c \
	openpty.c \
	dalan.c \
	procan.c \
	procan-cdefs.c \
	hostan.c \
	fdname.c \
	sysutils.c \
	utils.c \
	nestlex.c \
	vsnprintf_r.c \
	snprinterr.c \
	filan.c \
	sycls.c \
	sslcls.c

LOCAL_MODULE := socat

LOCAL_CFLAGS += -DVERSION=\"$(GIT_VERSION)\"
LOCAL_CFLAGS += -DANDROID -Wno-multichar -D_GNU_SOURCE -Wall -Wno-parentheses -pthread -DHAVE_CONFIG_H -DANDROID -DNO_XMALLOC -mandroid
LOCAL_CFLAGS += -DRAND_F_SSLEAY_RAND_BYTES=100 -DRAND_R_PRNG_NOT_SEEDED=100

LOCAL_STATIC_LIBRARIES += libssl libcrypto

LOCAL_C_INCLUDES += .

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := libssl
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libssl/local/armeabi/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../boringssl/src/include
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libcrypto
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libcrypto/local/armeabi/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../boringssl/src/include
include $(PREBUILT_STATIC_LIBRARY)
