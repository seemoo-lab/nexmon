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
#ifndef NETLINK_XFRM_SA_H_
#define NETLINK_XFRM_SA_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <linux/xfrm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xfrmnl_sa;

extern struct xfrmnl_sa*        xfrmnl_sa_alloc(void);
extern void                     xfrmnl_sa_put(struct xfrmnl_sa *);

extern int                      xfrmnl_sa_alloc_cache(struct nl_sock *, struct nl_cache **);
extern struct xfrmnl_sa*        xfrmnl_sa_get(struct nl_cache*, struct nl_addr*, unsigned int, unsigned int);

extern int                      xfrmnl_sa_parse(struct nlmsghdr *n, struct xfrmnl_sa **result);

extern int                      xfrmnl_sa_build_get_request(struct nl_addr*, unsigned int, unsigned int,
                                                            unsigned int, unsigned int, struct nl_msg **);
extern int                      xfrmnl_sa_get_kernel(struct nl_sock*, struct nl_addr*, unsigned int,
                                                     unsigned int, unsigned int, unsigned int, struct xfrmnl_sa**);

extern int                      xfrmnl_sa_build_add_request(struct xfrmnl_sa*, int, struct nl_msg **);
extern int                      xfrmnl_sa_add(struct nl_sock*, struct xfrmnl_sa*, int);

extern int                      xfrmnl_sa_build_update_request(struct xfrmnl_sa*, int, struct nl_msg **);
extern int                      xfrmnl_sa_update(struct nl_sock*, struct xfrmnl_sa*, int);

extern int                      xfrmnl_sa_build_delete_request(struct xfrmnl_sa*, int, struct nl_msg **);
extern int                      xfrmnl_sa_delete(struct nl_sock*, struct xfrmnl_sa*, int);

extern struct xfrmnl_sel*       xfrmnl_sa_get_sel (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_sel (struct xfrmnl_sa*, struct xfrmnl_sel*);

extern struct nl_addr*          xfrmnl_sa_get_daddr (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_daddr (struct xfrmnl_sa*, struct nl_addr*);

extern int                      xfrmnl_sa_get_spi (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_spi (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_proto (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_proto (struct xfrmnl_sa*, unsigned int);

extern struct nl_addr*          xfrmnl_sa_get_saddr (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_saddr (struct xfrmnl_sa*, struct nl_addr*);

extern struct xfrmnl_ltime_cfg* xfrmnl_sa_get_lifetime_cfg (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_lifetime_cfg (struct xfrmnl_sa*, struct xfrmnl_ltime_cfg*);

extern int                      xfrmnl_sa_get_curlifetime (struct xfrmnl_sa*, unsigned long long int*,
                                                           unsigned long long int*, unsigned long long int*,
                                                           unsigned long long int*);

extern int                      xfrmnl_sa_get_stats (struct xfrmnl_sa*, unsigned long long int*,
                                                     unsigned long long int*, unsigned long long int*);

extern int                      xfrmnl_sa_get_seq (struct xfrmnl_sa*);

extern int                      xfrmnl_sa_get_reqid (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_reqid (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_family (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_family (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_mode (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_mode (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_replay_window (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_replay_window (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_flags (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_flags (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_aead_params (struct xfrmnl_sa*, char*, unsigned int*,
                                                           unsigned int*, char*);
extern int                      xfrmnl_sa_set_aead_params (struct xfrmnl_sa*, const char*, unsigned int,
                                                           unsigned int, const char*);

extern int                      xfrmnl_sa_get_auth_params (struct xfrmnl_sa*, char*, unsigned int*,
                                                           unsigned int*, char*);
extern int                      xfrmnl_sa_set_auth_params (struct xfrmnl_sa*, const char*, unsigned int,
                                                           unsigned int, const char*);

extern int                      xfrmnl_sa_get_crypto_params (struct xfrmnl_sa*, char*, unsigned int*, char*);
extern int                      xfrmnl_sa_set_crypto_params (struct xfrmnl_sa*, const char*, unsigned int,
                                                             const char*);

extern int                      xfrmnl_sa_get_comp_params (struct xfrmnl_sa*, char*, unsigned int*, char*);
extern int                      xfrmnl_sa_set_comp_params (struct xfrmnl_sa*, const char*, unsigned int,
                                                           const char*);

extern int                      xfrmnl_sa_get_encap_tmpl (struct xfrmnl_sa*, unsigned int*, unsigned int*,
                                                          unsigned int*, struct nl_addr**);
extern int                      xfrmnl_sa_set_encap_tmpl (struct xfrmnl_sa*, unsigned int, unsigned int,
                                                          unsigned int, struct nl_addr*);

extern int                      xfrmnl_sa_get_tfcpad (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_tfcpad (struct xfrmnl_sa*, unsigned int);

extern struct nl_addr*          xfrmnl_sa_get_coaddr (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_coaddr (struct xfrmnl_sa*, struct nl_addr*);

extern int                      xfrmnl_sa_get_mark (struct xfrmnl_sa*, unsigned int*, unsigned int*);
extern int                      xfrmnl_sa_set_mark (struct xfrmnl_sa*, unsigned int, unsigned int);

extern int                      xfrmnl_sa_get_if_id (struct xfrmnl_sa*, uint32_t*);
extern int                      xfrmnl_sa_set_if_id (struct xfrmnl_sa*, uint32_t);

extern int                      xfrmnl_sa_get_sec_ctx (struct xfrmnl_sa*, unsigned int*, unsigned int*,
                                                       unsigned int*, unsigned int*, char*);
extern int                      xfrmnl_sa_set_sec_ctx (struct xfrmnl_sa*, unsigned int, unsigned int,
                                                       unsigned int, unsigned int, const char*);

extern int                      xfrmnl_sa_get_replay_maxage (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_replay_maxage (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_replay_maxdiff (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_set_replay_maxdiff (struct xfrmnl_sa*, unsigned int);

extern int                      xfrmnl_sa_get_replay_state (struct xfrmnl_sa*, unsigned int*,
                                                            unsigned int*, unsigned int*);
extern int                      xfrmnl_sa_set_replay_state (struct xfrmnl_sa*, unsigned int,
                                                            unsigned int, unsigned int);

extern int                      xfrmnl_sa_get_replay_state_esn (struct xfrmnl_sa*, unsigned int*, unsigned int*,
                                                                unsigned int*, unsigned int*, unsigned int*,
                                                                unsigned int*, unsigned int*);
extern int                      xfrmnl_sa_set_replay_state_esn (struct xfrmnl_sa*, unsigned int, unsigned int,
                                                                unsigned int, unsigned int, unsigned int,
                                                                unsigned int, unsigned int*);

extern int                      xfrmnl_sa_get_user_offload (struct xfrmnl_sa*, int*, uint8_t *);
extern int                      xfrmnl_sa_set_user_offload (struct xfrmnl_sa*, int, uint8_t);

extern int                      xfrmnl_sa_is_expiry_reached (struct xfrmnl_sa*);
extern int                      xfrmnl_sa_is_hardexpiry_reached (struct xfrmnl_sa*);

extern char*                    xfrmnl_sa_flags2str(int, char *, size_t);
extern int                      xfrmnl_sa_str2flag(const char *);

extern char*                    xfrmnl_sa_mode2str(int, char *, size_t);
extern int                      xfrmnl_sa_str2mode(const char *);

#ifdef __cplusplus
}
#endif

#endif
