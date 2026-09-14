/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2018 Volodymyr Bendiuga <volodymyr.bendiuga@gmail.com>
 */

/**
 * @ingroup act
 * @defgroup act_vlan VLAN Manipulation
 *
 * @{
 */

#include "nl-default.h"

#include <linux/tc_act/tc_vlan.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/route/act/vlan.h>

#include "tc-api.h"

struct rtnl_vlan
{
	struct tc_vlan v_parm;
	uint16_t       v_vid;
	uint16_t       v_proto;
	uint8_t        v_prio;
	uint32_t       v_flags;
};

#define VLAN_F_VID   (1 << 0)
#define VLAN_F_PROTO (1 << 1)
#define VLAN_F_PRIO  (1 << 2)
#define VLAN_F_ACT   (1 << 3)
#define VLAN_F_MODE  (1 << 4)

static struct nla_policy vlan_policy[TCA_VLAN_MAX + 1] = {
	[TCA_VLAN_PARMS]                = { .minlen = sizeof(struct tc_vlan) },
	[TCA_VLAN_PUSH_VLAN_ID]         = { .type = NLA_U16 },
	[TCA_VLAN_PUSH_VLAN_PROTOCOL]   = { .type = NLA_U16 },
	[TCA_VLAN_PUSH_VLAN_PRIORITY]   = { .type = NLA_U8 },
};

static int vlan_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_vlan *v = data;
	struct nlattr *tb[TCA_VLAN_MAX + 1];
	int err;

	err = tca_parse(tb, TCA_VLAN_MAX, tc, vlan_policy);
	if (err < 0)
		return err;

	v->v_flags = 0;
	if (!tb[TCA_VLAN_PARMS])
		return -NLE_MISSING_ATTR;
	else {
		nla_memcpy(&v->v_parm, tb[TCA_VLAN_PARMS], sizeof(v->v_parm));
		v->v_flags |= VLAN_F_ACT;
		v->v_flags |= VLAN_F_MODE;
	}

	if (tb[TCA_VLAN_PUSH_VLAN_ID]) {
		v->v_vid = nla_get_u16(tb[TCA_VLAN_PUSH_VLAN_ID]);
		v->v_flags |= VLAN_F_VID;
	}

	if (tb[TCA_VLAN_PUSH_VLAN_PROTOCOL]) {
		v->v_proto = nla_get_u16(tb[TCA_VLAN_PUSH_VLAN_PROTOCOL]);
		v->v_flags |= VLAN_F_PROTO;
	}

	if (tb[TCA_VLAN_PUSH_VLAN_PRIORITY]) {
		v->v_prio = nla_get_u8(tb[TCA_VLAN_PUSH_VLAN_PRIORITY]);
		v->v_flags |= VLAN_F_PRIO;
	}

	return 0;
}

static int vlan_msg_fill(struct rtnl_tc *tc, void *data, struct nl_msg *msg)
{
	struct rtnl_vlan *v = data;

	if (!v)
		return 0;
	if (!(v->v_flags & VLAN_F_MODE))
		return -NLE_MISSING_ATTR;

	NLA_PUT(msg, TCA_VLAN_PARMS, sizeof(v->v_parm), &v->v_parm);

	/* vid is required for PUSH & MODIFY modes */
	if ((v->v_parm.v_action != TCA_VLAN_ACT_POP) && !(v->v_flags & VLAN_F_VID))
		return -NLE_MISSING_ATTR;

	if (v->v_flags & VLAN_F_VID)
		NLA_PUT_U16(msg, TCA_VLAN_PUSH_VLAN_ID, v->v_vid);

	if (v->v_flags & VLAN_F_PROTO)
		NLA_PUT_U16(msg, TCA_VLAN_PUSH_VLAN_PROTOCOL, v->v_proto);

	if (v->v_flags & VLAN_F_PRIO)
		NLA_PUT_U8(msg, TCA_VLAN_PUSH_VLAN_PRIORITY, v->v_prio);

	return 0;

nla_put_failure:
	return -NLE_NOMEM;
}

