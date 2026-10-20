/*
 * lib/route/class.c            Queueing Classes
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup class
 * @defgroup class_obj Class Object
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/tc.h>
#include <netlink/route/class.h>
#include <netlink/route/class-modules.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/classifier.h>
#include <netlink/utils.h>

static void class_free_data(struct nl_object *obj)
{
	struct rtnl_class *class = (struct rtnl_class *) obj;
	struct rtnl_class_ops *cops;
	
	tca_free_data((struct rtnl_tca *) class);

	cops = rtnl_class_lookup_ops(class);
	if (cops && cops->co_free_data)
		cops->co_free_data(class);
}

static int class_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct rtnl_class *dst = nl_object_priv(_dst);
	struct rtnl_class *src = nl_object_priv(_src);
	struct rtnl_class_ops *cops;
	int err;

	err = tca_clone((struct rtnl_tca *) dst, (struct rtnl_tca *) src);
	if (err < 0)
		goto errout;

	cops = rtnl_class_lookup_ops(src);
	if (cops && cops->co_clone)
		err = cops->co_clone(dst, src);
errout:
	return err;
}

static void class_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_class *class = (struct rtnl_class *) obj;
	struct rtnl_class_ops *cops;

	tca_dump_line((struct rtnl_tca *) class, "class", p);

	cops = rtnl_class_lookup_ops(class);
	if (cops && cops->co_dump[NL_DUMP_LINE])
		cops->co_dump[NL_DUMP_LINE](class, p);
	nl_dump(p, "\n");
}

static void class_dump_details(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_class *class = (struct rtnl_class *) obj;
	struct rtnl_class_ops *cops;

	class_dump_line(obj, p);
	tca_dump_details((struct rtnl_tca *) class, p);
	
	if (class->c_info) {
		char buf[32];
		nl_dump(p, "child-qdisc %s ",
			rtnl_tc_handle2str(class->c_info, buf, sizeof(buf)));
	}

	cops = rtnl_class_lookup_ops(class);
	if (cops && cops->co_dump[NL_DUMP_DETAILS])
		cops->co_dump[NL_DUMP_DETAILS](class, p);
	else if (!class->c_info)
		nl_dump(p, "noop (no leaf qdisc)");

	nl_dump(p, "\n");
}

static void class_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_class *class = (struct rtnl_class *) obj;
	struct rtnl_class_ops *cops;

	class_dump_details(obj, p);
	tca_dump_stats((struct rtnl_tca *) class, p);
	nl_dump(p, "\n");

	cops = rtnl_class_lookup_ops(class);
	if (cops && cops->co_dump[NL_DUMP_STATS])
		cops->co_dump[NL_DUMP_STATS](class, p);
}

/**
 * @name Allocation/Freeing
 * @{
 */

struct rtnl_class *rtnl_class_alloc(void)
{
	return (struct rtnl_class *) nl_object_alloc(&class_obj_ops);
}

void rtnl_class_put(struct rtnl_class *class)
{
	nl_object_put((struct nl_object *) class);
}

/** @} */

/**
 * @name Leaf Qdisc
 * @{
 */

/**
 * Lookup the leaf qdisc of a class
 * @arg class		the parent class
 * @arg cache		a qdisc cache including at laest all qdiscs of the
 *                      interface the specified class is attached to
 * @return The qdisc from the cache or NULL if the class has no leaf qdisc
 */
struct rtnl_qdisc *rtnl_class_leaf_qdisc(struct rtnl_class *class,
					 struct nl_cache *cache)
{
	struct rtnl_qdisc *leaf;

	if (!class->c_info)
		return NULL;

	leaf = rtnl_qdisc_get_by_parent(cache, class->c_ifindex,
					class->c_handle);
	if (!leaf || leaf->q_handle != class->c_info)
		return NULL;

	return leaf;
}

/** @} */


/**
 * @name Iterators
 * @{
 */

/**
 * Call a callback for each child of a class
 * @arg class		the parent class
 * @arg cache		a class cache including all classes of the interface
 *                      the specified class is attached to
 * @arg cb              callback function
 * @arg arg             argument to be passed to callback function
 */
void rtnl_class_foreach_child(struct rtnl_class *class, struct nl_cache *cache,
			      void (*cb)(struct nl_object *, void *), void *arg)
{
	struct rtnl_class *filter;
	
	filter = rtnl_class_alloc();
	if (!filter)
		return;

	rtnl_class_set_parent(filter, class->c_handle);
	rtnl_class_set_ifindex(filter, class->c_ifindex);
	rtnl_class_set_kind(filter, class->c_kind);

	nl_cache_foreach_filter(cache, (struct nl_object *) filter, cb, arg);
	rtnl_class_put(filter);
}

/**
 * Call a callback for each classifier attached to the class
 * @arg class		the parent class
 * @arg cache		a filter cache including at least all the filters
 *                      attached to the specified class
 * @arg cb              callback function
 * @arg arg             argument to be passed to callback function
 */
void rtnl_class_foreach_cls(struct rtnl_class *class, struct nl_cache *cache,
			    void (*cb)(struct nl_object *, void *), void *arg)
{
	struct rtnl_cls *filter;

	filter = rtnl_cls_alloc();
	if (!filter)
		return;

	rtnl_cls_set_ifindex(filter, class->c_ifindex);
	rtnl_cls_set_parent(filter, class->c_parent);

	nl_cache_foreach_filter(cache, (struct nl_object *) filter, cb, arg);
	rtnl_cls_put(filter);
}

/** @} */


/**
 * @name Attributes
 * @{
 */

void rtnl_class_set_ifindex(struct rtnl_class *class, int ifindex)
{
	tca_set_ifindex((struct rtnl_tca *) class, ifindex);
}

int rtnl_class_get_ifindex(struct rtnl_class *class)
{
	return tca_get_ifindex((struct rtnl_tca *) class);
}

void rtnl_class_set_handle(struct rtnl_class *class, uint32_t handle)
{
	tca_set_handle((struct rtnl_tca *) class, handle);
}

uint32_t rtnl_class_get_handle(struct rtnl_class *class)
{
	return tca_get_handle((struct rtnl_tca *) class);
}

void rtnl_class_set_parent(struct rtnl_class *class, uint32_t parent)
{
	tca_set_parent((struct rtnl_tca *) class, parent);
}

uint32_t rtnl_class_get_parent(struct rtnl_class *class)
{
	return tca_get_parent((struct rtnl_tca *) class);
}

void rtnl_class_set_kind(struct rtnl_class *class, const char *name)
{
	tca_set_kind((struct rtnl_tca *) class, name);
	class->c_ops = __rtnl_class_lookup_ops(name);
}

char *rtnl_class_get_kind(struct rtnl_class *class)
{
	return tca_get_kind((struct rtnl_tca *) class);
}

uint64_t rtnl_class_get_stat(struct rtnl_class *class,
			     enum rtnl_tc_stats_id id)
{
	return tca_get_stat((struct rtnl_tca *) class, id);
}

/** @} */

struct nl_object_ops class_obj_ops = {
	.oo_name		= "route/class",
	.oo_size		= sizeof(struct rtnl_class),
	.oo_free_data         	= class_free_data,
	.oo_clone		= class_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= class_dump_line,
	    [NL_DUMP_DETAILS]	= class_dump_details,
	    [NL_DUMP_STATS]	= class_dump_stats,
	},
	.oo_compare		= tca_compare,
	.oo_id_attrs		= (TCA_ATTR_IFINDEX | TCA_ATTR_HANDLE),
};

/** @} */
