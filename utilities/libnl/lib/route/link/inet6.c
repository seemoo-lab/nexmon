/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2010 Thomas Graf <tgraf@suug.ch>
 */

#include "nl-default.h"

#include <linux/ipv6.h>
#include <linux/snmp.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/inet6.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-priv-static-route/nl-priv-static-route.h"

#define I6_ADDR_GEN_MODE_UNKNOWN	UINT8_MAX

struct inet6_data
{
	uint32_t		i6_flags;
	struct ifla_cacheinfo	i6_cacheinfo;
	uint32_t		i6_conf[DEVCONF_MAX];
	struct in6_addr		i6_token;
	uint8_t			i6_conf_len;
	uint8_t			i6_addr_gen_mode;
};

static void *inet6_alloc(struct rtnl_link *link)
{
	struct inet6_data *i6;

	i6 = calloc(1, sizeof(struct inet6_data));
	if (i6)
		i6->i6_addr_gen_mode = I6_ADDR_GEN_MODE_UNKNOWN;

	return i6;
}

static void *inet6_clone(struct rtnl_link *link, void *data)
{
	struct inet6_data *i6;

	if ((i6 = inet6_alloc(link)))
		memcpy(i6, data, sizeof(*i6));

	return i6;
}

static void inet6_free(struct rtnl_link *link, void *data)
{
	free(data);
}

static struct nla_policy inet6_policy[IFLA_INET6_MAX+1] = {
	[IFLA_INET6_FLAGS]		= { .type = NLA_U32 },
	[IFLA_INET6_CACHEINFO]		= { .minlen = sizeof(struct ifla_cacheinfo) },
	[IFLA_INET6_CONF]		= { .minlen = 4 },
	[IFLA_INET6_STATS]		= { .minlen = 8 },
	[IFLA_INET6_ICMP6STATS]		= { .minlen = 8 },
	[IFLA_INET6_TOKEN]		= { .minlen = sizeof(struct in6_addr) },
	[IFLA_INET6_ADDR_GEN_MODE]	= { .type = NLA_U8 },
};

static const uint8_t map_stat_id_from_IPSTATS_MIB_v1[__IPSTATS_MIB_MAX] = {
	/* 14a196807482e6fc74f15fc03176d5c08880588f^:include/linux/snmp.h
	 * version before the API change in commit 14a196807482e6fc74f15fc03176d5c08880588f.
	 * This version was valid since commit edf391ff17232f097d72441c9ad467bcb3b5db18, which
	 * predates support for parsing IFLA_PROTINFO in libnl3. Such an even older meaning of
	 * the flags is not supported in libnl3. */
	[ 1] = RTNL_LINK_IP6_INPKTS,                    /* IPSTATS_MIB_INPKTS                   */
	[ 2] = RTNL_LINK_IP6_INHDRERRORS,               /* IPSTATS_MIB_INHDRERRORS              */
	[ 3] = RTNL_LINK_IP6_INTOOBIGERRORS,            /* IPSTATS_MIB_INTOOBIGERRORS           */
	[ 4] = RTNL_LINK_IP6_INNOROUTES,                /* IPSTATS_MIB_INNOROUTES               */
	[ 5] = RTNL_LINK_IP6_INADDRERRORS,              /* IPSTATS_MIB_INADDRERRORS             */
	[ 6] = RTNL_LINK_IP6_INUNKNOWNPROTOS,           /* IPSTATS_MIB_INUNKNOWNPROTOS          */
	[ 7] = RTNL_LINK_IP6_INTRUNCATEDPKTS,           /* IPSTATS_MIB_INTRUNCATEDPKTS          */
	[ 8] = RTNL_LINK_IP6_INDISCARDS,                /* IPSTATS_MIB_INDISCARDS               */
	[ 9] = RTNL_LINK_IP6_INDELIVERS,                /* IPSTATS_MIB_INDELIVERS               */
	[10] = RTNL_LINK_IP6_OUTFORWDATAGRAMS,          /* IPSTATS_MIB_OUTFORWDATAGRAMS         */
	[11] = RTNL_LINK_IP6_OUTPKTS,                   /* IPSTATS_MIB_OUTPKTS                  */
	[12] = RTNL_LINK_IP6_OUTDISCARDS,               /* IPSTATS_MIB_OUTDISCARDS              */
	[13] = RTNL_LINK_IP6_OUTNOROUTES,               /* IPSTATS_MIB_OUTNOROUTES              */
	[14] = RTNL_LINK_IP6_REASMTIMEOUT,              /* IPSTATS_MIB_REASMTIMEOUT             */
	[15] = RTNL_LINK_IP6_REASMREQDS,                /* IPSTATS_MIB_REASMREQDS               */
	[16] = RTNL_LINK_IP6_REASMOKS,                  /* IPSTATS_MIB_REASMOKS                 */
	[17] = RTNL_LINK_IP6_REASMFAILS,                /* IPSTATS_MIB_REASMFAILS               */
	[18] = RTNL_LINK_IP6_FRAGOKS,                   /* IPSTATS_MIB_FRAGOKS                  */
	[19] = RTNL_LINK_IP6_FRAGFAILS,                 /* IPSTATS_MIB_FRAGFAILS                */
	[20] = RTNL_LINK_IP6_FRAGCREATES,               /* IPSTATS_MIB_FRAGCREATES              */
	[21] = RTNL_LINK_IP6_INMCASTPKTS,               /* IPSTATS_MIB_INMCASTPKTS              */
	[22] = RTNL_LINK_IP6_OUTMCASTPKTS,              /* IPSTATS_MIB_OUTMCASTPKTS             */
	[23] = RTNL_LINK_IP6_INBCASTPKTS,               /* IPSTATS_MIB_INBCASTPKTS              */
	[24] = RTNL_LINK_IP6_OUTBCASTPKTS,              /* IPSTATS_MIB_OUTBCASTPKTS             */
	[25] = RTNL_LINK_IP6_INOCTETS,                  /* IPSTATS_MIB_INOCTETS                 */
	[26] = RTNL_LINK_IP6_OUTOCTETS,                 /* IPSTATS_MIB_OUTOCTETS                */
	[27] = RTNL_LINK_IP6_INMCASTOCTETS,             /* IPSTATS_MIB_INMCASTOCTETS            */
	[28] = RTNL_LINK_IP6_OUTMCASTOCTETS,            /* IPSTATS_MIB_OUTMCASTOCTETS           */
	[29] = RTNL_LINK_IP6_INBCASTOCTETS,             /* IPSTATS_MIB_INBCASTOCTETS            */
	[30] = RTNL_LINK_IP6_OUTBCASTOCTETS,            /* IPSTATS_MIB_OUTBCASTOCTETS           */
};

