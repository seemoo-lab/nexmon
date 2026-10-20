# -*- Makefile -*-

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES =
noinst_LTLIBRARIES =
check_LTLIBRARIES =

clean_local =
check_PROGRAMS =
check_programs =
check_local =
check_build =

bin_PROGRAMS =
sbin_PROGRAMS =
noinst_PROGRAMS =

TESTS =

CLEANFILES =
EXTRA_DIST =

DISTCHECK_CONFIGURE_FLAGS = \
	--disable-dependency-tracking

pkgconfig_DATA = \
	libnl-3.0.pc \
	libnl-genl-3.0.pc \
	libnl-idiag-3.0.pc \
	libnl-nf-3.0.pc \
	libnl-route-3.0.pc \
	libnl-xfrm-3.0.pc \
	$(NULL)

extra_cppflags = \
	-std=gnu11 \
	$(NULL)

warn_cppflags = \
	-Wall \
	-Wextra \
	\
	-Wdeclaration-after-statement \
	-Wmissing-prototypes \
	-Wpointer-arith \
	-Wvla \
	\
	-Wno-unused-parameter \
	$(NULL)

defines_cppflags = \
	-D_NL_SYSCONFDIR_LIBNL=\"$(sysconfdir)/libnl\" \
	-D_NL_PKGLIBDIR=\"$(pkglibdir)\" \
	$(NULL)

###############################################################################

libnlincludedir = $(includedir)/libnl@MAJ_VERSION@

libnlinclude_netlinkdir = $(libnlincludedir)/netlink
libnlinclude_netlink_HEADERS = \
	include/netlink/addr.h \
	include/netlink/attr.h \
	include/netlink/cache-api.h \
	include/netlink/cache.h \
	include/netlink/data.h \
	include/netlink/errno.h \
	include/netlink/handlers.h \
	include/netlink/hash.h \
	include/netlink/hashtable.h \
	include/netlink/list.h \
	include/netlink/msg.h \
	include/netlink/netlink-compat.h \
	include/netlink/netlink-kernel.h \
	include/netlink/netlink.h \
	include/netlink/object-api.h \
	include/netlink/object.h \
	include/netlink/socket.h \
	include/netlink/types.h \
	include/netlink/utils.h \
	include/netlink/version.h \
	$(NULL)
libnlinclude_netlink_fib_lookupdir = $(libnlincludedir)/netlink/fib_lookup
libnlinclude_netlink_fib_lookup_HEADERS = \
	include/netlink/fib_lookup/lookup.h \
	include/netlink/fib_lookup/request.h \
	$(NULL)
libnlinclude_netlink_genldir = $(libnlincludedir)/netlink/genl
libnlinclude_netlink_genl_HEADERS = \
	include/netlink/genl/ctrl.h \
	include/netlink/genl/family.h \
	include/netlink/genl/genl.h \
	include/netlink/genl/mngt.h \
	$(NULL)
libnlinclude_netlink_idiagdir = $(libnlincludedir)/netlink/idiag
libnlinclude_netlink_idiag_HEADERS = \
	include/netlink/idiag/idiagnl.h \
	include/netlink/idiag/meminfo.h \
	include/netlink/idiag/msg.h \
	include/netlink/idiag/req.h \
	include/netlink/idiag/vegasinfo.h \
	$(NULL)
libnlinclude_netlink_netfilterdir = $(libnlincludedir)/netlink/netfilter
libnlinclude_netlink_netfilter_HEADERS = \
	include/netlink/netfilter/ct.h \
	include/netlink/netfilter/exp.h \
	include/netlink/netfilter/log.h \
	include/netlink/netfilter/log_msg.h \
	include/netlink/netfilter/netfilter.h \
	include/netlink/netfilter/nfnl.h \
	include/netlink/netfilter/queue.h \
	include/netlink/netfilter/queue_msg.h \
	$(NULL)
libnlinclude_netlink_routedir = $(libnlincludedir)/netlink/route
libnlinclude_netlink_route_HEADERS = \
	include/netlink/route/action.h \
	include/netlink/route/addr.h \
	include/netlink/route/class.h \
	include/netlink/route/classifier.h \
	include/netlink/route/link.h \
	include/netlink/route/mdb.h \
	include/netlink/route/neighbour.h \
	include/netlink/route/neightbl.h \
	include/netlink/route/netconf.h \
	include/netlink/route/nexthop.h \
	include/netlink/route/nh.h \
	include/netlink/route/pktloc.h \
	include/netlink/route/qdisc.h \
	include/netlink/route/route.h \
	include/netlink/route/rtnl.h \
	include/netlink/route/rule.h \
	include/netlink/route/tc-api.h \
	include/netlink/route/tc.h \
	$(NULL)
libnlinclude_netlink_route_actdir = $(libnlincludedir)/netlink/route/act
libnlinclude_netlink_route_act_HEADERS = \
	include/netlink/route/act/gact.h \
	include/netlink/route/act/mirred.h \
	include/netlink/route/act/nat.h \
	include/netlink/route/act/skbedit.h \
	include/netlink/route/act/vlan.h \
	$(NULL)
libnlinclude_netlink_route_clsdir = $(libnlincludedir)/netlink/route/cls
libnlinclude_netlink_route_cls_HEADERS = \
	include/netlink/route/cls/basic.h \
	include/netlink/route/cls/cgroup.h \
	include/netlink/route/cls/ematch.h \
	include/netlink/route/cls/flower.h \
	include/netlink/route/cls/fw.h \
	include/netlink/route/cls/matchall.h \
	include/netlink/route/cls/police.h \
	include/netlink/route/cls/u32.h \
	$(NULL)
libnlinclude_netlink_route_cls_ematchdir = $(libnlincludedir)/netlink/route/cls/ematch
libnlinclude_netlink_route_cls_ematch_HEADERS = \
	include/netlink/route/cls/ematch/cmp.h \
	include/netlink/route/cls/ematch/meta.h \
	include/netlink/route/cls/ematch/nbyte.h \
	include/netlink/route/cls/ematch/text.h \
	$(NULL)
