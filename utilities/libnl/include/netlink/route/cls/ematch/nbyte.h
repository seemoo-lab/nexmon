/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2010 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CLS_EMATCH_NBYTE_H_
#define NETLINK_CLS_EMATCH_NBYTE_H_

#include <netlink/netlink.h>
#include <netlink/route/cls/ematch.h>
#include <linux/tc_ematch/tc_em_nbyte.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void		rtnl_ematch_nbyte_set_offset(struct rtnl_ematch *,
						     uint8_t, uint16_t);
extern uint16_t		rtnl_ematch_nbyte_get_offset(struct rtnl_ematch *);
extern uint8_t		rtnl_ematch_nbyte_get_layer(struct rtnl_ematch *);
extern void		rtnl_ematch_nbyte_set_pattern(struct rtnl_ematch *,
						      uint8_t *, size_t);
extern uint8_t *	rtnl_ematch_nbyte_get_pattern(struct rtnl_ematch *);
extern size_t		rtnl_ematch_nbyte_get_len(struct rtnl_ematch *);

#ifdef __cplusplus
}
#endif

#endif
