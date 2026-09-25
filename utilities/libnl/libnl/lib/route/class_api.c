/*
 * lib/route/class_api.c            Queueing Classes Module API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup class
 * @defgroup class_api Class Modules
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/tc.h>
#include <netlink/route/class.h>
#include <netlink/route/class-modules.h>
#include <netlink/utils.h>

static struct rtnl_class_ops *class_ops_list;

/**
 * @name Module API
 * @{
 */

/**
 * Register a class module
 * @arg cops		class module operations
 */
int rtnl_class_register(struct rtnl_class_ops *cops)
{
	struct rtnl_class_ops *o, **op;

	if (!cops->co_kind[0])
		BUG();

	for (op = &class_ops_list; (o = *op) != NULL; op = &o->co_next)
		if (!strcasecmp(cops->co_kind, o->co_kind))
			return -NLE_EXIST;

	cops->co_next = NULL;
	*op = cops;

	return 0;
}

/**
 * Unregister a class module
 * @arg cops		class module operations
 */
int rtnl_class_unregister(struct rtnl_class_ops *cops)
{
	struct rtnl_class_ops *o, **op;

	for (op = &class_ops_list; (o = *op) != NULL; op = &o->co_next)
		if (!strcasecmp(cops->co_kind, o->co_kind))
			break;

	if (!o)
		return -NLE_OBJ_NOTFOUND;

	*op = cops->co_next;

	return 0;
}

struct rtnl_class_ops *__rtnl_class_lookup_ops(const char *kind)
{
	struct rtnl_class_ops *cops;

	for (cops = class_ops_list; cops; cops = cops->co_next)
		if (!strcmp(kind, cops->co_kind))
			return cops;

	return NULL;
}

/**
 * Lookup class operations for a class object
 * @arg class		Class object.
 *
 * @return Class operations or NULL if not found.
 */
struct rtnl_class_ops *rtnl_class_lookup_ops(struct rtnl_class *class)
{
	if (!class->c_ops)
		class->c_ops = __rtnl_class_lookup_ops(class->c_kind);

	return class->c_ops;
}


/** @} */

/** @} */
