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
#ifndef NETLINK_XFRM_LTIME_H_
#define NETLINK_XFRM_LTIME_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <linux/xfrm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xfrmnl_ltime_cfg;

/* Creation */
extern struct xfrmnl_ltime_cfg* xfrmnl_ltime_cfg_alloc(void);
extern struct xfrmnl_ltime_cfg* xfrmnl_ltime_cfg_clone(struct xfrmnl_ltime_cfg*);

/* Usage Management */
extern struct xfrmnl_ltime_cfg* xfrmnl_ltime_cfg_get(struct xfrmnl_ltime_cfg*);
extern void                     xfrmnl_ltime_cfg_put(struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_shared(struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_cmp(struct xfrmnl_ltime_cfg*, struct xfrmnl_ltime_cfg*);

/* Access Functions */
extern unsigned long long       xfrmnl_ltime_cfg_get_soft_bytelimit (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_soft_bytelimit (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_hard_bytelimit (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_hard_bytelimit (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_soft_packetlimit (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_soft_packetlimit (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_hard_packetlimit (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_hard_packetlimit (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_soft_addexpires (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_soft_addexpires (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_hard_addexpires (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_hard_addexpires (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_soft_useexpires (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_soft_useexpires (struct xfrmnl_ltime_cfg*, unsigned long long);

extern unsigned long long       xfrmnl_ltime_cfg_get_hard_useexpires (struct xfrmnl_ltime_cfg*);
extern int                      xfrmnl_ltime_cfg_set_hard_useexpires (struct xfrmnl_ltime_cfg*, unsigned long long);

#ifdef __cplusplus
}
#endif

#endif
