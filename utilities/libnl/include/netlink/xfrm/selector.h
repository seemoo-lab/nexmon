/*
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/
#ifndef NETLINK_XFRM_SEL_H_
#define NETLINK_XFRM_SEL_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <linux/xfrm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xfrmnl_sel;

/* Creation */
extern struct xfrmnl_sel*       xfrmnl_sel_alloc(void);
extern struct xfrmnl_sel*       xfrmnl_sel_clone(struct xfrmnl_sel*);

/* Usage Management */
extern struct xfrmnl_sel*       xfrmnl_sel_get(struct xfrmnl_sel*);
extern void                     xfrmnl_sel_put(struct xfrmnl_sel*);
extern int                      xfrmnl_sel_shared(struct xfrmnl_sel*);
extern int                      xfrmnl_sel_cmp(struct xfrmnl_sel*, struct xfrmnl_sel*);
extern void                     xfrmnl_sel_dump(struct xfrmnl_sel*, struct nl_dump_params *);

/* Access Functions */
extern struct nl_addr*          xfrmnl_sel_get_daddr (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_daddr (struct xfrmnl_sel*, struct nl_addr*);

extern struct nl_addr*          xfrmnl_sel_get_saddr (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_saddr (struct xfrmnl_sel*, struct nl_addr*);

extern int                      xfrmnl_sel_get_dport (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_dport (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_dportmask (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_dportmask (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_sport (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_sport (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_sportmask (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_sportmask (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_family (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_family (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_prefixlen_d (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_prefixlen_d (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_prefixlen_s (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_prefixlen_s (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_proto (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_proto (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_ifindex (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_ifindex (struct xfrmnl_sel*, unsigned int);

extern int                      xfrmnl_sel_get_userid (struct xfrmnl_sel*);
extern int                      xfrmnl_sel_set_userid (struct xfrmnl_sel*, unsigned int);

#ifdef __cplusplus
}
#endif

#endif
