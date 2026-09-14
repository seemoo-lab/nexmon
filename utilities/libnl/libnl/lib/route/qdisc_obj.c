/*
 * lib/route/qdisc_obj.c            Queueing Discipline Object
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup qdisc
 * @defgroup qdisc_obj Queueing Discipline Object
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/classifier.h>
#include <netlink/route/qdisc-modules.h>

static void qdisc_free_data(struct nl_object *obj)
{
	struct rtnl_qdisc *qdisc = (struct rtnl_qdisc *) obj;
	struct rtnl_qdisc_ops *qops;

	tca_free_data((struct rtnl_tca *) qdisc);

	qops = rtnl_qdisc_lookup_ops(qdisc);
	if (qops && qops->qo_free_data)
		qops->qo_free_data(qdisc);
}

static int qdisc_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct rtnl_qdisc *dst = (struct rtnl_qdisc *) _dst;
	struct rtnl_qdisc *src = (struct rtnl_qdisc *) _src;
	struct rtnl_qdisc_ops *qops;
	int err;

	err = tca_clone((struct rtnl_tca *) dst, (struct rtnl_tca *) src);
	if (err < 0)
		goto errout;

	qops = rtnl_qdisc_lookup_ops(src);
	if (qops && qops->qo_clone)
		err = qops->qo_clone(dst, src);
errout:
	return err;
}

static void qdisc_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_qdisc *qdisc = (struct rtnl_qdisc *) obj;
	struct rtnl_qdisc_ops *qops;
	
	tca_dump_line((struct rtnl_tca *) qdisc, "qdisc", p);

	qops = rtnl_qdisc_lookup_ops(qdisc);
	if (qops && qops->qo_dump[NL_DUMP_LINE])
		qops->qo_dump[NL_DUMP_LINE](qdisc, p);

	nl_dump(p, "\n");
}

static void qdisc_dump_details(struct nl_object *arg, struct nl_dump_params *p)
{
	struct rtnl_qdisc *qdisc = (struct rtnl_qdisc *) arg;
	struct rtnl_qdisc_ops *qops;

	qdisc_dump_line(arg, p);

	tca_dump_details((struct rtnl_tca *) qdisc, p);
	nl_dump(p, "refcnt %u ", qdisc->q_info);

	qops = rtnl_qdisc_lookup_ops(qdisc);
	if (qops && qops->qo_dump[NL_DUMP_DETAILS])
		qops->qo_dump[NL_DUMP_DETAILS](qdisc, p);

	nl_dump(p, "\n");
}

static void qdisc_dump_stats(struct nl_object *arg, struct nl_dump_params *p)
{
	struct rtnl_qdisc *qdisc = (struct rtnl_qdisc *) arg;
	struct rtnl_qdisc_ops *qops;

	qdisc_dump_details(arg, p);
	tca_dump_stats((struct rtnl_tca *) qdisc, p);
	nl_dump(p, "\n");

	qops = rtnl_qdisc_lookup_ops(qdisc);
	if (qops && qops->qo_dump[NL_DUMP_STATS])
		qops->qo_dump[NL_DUMP_STATS](qdisc, p);
}

/**
 * @name Allocation/Freeing
 * @{
 */

struct rtnl_qdisc *rtnl_qdisc_alloc(void)
{
	return (struct rtnl_qdisc *) nl_object_alloc(&qdisc_obj_ops);
}

void rtnl_qdisc_put(struct rtnl_qdisc *qdisc)
{
	nl_object_put((struct nl_object *) qdisc);
}

/** @} */

/**
 * @name Iterators
 * @{
 */

/**
 * Call a callback for each child class of a qdisc
 * @arg qdisc		the parent qdisc
 * @arg cache		a class cache including all classes of the interface
 *                      the specified qdisc is attached to
 * @arg cb              callback function
 * @arg arg             argument to be passed to callback function
 */
void rtnl_qdisc_foreach_child(struct rtnl_qdisc *qdisc, struct nl_cache *cache,
			      void (*cb)(struct nl_object *, void *), void *arg)
{
	struct rtnl_class *filter;
	
	filter = rtnl_class_alloc();
	if (!filter)
		return;

	rtnl_class_set_parent(filter, qdisc->q_handle);
	rtnl_class_set_ifindex(filter, qdisc->q_ifindex);
	rtnl_class_set_kind(filter, qdisc->q_kind);

	nl_cache_foreach_filter(cache, (struct nl_object *) filter, cb, arg);

	rtnl_class_put(filter);
}