static void vlan_free_data(struct rtnl_tc *tc, void *data)
{
}

static void vlan_dump_line(struct rtnl_tc *tc, void *data,
                           struct nl_dump_params *p)
{
	struct rtnl_vlan *v = data;

	if (!v)
		return;

	if (!(v->v_flags & VLAN_F_ACT))
		return;

	if (TC_ACT_EXT_CMP(v->v_parm.action, TC_ACT_GOTO_CHAIN))
		nl_dump(p, " goto chain %u", v->v_parm.action & TC_ACT_EXT_VAL_MASK);

	if (TC_ACT_EXT_CMP(v->v_parm.action, TC_ACT_JUMP))
		nl_dump(p, " jump %u", v->v_parm.action & TC_ACT_EXT_VAL_MASK);

	switch(v->v_parm.action){
	case TC_ACT_UNSPEC:
		nl_dump(p, " unspecified");
		break;
	case TC_ACT_PIPE:
		nl_dump(p, " pipe");
		break;
	case TC_ACT_STOLEN:
		nl_dump(p, " stolen");
		break;
	case TC_ACT_SHOT:
		nl_dump(p, " shot");
		break;
	case TC_ACT_QUEUED:
		nl_dump(p, " queued");
		break;
	case TC_ACT_REPEAT:
		nl_dump(p, " repeat");
		break;
	}
}

static void vlan_dump_details(struct rtnl_tc *tc, void *data,
                              struct nl_dump_params *p)
{
	struct rtnl_vlan *v = data;

	if (!v)
		return;

	if (v->v_flags & VLAN_F_MODE) {
		switch (v->v_parm.v_action) {
		case TCA_VLAN_ACT_POP:
			nl_dump(p, " mode POP");
			break;
		case TCA_VLAN_ACT_PUSH:
			nl_dump(p, " mode PUSH");
			break;
		case TCA_VLAN_ACT_MODIFY:
			nl_dump(p, " mode MODIFY");
			break;
		}
	}

	if (v->v_flags & VLAN_F_VID)
		nl_dump(p, " vlan id %u", v->v_vid);

	if (v->v_flags & VLAN_F_PRIO)
		nl_dump(p, " priority %u", v->v_prio);

	if (v->v_flags & VLAN_F_PROTO)
		nl_dump(p, " protocol %u", v->v_proto);
}

/**
 * @name Attribute Modifications
 * @{
 */

/**
 * Set vlan mode
 * @arg act             vlan action
 * @arg mode            one of (TCA_VLAN_ACT_*: POP, PUSH, MODIFY)
 * @return 0 on success or a negative error code.
 */
int rtnl_vlan_set_mode(struct rtnl_act *act, int mode)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	if (mode > TCA_VLAN_ACT_MODIFY)
		return -NLE_RANGE;

	v->v_parm.v_action = mode;
	v->v_flags |= VLAN_F_MODE;

	return 0;
}

/**
 * Get vlan mode
 * @arg act             vlan action
 * @arg out_mode        vlan mode output paramter
 * @return 0 on success if the vlan mode was returned or a negative error code.
*/
int rtnl_vlan_get_mode(struct rtnl_act *act, int *out_mode)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data_peek(TC_CAST(act))))
		return -NLE_INVAL;

	if (!(v->v_flags & VLAN_F_MODE))
		return -NLE_MISSING_ATTR;

	*out_mode = v->v_parm.v_action;
	return 0;
}

/**
 * Set general action
 * @arg act             vlan action
 * @arg action          one of (TCA_ACT_*: PIPE, SHOT, GOTO_CHAIN, etc)
 * @return 0 on success or a negative error code.
 */
int rtnl_vlan_set_action(struct rtnl_act *act, int action)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	v->v_parm.action = action;
	v->v_flags |= VLAN_F_ACT;

	return 0;
}

