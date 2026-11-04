/* src/iperf_config.h.  Generated for Android NDK from iperf_config.h.in.  */

/* Can bind to device via SO_BINDTODEVICE or IP_BOUND_IF. */
#define CAN_BIND_TO_DEVICE 1

/* Define to 1 if you have the 'clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define to 1 if you have the 'clock_nanosleep' function. */
#define HAVE_CLOCK_NANOSLEEP 1

/* Define to 1 if you have the 'cpuset_setaffinity' function. */
/* #undef HAVE_CPUSET_SETAFFINITY */

/* Have CPU affinity support. */
/* #undef HAVE_CPU_AFFINITY */

/* Define to 1 if you have the 'daemon' function. */
#define HAVE_DAEMON 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Have IP_MTU_DISCOVER/IP_DONTFRAG/IP_DONTFRAGMENT sockopt. */
/* #undef HAVE_DONT_FRAGMENT */

/* Define to 1 if you have the <endian.h> header file. */
#define HAVE_ENDIAN_H 1

/* Have IPv6 flowlabel support. */
/* #undef HAVE_FLOWLABEL */

/* Define to 1 if you have the 'getline' function. */
#define HAVE_GETLINE 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Have MPTCP protocol. */
/* #undef HAVE_IPPROTO_MPTCP */

/* Have IP_BOUND_IF sockopt. */
/* #undef HAVE_IP_BOUND_IF */

/* Have IP_DONTFRAG sockopt. */
/* #undef HAVE_IP_DONTFRAG */

/* Have IP_DONTFRAGMENT sockopt. */
/* #undef HAVE_IP_DONTFRAGMENT */

/* Have IP_MTU_DISCOVER sockopt. */
/* #undef HAVE_IP_MTU_DISCOVER */

/* Define to 1 if you have the <linux/tcp.h> header file. */
/* #undef HAVE_LINUX_TCP_H */

/* Have MSG_TRUNC recv option. */
#define HAVE_MSG_TRUNC 1

/* Define to 1 if you have the 'nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Define to 1 if you have the <netinet/sctp.h> header file. */
/* #undef HAVE_NETINET_SCTP_H */

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Have PTHREAD_PRIO_INHERIT. */
/* #undef HAVE_PTHREAD_PRIO_INHERIT */

/* Define to 1 if you have the 'sched_setaffinity' function. */
/* #undef HAVE_SCHED_SETAFFINITY */

/* Have SCTP support. */
/* #undef HAVE_SCTP_H */

/* Define to 1 if you have the 'sendfile' function. */
#define HAVE_SENDFILE 1

/* Define to 1 if you have the 'SetProcessAffinityMask' function. */
/* #undef HAVE_SETPROCESSAFFINITYMASK */

/* Have SHUT_WR socket shutdown option. */
#define HAVE_SOCKET_SHUTDOWN_SHUT_WR 1

/* Have SO_BINDTODEVICE sockopt. */
#define HAVE_SO_BINDTODEVICE 1

/* Have SO_MAX_PACING_RATE sockopt. */
/* #undef HAVE_SO_MAX_PACING_RATE */

/* OpenSSL Is Available */
/* #undef HAVE_SSL */

/* Define to 1 if you have the <stdatomic.h> header file. */
#define HAVE_STDATOMIC_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if the system has the type 'struct sctp_assoc_value'. */
/* #undef HAVE_STRUCT_SCTP_ASSOC_VALUE */

/* Define to 1 if you have the <sys/endian.h> header file. */
/* #undef HAVE_SYS_ENDIAN_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Have TCP_CONGESTION sockopt. */
#define HAVE_TCP_CONGESTION 1

/* Have tcpi_snd_wnd field in tcp_info. */
/* #undef HAVE_TCP_INFO_SND_WND */

/* Have TCP_KEEPIDLE sockopt. */
#define HAVE_TCP_KEEPALIVE 1

/* Have TCP_USER_TIMEOUT sockopt. */
/* #undef HAVE_TCP_USER_TIMEOUT */

/* Have UDP_GRO sockopt. */
/* #undef HAVE_UDP_GRO */

/* Have UDP_SEGMENT sockopt. */
/* #undef HAVE_UDP_SEGMENT */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
/* #undef LT_OBJDIR */

/* Name of package */
#define PACKAGE "iperf"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://github.com/esnet/iperf"

/* Define to the full name of this package. */
#define PACKAGE_NAME "iperf"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "iperf 3.22"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "iperf"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://software.es.net/iperf/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.22"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if all of the C89 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "3.22"

/* Define to empty if 'const' does not conform to ANSI C. */
/* #undef const */
