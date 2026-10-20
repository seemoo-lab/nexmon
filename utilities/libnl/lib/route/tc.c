/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2011 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup rtnl
 * @defgroup tc Traffic Control
 * @{
 */

#include "nl-default.h"

#include <linux/if_arp.h>
#include <linux/gen_stats.h>

#include <linux/atm.h>

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>

#include "tc-api.h"

/** @cond SKIP */

static struct nl_list_head tc_ops_list[__RTNL_TC_TYPE_MAX];
static struct rtnl_tc_type_ops *tc_type_ops[__RTNL_TC_TYPE_MAX];

static struct nla_policy tc_policy[TCA_MAX+1] = {
	[TCA_KIND]	= { .type = NLA_STRING,
			    .maxlen = TCKINDSIZ },
	[TCA_CHAIN]	= { .type = NLA_U32 },
	[TCA_STATS]	= { .minlen = sizeof(struct tc_stats) },
	[TCA_STATS2]	= { .type = NLA_NESTED },
};

int tca_parse(struct nlattr **tb, int maxattr, struct rtnl_tc *g,
	      const struct nla_policy *policy)
{
	
	if (g->ce_mask & TCA_ATTR_OPTS)
		return nla_parse(tb, maxattr,
				 (struct nlattr *) g->tc_opts->d_data,
				 g->tc_opts->d_size, policy);
	else {
		/* Ugly but tb[] must be in a defined state even if no
		 * attributes can be found. */
		memset(tb, 0, sizeof(struct nlattr *) * (maxattr + 1));
		return 0;
	}
}

static struct nla_policy tc_stats2_policy[TCA_STATS_MAX+1] = {
	[TCA_STATS_BASIC]    = { .minlen = sizeof(struct gnet_stats_basic) },
	[TCA_STATS_RATE_EST] = { .minlen = sizeof(struct gnet_stats_rate_est) },
	[TCA_STATS_QUEUE]    = { .minlen = sizeof(struct gnet_stats_queue) },
};

int rtnl_tc_msg_parse(struct nlmsghdr *n, struct rtnl_tc *tc)
{
	struct nl_cache *link_cache;
	struct rtnl_tc_ops *ops;
	struct nlattr *tb[TCA_MAX + 1];
	char kind[TCKINDSIZ];
	struct tcmsg *tm;
	int err;

	tc->ce_msgtype = n->nlmsg_type;

	err = nlmsg_parse(n, sizeof(*tm), tb, TCA_MAX, tc_policy);
	if (err < 0)
		return err;

	if (tb[TCA_KIND] == NULL)
		return -NLE_MISSING_ATTR;

	nla_strlcpy(kind, tb[TCA_KIND], sizeof(kind));
	rtnl_tc_set_kind(tc, kind);

	if (tb[TCA_CHAIN])
	        rtnl_tc_set_chain(tc, nla_get_u32(tb[TCA_CHAIN]));

	tm = nlmsg_data(n);
	tc->tc_family  = tm->tcm_family;
	tc->tc_ifindex = tm->tcm_ifindex;
	tc->tc_handle  = tm->tcm_handle;
	tc->tc_parent  = tm->tcm_parent;
	tc->tc_info    = tm->tcm_info;

	tc->ce_mask |= (TCA_ATTR_FAMILY | TCA_ATTR_IFINDEX | TCA_ATTR_HANDLE|
		        TCA_ATTR_PARENT | TCA_ATTR_INFO);

	if (tb[TCA_OPTIONS]) {
		tc->tc_opts = nl_data_alloc_attr(tb[TCA_OPTIONS]);
		if (!tc->tc_opts)
			return -NLE_NOMEM;
		tc->ce_mask |= TCA_ATTR_OPTS;
	}

	if (tb[TCA_STATS2]) {
		struct nlattr *tbs[TCA_STATS_MAX + 1];

		err = nla_parse_nested(tbs, TCA_STATS_MAX, tb[TCA_STATS2],
				       tc_stats2_policy);
		if (err < 0)
			return err;

		if (tbs[TCA_STATS_BASIC]) {
			struct gnet_stats_basic bs;
			
			memcpy(&bs, nla_data(tbs[TCA_STATS_BASIC]), sizeof(bs));
			tc->tc_stats[RTNL_TC_BYTES]	= bs.bytes;
			tc->tc_stats[RTNL_TC_PACKETS]	= bs.packets;
		}

		if (tbs[TCA_STATS_RATE_EST]) {
			struct gnet_stats_rate_est *re;

			re = nla_data(tbs[TCA_STATS_RATE_EST]);
			tc->tc_stats[RTNL_TC_RATE_BPS]	= re->bps;
			tc->tc_stats[RTNL_TC_RATE_PPS]	= re->pps;
		}
		
		if (tbs[TCA_STATS_QUEUE]) {
			struct gnet_stats_queue *q;

			q = nla_data(tbs[TCA_STATS_QUEUE]);
			tc->tc_stats[RTNL_TC_QLEN]	= q->qlen;
			tc->tc_stats[RTNL_TC_BACKLOG]	= q->backlog;
			tc->tc_stats[RTNL_TC_DROPS]	= q->drops;
			tc->tc_stats[RTNL_TC_REQUEUES]	= q->requeues;
			tc->tc_stats[RTNL_TC_OVERLIMITS]	= q->overlimits;
		}

		tc->ce_mask |= TCA_ATTR_STATS;
		
		if (tbs[TCA_STATS_APP]) {
			tc->tc_xstats = nl_data_alloc_attr(tbs[TCA_STATS_APP]);
			if (tc->tc_xstats == NULL)
				return -NLE_NOMEM;
			tc->ce_mask |= TCA_ATTR_XSTATS;
		} else
			goto compat_xstats;
	} else {
		if (tb[TCA_STATS]) {
			struct tc_stats st;

			memcpy(&st, nla_data(tb[TCA_STATS]), sizeof(st));
			tc->tc_stats[RTNL_TC_BYTES]	= st.bytes;
			tc->tc_stats[RTNL_TC_PACKETS]	= st.packets;
			tc->tc_stats[RTNL_TC_RATE_BPS]	= st.bps;
			tc->tc_stats[RTNL_TC_RATE_PPS]	= st.pps;
			tc->tc_stats[RTNL_TC_QLEN]	= st.qlen;
			tc->tc_stats[RTNL_TC_BACKLOG]	= st.backlog;
			tc->tc_stats[RTNL_TC_DROPS]	= st.drops;
			tc->tc_stats[RTNL_TC_OVERLIMITS]= st.overlimits;

			tc->ce_mask |= TCA_ATTR_STATS;
		}

compat_xstats:
		if (tb[TCA_XSTATS]) {
			tc->tc_xstats = nl_data_alloc_attr(tb[TCA_XSTATS]);
			if (tc->tc_xstats == NULL)
				return -NLE_NOMEM;
			tc->ce_mask |= TCA_ATTR_XSTATS;
		}
	}

	ops = rtnl_tc_get_ops(tc);
	if (ops && ops->to_msg_parser) {
		void *data = rtnl_tc_data(tc);

		if (!data)
			return -NLE_NOMEM;

		err = ops->to_msg_parser(tc, data);
		if (err < 0)
			return err;
	}

	if ((link_cache = __nl_cache_mngt_require("route/link"))) {
		struct rtnl_link *link;

		if ((link = rtnl_link_get(link_cache, tc->tc_ifindex))) {
			rtnl_tc_set_link(tc, link);

			/* rtnl_tc_set_link incs refcnt */
			rtnl_link_put(link);
		}
	}

	return 0;
}

int rtnl_tc_msg_build(struct rtnl_tc *tc, int type, int flags,
		      struct nl_msg **result)
{
	struct nl_msg *msg;
	struct rtnl_tc_ops *ops;
	struct tcmsg tchdr = {
		.tcm_family = AF_UNSPEC,
		.tcm_ifindex = tc->tc_ifindex,
		.tcm_handle = tc->tc_handle,
		.tcm_parent = tc->tc_parent,
	};
	int err;

	msg = nlmsg_alloc_simple(type, flags);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &tchdr, sizeof(tchdr), NLMSG_ALIGNTO) < 0) {
		err = -NLE_MSGSIZE;
		goto out_err;
	}

	if (tc->ce_mask & TCA_ATTR_KIND)
		NLA_PUT_STRING(msg, TCA_KIND, tc->tc_kind);

	if (tc->ce_mask & TCA_ATTR_CHAIN)
	        NLA_PUT_U32(msg, TCA_CHAIN, tc->tc_chain);

	ops = rtnl_tc_get_ops(tc);
	if (ops && (ops->to_msg_fill || ops->to_msg_fill_raw)) {
		struct nlattr *opts;
		void *data = rtnl_tc_data(tc);

		if (ops->to_msg_fill) {
			if (!(opts = nla_nest_start(msg, TCA_OPTIONS))) {
				err = -NLE_NOMEM;
				goto out_err;
			}

			if ((err = ops->to_msg_fill(tc, data, msg)) < 0)
				goto out_err;

			if (strcmp("cgroup", tc->tc_kind))
				nla_nest_end(msg, opts);
			else
				nla_nest_end_keep_empty(msg, opts);
		} else if ((err = ops->to_msg_fill_raw(tc, data, msg)) < 0)
			goto out_err;
	}

	*result = msg;
	return 0;