/**
 * Get general action
 * @arg act             vlan action
 * @arg out_action      output parameter
 * @return general 0 if out_action was set or a negative error code.
*/
int rtnl_vlan_get_action(struct rtnl_act *act, int *out_action)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data_peek(TC_CAST(act))))
		return -NLE_INVAL;

	if (!(v->v_flags & VLAN_F_ACT))
		return -NLE_MISSING_ATTR;

	*out_action = v->v_parm.action;
	return 0;
}

/**
 * Set protocol
 * @arg act             vlan action
 * @arg protocol        one of (ETH_P_8021Q || ETH_P_8021AD)
 * @return 0 on success or a negative error code.
 */
int rtnl_vlan_set_protocol(struct rtnl_act *act, uint16_t protocol)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	v->v_proto = protocol;
	v->v_flags |= VLAN_F_PROTO;

	return 0;
}

/**
 * Get protocol
 * @arg act             vlan action
 * @arg out_protocol    protocol output argument
 * @return 0 if the protocol was returned or a negative error code.
*/
int rtnl_vlan_get_protocol(struct rtnl_act *act, uint16_t *out_protocol)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data_peek(TC_CAST(act))))
		return -NLE_INVAL;

	if (!(v->v_flags & VLAN_F_PROTO))
		return -NLE_MISSING_ATTR;

	*out_protocol = v->v_proto;
	return 0;
}

/**
 * Set vlan id
 * @arg act             vlan action
 * @arg vid             vlan id
 * @return 0 on success or a negative error code.
 */
int rtnl_vlan_set_vlan_id(struct rtnl_act *act, uint16_t vid)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	if (vid > 4095)
		return -NLE_RANGE;

	v->v_vid = vid;
	v->v_flags |= VLAN_F_VID;

	return 0;
}

/**
 * Get vlan id
 * @arg act             vlan action
 * @arg out_vid         output vlan id
 * @return 0 if the vlan id was returned or a negative error code.
*/
int rtnl_vlan_get_vlan_id(struct rtnl_act *act, uint16_t *out_vid)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data_peek(TC_CAST(act))))
		return -NLE_INVAL;

	if (!(v->v_flags & VLAN_F_VID))
		return -NLE_MISSING_ATTR;

	*out_vid = v->v_vid;
	return 0;
}

/**
 * Set vlan prio
 * @arg act             vlan action
 * @arg prio            vlan priority (0 - 7)
 * @return 0 on success or a negative error code.
 */
int rtnl_vlan_set_vlan_prio(struct rtnl_act *act, uint8_t prio)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	if (prio > 7)
		return -NLE_RANGE;

	v->v_prio = prio;
	v->v_flags |= VLAN_F_PRIO;

	return 0;
}

/**
 * Get vlan prio
 * @arg act             vlan action
 * @arg out_prio        the output vlan prio
 * @return 0 if the vlan prio was returned or a negative error code.
*/
int rtnl_vlan_get_vlan_prio(struct rtnl_act *act, uint8_t *out_prio)
{
	struct rtnl_vlan *v;

	if (!(v = (struct rtnl_vlan *) rtnl_tc_data_peek(TC_CAST(act))))
		return -NLE_INVAL;

	if (!(v->v_flags & VLAN_F_PRIO))
		return -NLE_MISSING_ATTR;

	*out_prio = v->v_prio;
	return 0;
}

/** @} */

static struct rtnl_tc_ops vlan_ops = {
	.to_kind                = "vlan",
	.to_type                = RTNL_TC_TYPE_ACT,
	.to_size                = sizeof(struct rtnl_vlan),
	.to_msg_parser          = vlan_msg_parser,
	.to_free_data           = vlan_free_data,
	.to_clone               = NULL,
	.to_msg_fill            = vlan_msg_fill,
	.to_dump = {
	    [NL_DUMP_LINE]      = vlan_dump_line,
	    [NL_DUMP_DETAILS]   = vlan_dump_details,
	},
};

static void _nl_init vlan_init(void)
{
	rtnl_tc_register(&vlan_ops);
}

static void _nl_exit vlan_exit(void)
{
	rtnl_tc_unregister(&vlan_ops);
}

/** @} */