libnlinclude_netlink_route_linkdir = $(libnlincludedir)/netlink/route/link
libnlinclude_netlink_route_link_HEADERS = \
	include/netlink/route/link/api.h \
	include/netlink/route/link/bonding.h \
	include/netlink/route/link/bridge_info.h \
	include/netlink/route/link/bridge.h \
	include/netlink/route/link/can.h \
	include/netlink/route/link/geneve.h \
	include/netlink/route/link/inet.h \
	include/netlink/route/link/inet6.h \
	include/netlink/route/link/info-api.h \
	include/netlink/route/link/ip6gre.h \
	include/netlink/route/link/ip6tnl.h \
	include/netlink/route/link/ip6vti.h \
	include/netlink/route/link/ipgre.h \
	include/netlink/route/link/ipip.h \
	include/netlink/route/link/ipvlan.h \
	include/netlink/route/link/ipvti.h \
	include/netlink/route/link/macsec.h \
	include/netlink/route/link/macvlan.h \
	include/netlink/route/link/macvtap.h \
	include/netlink/route/link/ppp.h \
	include/netlink/route/link/sit.h \
	include/netlink/route/link/sriov.h \
	include/netlink/route/link/team.h \
	include/netlink/route/link/veth.h \
	include/netlink/route/link/vlan.h \
	include/netlink/route/link/vrf.h \
	include/netlink/route/link/vxlan.h \
	include/netlink/route/link/xfrmi.h \
	$(NULL)
libnlinclude_netlink_route_qdiscdir = $(libnlincludedir)/netlink/route/qdisc
libnlinclude_netlink_route_qdisc_HEADERS = \
	include/netlink/route/qdisc/cbq.h \
	include/netlink/route/qdisc/dsmark.h \
	include/netlink/route/qdisc/fifo.h \
	include/netlink/route/qdisc/fq_codel.h \
	include/netlink/route/qdisc/hfsc.h \
	include/netlink/route/qdisc/htb.h \
	include/netlink/route/qdisc/mqprio.h \
	include/netlink/route/qdisc/netem.h \
	include/netlink/route/qdisc/plug.h \
	include/netlink/route/qdisc/prio.h \
	include/netlink/route/qdisc/red.h \
	include/netlink/route/qdisc/sfq.h \
	include/netlink/route/qdisc/tbf.h \
	$(NULL)
libnlinclude_netlink_xfrmdir = $(libnlincludedir)/netlink/xfrm
libnlinclude_netlink_xfrm_HEADERS = \
	include/netlink/xfrm/ae.h \
	include/netlink/xfrm/lifetime.h \
	include/netlink/xfrm/sa.h \
	include/netlink/xfrm/selector.h \
	include/netlink/xfrm/sp.h \
	include/netlink/xfrm/template.h \
	$(NULL)

if ENABLE_CLI
libnlinclude_netlink_clidir = $(libnlincludedir)/netlink/cli
libnlinclude_netlink_cli_HEADERS = \
	include/netlink/cli/addr.h \
	include/netlink/cli/class.h \
	include/netlink/cli/cls.h \
	include/netlink/cli/ct.h \
	include/netlink/cli/exp.h \
	include/netlink/cli/link.h \
	include/netlink/cli/mdb.h \
	include/netlink/cli/neigh.h \
	include/netlink/cli/nh.h \
	include/netlink/cli/qdisc.h \
	include/netlink/cli/route.h \
	include/netlink/cli/rule.h \
	include/netlink/cli/tc.h \
	include/netlink/cli/utils.h \
	$(NULL)
endif

public_headers = \
	$(libnlinclude_netlink_HEADERS) \
	$(libnlinclude_netlink_fib_lookup_HEADERS) \
	$(libnlinclude_netlink_genl_HEADERS) \
	$(libnlinclude_netlink_idiag_HEADERS) \
	$(libnlinclude_netlink_netfilter_HEADERS) \
	$(libnlinclude_netlink_route_HEADERS) \
	$(libnlinclude_netlink_route_act_HEADERS) \
	$(libnlinclude_netlink_route_cls_HEADERS) \
	$(libnlinclude_netlink_route_cls_ematch_HEADERS) \
	$(libnlinclude_netlink_route_link_HEADERS) \
	$(libnlinclude_netlink_route_qdisc_HEADERS) \
	$(libnlinclude_netlink_xfrm_HEADERS) \
	$(libnlinclude_netlink_cli_HEADERS) \
	$(NULL)

noinst_HEADERS = \
	include/base/nl-base-utils.h \
	include/linux-private/linux/atm.h \
	include/linux-private/linux/atmapi.h \
	include/linux-private/linux/atmioc.h \
	include/linux-private/linux/atmsap.h \
	include/linux-private/linux/byteorder/little_endian.h \
	include/linux-private/linux/can/netlink.h \
	include/linux-private/linux/const.h \
	include/linux-private/linux/errno.h \
	include/linux-private/linux/ethtool.h \
	include/linux-private/linux/falloc.h \
	include/linux-private/linux/fib_rules.h \
	include/linux-private/linux/gen_stats.h \
	include/linux-private/linux/genetlink.h \
	include/linux-private/linux/hdlc/ioctl.h \
	include/linux-private/linux/if.h \
	include/linux-private/linux/if_addr.h \
	include/linux-private/linux/if_arp.h \
	include/linux-private/linux/if_bridge.h \
	include/linux-private/linux/if_ether.h \
	include/linux-private/linux/if_link.h \
	include/linux-private/linux/if_macsec.h \
	include/linux-private/linux/if_packet.h \
	include/linux-private/linux/if_tunnel.h \
	include/linux-private/linux/if_vlan.h \
	include/linux-private/linux/in.h \
	include/linux-private/linux/in6.h \
	include/linux-private/linux/in_route.h \
	include/linux-private/linux/inet_diag.h \
	include/linux-private/linux/ioctl.h \
	include/linux-private/linux/ip.h \
	include/linux-private/linux/ipv6.h \
	include/linux-private/linux/libc-compat.h \
	include/linux-private/linux/limits.h \
	include/linux-private/linux/lwtunnel.h \
	include/linux-private/linux/mpls.h \
	include/linux-private/linux/mpls_iptunnel.h \
	include/linux-private/linux/neighbour.h \
	include/linux-private/linux/netconf.h \
	include/linux-private/linux/netdevice.h \
	include/linux-private/linux/netfilter.h \
	include/linux-private/linux/netfilter/nf_conntrack_common.h \
	include/linux-private/linux/netfilter/nf_conntrack_tcp.h \
	include/linux-private/linux/netfilter/nfnetlink.h \
	include/linux-private/linux/netfilter/nfnetlink_compat.h \
	include/linux-private/linux/netfilter/nfnetlink_conntrack.h \
	include/linux-private/linux/netfilter/nfnetlink_log.h \
	include/linux-private/linux/netfilter/nfnetlink_queue.h \
	include/linux-private/linux/netlink.h \
	include/linux-private/linux/nexthop.h \
	include/linux-private/linux/pkt_cls.h \
	include/linux-private/linux/pkt_sched.h \
	include/linux-private/linux/posix_types.h \
	include/linux-private/linux/rtnetlink.h \
	include/linux-private/linux/seg6.h \
	include/linux-private/linux/seg6_hmac.h \
	include/linux-private/linux/seg6_iptunnel.h \
	include/linux-private/linux/seg6_local.h \
	include/linux-private/linux/snmp.h \
	include/linux-private/linux/sock_diag.h \
	include/linux-private/linux/socket.h \
	include/linux-private/linux/stddef.h \
	include/linux-private/linux/swab.h \
	include/linux-private/linux/taskstats.h \
	include/linux-private/linux/tc_act/tc_gact.h \
	include/linux-private/linux/tc_act/tc_mirred.h \
	include/linux-private/linux/tc_act/tc_nat.h \
	include/linux-private/linux/tc_act/tc_skbedit.h \
	include/linux-private/linux/tc_act/tc_vlan.h \
	include/linux-private/linux/tc_ematch/tc_em_cmp.h \
	include/linux-private/linux/tc_ematch/tc_em_meta.h \
	include/linux-private/linux/tc_ematch/tc_em_nbyte.h \
	include/linux-private/linux/tc_ematch/tc_em_text.h \
	include/linux-private/linux/types.h \
	include/linux-private/linux/version.h \
	include/linux-private/linux/veth.h \
	include/linux-private/linux/xfrm.h \
	include/nl-aux-core/nl-core.h \
	include/nl-aux-route/nl-route.h \
	include/nl-aux-xfrm/nl-xfrm.h \
	include/nl-default.h \
	include/nl-priv-dynamic-core/cache-api.h \
	include/nl-priv-dynamic-core/nl-core.h \
	include/nl-priv-dynamic-core/object-api.h \
	include/nl-priv-dynamic-route/nl-priv-dynamic-route.h \
	include/nl-priv-static-route/nl-priv-static-route.h \
	\
	third_party/c-list/src/c-list.h \
	\
	$(NULL)