nla_put_failure:
	err = -NLE_NOMEM;
out_err:
	nlmsg_free(msg);
	return err;
}


/** @endcond */

/**
 * @name Attributes
 * @{
 */

/**
 * Set interface index of traffic control object
 * @arg tc		traffic control object
 * @arg ifindex		interface index.
 *
 * Sets the interface index of a traffic control object. The interface
 * index defines the network device which this tc object is attached to.
 * This function will overwrite any network device assigned with previous
 * calls to rtnl_tc_set_ifindex() or rtnl_tc_set_link().
 */
void rtnl_tc_set_ifindex(struct rtnl_tc *tc, int ifindex)
{
	/* Obsolete possible old link reference */
	rtnl_link_put(tc->tc_link);
	tc->tc_link = NULL;
	tc->ce_mask &= ~TCA_ATTR_LINK;

	tc->tc_ifindex = ifindex;
	tc->ce_mask |= TCA_ATTR_IFINDEX;
}

/**
 * Return interface index of traffic control object
 * @arg tc		traffic control object
 */
int rtnl_tc_get_ifindex(struct rtnl_tc *tc)
{
	return tc->tc_ifindex;
}

/**
 * Set link of traffic control object
 * @arg tc		traffic control object
 * @arg link		link object
 *
 * Sets the link of a traffic control object. This function serves
 * the same purpose as rtnl_tc_set_ifindex() but due to the continued
 * allowed access to the link object it gives it the possibility to
 * retrieve sane default values for the the MTU and the linktype.
 * Always prefer this function over rtnl_tc_set_ifindex() if you can
 * spare to have an additional link object around.
 */
