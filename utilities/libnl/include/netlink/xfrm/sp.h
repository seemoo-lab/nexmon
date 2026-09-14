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
#ifndef NETLINK_XFRM_SP_H_
#define NETLINK_XFRM_SP_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <netlink/xfrm/template.h>
#include <netlink/xfrm/lifetime.h>
#include <linux/xfrm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xfrmnl_sp;

extern struct xfrmnl_sp*        xfrmnl_sp_alloc(void);
extern void                     xfrmnl_sp_put(struct xfrmnl_sp *);

extern int                      xfrmnl_sp_alloc_cache(struct nl_sock *, struct nl_cache **);
extern struct xfrmnl_sp*        xfrmnl_sp_get(struct nl_cache*, unsigned int, unsigned int);

extern int                      xfrmnl_sp_parse(struct nlmsghdr *n, struct xfrmnl_sp **result);

extern int                      xfrmnl_sp_build_get_request(unsigned int, unsigned int, unsigned int,
                                                            unsigned int, struct nl_msg **);
extern int                      xfrmnl_sp_get_kernel(struct nl_sock*, unsigned int, unsigned int,
                                                     unsigned int, unsigned int, struct xfrmnl_sp**);

extern int                      xfrmnl_sp_add(struct nl_sock*, struct xfrmnl_sp*, int);
extern int                      xfrmnl_sp_build_add_request(struct xfrmnl_sp*, int, struct nl_msg **);

extern int                      xfrmnl_sp_update(struct nl_sock*, struct xfrmnl_sp*, int);
extern int                      xfrmnl_sp_build_update_request(struct xfrmnl_sp*, int, struct nl_msg **);

extern int                      xfrmnl_sp_delete(struct nl_sock*, struct xfrmnl_sp*, int);
extern int                      xfrmnl_sp_build_delete_request(struct xfrmnl_sp*, int, struct nl_msg **);

extern struct xfrmnl_sel*       xfrmnl_sp_get_sel (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_sel (struct xfrmnl_sp*, struct xfrmnl_sel*);

extern struct xfrmnl_ltime_cfg* xfrmnl_sp_get_lifetime_cfg (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_lifetime_cfg (struct xfrmnl_sp*, struct xfrmnl_ltime_cfg*);

extern int                      xfrmnl_sp_get_curlifetime (struct xfrmnl_sp*, unsigned long long int*,
                                                           unsigned long long int*, unsigned long long int*,
                                                           unsigned long long int*);

extern int                      xfrmnl_sp_get_priority (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_priority (struct xfrmnl_sp*, unsigned int);

extern int                      xfrmnl_sp_get_index (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_index (struct xfrmnl_sp*, unsigned int);

extern int                      xfrmnl_sp_get_dir (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_dir (struct xfrmnl_sp*, unsigned int);

extern int                      xfrmnl_sp_get_action (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_action (struct xfrmnl_sp*, unsigned int);

extern int                      xfrmnl_sp_get_flags (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_flags (struct xfrmnl_sp*, unsigned int);

extern int                      xfrmnl_sp_get_share (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_share (struct xfrmnl_sp*, unsigned int);

extern int                      xfrmnl_sp_get_sec_ctx (struct xfrmnl_sp*, unsigned int*, unsigned int*,
                                                       unsigned int*, unsigned int*, unsigned int*, char*);
extern int                      xfrmnl_sp_set_sec_ctx (struct xfrmnl_sp*, unsigned int, unsigned int,
                                                       unsigned int, unsigned int, unsigned int, char*);

extern int                      xfrmnl_sp_get_userpolicy_type (struct xfrmnl_sp*);
extern int                      xfrmnl_sp_set_userpolicy_type (struct xfrmnl_sp*, unsigned int);

extern void                     xfrmnl_sp_add_usertemplate(struct xfrmnl_sp*, struct xfrmnl_user_tmpl*);
extern void                     xfrmnl_sp_remove_usertemplate(struct xfrmnl_sp*, struct xfrmnl_user_tmpl*);
extern struct nl_list_head*     xfrmnl_sp_get_usertemplates(struct xfrmnl_sp*);
extern int                      xfrmnl_sp_get_nusertemplates(struct xfrmnl_sp*);
extern void                     xfrmnl_sp_foreach_usertemplate(struct xfrmnl_sp*,
                                                               void (*cb)(struct xfrmnl_user_tmpl*, void *),
                                                               void *arg);
extern struct xfrmnl_user_tmpl* xfrmnl_sp_usertemplate_n(struct xfrmnl_sp*, int);

extern int                      xfrmnl_sp_get_mark (struct xfrmnl_sp*, unsigned int*, unsigned int*);
extern int                      xfrmnl_sp_set_mark (struct xfrmnl_sp*, unsigned int, unsigned int);

extern int                      xfrmnl_sp_get_if_id (struct xfrmnl_sp*, uint32_t*);
extern int                      xfrmnl_sp_set_if_id (struct xfrmnl_sp*, uint32_t);

extern char*                    xfrmnl_sp_action2str(int, char *, size_t);
extern int                      xfrmnl_sp_str2action(const char *);

extern char*                    xfrmnl_sp_flags2str(int, char *, size_t);
extern int                      xfrmnl_sp_str2flag(const char *);

extern char*                    xfrmnl_sp_type2str(int, char *, size_t);
extern int                      xfrmnl_sp_str2type(const char *);

extern char*                    xfrmnl_sp_dir2str(int, char *, size_t);
extern int                      xfrmnl_sp_str2dir(const char *);

extern char*                    xfrmnl_sp_share2str(int, char *, size_t);
extern int                      xfrmnl_sp_str2share(const char *);

extern int                      xfrmnl_sp_index2dir (unsigned int);


#ifdef __cplusplus
}
#endif

#endif