static const uint8_t map_stat_id_from_IPSTATS_MIB_v2[__IPSTATS_MIB_MAX] = {
	/* d8ec26d7f8287f5788a494f56e8814210f0e64be:include/uapi/linux/snmp.h
	 * version since the API change in commit 14a196807482e6fc74f15fc03176d5c08880588f */
	[ 1] = RTNL_LINK_IP6_INPKTS,                    /* IPSTATS_MIB_INPKTS                   */
	[ 2] = RTNL_LINK_IP6_INOCTETS,                  /* IPSTATS_MIB_INOCTETS                 */
	[ 3] = RTNL_LINK_IP6_INDELIVERS,                /* IPSTATS_MIB_INDELIVERS               */
	[ 4] = RTNL_LINK_IP6_OUTFORWDATAGRAMS,          /* IPSTATS_MIB_OUTFORWDATAGRAMS         */
	[ 5] = RTNL_LINK_IP6_OUTPKTS,                   /* IPSTATS_MIB_OUTPKTS                  */
	[ 6] = RTNL_LINK_IP6_OUTOCTETS,                 /* IPSTATS_MIB_OUTOCTETS                */
	[ 7] = RTNL_LINK_IP6_INHDRERRORS,               /* IPSTATS_MIB_INHDRERRORS              */
	[ 8] = RTNL_LINK_IP6_INTOOBIGERRORS,            /* IPSTATS_MIB_INTOOBIGERRORS           */
	[ 9] = RTNL_LINK_IP6_INNOROUTES,                /* IPSTATS_MIB_INNOROUTES               */
	[10] = RTNL_LINK_IP6_INADDRERRORS,              /* IPSTATS_MIB_INADDRERRORS             */
	[11] = RTNL_LINK_IP6_INUNKNOWNPROTOS,           /* IPSTATS_MIB_INUNKNOWNPROTOS          */
	[12] = RTNL_LINK_IP6_INTRUNCATEDPKTS,           /* IPSTATS_MIB_INTRUNCATEDPKTS          */
	[13] = RTNL_LINK_IP6_INDISCARDS,                /* IPSTATS_MIB_INDISCARDS               */
	[14] = RTNL_LINK_IP6_OUTDISCARDS,               /* IPSTATS_MIB_OUTDISCARDS              */
	[15] = RTNL_LINK_IP6_OUTNOROUTES,               /* IPSTATS_MIB_OUTNOROUTES              */
	[16] = RTNL_LINK_IP6_REASMTIMEOUT,              /* IPSTATS_MIB_REASMTIMEOUT             */
	[17] = RTNL_LINK_IP6_REASMREQDS,                /* IPSTATS_MIB_REASMREQDS               */
	[18] = RTNL_LINK_IP6_REASMOKS,                  /* IPSTATS_MIB_REASMOKS                 */
	[19] = RTNL_LINK_IP6_REASMFAILS,                /* IPSTATS_MIB_REASMFAILS               */
	[20] = RTNL_LINK_IP6_FRAGOKS,                   /* IPSTATS_MIB_FRAGOKS                  */
	[21] = RTNL_LINK_IP6_FRAGFAILS,                 /* IPSTATS_MIB_FRAGFAILS                */
	[22] = RTNL_LINK_IP6_FRAGCREATES,               /* IPSTATS_MIB_FRAGCREATES              */
	[23] = RTNL_LINK_IP6_INMCASTPKTS,               /* IPSTATS_MIB_INMCASTPKTS              */
	[24] = RTNL_LINK_IP6_OUTMCASTPKTS,              /* IPSTATS_MIB_OUTMCASTPKTS             */
	[25] = RTNL_LINK_IP6_INBCASTPKTS,               /* IPSTATS_MIB_INBCASTPKTS              */
	[26] = RTNL_LINK_IP6_OUTBCASTPKTS,              /* IPSTATS_MIB_OUTBCASTPKTS             */
	[27] = RTNL_LINK_IP6_INMCASTOCTETS,             /* IPSTATS_MIB_INMCASTOCTETS            */
	[28] = RTNL_LINK_IP6_OUTMCASTOCTETS,            /* IPSTATS_MIB_OUTMCASTOCTETS           */
	[29] = RTNL_LINK_IP6_INBCASTOCTETS,             /* IPSTATS_MIB_INBCASTOCTETS            */
	[30] = RTNL_LINK_IP6_OUTBCASTOCTETS,            /* IPSTATS_MIB_OUTBCASTOCTETS           */
	[31] = RTNL_LINK_IP6_CSUMERRORS,                /* IPSTATS_MIB_CSUMERRORS               */
	[32] = RTNL_LINK_IP6_NOECTPKTS,                 /* IPSTATS_MIB_NOECTPKTS                */
	[33] = RTNL_LINK_IP6_ECT1PKTS,                  /* IPSTATS_MIB_ECT1PKTS                 */
	[34] = RTNL_LINK_IP6_ECT0PKTS,                  /* IPSTATS_MIB_ECT0PKTS                 */
	[35] = RTNL_LINK_IP6_CEPKTS,                    /* IPSTATS_MIB_CEPKTS                   */
	[36] = RTNL_LINK_REASM_OVERLAPS,                /* IPSTATS_MIB_REASM_OVERLAPS           */
};