void rtnl_tc_set_link(struct rtnl_tc *tc, struct rtnl_link *link)
{
	rtnl_link_put(tc->tc_link);

	if (!link)
		return;
	if (!link->l_index)
		BUG();

	nl_object_get(OBJ_CAST(link));
	tc->tc_link = link;
	tc->tc_ifindex = link->l_index;
	tc->ce_mask |= TCA_ATTR_LINK | TCA_ATTR_IFINDEX;
}

/**
 * Get link of traffic control object
 * @arg tc		traffic control object
 *
 * Returns the link of a traffic control object. The link is only
 * returned if it has been set before via rtnl_tc_set_link() or
 * if a link cache was available while parsing the tc object. This
 * function may still return NULL even if an ifindex is assigned to
 * the tc object. It will _not_ look up the link by itself.
 *
 * @note The returned link will have its reference counter incremented.
 *       It is in the responsibility of the caller to return the
 *       reference.
 *
 * @return link object or NULL if not set.
 */
struct rtnl_link *rtnl_tc_get_link(struct rtnl_tc *tc)
{
	if (tc->tc_link) {
		nl_object_get(OBJ_CAST(tc->tc_link));
		return tc->tc_link;
	}

	return NULL;
}

/**
 * Set the Maximum Transmission Unit (MTU) of traffic control object
 * @arg tc		traffic control object
 * @arg mtu		largest packet size expected
 *
 * Sets the MTU of a traffic control object. Not all traffic control
 * objects will make use of this but it helps while calculating rate
 * tables. This value is typically derived directly from the link
 * the tc object is attached to if the link has been assigned via
 * rtnl_tc_set_link(). It is usually not necessary to set the MTU
 * manually, this function is provided to allow overwriting the derived
 * value.
 */
void rtnl_tc_set_mtu(struct rtnl_tc *tc, uint32_t mtu)
{
	tc->tc_mtu = mtu;
	tc->ce_mask |= TCA_ATTR_MTU;
}

/**
 * Return the MTU of traffic control object
 * @arg tc		traffic control object
 *
 * Returns the MTU of a traffic control object which has been set via:
 * -# User specified value set via rtnl_tc_set_mtu()
 * -# Dervied from link set via rtnl_tc_set_link()
 * -# Fall back to default: ethernet = 1500
 */
uint32_t rtnl_tc_get_mtu(struct rtnl_tc *tc)
{
	if (tc->ce_mask & TCA_ATTR_MTU)
		return tc->tc_mtu;
	else if (tc->ce_mask & TCA_ATTR_LINK)
		return tc->tc_link->l_mtu;
	else
		return 1500; /* default to ethernet */
}

/**
 * Set the Minimum Packet Unit (MPU) of a traffic control object
 * @arg tc		traffic control object
 * @arg mpu		minimum packet size expected
 *
 * Sets the MPU of a traffic contorl object. It specifies the minimum
 * packet size to ever hit this traffic control object. Not all traffic
 * control objects will make use of this but it helps while calculating
 * rate tables.
 */
void rtnl_tc_set_mpu(struct rtnl_tc *tc, uint32_t mpu)
{
	tc->tc_mpu = mpu;
	tc->ce_mask |= TCA_ATTR_MPU;
}

/**
 * Return the Minimum Packet Unit (MPU) of a traffic control object
 * @arg tc		traffic control object
 *
 * @return The MPU previously set via rtnl_tc_set_mpu() or 0.
 */
