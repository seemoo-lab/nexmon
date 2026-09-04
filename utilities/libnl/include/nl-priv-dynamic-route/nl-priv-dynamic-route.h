/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#ifndef __NL_PRIVATE_TYPES_NL_ROUTE_H__
#define __NL_PRIVATE_TYPES_NL_ROUTE_H__

struct rtnl_ematch {
	uint16_t e_id;
	uint16_t e_kind;
	uint16_t e_flags;
	uint16_t e_index;
	size_t e_datalen;

	struct nl_list_head e_childs;
	struct nl_list_head e_list;
	struct rtnl_ematch_ops *e_ops;

	void *e_data;
};

struct rtnl_ematch_tree {
	uint16_t et_progid;
	struct nl_list_head et_list;
};

/*****************************************************************************/

/**
 * Traffic control object operations
 * @ingroup tc
 *
 * This structure holds function pointers and settings implementing
 * the features of each traffic control object implementation.
 */
struct rtnl_tc_ops {
	/**
	 * Name of traffic control module
	 */
	char *to_kind;

	/**
	 * Type of traffic control object
	 */
	enum rtnl_tc_type to_type;

	/**
	 * Size of private data
	 */
	size_t to_size;

	/**
	 * Dump callbacks
	 */
	void (*to_dump[NL_DUMP_MAX + 1])(struct rtnl_tc *, void *,
					 struct nl_dump_params *);
	/**
	 * Used to fill the contents of TCA_OPTIONS
	 */
	int (*to_msg_fill)(struct rtnl_tc *, void *, struct nl_msg *);

	/**
	 * Uesd to to fill tc related messages, unlike with to_msg_fill,
	 * the contents is not encapsulated with a TCA_OPTIONS nested
	 * attribute.
	 */
	int (*to_msg_fill_raw)(struct rtnl_tc *, void *, struct nl_msg *);

	/**
	 * TCA_OPTIONS message parser
	 */
	int (*to_msg_parser)(struct rtnl_tc *, void *);

	/**
	 * Called before a tc object is destroyed
	 */
	void (*to_free_data)(struct rtnl_tc *, void *);

	/**
	 * Called whenever a classifier object needs to be cloned
	 */
	int (*to_clone)(void *, void *);

	/**
	 * Internal, don't touch
	 */
	struct nl_list_head to_list;
};

extern struct rtnl_tc_ops *rtnl_tc_lookup_ops(enum rtnl_tc_type, const char *);

struct rtnl_tc_ops *rtnl_tc_get_ops(struct rtnl_tc *);

struct rtnl_nexthop {
	uint8_t rtnh_flags;
	uint8_t rtnh_flag_mask;
	uint8_t rtnh_weight;
	/* 1 byte spare */
	uint32_t rtnh_ifindex;
	struct nl_addr *rtnh_gateway;
	uint32_t ce_mask; /* HACK to support attr macros */
	struct nl_list_head rtnh_list;
	uint32_t rtnh_realms;
	struct nl_addr *rtnh_newdst;
	struct nl_addr *rtnh_via;
	struct rtnl_nh_encap *rtnh_encap;
};

#endif /* __NL_PRIVATE_TYPES_NL_ROUTE_H__ */