const uint8_t *const _nltst_map_stat_id_from_IPSTATS_MIB_v2 = map_stat_id_from_IPSTATS_MIB_v2;

static int inet6_parse_protinfo(struct rtnl_link *link, struct nlattr *attr,
				void *data)
{
	struct inet6_data *i6 = data;
	struct nlattr *tb[IFLA_INET6_MAX+1];
	int err;

	err = nla_parse_nested(tb, IFLA_INET6_MAX, attr, inet6_policy);
	if (err < 0)
		return err;
	if (tb[IFLA_INET6_CONF] && nla_len(tb[IFLA_INET6_CONF]) % 4)
		return -EINVAL;
	if (tb[IFLA_INET6_STATS] && nla_len(tb[IFLA_INET6_STATS]) % 8)
		return -EINVAL;
	if (tb[IFLA_INET6_ICMP6STATS] && nla_len(tb[IFLA_INET6_ICMP6STATS]) % 8)
		return -EINVAL;

	if (tb[IFLA_INET6_FLAGS])
		i6->i6_flags = nla_get_u32(tb[IFLA_INET6_FLAGS]);

	if (tb[IFLA_INET6_CACHEINFO])
		nla_memcpy(&i6->i6_cacheinfo, tb[IFLA_INET6_CACHEINFO],
			   sizeof(i6->i6_cacheinfo));

	if (tb[IFLA_INET6_CONF]) {
		i6->i6_conf_len = _NL_MIN(ARRAY_SIZE(i6->i6_conf),
				      nla_len(tb[IFLA_INET6_CONF]) /
					      sizeof(i6->i6_conf[0]));
		nla_memcpy(&i6->i6_conf, tb[IFLA_INET6_CONF],
			   sizeof(i6->i6_conf[0]) * i6->i6_conf_len);
	}

	if (tb[IFLA_INET6_TOKEN])
		nla_memcpy(&i6->i6_token, tb[IFLA_INET6_TOKEN],
		           sizeof(struct in6_addr));

	if (tb[IFLA_INET6_ADDR_GEN_MODE])
		i6->i6_addr_gen_mode = nla_get_u8 (tb[IFLA_INET6_ADDR_GEN_MODE]);

	/*
	 * Due to 32bit data alignment, these addresses must be copied to an
	 * aligned location prior to access.
	 */
	if (tb[IFLA_INET6_STATS]) {
		unsigned char *cnt = nla_data(tb[IFLA_INET6_STATS]);
		uint64_t stat;
		int i;
		int len = nla_len(tb[IFLA_INET6_STATS]) / 8;
		const uint8_t *map_stat_id = map_stat_id_from_IPSTATS_MIB_v2;

		if (len < 32 ||
		    (tb[IFLA_INET6_ICMP6STATS] && nla_len(tb[IFLA_INET6_ICMP6STATS]) < 6)) {
			/* kernel commit 14a196807482e6fc74f15fc03176d5c08880588f reordered the values.
			 * The later commit 6a5dc9e598fe90160fee7de098fa319665f5253e added values
			 * IPSTATS_MIB_CSUMERRORS/ICMP6_MIB_CSUMERRORS. If the netlink is shorter
			 * then this, assume that the kernel uses the previous meaning of the
			 * enumeration. */
			map_stat_id = map_stat_id_from_IPSTATS_MIB_v1;
		}

		len = _NL_MIN(__IPSTATS_MIB_MAX, len);
		for (i = 1; i < len; i++) {
			memcpy(&stat, &cnt[i * sizeof(stat)], sizeof(stat));
			rtnl_link_set_stat(link, map_stat_id[i], stat);
		}
	}

	if (tb[IFLA_INET6_ICMP6STATS]) {
#define _NL_ICMP6_MIB_MAX 6
		unsigned char *cnt = nla_data(tb[IFLA_INET6_ICMP6STATS]);
		uint64_t stat;
		int i;
		int len = _NL_MIN(_NL_ICMP6_MIB_MAX, nla_len(tb[IFLA_INET6_ICMP6STATS]) / 8);

		_NL_STATIC_ASSERT (__ICMP6_MIB_MAX >= _NL_ICMP6_MIB_MAX);
		_NL_STATIC_ASSERT (RTNL_LINK_ICMP6_CSUMERRORS - RTNL_LINK_ICMP6_INMSGS + 1 == 5);

		for (i = 1; i < len; i++) {
			memcpy(&stat, &cnt[i * sizeof(stat)], sizeof(stat));
			rtnl_link_set_stat(link, RTNL_LINK_ICMP6_INMSGS + i - 1,
					   stat);
		}
	}

	return 0;
}