uint32_t rtnl_tc_get_mpu(struct rtnl_tc *tc)
{
	return tc->tc_mpu;
}

/**
 * Set per packet overhead of a traffic control object
 * @arg tc		traffic control object
 * @arg overhead	overhead per packet in bytes
 *
 * Sets the per packet overhead in bytes occuring on the link not seen
 * by the kernel. This value can be used to correct size calculations
 * if the packet size on the wire does not match the packet sizes seen
 * in the network stack. Not all traffic control objects will make use
 * this but it helps while calculating accurate packet sizes in the
 * kernel.
 */
void rtnl_tc_set_overhead(struct rtnl_tc *tc, uint32_t overhead)
{
	tc->tc_overhead = overhead;
	tc->ce_mask |= TCA_ATTR_OVERHEAD;
}

/**
 * Return per packet overhead of a traffic control object
 * @arg tc		traffic control object
 *
 * @return The overhead previously set by rtnl_tc_set_overhead() or 0.
 */
uint32_t rtnl_tc_get_overhead(struct rtnl_tc *tc)
{
	return tc->tc_overhead;
}

/**
 * Set the linktype of a traffic control object
 * @arg tc		traffic control object
 * @arg type		type of link (e.g. ARPHRD_ATM, ARPHRD_ETHER)
 *
 * Overwrites the type of link this traffic control object is attached to.
 * This value is typically derived from the link this tc object is attached
 * if the link has been assigned via rtnl_tc_set_link(). It is usually not
 * necessary to set the linktype manually. This function is provided to
 * allow overwriting the linktype.
 */
void rtnl_tc_set_linktype(struct rtnl_tc *tc, uint32_t type)
{
	tc->tc_linktype = type;
	tc->ce_mask |= TCA_ATTR_LINKTYPE;
}

/**
 * Return the linktype of a traffic control object
 * @arg tc		traffic control object
 *
 * Returns the linktype of the link the traffic control object is attached to:
 * -# User specified value via rtnl_tc_set_linktype()
 * -# Value derived from link set via rtnl_tc_set_link()
 * -# Default fall-back: ARPHRD_ETHER
 */
uint32_t rtnl_tc_get_linktype(struct rtnl_tc *tc)
{
	if (tc->ce_mask & TCA_ATTR_LINKTYPE)
		return tc->tc_linktype;
	else if (tc->ce_mask & TCA_ATTR_LINK)
		return tc->tc_link->l_arptype;
	else
		return ARPHRD_ETHER; /* default to ethernet */
}

/**
 * Set identifier of traffic control object
 * @arg tc		traffic control object
 * @arg id		unique identifier
 */
void rtnl_tc_set_handle(struct rtnl_tc *tc, uint32_t id)
{
	tc->tc_handle = id;
	tc->ce_mask |= TCA_ATTR_HANDLE;
}

/**
 * Return identifier of a traffic control object
 * @arg tc		traffic control object
 */
uint32_t rtnl_tc_get_handle(struct rtnl_tc *tc)
{
	return tc->tc_handle;
}

/**
 * Set the parent identifier of a traffic control object
 * @arg tc		traffic control object
 * @arg parent		identifier of parent traffif control object
 *
 */
void rtnl_tc_set_parent(struct rtnl_tc *tc, uint32_t parent)
{
	tc->tc_parent = parent;
	tc->ce_mask |= TCA_ATTR_PARENT;
}

/**
 * Return parent identifier of a traffic control object
 * @arg tc		traffic control object
 */
uint32_t rtnl_tc_get_parent(struct rtnl_tc *tc)
{
	return tc->tc_parent;
}

/**
 * Define the type of traffic control object
 * @arg tc		traffic control object
 * @arg kind		name of the tc object type
 *
 * @return 0 on success or a negative error code
 */
int rtnl_tc_set_kind(struct rtnl_tc *tc, const char *kind)
{
	if (tc->ce_mask & TCA_ATTR_KIND)
		return -NLE_EXIST;

	if (   !kind
	    || strlen (kind) >= sizeof (tc->tc_kind))
		return -NLE_INVAL;

	_nl_strncpy_assert(tc->tc_kind, kind, sizeof(tc->tc_kind));

	tc->ce_mask |= TCA_ATTR_KIND;

	/* Force allocation of data */
	rtnl_tc_data(tc);

	return 0;
}

/**
 * Return kind of traffic control object
 * @arg tc		traffic control object
 *
 * @return Kind of traffic control object or NULL if not set.
 */
char *rtnl_tc_get_kind(struct rtnl_tc *tc)
{
	if (tc->ce_mask & TCA_ATTR_KIND)
		return tc->tc_kind;
	else
		return NULL;
}

