/*
 * lib/route/qdisc_api.c            Queueing Discipline Module API
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
 * @defgroup qdisc_api Queueing Discipline Modules
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

static struct rtnl_qdisc_ops *qdisc_ops_list;

/**
 * @name Module API
 * @{
 */

/**
 * Register a qdisc module
 * @arg qops		qdisc module operations
 */
int rtnl_qdisc_register(struct rtnl_qdisc_ops *qops)
{
	struct rtnl_qdisc_ops *o, **op;

	if (!qops->qo_kind[0])
		BUG();

	for (op = &qdisc_ops_list; (o = *op) != NULL; op = &o->qo_next)
		if (!strcasecmp(qops->qo_kind, o->qo_kind))
			return -NLE_EXIST;

	qops->qo_next = NULL;
	*op = qops;

	return 0;
}

/**
 * Unregister a qdisc module
 * @arg qops		qdisc module operations
 */
int rtnl_qdisc_unregister(struct rtnl_qdisc_ops *qops)
{
	struct rtnl_qdisc_ops *o, **op;

	for (op = &qdisc_ops_list; (o = *op) != NULL; op = &o->qo_next)
		if (!strcasecmp(qops->qo_kind, o->qo_kind))
			break;

	if (!o)
		return -NLE_OBJ_NOTFOUND;

	*op = qops->qo_next;

	return 0;
}

struct rtnl_qdisc_ops *__rtnl_qdisc_lookup_ops(const char *kind)
{
	struct rtnl_qdisc_ops *qops;

	for (qops = qdisc_ops_list; qops; qops = qops->qo_next)
		if (!strcmp(kind, qops->qo_kind))
			return qops;

	return NULL;
}

struct rtnl_qdisc_ops *rtnl_qdisc_lookup_ops(struct rtnl_qdisc *qdisc)
{
	if (!qdisc->q_ops)
		qdisc->q_ops = __rtnl_qdisc_lookup_ops(qdisc->q_kind);

	return qdisc->q_ops;
}

/** @} */

/** @} */
