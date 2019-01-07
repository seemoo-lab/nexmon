LOCAL_PATH:= $(call my-dir)

tcpdump_src_files := \
  addrtoname.c \
  af.c \
  bpf_dump.c \
  checksum.c \
  cpack.c \
  gmpls.c \
  gmt2local.c \
  in_cksum.c \
  ipproto.c \
  l2vpn.c \
  machdep.c \
  nlpid.c \
  oui.c \
  parsenfsfh.c \
  print-802_11.c \
  print-802_15_4.c \
  print-ah.c \
  print-ahcp.c \
  print-aodv.c \
  print-aoe.c \
  print-ap1394.c \
  print-arcnet.c \
  print-arp.c \
  print-ascii.c \
  print-atalk.c \
  print-atm.c \
  print-babel.c \
  print-beep.c \
  print-bfd.c \
  print-bgp.c \
  print-bootp.c \
  print-bt.c \
  print-calm-fast.c \
  print-carp.c \
  print-cdp.c \
  print-cfm.c \
  print-chdlc.c \
  print-cip.c \
  print-cnfp.c \
  print-dccp.c \
  print-decnet.c \
  print-dhcp6.c \
  print-domain.c \
  print-dtp.c \
  print-dvmrp.c \
  print-eap.c \
  print-egp.c \
  print-eigrp.c \
  print-enc.c \
  print-esp.c \
  print-ether.c \
  print-fddi.c \
  print-forces.c \
  print-frag6.c \
  print-fr.c \
  print-ftp.c \
  print-geneve.c \
  print-geonet.c \
  print-gre.c \
  print-hsrp.c \
  print-http.c \
  print-icmp6.c \
  print-icmp.c \
  print-igmp.c \
  print-igrp.c \
  print-ip6.c \
  print-ip6opts.c \
  print-ip.c \
  print-ipcomp.c \
  print-ipfc.c \
  print-ipnet.c \
  print-ipx.c \
  print-isakmp.c \
  print-isoclns.c \
  print-juniper.c \
  print-krb.c \
  print-l2tp.c \
  print-lane.c \
  print-ldp.c \
  print-llc.c \
  print-lldp.c \
  print-lmp.c \
  print-loopback.c \
  print-lspping.c \
  print-lwapp.c \
  print-lwres.c \
  print-m3ua.c \
  print-mobile.c \
  print-mobility.c \
  print-mpcp.c \
  print-mpls.c \
  print-mptcp.c \
  print-msdp.c \
  print-msnlb.c \
  print-nflog.c \
  print-nfs.c \
  print-ntp.c \
  print-null.c \
  print-olsr.c \
  print-openflow-1.0.c \
  print-openflow.c \
  print-ospf6.c \
  print-ospf.c \
  print-otv.c \
  print-pgm.c \
  print-pim.c \
  print-pktap.c \
  print-ppi.c \
  print-ppp.c \
  print-pppoe.c \
  print-pptp.c \
  print-radius.c \
  print-raw.c \
  print-rip.c \
  print-ripng.c \
  print-rpki-rtr.c \
  print-rrcp.c \
  print-rsvp.c \
  print-rt6.c \
  print-rtsp.c \
  print-rx.c \
  print-sctp.c \
  print-sflow.c \
  print-sip.c \
  print-sl.c \
  print-sll.c \
  print-slow.c \
  print-smb.c \
  print-smtp.c \
  print-snmp.c \
  print-stp.c \
  print-sunatm.c \
  print-sunrpc.c \
  print-symantec.c \
  print-syslog.c \
  print-tcp.c \
  print-telnet.c \
  print-tftp.c \
  print-timed.c \
  print-tipc.c \
  print-token.c \
  print-udld.c \
  print-udp.c \
  print-usb.c \
  print-vjc.c \
  print-vqp.c \
  print-vrrp.c \
  print-vtp.c \
  print-vxlan.c \
  print-wb.c \
  print-zephyr.c \
  print-zeromq.c \
  setsignal.c \
  signature.c \
  smbutil.c \
  tcpdump.c \
  util.c \
  version.c \

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(tcpdump_src_files)
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -D_U_="__attribute__((unused))"
LOCAL_CFLAGS += -Werror
LOCAL_STATIC_LIBRARIES += libpcap libssl libcrypto
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := tcpdump
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


include $(CLEAR_VARS)
LOCAL_MODULE := libpcap
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libpcap/local/armeabi/libpcap.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libpcap
include $(PREBUILT_STATIC_LIBRARY)