/**
 * Return value of a statistical counter of a traffic control object
 * @arg tc		traffic control object
 * @arg id		identifier of statistical counter
 *
 * @return Value of requested statistic counter or 0.
 */
uint64_t rtnl_tc_get_stat(struct rtnl_tc *tc, enum rtnl_tc_stat id)
{
	if ((unsigned int) id > RTNL_TC_STATS_MAX)
		return 0;

	return tc->tc_stats[id];
}

/**
 * Set the chain index of a traffic control object
 * @arg tc		traffic control object
 * @arg chain		chain index of traffic control object
 *
 */
void rtnl_tc_set_chain(struct rtnl_tc *tc, uint32_t chain)
{
	tc->tc_chain = chain;
	tc->ce_mask |= TCA_ATTR_CHAIN;
}

/**
 * Return chain index of traffic control object
 * @arg tc		traffic control object
 * @arg out_value       output argument.
 *
 * @return 0 of the output value was successfully returned, or a negative
 *   error code on failure.
 */
int rtnl_tc_get_chain(struct rtnl_tc *tc, uint32_t *out_value)
{
	if (!(tc->ce_mask & TCA_ATTR_CHAIN))
		return -NLE_MISSING_ATTR;
	*out_value = tc->tc_chain;
	return 0;
}

/** @} */

/**
 * @name Utilities
 * @{
 */

static const struct trans_tbl tc_stats[] = {
	__ADD(RTNL_TC_PACKETS, packets),
	__ADD(RTNL_TC_BYTES, bytes),
	__ADD(RTNL_TC_RATE_BPS, rate_bps),
	__ADD(RTNL_TC_RATE_PPS, rate_pps),
	__ADD(RTNL_TC_QLEN, qlen),
	__ADD(RTNL_TC_BACKLOG, backlog),
	__ADD(RTNL_TC_DROPS, drops),
	__ADD(RTNL_TC_REQUEUES, requeues),
	__ADD(RTNL_TC_OVERLIMITS, overlimits),
};

char *rtnl_tc_stat2str(enum rtnl_tc_stat st, char *buf, size_t len)
{
	return __type2str(st, buf, len, tc_stats, ARRAY_SIZE(tc_stats));
}

int rtnl_tc_str2stat(const char *name)
{
	return __str2type(name, tc_stats, ARRAY_SIZE(tc_stats));
}

/**
 * Calculate time required to transmit buffer at a specific rate
 * @arg bufsize		Size of buffer to be transmited in bytes.
 * @arg rate		Transmit rate in bytes per second.
 *
 * Calculates the number of micro seconds required to transmit a
 * specific buffer at a specific transmit rate.
 *
 * @f[
 *   txtime=\frac{bufsize}{rate}10^6
 * @f]
 * 
 * @return Required transmit time in micro seconds.
 */
int rtnl_tc_calc_txtime(int bufsize, int rate)
{
	return ((double) bufsize / (double) rate) * 1000000.0;
}

/**
 * Calculate buffer size able to transmit in a specific time and rate.
 * @arg txtime		Available transmit time in micro seconds.
 * @arg rate		Transmit rate in bytes per second.
 *
 * Calculates the size of the buffer that can be transmitted in a
 * specific time period at a specific transmit rate.
 *
 * @f[
 *   bufsize=\frac{{txtime} \times {rate}}{10^6}
 * @f]
 *
 * @return Size of buffer in bytes.
 */
int rtnl_tc_calc_bufsize(int txtime, int rate)
{
	return ((double) txtime * (double) rate) / 1000000.0;
}

/**
 * Calculate the binary logarithm for a specific cell size
 * @arg cell_size	Size of cell, must be a power of two.
 * @return Binary logarithm of cell size or a negative error code.
 */
int rtnl_tc_calc_cell_log(int cell_size)
{
	int i;

	for (i = 0; i < 32; i++)
		if ((((uint32_t)1u) << i) == ((uint32_t)cell_size))
			return i;

	return -NLE_INVAL;
}


/** @} */

/**
 * @name Rate Tables
 * @{
 */

/*
 * COPYRIGHT NOTE:
 * align_to_atm() and adjust_size() derived/coped from iproute2 source.
 */

/*
 * The align to ATM cells is used for determining the (ATM) SAR
 * alignment overhead at the ATM layer. (SAR = Segmentation And
 * Reassembly).  This is for example needed when scheduling packet on
 * an ADSL connection.  Note that the extra ATM-AAL overhead is _not_
 * included in this calculation. This overhead is added in the kernel
 * before doing the rate table lookup, as this gives better precision
 * (as the table will always be aligned for 48 bytes).
 *  --Hawk, d.7/11-2004. <hawk@diku.dk>
 */