static int inet6_fill_af(struct rtnl_link *link, struct nl_msg *msg, void *data)
{
	struct inet6_data *id = data;

	if (id->i6_addr_gen_mode != I6_ADDR_GEN_MODE_UNKNOWN)
		NLA_PUT_U8(msg, IFLA_INET6_ADDR_GEN_MODE, id->i6_addr_gen_mode);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

/* These live in include/net/if_inet6.h and should be moved to include/linux */
#define IF_RA_OTHERCONF	0x80
#define IF_RA_MANAGED	0x40
#define IF_RA_RCVD	0x20
#define IF_RS_SENT	0x10
#define IF_READY	0x80000000

static const struct trans_tbl inet6_flags[] = {
	__ADD(IF_RA_OTHERCONF, ra_otherconf),
	__ADD(IF_RA_MANAGED, ra_managed),
	__ADD(IF_RA_RCVD, ra_rcvd),
	__ADD(IF_RS_SENT, rs_sent),
	__ADD(IF_READY, ready),
};

char *rtnl_link_inet6_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str(flags, buf, len, inet6_flags,
			   ARRAY_SIZE(inet6_flags));
}

int rtnl_link_inet6_str2flags(const char *name)
{
	return __str2flags(name, inet6_flags, ARRAY_SIZE(inet6_flags));
}