/**
 * Call a callback for each filter attached to the qdisc
 * @arg qdisc		the parent qdisc
 * @arg cache		a filter cache including at least all the filters
 *                      attached to the specified qdisc
 * @arg cb              callback function
 * @arg arg             argument to be passed to callback function
 */
void rtnl_qdisc_foreach_cls(struct rtnl_qdisc *qdisc, struct nl_cache *cache,
			    void (*cb)(struct nl_object *, void *), void *arg)
{
	struct rtnl_cls *filter;

	filter = rtnl_cls_alloc();
	if (!filter)
		return;

	rtnl_cls_set_ifindex(filter, qdisc->q_ifindex);
	rtnl_cls_set_parent(filter, qdisc->q_parent);

	nl_cache_foreach_filter(cache, (struct nl_object *) filter, cb, arg);
	rtnl_cls_put(filter);
}

/** @} */

/**
 * @name Attributes
 * @{
 */

void rtnl_qdisc_set_ifindex(struct rtnl_qdisc *qdisc, int ifindex)
{
	tca_set_ifindex((struct rtnl_tca *) qdisc, ifindex);
}

int rtnl_qdisc_get_ifindex(struct rtnl_qdisc *qdisc)
{
	return tca_get_ifindex((struct rtnl_tca *) qdisc);
}

void rtnl_qdisc_set_handle(struct rtnl_qdisc *qdisc, uint32_t handle)
{
	tca_set_handle((struct rtnl_tca *) qdisc, handle);
}

uint32_t rtnl_qdisc_get_handle(struct rtnl_qdisc *qdisc)
{
	return tca_get_handle((struct rtnl_tca *) qdisc);
}

void rtnl_qdisc_set_parent(struct rtnl_qdisc *qdisc, uint32_t parent)
{
	tca_set_parent((struct rtnl_tca *) qdisc, parent);
}

uint32_t rtnl_qdisc_get_parent(struct rtnl_qdisc *qdisc)
{
	return tca_get_parent((struct rtnl_tca *) qdisc);
}

void rtnl_qdisc_set_kind(struct rtnl_qdisc *qdisc, const char *name)
{
	tca_set_kind((struct rtnl_tca *) qdisc, name);
	qdisc->q_ops = __rtnl_qdisc_lookup_ops(name);
}

char *rtnl_qdisc_get_kind(struct rtnl_qdisc *qdisc)
{
	return tca_get_kind((struct rtnl_tca *) qdisc);
}

uint64_t rtnl_qdisc_get_stat(struct rtnl_qdisc *qdisc,
			     enum rtnl_tc_stats_id id)
{
	return tca_get_stat((struct rtnl_tca *) qdisc, id);
}

/** @} */

/**
 * @name Qdisc Specific Options
 * @{
 */

/**
 * Return qdisc specific options for use in TCA_OPTIONS
 * @arg qdisc		qdisc carrying the optiosn
 * 
 * @return new headerless netlink message carrying the options as payload
 */
struct nl_msg *rtnl_qdisc_get_opts(struct rtnl_qdisc *qdisc)
{
	struct rtnl_qdisc_ops *ops;

	ops = rtnl_qdisc_lookup_ops(qdisc);
	if (ops && ops->qo_get_opts)
		return ops->qo_get_opts(qdisc);

	return NULL;
}

/** @} */

struct nl_object_ops qdisc_obj_ops = {
	.oo_name		= "route/qdisc",
	.oo_size		= sizeof(struct rtnl_qdisc),
	.oo_free_data		= qdisc_free_data,
	.oo_clone		= qdisc_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= qdisc_dump_line,
	    [NL_DUMP_DETAILS]	= qdisc_dump_details,
	    [NL_DUMP_STATS]	= qdisc_dump_stats,
	},
	.oo_compare		= tca_compare,
	.oo_id_attrs		= (TCA_ATTR_IFINDEX | TCA_ATTR_HANDLE),
};

/** @} */