static unsigned int align_to_atm(unsigned int size)
{
	int linksize, cells;
	cells = size / ATM_CELL_PAYLOAD;
	if ((size % ATM_CELL_PAYLOAD) > 0)
		cells++;

	linksize = cells * ATM_CELL_SIZE; /* Use full cell size to add ATM tax */
	return linksize;
}

static unsigned int adjust_size(unsigned int size, unsigned int mpu,
				uint32_t linktype)
{
	if (size < mpu)
		size = mpu;

	switch (linktype) {
	case ARPHRD_ATM:
		return align_to_atm(size);

	case ARPHRD_ETHER:
	default:
		return size;
	}
}

/**
 * Compute a transmission time lookup table
 * @arg tc		traffic control object
 * @arg spec		Rate specification
 * @arg dst		Destination buffer of RTNL_TC_RTABLE_SIZE uint32_t[].
 *
 * Computes a table of RTNL_TC_RTABLE_SIZE entries specyfing the
 * transmission times for various packet sizes, e.g. the transmission
 * time for a packet of size \c pktsize could be looked up:
 * @code
 * txtime = table[pktsize >> log2(mtu)];
 * @endcode
 */
int rtnl_tc_build_rate_table(struct rtnl_tc *tc, struct rtnl_ratespec *spec,
			     uint32_t *dst)
{
	uint32_t mtu = rtnl_tc_get_mtu(tc);
	uint32_t linktype = rtnl_tc_get_linktype(tc);
	uint8_t cell_log = spec->rs_cell_log;
	unsigned int size, i;

	spec->rs_mpu = rtnl_tc_get_mpu(tc);
	spec->rs_overhead = rtnl_tc_get_overhead(tc);

	if (mtu == 0)
		mtu = 2047;

	if (cell_log == UINT8_MAX) {
		/*
		 * cell_log not specified, calculate it. It has to specify the
		 * minimum number of rshifts required to break the MTU to below
		 * RTNL_TC_RTABLE_SIZE.
		 */
		cell_log = 0;
		while ((mtu >> cell_log) >= RTNL_TC_RTABLE_SIZE)
			cell_log++;
	}

	for (i = 0; i < RTNL_TC_RTABLE_SIZE; i++) {
		size = adjust_size((i + 1) << cell_log, spec->rs_mpu, linktype);
		dst[i] = nl_us2ticks(rtnl_tc_calc_txtime64(size, spec->rs_rate64));
	}

	spec->rs_cell_align = -1;
	spec->rs_cell_log = cell_log;

	return 0;
}

/** @} */

/**
 * @name TC implementation of cache functions
 */

void rtnl_tc_free_data(struct nl_object *obj)
{
	struct rtnl_tc *tc = TC_CAST(obj);
	struct rtnl_tc_ops *ops;
	
	rtnl_link_put(tc->tc_link);
	nl_data_free(tc->tc_opts);
	nl_data_free(tc->tc_xstats);

	if (tc->tc_subdata) {
		ops = rtnl_tc_get_ops(tc);
		if (ops && ops->to_free_data)
			ops->to_free_data(tc, nl_data_get(tc->tc_subdata));

		nl_data_free(tc->tc_subdata);
	}
}

int rtnl_tc_clone(struct nl_object *dstobj, struct nl_object *srcobj)
{
	struct rtnl_tc *dst = TC_CAST(dstobj);
	struct rtnl_tc *src = TC_CAST(srcobj);
	struct rtnl_tc_ops *ops;

	dst->tc_opts = NULL;
	dst->tc_xstats = NULL;
	dst->tc_subdata = NULL;
	dst->tc_link = NULL;
	dst->tc_ops = NULL;

	if (src->tc_link) {
		nl_object_get(OBJ_CAST(src->tc_link));
		dst->tc_link = src->tc_link;
	}

	dst->ce_mask &= ~(TCA_ATTR_OPTS |
	                  TCA_ATTR_XSTATS);

	if (src->tc_opts) {
		dst->tc_opts = nl_data_clone(src->tc_opts);
		if (!dst->tc_opts)
			return -NLE_NOMEM;
		dst->ce_mask |= TCA_ATTR_OPTS;
	}

	if (src->tc_xstats) {
		dst->tc_xstats = nl_data_clone(src->tc_xstats);
		if (!dst->tc_xstats)
			return -NLE_NOMEM;
		dst->ce_mask |= TCA_ATTR_XSTATS;
	}

	if (src->tc_subdata) {
		if (!(dst->tc_subdata = nl_data_clone(src->tc_subdata))) {
			return -NLE_NOMEM;
		}

		/* Warning: if the data contains pointer, then at this point, dst->tc_subdata
		 * will alias those pointers.
		 *
		 * ops->to_clone() MUST fix that.
		 *
		 * If the type is actually "struct rtnl_act", then to_clone() must also
		 * fix dangling "a_next" pointer. */

		ops = rtnl_tc_get_ops(src);
		if (ops && ops->to_clone) {
			return ops->to_clone(rtnl_tc_data(dst), rtnl_tc_data(src));
		}
	}

	return 0;
}