static const struct trans_tbl inet6_devconf[] = {
	__ADD(DEVCONF_FORWARDING, forwarding),
	__ADD(DEVCONF_HOPLIMIT, hoplimit),
	__ADD(DEVCONF_MTU6, mtu6),
	__ADD(DEVCONF_ACCEPT_RA, accept_ra),
	__ADD(DEVCONF_ACCEPT_REDIRECTS, accept_redirects),
	__ADD(DEVCONF_AUTOCONF, autoconf),
	__ADD(DEVCONF_DAD_TRANSMITS, dad_transmits),
	__ADD(DEVCONF_RTR_SOLICITS, rtr_solicits),
	__ADD(DEVCONF_RTR_SOLICIT_INTERVAL, rtr_solicit_interval),
	__ADD(DEVCONF_RTR_SOLICIT_DELAY, rtr_solicit_delay),
	__ADD(DEVCONF_USE_TEMPADDR, use_tempaddr),
	__ADD(DEVCONF_TEMP_VALID_LFT, temp_valid_lft),
	__ADD(DEVCONF_TEMP_PREFERED_LFT, temp_prefered_lft),
	__ADD(DEVCONF_REGEN_MAX_RETRY, regen_max_retry),
	__ADD(DEVCONF_MAX_DESYNC_FACTOR, max_desync_factor),
	__ADD(DEVCONF_MAX_ADDRESSES, max_addresses),
	__ADD(DEVCONF_FORCE_MLD_VERSION, force_mld_version),
	__ADD(DEVCONF_ACCEPT_RA_DEFRTR, accept_ra_defrtr),
	__ADD(DEVCONF_ACCEPT_RA_PINFO, accept_ra_pinfo),
	__ADD(DEVCONF_ACCEPT_RA_RTR_PREF, accept_ra_rtr_pref),
	__ADD(DEVCONF_RTR_PROBE_INTERVAL, rtr_probe_interval),
	__ADD(DEVCONF_ACCEPT_RA_RT_INFO_MAX_PLEN, accept_ra_rt_info),
	__ADD(DEVCONF_PROXY_NDP, proxy_ndp),
	__ADD(DEVCONF_OPTIMISTIC_DAD, optimistic_dad),
	__ADD(DEVCONF_ACCEPT_SOURCE_ROUTE, accept_source_route),
	__ADD(DEVCONF_MC_FORWARDING, mc_forwarding),
	__ADD(DEVCONF_DISABLE_IPV6, disable_ipv6),
	__ADD(DEVCONF_ACCEPT_DAD, accept_dad),
	__ADD(DEVCONF_FORCE_TLLAO, force_tllao),
};

static char *inet6_devconf2str(int type, char *buf, size_t len)
{
	return __type2str(type, buf, len, inet6_devconf,
			  ARRAY_SIZE(inet6_devconf));
}

static const struct trans_tbl inet6_addr_gen_mode[] = {
	__ADD(IN6_ADDR_GEN_MODE_EUI64, eui64),
	__ADD(IN6_ADDR_GEN_MODE_NONE, none),
	__ADD(IN6_ADDR_GEN_MODE_STABLE_PRIVACY, stable_privacy),
};

const char *rtnl_link_inet6_addrgenmode2str(uint8_t mode, char *buf, size_t len)
{
	return __type2str(mode, buf, len, inet6_addr_gen_mode,
			  ARRAY_SIZE(inet6_addr_gen_mode));
}

uint8_t rtnl_link_inet6_str2addrgenmode(const char *mode)
{
	return (uint8_t) __str2type(mode, inet6_addr_gen_mode,
			            ARRAY_SIZE(inet6_addr_gen_mode));
}

static void inet6_dump_details(struct rtnl_link *link,
				struct nl_dump_params *p, void *data)
{
	struct inet6_data *i6 = data;
	struct nl_addr *addr;
	int i, n = 0;
	char buf[64];

	nl_dump_line(p, "    ipv6 max-reasm-len %s",
	             nl_size2str(i6->i6_cacheinfo.max_reasm_len, buf, sizeof(buf)));

	nl_dump(p, " <%s>\n",
	        rtnl_link_inet6_flags2str(i6->i6_flags, buf, sizeof(buf)));

	nl_dump_line(p, "      create-stamp %.2fs reachable-time %s",
	             (double) i6->i6_cacheinfo.tstamp / 100.,
	             nl_msec2str(i6->i6_cacheinfo.reachable_time, buf, sizeof(buf)));

	nl_dump(p, " retrans-time %s\n",
	        nl_msec2str(i6->i6_cacheinfo.retrans_time, buf, sizeof(buf)));

	addr = nl_addr_build(AF_INET6, &i6->i6_token, sizeof(i6->i6_token));
	nl_dump(p, "      token %s\n",
	        nl_addr2str(addr, buf, sizeof(buf)));
	nl_addr_put(addr);

	nl_dump(p, "      link-local address mode %s\n",
	        rtnl_link_inet6_addrgenmode2str(i6->i6_addr_gen_mode,
	                                        buf, sizeof(buf)));

	nl_dump_line(p, "      devconf:\n");
	nl_dump_line(p, "      ");