###############################################################################

default_includes = \
	-I$(srcdir)/include/linux-private \
	-I$(srcdir)/include \
	-I$(builddir)/include \
	-I$(srcdir)/third_party/c-list/src \
	$(NULL)

default_cppflags = \
	$(extra_cppflags) \
	$(warn_cppflags) \
	$(defines_cppflags) \
	$(default_includes) \
	$(NULL)

###############################################################################

lib_cppflags = \
	$(default_cppflags) \
	$(NULL)

###############################################################################

# Hack to avoid using ylwrap. It does not function correctly in combination
# with --header-file=

lib/route/pktloc_grammar.h: lib/route/pktloc_grammar.c
	@true

lib/route/pktloc_grammar.c: lib/route/pktloc_grammar.l lib/route/.dirstamp
	$(AM_V_GEN) $(FLEX) --header-file=lib/route/pktloc_grammar.h $(LFLAGS) -o $@ $<

lib/route/pktloc_syntax.h: lib/route/pktloc_syntax.c
	@true

lib/route/pktloc_syntax.c: lib/route/pktloc_syntax.y lib/route/.dirstamp
	$(AM_V_GEN) $(YACC) -d $(YFLAGS) -o $@ $<

lib/route/cls/ematch_grammar.h: lib/route/cls/ematch_grammar.c
	@true

lib/route/cls/ematch_grammar.c: lib/route/cls/ematch_grammar.l lib/route/cls/.dirstamp
	$(AM_V_GEN) $(FLEX) --header-file=lib/route/cls/ematch_grammar.h $(LFLAGS) -o $@ $<

lib/route/cls/ematch_syntax.h: lib/route/cls/ematch_syntax.c
	@true

lib/route/cls/ematch_syntax.c: lib/route/cls/ematch_syntax.y lib/route/cls/.dirstamp
	$(AM_V_GEN) $(YACC) -d $(YFLAGS) -o $@ $<

grammar_files_sources = \
	lib/route/cls/ematch_grammar.c \
	lib/route/cls/ematch_syntax.c \
	lib/route/pktloc_grammar.c \
	lib/route/pktloc_syntax.c \
	$(NULL)

grammar_files_headers = $(grammar_files_sources:%.c=%.h)

noinst_LTLIBRARIES += lib/route/libnl-grammar.la

nodist_lib_route_libnl_grammar_la_SOURCES = \
	$(grammar_files_sources)
lib_route_libnl_grammar_la_CPPFLAGS = \
	$(lib_cppflags) \
	-I$(srcdir)/lib/route \
	-Wno-unused-but-set-variable \
	$(NULL)

$(lib_route_libnl_grammar_la_OBJECTS): $(grammar_files_headers)

CLEANFILES += \
	$(grammar_files_sources) \
	$(grammar_files_headers)

EXTRA_DIST += \
	lib/route/cls/ematch_grammar.l \
	lib/route/cls/ematch_syntax.y \
	lib/route/pktloc_grammar.l \
	lib/route/pktloc_syntax.y \
	$(NULL)

###############################################################################

lib_LTLIBRARIES += lib/libnl-3.la

lib_libnl_3_la_SOURCES = \
	lib/addr.c \
	lib/attr.c \
	lib/cache.c \
	lib/cache_mngr.c \
	lib/cache_mngt.c \
	lib/data.c \
	lib/error.c \
	lib/handlers.c \
	lib/hash.c \
	lib/hashtable-api.h \
	lib/hashtable.c \
	lib/mpls.c \
	lib/mpls.h \
	lib/msg.c \
	lib/nl-core.h \
	lib/nl.c \
	lib/object.c \
	lib/socket.c \
	lib/utils.c \
	lib/version.c \
	$(NULL)
EXTRA_lib_libnl_3_la_DEPENDENCIES = \
	libnl-3.sym
lib_libnl_3_la_CPPFLAGS = \
	$(lib_cppflags)
lib_libnl_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-Wl,--version-script=$(srcdir)/libnl-3.sym

lib_LTLIBRARIES += lib/libnl-route-3.la