static int tc_dump(struct rtnl_tc *tc, enum nl_dump_type type,
		   struct nl_dump_params *p)
{
	struct rtnl_tc_type_ops *type_ops;
	struct rtnl_tc_ops *ops;
	void *data = rtnl_tc_data(tc);

	type_ops = tc_type_ops[tc->tc_type];
	if (type_ops && type_ops->tt_dump[type])
		type_ops->tt_dump[type](tc, p);

	ops = rtnl_tc_get_ops(tc);
	if (ops && ops->to_dump[type]) {
		ops->to_dump[type](tc, data, p);
		return 1;
	}

	return 0;
}

void rtnl_tc_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_tc_type_ops *type_ops;
	struct rtnl_tc *tc = TC_CAST(obj);
	struct nl_cache *link_cache;
	char buf[32];

	nl_new_line(p);

	type_ops = tc_type_ops[tc->tc_type];
	if (type_ops && type_ops->tt_dump_prefix)
		nl_dump(p, "%s ", type_ops->tt_dump_prefix);

	nl_dump(p, "%s ", tc->tc_kind);

	if ((link_cache = nl_cache_mngt_require_safe("route/link"))) {
		nl_dump(p, "dev %s ",
			rtnl_link_i2name(link_cache, tc->tc_ifindex,
					 buf, sizeof(buf)));
	} else
		nl_dump(p, "dev %u ", tc->tc_ifindex);
	
	nl_dump(p, "id %s ",
		rtnl_tc_handle2str(tc->tc_handle, buf, sizeof(buf)));
	
	nl_dump(p, "parent %s",
		rtnl_tc_handle2str(tc->tc_parent, buf, sizeof(buf)));

	tc_dump(tc, NL_DUMP_LINE, p);
	nl_dump(p, "\n");

	if (link_cache)
		nl_cache_put(link_cache);
}

void rtnl_tc_dump_details(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_tc *tc = TC_CAST(obj);

	rtnl_tc_dump_line(OBJ_CAST(tc), p);

	nl_dump_line(p, "  ");

	if (tc->ce_mask & TCA_ATTR_MTU)
		nl_dump(p, " mtu %u", tc->tc_mtu);

	if (tc->ce_mask & TCA_ATTR_MPU)
		nl_dump(p, " mpu %u", tc->tc_mpu);

	if (tc->ce_mask & TCA_ATTR_OVERHEAD)
		nl_dump(p, " overhead %u", tc->tc_overhead);

	if (!tc_dump(tc, NL_DUMP_DETAILS, p))
		nl_dump(p, "no options");
	nl_dump(p, "\n");
}

void rtnl_tc_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_tc *tc = TC_CAST(obj);
	char *unit;
	float res;

	rtnl_tc_dump_details(OBJ_CAST(tc), p);

	nl_dump_line(p,
	             "  stats: %-14s %-10s   %-10s %-10s %-10s %-10s\n",
	             "bytes", "packets", "drops", "overlimits", "qlen", "backlog");

	res = nl_cancel_down_bytes(tc->tc_stats[RTNL_TC_BYTES], &unit);

	nl_dump_line(
		p,
		"       %10.2f %3s   %10llu   %-10llu %-10llu %-10llu %-10llu\n",
		res, unit, (long long unsigned)tc->tc_stats[RTNL_TC_PACKETS],
		(long long unsigned)tc->tc_stats[RTNL_TC_DROPS],
		(long long unsigned)tc->tc_stats[RTNL_TC_OVERLIMITS],
		(long long unsigned)tc->tc_stats[RTNL_TC_QLEN],
		(long long unsigned)tc->tc_stats[RTNL_TC_BACKLOG]);

	res = nl_cancel_down_bytes(tc->tc_stats[RTNL_TC_RATE_BPS], &unit);

	nl_dump_line(p, "       %10.2f %3s/s %10llu/s\n", res, unit,
		     (long long unsigned)tc->tc_stats[RTNL_TC_RATE_PPS]);
}

uint64_t rtnl_tc_compare(struct nl_object *aobj, struct nl_object *bobj,
			 uint64_t attrs, int flags)
{
	struct rtnl_tc *a = TC_CAST(aobj);
	struct rtnl_tc *b = TC_CAST(bobj);
	uint64_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(TCA_ATTR_HANDLE, a->tc_handle != b->tc_handle);
	diff |= _DIFF(TCA_ATTR_PARENT, a->tc_parent != b->tc_parent);
	diff |= _DIFF(TCA_ATTR_IFINDEX, a->tc_ifindex != b->tc_ifindex);
	diff |= _DIFF(TCA_ATTR_KIND, strcmp(a->tc_kind, b->tc_kind));
#undef _DIFF

	return diff;
}