	for (i = 0; i < (int) i6->i6_conf_len; i++) {
		char buf2[64];
		uint32_t value = i6->i6_conf[i];
		int x, offset;

		switch (i) {
		case DEVCONF_TEMP_VALID_LFT:
		case DEVCONF_TEMP_PREFERED_LFT:
			nl_msec2str((uint64_t) value * 1000., buf2, sizeof(buf2));
			break;

		case DEVCONF_RTR_PROBE_INTERVAL:
		case DEVCONF_RTR_SOLICIT_INTERVAL:
		case DEVCONF_RTR_SOLICIT_DELAY:
			nl_msec2str(value, buf2, sizeof(buf2));
			break;

		default:
			snprintf(buf2, sizeof(buf2), "%u", value);
			break;
		}

		inet6_devconf2str(i, buf, sizeof(buf));

		offset = 23 - strlen(buf2);
		if (offset < 0)
			offset = 0;

		for (x = strlen(buf); x < offset; x++)
			buf[x] = ' ';

		_nl_strncpy_trunc(&buf[offset], buf2, sizeof(buf) - offset);

		nl_dump_line(p, "%s", buf);

		if (++n == 3) {
			nl_dump(p, "\n");
			nl_dump_line(p, "      ");
			n = 0;
		} else
			nl_dump(p, "  ");
	}

	if (n != 0)
		nl_dump(p, "\n");
}

static void inet6_dump_stats(struct rtnl_link *link,
			     struct nl_dump_params *p, void *data)
{
	double octets;
	char *octetsUnit;

	nl_dump(p, "    IPv6:       InPkts           InOctets     "
		   "    InDiscards         InDelivers\n");
	nl_dump(p, "    %18" PRIu64 " ", link->l_stats[RTNL_LINK_IP6_INPKTS]);

	octets = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_IP6_INOCTETS],
				      &octetsUnit);
	if (octets)
		nl_dump(p, "%14.2f %3s ", octets, octetsUnit);
	else
		nl_dump(p, "%16u B ", 0);
	
	nl_dump(p, "%18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_INDISCARDS],
		link->l_stats[RTNL_LINK_IP6_INDELIVERS]);

	nl_dump(p, "               OutPkts          OutOctets     "
		   "   OutDiscards        OutForwards\n");

	nl_dump(p, "    %18" PRIu64 " ", link->l_stats[RTNL_LINK_IP6_OUTPKTS]);

	octets = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_IP6_OUTOCTETS],
				      &octetsUnit);
	if (octets)
		nl_dump(p, "%14.2f %3s ", octets, octetsUnit);
	else
		nl_dump(p, "%16u B ", 0);

	nl_dump(p, "%18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_OUTDISCARDS],
		link->l_stats[RTNL_LINK_IP6_OUTFORWDATAGRAMS]);

	nl_dump(p, "           InMcastPkts      InMcastOctets     "
		   "   InBcastPkts     InBcastOctests\n");

	nl_dump(p, "    %18" PRIu64 " ", link->l_stats[RTNL_LINK_IP6_INMCASTPKTS]);

	octets = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_IP6_INMCASTOCTETS],
				      &octetsUnit);
	if (octets)
		nl_dump(p, "%14.2f %3s ", octets, octetsUnit);
	else
		nl_dump(p, "%16u B ", 0);

	nl_dump(p, "%18" PRIu64 " ", link->l_stats[RTNL_LINK_IP6_INBCASTPKTS]);
	octets = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_IP6_INBCASTOCTETS],
				      &octetsUnit);
	if (octets)
		nl_dump(p, "%14.2f %3s\n", octets, octetsUnit);
	else
		nl_dump(p, "%16u B\n", 0);

	nl_dump(p, "          OutMcastPkts     OutMcastOctets     "
		   "  OutBcastPkts    OutBcastOctests\n");

	nl_dump(p, "    %18" PRIu64 " ", link->l_stats[RTNL_LINK_IP6_OUTMCASTPKTS]);

	octets = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_IP6_OUTMCASTOCTETS],
				      &octetsUnit);
	if (octets)
		nl_dump(p, "%14.2f %3s ", octets, octetsUnit);
	else
		nl_dump(p, "%16u B ", 0);

	nl_dump(p, "%18" PRIu64 " ", link->l_stats[RTNL_LINK_IP6_OUTBCASTPKTS]);
	octets = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_IP6_OUTBCASTOCTETS],
				      &octetsUnit);
	if (octets)
		nl_dump(p, "%14.2f %3s\n", octets, octetsUnit);
	else
		nl_dump(p, "%16u B\n", 0);

	nl_dump(p, "              ReasmOKs         ReasmFails     "
		   "    ReasmReqds       ReasmTimeout\n");
	nl_dump(p, "    %18" PRIu64 " %18" PRIu64 " %18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_REASMOKS],
		link->l_stats[RTNL_LINK_IP6_REASMFAILS],
		link->l_stats[RTNL_LINK_IP6_REASMREQDS],
		link->l_stats[RTNL_LINK_IP6_REASMTIMEOUT]);

	nl_dump(p, "               FragOKs          FragFails    "
		   "    FragCreates\n");
	nl_dump(p, "    %18" PRIu64 " %18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_FRAGOKS],
		link->l_stats[RTNL_LINK_IP6_FRAGFAILS],
		link->l_stats[RTNL_LINK_IP6_FRAGCREATES]);

	nl_dump(p, "           InHdrErrors      InTooBigErrors   "
		   "     InNoRoutes       InAddrErrors\n");
	nl_dump(p, "    %18" PRIu64 " %18" PRIu64 " %18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_INHDRERRORS],
		link->l_stats[RTNL_LINK_IP6_INTOOBIGERRORS],
		link->l_stats[RTNL_LINK_IP6_INNOROUTES],
		link->l_stats[RTNL_LINK_IP6_INADDRERRORS]);

	nl_dump(p, "       InUnknownProtos     InTruncatedPkts   "
		   "    OutNoRoutes       InCsumErrors\n");
	nl_dump(p, "    %18" PRIu64 " %18" PRIu64 " %18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_INUNKNOWNPROTOS],
		link->l_stats[RTNL_LINK_IP6_INTRUNCATEDPKTS],
		link->l_stats[RTNL_LINK_IP6_OUTNOROUTES],
		link->l_stats[RTNL_LINK_IP6_CSUMERRORS]);

	nl_dump(p, "           InNoECTPkts          InECT1Pkts   "
		   "     InECT0Pkts           InCEPkts\n");
	nl_dump(p, "    %18" PRIu64 " %18" PRIu64 " %18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_IP6_NOECTPKTS],
		link->l_stats[RTNL_LINK_IP6_ECT1PKTS],
		link->l_stats[RTNL_LINK_IP6_ECT0PKTS],
		link->l_stats[RTNL_LINK_IP6_CEPKTS]);

	nl_dump(p, "    ICMPv6:     InMsgs           InErrors        "
		   "    OutMsgs          OutErrors       InCsumErrors\n");
	nl_dump(p, "    %18" PRIu64 " %18" PRIu64 " %18" PRIu64 " %18" PRIu64 " %18" PRIu64 "\n",
		link->l_stats[RTNL_LINK_ICMP6_INMSGS],
		link->l_stats[RTNL_LINK_ICMP6_INERRORS],
		link->l_stats[RTNL_LINK_ICMP6_OUTMSGS],
		link->l_stats[RTNL_LINK_ICMP6_OUTERRORS],
		link->l_stats[RTNL_LINK_ICMP6_CSUMERRORS]);
}