lib_libnl_route_3_la_SOURCES = \
	lib/fib_lookup/lookup.c \
	lib/fib_lookup/request.c \
	lib/route/act.c \
	lib/route/act/gact.c \
	lib/route/act/mirred.c \
	lib/route/act/nat.c \
	lib/route/act/skbedit.c \
	lib/route/act/vlan.c \
	lib/route/addr.c \
	lib/route/class.c \
	lib/route/classid.c \
	lib/route/cls.c \
	lib/route/cls/basic.c \
	lib/route/cls/cgroup.c \
	lib/route/cls/ematch.c \
	lib/route/cls/ematch/cmp.c \
	lib/route/cls/ematch/container.c \
	lib/route/cls/ematch/meta.c \
	lib/route/cls/ematch/nbyte.c \
	lib/route/cls/ematch/text.c \
	lib/route/cls/flower.c \
	lib/route/cls/fw.c \
	lib/route/cls/mall.c \
	lib/route/cls/police.c \
	lib/route/cls/u32.c \
	lib/route/link-sriov.h \
	lib/route/link.c \
	lib/route/link/api.c \
	lib/route/link/bonding.c \
	lib/route/link/bridge.c \
	lib/route/link/bridge_info.c \
	lib/route/link/can.c \
	lib/route/link/dummy.c \
	lib/route/link/geneve.c \
	lib/route/link/ifb.c \
	lib/route/link/inet.c \
	lib/route/link/inet6.c \
	lib/route/link/ip6gre.c \
	lib/route/link/ip6tnl.c \
	lib/route/link/ip6vti.c \
	lib/route/link/ipgre.c \
	lib/route/link/ipip.c \
	lib/route/link/ipvlan.c \
	lib/route/link/ipvti.c \
	lib/route/link/link-api.h \
	lib/route/link/macsec.c \
	lib/route/link/macvlan.c \
	lib/route/link/ppp.c \
	lib/route/link/sit.c \
	lib/route/link/sriov.c \
	lib/route/link/team.c \
	lib/route/link/veth.c \
	lib/route/link/vlan.c \
	lib/route/link/vrf.c \
	lib/route/link/vxlan.c \
	lib/route/link/xfrmi.c \
	lib/route/mdb.c \
	lib/route/neigh.c \
	lib/route/neightbl.c \
	lib/route/netconf.c \
	lib/route/nexthop-encap.h \
	lib/route/nexthop.c \
	lib/route/nexthop_encap.c \
	lib/route/nh.c \
	lib/route/nh_encap_ila.c \
	lib/route/nh_encap_ip.c \
	lib/route/nh_encap_ip6.c \
	lib/route/nh_encap_mpls.c \
	lib/route/nl-route.h \
	lib/route/pktloc.c \
	lib/route/qdisc.c \
	lib/route/qdisc/blackhole.c \
	lib/route/qdisc/cbq.c \
	lib/route/qdisc/dsmark.c \
	lib/route/qdisc/fifo.c \
	lib/route/qdisc/fq_codel.c \
	lib/route/qdisc/hfsc.c \
	lib/route/qdisc/htb.c \
	lib/route/qdisc/ingress.c \
	lib/route/qdisc/mqprio.c \
	lib/route/qdisc/netem.c \
	lib/route/qdisc/plug.c \
	lib/route/qdisc/prio.c \
	lib/route/qdisc/red.c \
	lib/route/qdisc/sfq.c \
	lib/route/qdisc/tbf.c \
	lib/route/route.c \
	lib/route/route_obj.c \
	lib/route/route_utils.c \
	lib/route/rtnl.c \
	lib/route/rule.c \
	lib/route/tc-api.h \
	lib/route/tc.c \
	$(NULL)
EXTRA_lib_libnl_route_3_la_DEPENDENCIES = \
	libnl-route-3.sym
lib_libnl_route_3_la_CPPFLAGS = \
	$(lib_cppflags) \
	-I$(srcdir)/lib/route \
	-I$(builddir)/lib/route \
	-I$(builddir)/lib/route/cls \
	$(NULL)
lib_libnl_route_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-Wl,--version-script=$(srcdir)/libnl-route-3.sym
lib_libnl_route_3_la_LIBADD = \
	lib/libnl-3.la \
	lib/route/libnl-grammar.la \
	$(NULL)

$(lib_libnl_route_3_la_OBJECTS): $(grammar_files_headers)

lib_LTLIBRARIES += lib/libnl-idiag-3.la

lib_libnl_idiag_3_la_SOURCES = \
	lib/idiag/idiag.c \
	lib/idiag/idiag_meminfo_obj.c \
	lib/idiag/idiag_msg_obj.c \
	lib/idiag/idiag_req_obj.c \
	lib/idiag/idiag_vegasinfo_obj.c \
	lib/idiag/nl-idiag.h \
	$(NULL)
EXTRA_lib_libnl_idiag_3_la_DEPENDENCIES = \
	libnl-idiag-3.sym
lib_libnl_idiag_3_la_CPPFLAGS = \
	$(lib_cppflags)
lib_libnl_idiag_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-Wl,--version-script=$(srcdir)/libnl-idiag-3.sym
lib_libnl_idiag_3_la_LIBADD = \
	lib/libnl-3.la

lib_LTLIBRARIES += lib/libnl-genl-3.la

lib_libnl_genl_3_la_SOURCES = \
	lib/genl/ctrl.c \
	lib/genl/family.c \
	lib/genl/genl.c \
	lib/genl/mngt.c \
	lib/genl/nl-genl.h \
	$(NULL)
EXTRA_lib_libnl_genl_3_la_DEPENDENCIES = \
	libnl-genl-3.sym
lib_libnl_genl_3_la_CPPFLAGS = \
	$(lib_cppflags)
lib_libnl_genl_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-Wl,--version-script=$(srcdir)/libnl-genl-3.sym
lib_libnl_genl_3_la_LIBADD = \
	lib/libnl-3.la

lib_LTLIBRARIES += lib/libnl-nf-3.la

lib_libnl_nf_3_la_SOURCES = \
	lib/netfilter/ct.c \
	lib/netfilter/ct_obj.c \
	lib/netfilter/exp.c \
	lib/netfilter/exp_obj.c \
	lib/netfilter/log.c \
	lib/netfilter/log_msg.c \
	lib/netfilter/log_msg_obj.c \
	lib/netfilter/log_obj.c \
	lib/netfilter/netfilter.c \
	lib/netfilter/nfnl.c \
	lib/netfilter/nl-netfilter.h \
	lib/netfilter/queue.c \
	lib/netfilter/queue_msg.c \
	lib/netfilter/queue_msg_obj.c \
	lib/netfilter/queue_obj.c \
	$(NULL)
lib_libnl_nf_3_la_CPPFLAGS = \
	$(lib_cppflags)
EXTRA_lib_libnl_nf_3_la_DEPENDENCIES = \
	libnl-nf-3.sym
lib_libnl_nf_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-Wl,--version-script=$(srcdir)/libnl-nf-3.sym
lib_libnl_nf_3_la_LIBADD  = \
	lib/libnl-3.la \
	lib/libnl-route-3.la

lib_LTLIBRARIES += lib/libnl-xfrm-3.la

lib_libnl_xfrm_3_la_SOURCES = \
	lib/xfrm/ae.c \
	lib/xfrm/lifetime.c \
	lib/xfrm/nl-xfrm.h \
	lib/xfrm/sa.c \
	lib/xfrm/selector.c \
	lib/xfrm/sp.c \
	lib/xfrm/template.c \
	$(NULL)
