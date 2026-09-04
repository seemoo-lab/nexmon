/*
 * lib/route/cls_api.c       Classifier Module API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup cls
 * @defgroup cls_api Classifier Modules
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/tc.h>
#include <netlink/route/classifier.h>
#include <netlink/route/classifier-modules.h>
#include <netlink/route/link.h>

static struct rtnl_cls_ops *cls_ops_list;

/**
 * @name Classifier Module API
 * @{
 */

/**
 * Register a classifier module
 * @arg cops		classifier module operations
 */
int rtnl_cls_register(struct rtnl_cls_ops *cops)
{
	struct rtnl_cls_ops *o, **op;

	if (!cops->co_kind)
		BUG();

	for (op = &cls_ops_list; (o = *op) != NULL; op = &o->co_next)
		if (!strcasecmp(cops->co_kind, o->co_kind))
			return -NLE_EXIST;

	cops->co_next = NULL;
	*op = cops;

	return 0;
}

/**
 * Unregister a classifier module
 * @arg cops		classifier module operations
 */
int rtnl_cls_unregister(struct rtnl_cls_ops *cops)
{
	struct rtnl_cls_ops *o, **op;

	for (op = &cls_ops_list; (o = *op) != NULL; op = &o->co_next)
		if (!strcasecmp(cops->co_kind, o->co_kind))
			break;

	if (!o)
		return -NLE_OBJ_NOTFOUND;

	*op = cops->co_next;

	return 0;
}

struct rtnl_cls_ops *__rtnl_cls_lookup_ops(const char *kind)
{
	struct rtnl_cls_ops *cops;

	for (cops = cls_ops_list; cops; cops = cops->co_next)
		if (!strcmp(kind, cops->co_kind))
			return cops;

	return NULL;
}

/**
 * Lookup classifier operations for a classifier object
 * @arg cls		Classifier object.
 *
 * @return Classifier operations or NULL if not found.
 */
struct rtnl_cls_ops *rtnl_cls_lookup_ops(struct rtnl_cls *cls)
{
	if (!cls->c_ops)
		cls->c_ops = __rtnl_cls_lookup_ops(cls->c_kind);

	return cls->c_ops;
}


/** @} */

/** @} */
