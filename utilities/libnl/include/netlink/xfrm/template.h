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
#ifndef NETLINK_XFRM_TEMPL_H_
#define NETLINK_XFRM_TEMPL_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <linux/xfrm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xfrmnl_user_tmpl;

/* Creation */
extern struct xfrmnl_user_tmpl* xfrmnl_user_tmpl_alloc(void);
extern struct xfrmnl_user_tmpl* xfrmnl_user_tmpl_clone(struct xfrmnl_user_tmpl*);
extern void                     xfrmnl_user_tmpl_free(struct xfrmnl_user_tmpl* utmpl);

/* Utility functions */
extern int                      xfrmnl_user_tmpl_cmp(struct xfrmnl_user_tmpl*, struct xfrmnl_user_tmpl*);
extern void                     xfrmnl_user_tmpl_dump(struct xfrmnl_user_tmpl*, struct nl_dump_params*);

/* Access Functions */
extern struct nl_addr*          xfrmnl_user_tmpl_get_daddr (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_daddr (struct xfrmnl_user_tmpl*, struct nl_addr*);

extern int                      xfrmnl_user_tmpl_get_spi (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_spi (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_proto (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_proto (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_family (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_family (struct xfrmnl_user_tmpl*, unsigned int);

extern struct nl_addr*          xfrmnl_user_tmpl_get_saddr (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_saddr (struct xfrmnl_user_tmpl*, struct nl_addr*);

extern int                      xfrmnl_user_tmpl_get_reqid (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_reqid (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_mode (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_mode (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_share (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_share (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_optional (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_optional (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_aalgos (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_aalgos (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_ealgos (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_ealgos (struct xfrmnl_user_tmpl*, unsigned int);

extern int                      xfrmnl_user_tmpl_get_calgos (struct xfrmnl_user_tmpl*);
extern int                      xfrmnl_user_tmpl_set_calgos (struct xfrmnl_user_tmpl*, unsigned int);

extern char*                    xfrmnl_user_tmpl_mode2str(int, char *, size_t);
extern int                      xfrmnl_user_tmpl_str2mode(const char *);

#ifdef __cplusplus
}
#endif

#endif