lib_libnl_xfrm_3_la_CPPFLAGS = \
	$(lib_cppflags)
lib_libnl_xfrm_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-Wl,--version-script=$(srcdir)/libnl-xfrm-3.sym
EXTRA_lib_libnl_xfrm_3_la_DEPENDENCIES = \
	libnl-xfrm-3.sym
lib_libnl_xfrm_3_la_LIBADD = \
	lib/libnl-3.la

lib_cli_ltlibraries_cls = \
	lib/cli/cls/basic.la \
	lib/cli/cls/cgroup.la \
	$(NULL)
lib_cli_ltlibraries_qdisc = \
	lib/cli/qdisc/bfifo.la \
	lib/cli/qdisc/blackhole.la \
	lib/cli/qdisc/fq_codel.la \
	lib/cli/qdisc/hfsc.la \
	lib/cli/qdisc/htb.la \
	lib/cli/qdisc/ingress.la \
	lib/cli/qdisc/pfifo.la \
	lib/cli/qdisc/plug.la \
	$(NULL)

if ENABLE_CLI
pkglib_clsdir = $(pkglibdir)/cli/cls
pkglib_qdiscdir = $(pkglibdir)/cli/qdisc
pkglib_cls_LTLIBRARIES = $(lib_cli_ltlibraries_cls)
pkglib_qdisc_LTLIBRARIES = $(lib_cli_ltlibraries_qdisc)
else
check_LTLIBRARIES += \
	$(lib_cli_ltlibraries_cls) \
	$(lib_cli_ltlibraries_qdisc)
endif

lib_cli_ldflags = \
	-module -avoid-version

lib_cli_cls_basic_la_CPPFLAGS       = $(lib_cppflags)
lib_cli_cls_basic_la_LDFLAGS        = $(lib_cli_ldflags)
lib_cli_cls_cgroup_la_CPPFLAGS      = $(lib_cppflags)
lib_cli_cls_cgroup_la_LDFLAGS       = $(lib_cli_ldflags)
lib_cli_qdisc_bfifo_la_CPPFLAGS     = $(lib_cppflags)
lib_cli_qdisc_bfifo_la_LDFLAGS      = $(lib_cli_ldflags)
lib_cli_qdisc_blackhole_la_CPPFLAGS = $(lib_cppflags)
lib_cli_qdisc_blackhole_la_LDFLAGS  = $(lib_cli_ldflags)
lib_cli_qdisc_fq_codel_la_CPPFLAGS  = $(lib_cppflags)
lib_cli_qdisc_fq_codel_la_LDFLAGS   = $(lib_cli_ldflags)
lib_cli_qdisc_hfsc_la_CPPFLAGS      = $(lib_cppflags)
lib_cli_qdisc_hfsc_la_LDFLAGS       = $(lib_cli_ldflags)
lib_cli_qdisc_htb_la_CPPFLAGS       = $(lib_cppflags)
lib_cli_qdisc_htb_la_LDFLAGS        = $(lib_cli_ldflags)
lib_cli_qdisc_ingress_la_CPPFLAGS   = $(lib_cppflags)
lib_cli_qdisc_ingress_la_LDFLAGS    = $(lib_cli_ldflags)
lib_cli_qdisc_pfifo_la_CPPFLAGS     = $(lib_cppflags)
lib_cli_qdisc_pfifo_la_LDFLAGS      = $(lib_cli_ldflags)
lib_cli_qdisc_plug_la_CPPFLAGS      = $(lib_cppflags)
lib_cli_qdisc_plug_la_LDFLAGS       = $(lib_cli_ldflags)

###############################################################################

src_lib_ldflags =

if ENABLE_CLI
lib_LTLIBRARIES += src/lib/libnl-cli-3.la
src_lib_ldflags += -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
else
check_LTLIBRARIES += src/lib/libnl-cli-3.la
endif

src_lib_libnl_cli_3_la_SOURCES = \
	src/lib/addr.c \
	src/lib/class.c \
	src/lib/cls.c \
	src/lib/ct.c \
	src/lib/exp.c \
	src/lib/link.c \
	src/lib/neigh.c \
	src/lib/nh.c \
	src/lib/qdisc.c \
	src/lib/route.c \
	src/lib/rule.c \
	src/lib/tc.c \
	src/lib/utils.c \
	$(NULL)
EXTRA_src_lib_libnl_cli_3_la_DEPENDENCIES = \
	libnl-cli-3.sym
src_lib_libnl_cli_3_la_CPPFLAGS = \
	$(default_cppflags) \
	$(NULL)
src_lib_libnl_cli_3_la_LDFLAGS = \
	$(src_lib_ldflags) \
	-Wl,--version-script=$(srcdir)/libnl-cli-3.sym
src_lib_libnl_cli_3_la_LIBADD = \
	lib/libnl-3.la \
	lib/libnl-route-3.la \
	lib/libnl-nf-3.la \
	lib/libnl-genl-3.la \
	-ldl \
	$(NULL)

###############################################################################

src_cppflags = \
	$(default_cppflags) \
	$(NULL)

src_ldadd = \
	src/lib/libnl-cli-3.la \
	lib/libnl-3.la \
	lib/libnl-nf-3.la \
	lib/libnl-genl-3.la \
	lib/libnl-route-3.la \
	lib/libnl-idiag-3.la \
	$(NULL)

cli_programs = \
	src/genl-ctrl-list \
	src/idiag-socket-details \
	src/nf-ct-add \
	src/nf-ct-events \
	src/nf-ct-list \
	src/nf-exp-add \
	src/nf-exp-delete \
	src/nf-exp-list \
	src/nf-log \
	src/nf-monitor \
	src/nf-queue \
	src/nl-addr-add \
	src/nl-addr-delete \
	src/nl-addr-list \
	src/nl-class-add \
	src/nl-class-delete \
	src/nl-class-list \
	src/nl-classid-lookup \
	src/nl-cls-add \
	src/nl-cls-delete \
	src/nl-cls-list \
	src/nl-fib-lookup \
	src/nl-link-enslave \
	src/nl-link-ifindex2name \
	src/nl-link-list \
	src/nl-link-name2ifindex \
	src/nl-link-release \
	src/nl-link-set \
	src/nl-link-stats \
	src/nl-list-caches \
	src/nl-list-sockets \
	src/nl-monitor \
	src/nl-neigh-add \
	src/nl-neigh-delete \
	src/nl-neigh-list \
	src/nl-neightbl-list \
	src/nl-nh-list \
	src/nl-pktloc-lookup \
	src/nl-qdisc-add \
	src/nl-qdisc-delete \
	src/nl-qdisc-list \
	src/nl-route-add \
	src/nl-route-delete \
	src/nl-route-get \
	src/nl-route-list \
	src/nl-rule-list \
	src/nl-tctree-list \
	src/nl-util-addr \
	$(NULL)

