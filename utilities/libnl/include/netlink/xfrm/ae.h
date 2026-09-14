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
#ifndef NETLINK_XFRM_AE_H_
#define NETLINK_XFRM_AE_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <linux/xfrm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xfrmnl_ae;

extern struct xfrmnl_ae*xfrmnl_ae_alloc(void);
extern void             xfrmnl_ae_put(struct xfrmnl_ae *);

extern int              xfrmnl_ae_get_kernel(struct nl_sock*, struct nl_addr*, unsigned int, unsigned int,
                                             unsigned int, unsigned int, struct xfrmnl_ae**);
extern int              xfrmnl_ae_set(struct nl_sock*, struct xfrmnl_ae*, int);

extern int              xfrmnl_ae_parse(struct nlmsghdr*, struct xfrmnl_ae **);
extern int              xfrmnl_ae_build_get_request(struct nl_addr*, unsigned int, unsigned int,
                                                    unsigned int, unsigned int, struct nl_msg **);

extern struct nl_addr*  xfrmnl_ae_get_daddr (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_daddr (struct xfrmnl_ae*, struct nl_addr*);

extern int              xfrmnl_ae_get_spi (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_spi (struct xfrmnl_ae*, unsigned int);

extern int              xfrmnl_ae_get_family (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_family (struct xfrmnl_ae*, unsigned int);

extern int              xfrmnl_ae_get_proto (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_proto (struct xfrmnl_ae*, unsigned int);

extern struct nl_addr*  xfrmnl_ae_get_saddr (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_saddr (struct xfrmnl_ae*, struct nl_addr*);

extern int              xfrmnl_ae_get_flags (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_flags (struct xfrmnl_ae*, unsigned int);

extern int              xfrmnl_ae_get_reqid (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_reqid (struct xfrmnl_ae*, unsigned int);

extern int              xfrmnl_ae_get_mark (struct xfrmnl_ae*, unsigned int*, unsigned int*);
extern int              xfrmnl_ae_set_mark (struct xfrmnl_ae*, unsigned int, unsigned int);

extern int              xfrmnl_ae_get_curlifetime (struct xfrmnl_ae*, unsigned long long int*,
                                                   unsigned long long int*, unsigned long long int*,
                                                   unsigned long long int*);
extern int              xfrmnl_ae_set_curlifetime (struct xfrmnl_ae*, unsigned long long int,
                                                   unsigned long long int, unsigned long long int,
                                                   unsigned long long int);

extern int              xfrmnl_ae_get_replay_maxage (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_replay_maxage (struct xfrmnl_ae*, unsigned int);

extern int              xfrmnl_ae_get_replay_maxdiff (struct xfrmnl_ae*);
extern int              xfrmnl_ae_set_replay_maxdiff (struct xfrmnl_ae*, unsigned int);

extern int              xfrmnl_ae_get_replay_state (struct xfrmnl_ae*, unsigned int*, unsigned int*, unsigned int*);
extern int              xfrmnl_ae_set_replay_state (struct xfrmnl_ae*, unsigned int, unsigned int, unsigned int);

extern int              xfrmnl_ae_get_replay_state_esn (struct xfrmnl_ae*, unsigned int*, unsigned int*, unsigned int*,
                                                        unsigned int*, unsigned int*, unsigned int*, unsigned int*);
extern int              xfrmnl_ae_set_replay_state_esn (struct xfrmnl_ae*, unsigned int, unsigned int, unsigned int,
                                                        unsigned int, unsigned int, unsigned int, unsigned int*);

extern char*            xfrmnl_ae_flags2str(int, char *, size_t);
extern int              xfrmnl_ae_str2flag(const char *);

#ifdef __cplusplus
}
#endif

#endif