static const struct nla_policy protinfo_policy = {
	.type			= NLA_NESTED,
};

static struct rtnl_link_af_ops inet6_ops = {
	.ao_family			= AF_INET6,
	.ao_alloc			= &inet6_alloc,
	.ao_clone			= &inet6_clone,
	.ao_free			= &inet6_free,
	.ao_parse_protinfo		= &inet6_parse_protinfo,
	.ao_parse_af			= &inet6_parse_protinfo,
	.ao_fill_af			= &inet6_fill_af,
	.ao_dump[NL_DUMP_DETAILS]	= &inet6_dump_details,
	.ao_dump[NL_DUMP_STATS]		= &inet6_dump_stats,
	.ao_protinfo_policy		= &protinfo_policy,
};

/**
 * Return IPv6 specific flags
 * @arg link		Link object
 * @arg out_flags	Flags on success
 *
 * Returns the link's IPv6 flags.
 *
 * @return 0 on success
 * @return -NLE_NOATTR configuration setting not available
 */
int rtnl_link_inet6_get_flags(struct rtnl_link *link, uint32_t* out_flags)
{
	struct inet6_data *id = NULL;

	if (!(id = rtnl_link_af_data(link, &inet6_ops)))
		return -NLE_NOATTR;

	*out_flags = id->i6_flags;
	return 0;
}

/**
 * Set IPv6 specific flags
 * @arg link		Link object
 * @arg flags		Flags to set
 *
 * Sets the link's IPv6 specific flags. Overwrites currently set flags.
 *
 * @return 0 on success
 * @return -NLE_NOMEM could not allocate inet6 data
 */
int rtnl_link_inet6_set_flags(struct rtnl_link *link, uint32_t flags)
{
	struct inet6_data *id;

	if (!(id = rtnl_link_af_alloc(link, &inet6_ops)))
		return -NLE_NOMEM;

	id->i6_flags = flags;
	return 0;
}