if ENABLE_CLI
if ENABLE_CLI_INSTALL_BIN
bin_PROGRAMS += $(cli_programs)
else
if ENABLE_CLI_INSTALL_SBIN
sbin_PROGRAMS += $(cli_programs)
else
noinst_PROGRAMS += $(cli_programs)
endif
endif
else
check_PROGRAMS += $(cli_programs)
endif

src_genl_ctrl_list_CPPFLAGS =       $(src_cppflags)
src_genl_ctrl_list_LDADD =          $(src_ldadd)
src_idiag_socket_details_CPPFLAGS = $(src_cppflags)
src_idiag_socket_details_LDADD =    $(src_ldadd)
src_nf_ct_add_CPPFLAGS =            $(src_cppflags)
src_nf_ct_add_LDADD =               $(src_ldadd)
src_nf_ct_events_CPPFLAGS =         $(src_cppflags)
src_nf_ct_events_LDADD =            $(src_ldadd)
src_nf_ct_list_CPPFLAGS =           $(src_cppflags)
src_nf_ct_list_LDADD =              $(src_ldadd)
src_nf_exp_add_CPPFLAGS =           $(src_cppflags)
src_nf_exp_add_LDADD =              $(src_ldadd)
src_nf_exp_delete_CPPFLAGS =        $(src_cppflags)
src_nf_exp_delete_LDADD =           $(src_ldadd)
src_nf_exp_list_CPPFLAGS =          $(src_cppflags)
src_nf_exp_list_LDADD =             $(src_ldadd)
src_nf_log_CPPFLAGS =               $(src_cppflags)
src_nf_log_LDADD =                  $(src_ldadd)
src_nf_monitor_CPPFLAGS =           $(src_cppflags)
src_nf_monitor_LDADD =              $(src_ldadd)
src_nf_queue_CPPFLAGS =             $(src_cppflags)
src_nf_queue_LDADD =                $(src_ldadd)
src_nl_addr_add_CPPFLAGS =          $(src_cppflags)
src_nl_addr_add_LDADD =             $(src_ldadd)
src_nl_addr_delete_CPPFLAGS =       $(src_cppflags)
src_nl_addr_delete_LDADD =          $(src_ldadd)
src_nl_addr_list_CPPFLAGS =         $(src_cppflags)
src_nl_addr_list_LDADD =            $(src_ldadd)
src_nl_class_add_CPPFLAGS =         $(src_cppflags)
src_nl_class_add_LDADD =            $(src_ldadd)
src_nl_class_delete_CPPFLAGS =      $(src_cppflags)
src_nl_class_delete_LDADD =         $(src_ldadd)
src_nl_class_list_CPPFLAGS =        $(src_cppflags)
src_nl_class_list_LDADD =           $(src_ldadd)
src_nl_classid_lookup_CPPFLAGS =    $(src_cppflags)
src_nl_classid_lookup_LDADD =       $(src_ldadd)
src_nl_cls_add_CPPFLAGS =           $(src_cppflags)
src_nl_cls_add_LDADD =              $(src_ldadd)
src_nl_cls_delete_CPPFLAGS =        $(src_cppflags)
src_nl_cls_delete_LDADD =           $(src_ldadd)
src_nl_cls_list_CPPFLAGS =          $(src_cppflags)
src_nl_cls_list_LDADD =             $(src_ldadd)
src_nl_fib_lookup_CPPFLAGS =        $(src_cppflags)
src_nl_fib_lookup_LDADD =           $(src_ldadd)
src_nl_link_enslave_CPPFLAGS =      $(src_cppflags)
src_nl_link_enslave_LDADD =         $(src_ldadd)
src_nl_link_ifindex2name_CPPFLAGS = $(src_cppflags)
src_nl_link_ifindex2name_LDADD =    $(src_ldadd)
src_nl_link_list_CPPFLAGS =         $(src_cppflags)
src_nl_link_list_LDADD =            $(src_ldadd)
src_nl_link_name2ifindex_CPPFLAGS = $(src_cppflags)
src_nl_link_name2ifindex_LDADD =    $(src_ldadd)
src_nl_link_release_CPPFLAGS =      $(src_cppflags)
src_nl_link_release_LDADD =         $(src_ldadd)
src_nl_link_set_CPPFLAGS =          $(src_cppflags)
src_nl_link_set_LDADD =             $(src_ldadd)
src_nl_link_stats_CPPFLAGS =        $(src_cppflags)
src_nl_link_stats_LDADD =           $(src_ldadd)
src_nl_list_caches_CPPFLAGS =       $(src_cppflags)
src_nl_list_caches_LDADD =          $(src_ldadd)
src_nl_list_sockets_CPPFLAGS =      $(src_cppflags)
src_nl_list_sockets_LDADD =         $(src_ldadd)
src_nl_monitor_CPPFLAGS =           $(src_cppflags)
src_nl_monitor_LDADD =              $(src_ldadd)
src_nl_neigh_add_CPPFLAGS =         $(src_cppflags)
src_nl_neigh_add_LDADD =            $(src_ldadd)
src_nl_neigh_delete_CPPFLAGS =      $(src_cppflags)
src_nl_neigh_delete_LDADD =         $(src_ldadd)
src_nl_neigh_list_CPPFLAGS =        $(src_cppflags)
src_nl_neigh_list_LDADD =           $(src_ldadd)
src_nl_neightbl_list_CPPFLAGS =     $(src_cppflags)
src_nl_neightbl_list_LDADD =        $(src_ldadd)
src_nl_nh_list_CPPFLAGS =           $(src_cppflags)
src_nl_nh_list_LDADD =              $(src_ldadd)
src_nl_pktloc_lookup_CPPFLAGS =     $(src_cppflags)
src_nl_pktloc_lookup_LDADD =        $(src_ldadd)
src_nl_qdisc_add_CPPFLAGS =         $(src_cppflags)
src_nl_qdisc_add_LDADD =            $(src_ldadd)
src_nl_qdisc_delete_CPPFLAGS =      $(src_cppflags)
src_nl_qdisc_delete_LDADD =         $(src_ldadd)
src_nl_qdisc_list_CPPFLAGS =        $(src_cppflags)
src_nl_qdisc_list_LDADD =           $(src_ldadd)
src_nl_route_add_CPPFLAGS =         $(src_cppflags)
src_nl_route_add_LDADD =            $(src_ldadd)
src_nl_route_delete_CPPFLAGS =      $(src_cppflags)
src_nl_route_delete_LDADD =         $(src_ldadd)
src_nl_route_get_CPPFLAGS =         $(src_cppflags)
src_nl_route_get_LDADD =            $(src_ldadd)
src_nl_route_list_CPPFLAGS =        $(src_cppflags)
src_nl_route_list_LDADD =           $(src_ldadd)
src_nl_rule_list_CPPFLAGS =         $(src_cppflags)
src_nl_rule_list_LDADD =            $(src_ldadd)
src_nl_tctree_list_CPPFLAGS =       $(src_cppflags)
src_nl_tctree_list_LDADD =          $(src_ldadd)
src_nl_util_addr_CPPFLAGS =         $(src_cppflags)
src_nl_util_addr_LDADD =            $(src_ldadd)