/** @} */

/**
 * @name Modules API
 */

struct rtnl_tc_ops *rtnl_tc_lookup_ops(enum rtnl_tc_type type, const char *kind)
{
	struct rtnl_tc_ops *ops;

	nl_list_for_each_entry(ops, &tc_ops_list[type], to_list)
		if (!strcmp(kind, ops->to_kind))
			return ops;

	return NULL;
}

struct rtnl_tc_ops *rtnl_tc_get_ops(struct rtnl_tc *tc)
{
	if (!tc->tc_ops)
		tc->tc_ops = rtnl_tc_lookup_ops(tc->tc_type, tc->tc_kind);

	return tc->tc_ops;
}

/**
 * Register a traffic control module
 * @arg ops		traffic control module operations
 */
int rtnl_tc_register(struct rtnl_tc_ops *ops)
{
	static int init = 0;

	/*
	 * Initialiation hack, make sure list is initialized when
	 * the first tc module registers. Putting this in a
	 * separate _nl_init would required correct ordering of init
	 * functions
	 */
	if (!init) {
		int i;

		for (i = 0; i < __RTNL_TC_TYPE_MAX; i++)
			nl_init_list_head(&tc_ops_list[i]);

		init = 1;
	}

	if (!ops->to_kind || ops->to_type > RTNL_TC_TYPE_MAX)
		BUG();

	if (rtnl_tc_lookup_ops(ops->to_type, ops->to_kind))
		return -NLE_EXIST;

	nl_list_add_tail(&ops->to_list, &tc_ops_list[ops->to_type]);

	return 0;
}

/**
 * Unregister a traffic control module
 * @arg ops		traffic control module operations
 */
void rtnl_tc_unregister(struct rtnl_tc_ops *ops)
{
	nl_list_del(&ops->to_list);
}

/**
 * Returns the private data of the traffic control object.
 * Contrary to rtnl_tc_data(), this returns NULL if the data is
 * not yet allocated
 * @arg tc		traffic control object
 *
 * @return pointer to the private data or NULL if not allocated.
 */
void *rtnl_tc_data_peek(struct rtnl_tc *tc)
{
	return tc->tc_subdata ? nl_data_get(tc->tc_subdata) : NULL;
}

/**
 * Return pointer to private data of traffic control object
 * @arg tc		traffic control object
 *
 * Allocates the private traffic control object data section
 * as necessary and returns it.
 *
 * @return Pointer to private tc data or NULL if allocation failed.
 */
void *rtnl_tc_data(struct rtnl_tc *tc)
{
	if (!tc->tc_subdata) {
		size_t size;

		if (!tc->tc_ops) {
			if (!rtnl_tc_get_ops(tc))
				return NULL;
		}

		if (!(size = tc->tc_ops->to_size))
			BUG();

		if (!(tc->tc_subdata = nl_data_alloc(NULL, size)))
			return NULL;
	}

	return nl_data_get(tc->tc_subdata);
}

/**
 * Check traffic control object type and return private data section 
 * @arg tc		traffic control object
 * @arg ops		expected traffic control object operations
 * @arg err		the place where saves the error code if fails
 *
 * Checks whether the traffic control object matches the type
 * specified with the traffic control object operations. If the
 * type matches, the private tc object data is returned. If type
 * mismatches, APPBUG() will print a application bug warning.
 *
 * @see rtnl_tc_data()
 *
 * @return Pointer to private tc data or NULL if type mismatches.
 */
void *rtnl_tc_data_check(struct rtnl_tc *tc, struct rtnl_tc_ops *ops, int *err)
{
	void *ret;

	if (tc->tc_ops != ops) {
		char buf[64];

		snprintf(buf, sizeof(buf),
			 "tc object %p used in %s context but is of type %s",
			 tc, ops->to_kind, tc->tc_ops->to_kind);
		APPBUG(buf);

		if (err)
			*err = -NLE_OPNOTSUPP;
		return NULL;
	}

	ret = rtnl_tc_data(tc);
	if (ret == NULL) {
		if (err)
			*err = -NLE_NOMEM;
	}

	return ret;
}

struct nl_af_group tc_groups[] = {
	{ AF_UNSPEC,	RTNLGRP_TC },
	{ END_OF_GROUP_LIST },
};


void rtnl_tc_type_register(struct rtnl_tc_type_ops *ops)
{
	if (ops->tt_type > RTNL_TC_TYPE_MAX)
		BUG();

	tc_type_ops[ops->tt_type] = ops;
}

void rtnl_tc_type_unregister(struct rtnl_tc_type_ops *ops)
{
	if (ops->tt_type > RTNL_TC_TYPE_MAX)
		BUG();

	tc_type_ops[ops->tt_type] = NULL;
}

/** @} */

/** @} */
