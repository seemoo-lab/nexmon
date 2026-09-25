/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Beniamino Galvani <bgalvani@redhat.com>
 */

#ifndef NETLINK_LINK_MACVTAP_H_
#define NETLINK_LINK_MACVTAP_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *rtnl_link_macvtap_alloc(void);

extern int               rtnl_link_is_macvtap(struct rtnl_link *);

extern char *            rtnl_link_macvtap_mode2str(int, char *, size_t);
extern int               rtnl_link_macvtap_str2mode(const char *);

extern char *            rtnl_link_macvtap_flags2str(int, char *, size_t);
extern int               rtnl_link_macvtap_str2flags(const char *);

extern int               rtnl_link_macvtap_set_mode(struct rtnl_link *,
                                                    uint32_t);
extern uint32_t          rtnl_link_macvtap_get_mode(struct rtnl_link *);

extern int               rtnl_link_macvtap_set_flags(struct rtnl_link *,
                                                     uint16_t);
extern int               rtnl_link_macvtap_unset_flags(struct rtnl_link *,
                                                       uint16_t);
extern uint16_t          rtnl_link_macvtap_get_flags(struct rtnl_link *);

#ifdef __cplusplus
}
#endif

#endif