###############################################################################

tests_cppflags = \
	$(default_cppflags) \
	$(NULL)

tests_ldadd = \
	lib/libnl-3.la \
	lib/libnl-nf-3.la \
	lib/libnl-genl-3.la \
	lib/libnl-route-3.la \
	$(NULL)

if WITH_CHECK
check_LTLIBRARIES += tests/libnl-test-util.la
endif

tests_libnl_test_util_la_SOURCES = \
	tests/nl-test-util.h \
	tests/nl-test-util.c \
	$(NULL)
tests_libnl_test_util_la_CPPFLAGS = \
	$(tests_cppflags) \
	$(CHECK_CFLAGS) \
	$(NULL)
tests_libnl_test_util_la_LIBADD = \
	$(tests_ldadd) \
	$(CHECK_LIBS) \
	$(NULL)

check_PROGRAMS += \
	tests/test-complex-HTB-with-hash-filters \
	tests/test-create-bond \
	tests/test-create-bridge \
	tests/test-create-geneve \
	tests/test-create-ifb \
	tests/test-create-ipgre \
	tests/test-create-ipgretap \
	tests/test-create-ipip \
	tests/test-create-ipvlan \
	tests/test-create-ipvti \
	tests/test-create-macsec \
	tests/test-create-macvlan \
	tests/test-create-macvtap \
	tests/test-create-sit \
	tests/test-create-veth \
	tests/test-create-vlan \
	tests/test-create-vrf \
	tests/test-create-vxlan \
	tests/test-create-xfrmi \
	tests/test-delete-link \
	tests/test-loopback-up-down \
	tests/test-socket-creation \
	tests/test-u32-filter-with-actions \
	$(NULL)

tests_test_complex_HTB_with_hash_filters_CPPFLAGS = $(tests_cppflags)
tests_test_complex_HTB_with_hash_filters_LDADD    = $(tests_ldadd)
tests_test_create_bond_CPPFLAGS                   = $(tests_cppflags)
tests_test_create_bond_LDADD                      = $(tests_ldadd)
tests_test_create_bridge_CPPFLAGS                 = $(tests_cppflags)
tests_test_create_bridge_LDADD                    = $(tests_ldadd)
tests_test_create_geneve_CPPFLAGS                 = $(tests_cppflags)
tests_test_create_geneve_LDADD                    = $(tests_ldadd)
tests_test_create_ifb_CPPFLAGS                    = $(tests_cppflags)
tests_test_create_ifb_LDADD                       = $(tests_ldadd)
tests_test_create_ipgre_CPPFLAGS                  = $(tests_cppflags)
tests_test_create_ipgre_LDADD                     = $(tests_ldadd)
tests_test_create_ipgretap_CPPFLAGS               = $(tests_cppflags)
tests_test_create_ipgretap_LDADD                  = $(tests_ldadd)
tests_test_create_ipip_CPPFLAGS                   = $(tests_cppflags)
tests_test_create_ipip_LDADD                      = $(tests_ldadd)
tests_test_create_ipvlan_CPPFLAGS                 = $(tests_cppflags)
tests_test_create_ipvlan_LDADD                    = $(tests_ldadd)
tests_test_create_ipvti_CPPFLAGS                  = $(tests_cppflags)
tests_test_create_ipvti_LDADD                     = $(tests_ldadd)
tests_test_create_macsec_CPPFLAGS                 = $(tests_cppflags)
tests_test_create_macsec_LDADD                    = $(tests_ldadd)
tests_test_create_macvlan_CPPFLAGS                = $(tests_cppflags)
tests_test_create_macvlan_LDADD                   = $(tests_ldadd)
tests_test_create_macvtap_CPPFLAGS                = $(tests_cppflags)
tests_test_create_macvtap_LDADD                   = $(tests_ldadd)
tests_test_create_sit_CPPFLAGS                    = $(tests_cppflags)
tests_test_create_sit_LDADD                       = $(tests_ldadd)
tests_test_create_veth_CPPFLAGS                   = $(tests_cppflags)
tests_test_create_veth_LDADD                      = $(tests_ldadd)
tests_test_create_vlan_CPPFLAGS                   = $(tests_cppflags)
tests_test_create_vlan_LDADD                      = $(tests_ldadd)
tests_test_create_vrf_CPPFLAGS                    = $(tests_cppflags)
tests_test_create_vrf_LDADD                       = $(tests_ldadd)
tests_test_create_vxlan_CPPFLAGS                  = $(tests_cppflags)
tests_test_create_vxlan_LDADD                     = $(tests_ldadd)
tests_test_create_xfrmi_CPPFLAGS                  = $(tests_cppflags)
tests_test_create_xfrmi_LDADD                     = $(tests_ldadd)
tests_test_delete_link_CPPFLAGS                   = $(tests_cppflags)
tests_test_delete_link_LDADD                      = $(tests_ldadd)
tests_test_loopback_up_down_CPPFLAGS              = $(tests_cppflags)
tests_test_loopback_up_down_LDADD                 = $(tests_ldadd)
tests_test_socket_creation_CPPFLAGS               = $(tests_cppflags)
tests_test_socket_creation_LDADD                  = $(tests_ldadd)
tests_test_u32_filter_with_actions_CPPFLAGS       = $(tests_cppflags)
tests_test_u32_filter_with_actions_LDADD          = $(tests_ldadd)

check_PROGRAMS += \
	tests/test-cache-mngr \
	tests/test-genl \
	tests/test-nf-cache-mngr \
	$(NULL)

tests_cli_ldadd = \
	$(tests_ldadd) \
	src/lib/libnl-cli-3.la \
	$(NULL)

tests_test_cache_mngr_CPPFLAGS                    = $(tests_cppflags)
tests_test_cache_mngr_LDADD                       = $(tests_cli_ldadd)
tests_test_genl_CPPFLAGS                          = $(tests_cppflags)
tests_test_genl_LDADD                             = $(tests_cli_ldadd)
tests_test_nf_cache_mngr_CPPFLAGS                 = $(tests_cppflags)
tests_test_nf_cache_mngr_LDADD                    = $(tests_cli_ldadd)