/**
 * Get IPv6 tokenized interface identifier
 * @arg link		Link object
 * @arg token		Tokenized interface identifier on success
 *
 * Returns the link's IPv6 tokenized interface identifier.
 *
 * @return 0 on success
 * @return -NLE_NOMEM  failure to allocate struct nl_addr result
 * @return -NLE_NOATTR configuration setting not available
 * @return -NLE_NOADDR tokenized interface identifier is not set
 */
int rtnl_link_inet6_get_token(struct rtnl_link *link, struct nl_addr **addr)
{
	struct inet6_data *id;

	if (!(id = rtnl_link_af_data(link, &inet6_ops)))
		return -NLE_NOATTR;

	*addr = nl_addr_build(AF_INET6, &id->i6_token, sizeof(id->i6_token));
	if (!*addr)
		return -NLE_NOMEM;
	if (nl_addr_iszero(*addr)) {
		nl_addr_put(*addr);
		*addr = NULL;
		return -NLE_NOADDR;
	}

	return 0;
}

/**
 * Set IPv6 tokenized interface identifier
 * @arg link		Link object
 * @arg token		Tokenized interface identifier
 *
 * Sets the link's IPv6 tokenized interface identifier.
 *
 * @return 0 on success
 * @return -NLE_NOMEM could not allocate inet6 data
 * @return -NLE_INVAL addr is not a valid inet6 address
 */
int rtnl_link_inet6_set_token(struct rtnl_link *link, struct nl_addr *addr)
{
	struct inet6_data *id;

	if ((nl_addr_get_family(addr) != AF_INET6) ||
	    (nl_addr_get_len(addr) != sizeof(id->i6_token)))
		return -NLE_INVAL;

	if (!(id = rtnl_link_af_alloc(link, &inet6_ops)))
		return -NLE_NOMEM;

	memcpy(&id->i6_token, nl_addr_get_binary_addr(addr),
	       sizeof(id->i6_token));
	return 0;
}

/**
 * Get IPv6 link-local address generation mode
 * @arg link		Link object
 * @arg mode		Generation mode on success
 *
 * Returns the link's IPv6 link-local address generation mode.
 *
 * @return 0 on success
 * @return -NLE_NOATTR configuration setting not available
 * @return -NLE_INVAL generation mode unknown. If the link was received via
 *                    netlink, it means that address generation mode is not
 *                    supported by the kernel.
 */
int rtnl_link_inet6_get_addr_gen_mode(struct rtnl_link *link, uint8_t *mode)
{
	struct inet6_data *id;

	if (!(id = rtnl_link_af_data(link, &inet6_ops)))
		return -NLE_NOATTR;

	if (id->i6_addr_gen_mode == I6_ADDR_GEN_MODE_UNKNOWN)
		return -NLE_INVAL;

	*mode = id->i6_addr_gen_mode;
	return 0;
}

/**
 * Set IPv6 link-local address generation mode
 * @arg link		Link object
 * @arg mode		Generation mode
 *
 * Sets the link's IPv6 link-local address generation mode.
 *
 * @return 0 on success
 * @return -NLE_NOMEM could not allocate inet6 data
 */
int rtnl_link_inet6_set_addr_gen_mode(struct rtnl_link *link, uint8_t mode)
{
	struct inet6_data *id;

	if (!(id = rtnl_link_af_alloc(link, &inet6_ops)))
		return -NLE_NOMEM;

	id->i6_addr_gen_mode = mode;
	return 0;
}

/**
 * Get value of a ipv6 link configuration setting
 * @arg link		Link object
 * @arg cfgid		Configuration identifier
 * @arg res		Result pointer
 *
 * Stores the value of the specified configuration setting in the provided
 * result pointer.
 *
 * @return 0 on success or a negative error code.
 * @return -NLE_RANGE cfgid is out of range or not provided by kernel.
 * @return -NLE_NOATTR configuration setting not available
 */
int rtnl_link_inet6_get_conf(struct rtnl_link *link, unsigned int cfgid,
			     uint32_t *res)
{
	struct inet6_data *id;

	if (!(id = rtnl_link_af_data(link, &inet6_ops)))
		return -NLE_NOATTR;

	if (cfgid >= id->i6_conf_len)
		return -NLE_RANGE;

	*res = id->i6_conf[cfgid];

	return 0;
}


static void _nl_init inet6_init(void)
{
	rtnl_link_af_register(&inet6_ops);
}

static void _nl_exit inet6_exit(void)
{
	rtnl_link_af_unregister(&inet6_ops);
}
