# -*- Makefile -*-

AM_CPPFLAGS  = -Wall -I${top_srcdir}/include -I${top_builddir}/include -D_GNU_SOURCE -DSYSCONFDIR=\"$(sysconfdir)/libnl\"

lib_LTLIBRARIES = \
	libnl.la libnl-genl.la libnl-route.la libnl-nf.la

libnl_la_LDFLAGS = -version-info 2:0:0
libnl_la_SOURCES = \
	addr.c attr.c cache.c cache_mngr.c cache_mngt.c data.c doc.c \
	error.c handlers.c msg.c nl.c object.c socket.c utils.c

libnl_genl_la_LDFLAGS = -version-info 2:0:0
libnl_genl_la_LIBADD  = libnl.la
libnl_genl_la_SOURCES = \
	genl/ctrl.c genl/family.c genl/genl.c genl/mngt.c

libnl_nf_la_LDFLAGS = -version-info 2:0:0
libnl_nf_la_LIBADD  = libnl-route.la
libnl_nf_la_SOURCES = \
	netfilter/ct.c netfilter/ct_obj.c netfilter/log.c \
	netfilter/log_msg.c netfilter/log_msg_obj.c netfilter/log_obj.c \
	netfilter/netfilter.c netfilter/nfnl.c netfilter/queue.c \
	netfilter/queue_msg.c netfilter/queue_msg_obj.c netfilter/queue_obj.c

CLEANFILES = \
	route/pktloc_grammar.c route/pktloc_grammar.h \
	route/pktloc_syntax.c route/pktloc_syntax.h

# Hack to avoid using ylwrap. It does not function correctly in combination
# with --header-file=
route/pktloc_grammar.c: route/pktloc_grammar.l
	$(LEX) --header-file=route/pktloc_grammar.h $(LFLAGS) -o $@ $^

route/pktloc_syntax.c: route/pktloc_syntax.y
	$(YACC) -d $(YFLAGS) -o $@ $^

libnl_route_la_LDFLAGS = -version-info 2:0:0
libnl_route_la_LIBADD  = libnl.la
libnl_route_la_SOURCES = \
	route/addr.c route/class.c route/class_api.c route/class_obj.c \
	route/cls.c route/cls_api.c route/cls_obj.c route/link.c \
	route/neigh.c route/neightbl.c route/nexthop.c route/qdisc.c \
	route/qdisc_api.c route/qdisc_obj.c route/route.c route/route_obj.c \
	route/route_utils.c route/rtnl.c route/rule.c route/tc.c \
	\
	route/cls/fw.c route/cls/police.c route/cls/u32.c \
	\
	route/link/api.c route/link/vlan.c \
	\
	route/sch/blackhole.c route/sch/cbq.c route/sch/dsmark.c \
	route/sch/fifo.c route/sch/htb.c route/sch/netem.c route/sch/prio.c \
	route/sch/red.c route/sch/sfq.c route/sch/tbf.c \
	\
	fib_lookup/lookup.c fib_lookup/request.c \
	\
	route/pktloc_syntax.c route/pktloc_grammar.c route/pktloc.c