if WITH_CHECK
check_programs += tests/check-all
endif

tests_check_all_SOURCES = \
	tests/check-all.c \
	tests/cksuite-all-addr.c \
	tests/cksuite-all-attr.c \
	tests/cksuite-all-ematch-tree-clone.c \
	tests/cksuite-all-netns.c \
	tests/cksuite-all.h \
	tests/cksuite-link-ip6tnl.c \
	tests/cksuite-route-nexthop.c \
	tests/cksuite-route-nh.c \
	$(NULL)

tests_check_all_CPPFLAGS = \
	$(tests_cppflags) \
	$(CHECK_CFLAGS)

tests_check_all_LDADD = \
	$(tests_ldadd) \
	tests/libnl-test-util.la \
	$(CHECK_LIBS) \
	$(NULL)

if WITH_CHECK
if ENABLE_STATIC
check_programs += tests/check-direct
endif
endif

tests_check_direct_SOURCES = \
	tests/check-direct.c \
	$(NULL)

tests_check_direct_CPPFLAGS = \
	$(tests_cppflags) \
	$(CHECK_CFLAGS) \
	$(NULL)

tests_check_direct_LDFLAGS = \
	-static \
	$(NULL)

tests_check_direct_LDADD = \
	$(tests_ldadd) \
	tests/libnl-test-util.la \
	$(CHECK_LIBS) \
	$(NULL)

###############################################################################

dist_man8_MANS = \
	man/genl-ctrl-list.8 \
	man/nl-classid-lookup.8 \
	man/nl-pktloc-lookup.8 \
	man/nl-qdisc-add.8 \
	man/nl-qdisc-delete.8 \
	man/nl-qdisc-list.8 \
	$(NULL)

###############################################################################

python-build:
	cd "$(srcdir)/python"; \
	python setup.py build

.PHONY: python-build

EXTRA_DIST += \
	python/README \
	\
	python/doc/conf.py \
	python/doc/core.rst \
	python/doc/index.rst \
	python/doc/route_addr.rst \
	python/doc/route.rst \
	\
	python/examples/iface.py \
	python/examples/nl80211.py \
	python/examples/wiphy.py \
	\
	python/netlink/capi.i \
	python/netlink/__init__.py \
	python/netlink/core.py \
	python/netlink/util.py \
	python/netlink/utils.h \
	\
	python/netlink/genl/capi.i \
	python/netlink/genl/__init__.py \
	\
	python/netlink/route/capi.i \
	python/netlink/route/__init__.py \
	python/netlink/route/address.py \
	python/netlink/route/link.py \
	python/netlink/route/tc.py \
	python/netlink/route/links/__init__.py \
	python/netlink/route/links/dummy.py \
	python/netlink/route/links/inet.py \
	python/netlink/route/links/vlan.py \
	python/netlink/route/qdisc/__init__.py \
	python/netlink/route/qdisc/htb.py \
	\
	python/tests/test-create-bridge.py

###############################################################################

%.build-headers-test.c: %
	mkdir -p "$(dir $@)"
	printf "#include <$$(echo "$<" | sed 's|.*\<include/netlink/|netlink/|')>\nint main(void) { return 0; }" > $@

%.build-headers-test.o: %.build-headers-test.c
	$(CC) -Wall -Werror -D_NL_NO_WARN_DEPRECATED_HEADER -I$(srcdir)/include -I$(builddir)/include -c -o $@ $<

BUILD_HEADERS_OBJS = $(patsubst %,%.build-headers-test.o,$(public_headers))

# Test whether the public headers are all self-contained and can be build.
# This test is not hooked up as `make check`.
check-build-headers: $(BUILD_HEADERS_OBJS)

CLEANFILES += $(BUILD_HEADERS_OBJS)

check_build += check-build-headers

###############################################################################

if HAS_CXX
%.build-headers-test-cxx.cpp: %
	mkdir -p "$(dir $@)"
	printf "#include <cstdio>\n#include <$$(echo "$<" | sed 's|.*\<include/netlink/|netlink/|')>\nint main(void) { return 0; }" > $@

%.build-headers-test-cxx.o: %.build-headers-test-cxx.cpp %.build-headers-test.o
	$(CXX) -Wall -Werror -D_NL_NO_WARN_DEPRECATED_HEADER -I$(srcdir)/include -I$(builddir)/include -c -o $@ $<

BUILD_HEADERS_OBJS_CXX = $(patsubst %,%.build-headers-test-cxx.o,$(public_headers))

CLEANFILES += $(BUILD_HEADERS_OBJS_CXX)

check-build-headers-cxx: $(BUILD_HEADERS_OBJS_CXX)

check_build += check-build-headers-cxx
endif

###############################################################################

EXTRA_DIST += \
	third_party/c-list/AUTHORS \
	third_party/c-list/.editorconfig \
	third_party/c-list/.github/workflows/ci.yml \
	third_party/c-list/meson.build \
	third_party/c-list/NEWS.md \
	third_party/c-list/README.md \
	third_party/c-list/src/c-list.h \
	third_party/c-list/src/meson.build \
	third_party/c-list/src/test-api.c \
	third_party/c-list/src/test-basic.c \
	third_party/c-list/src/test-embed.c \
	$(NULL)

check-local-c-list:
	"$(srcdir)/tools/check-c-list.sh"

check_local += check-local-c-list

clean-local-c-list:
	rm -rf ./build-c-list/

clean_local += clean-local-c-list

EXTRA_DIST += tools/check-c-list.sh

###############################################################################

clean-local: $(clean_local)

check-local: $(check_build) $(check_local)

.PHONY: $(check_local)

check_PROGRAMS += $(check_programs)
TESTS += $(check_programs)

if ENABLE_CLI
pkgconfig_DATA += libnl-cli-3.0.pc
endif

pkgsysconfdir = $(sysconfdir)/libnl
pkgsysconf_DATA = \
	etc/pktloc \
	etc/classid

EXTRA_DIST += \
	$(pkgsysconf_DATA) \
	libnl-3.sym \
	libnl-cli-3.sym \
	libnl-genl-3.sym \
	libnl-idiag-3.sym \
	libnl-nf-3.sym \
	libnl-route-3.sym \
	libnl-xfrm-3.sym \
	$(NULL)

###############################################################################

check-build: all $(check_PROGRAMS) $(check_LTLIBRARIES) $(check_build)

check-progs: check-build

.PHONY: check-progs check-build

###############################################################################
