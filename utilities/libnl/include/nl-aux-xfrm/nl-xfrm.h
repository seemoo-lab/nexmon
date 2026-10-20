/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef __NETLINK_NL_AUX_XFRM_NL_XFRM_H__
#define __NETLINK_NL_AUX_XFRM_NL_XFRM_H__

#include "base/nl-base-utils.h"

struct xfrmnl_sp;
void xfrmnl_sp_put(struct xfrmnl_sp *sp);
#define _nl_auto_xfrmnl_sp _nl_auto(_nl_auto_xfrmnl_sp_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct xfrmnl_sp *, _nl_auto_xfrmnl_sp_fcn,
			   xfrmnl_sp_put);

struct xfrmnl_sa;
void xfrmnl_sa_put(struct xfrmnl_sa *sa);
#define _nl_auto_xfrmnl_sa _nl_auto(_nl_auto_xfrmnl_sa_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct xfrmnl_sa *, _nl_auto_xfrmnl_sa_fcn,
			   xfrmnl_sa_put);

struct xfrmnl_ae;
void xfrmnl_ae_put(struct xfrmnl_ae *ae);
#define _nl_auto_xfrmnl_ae _nl_auto(_nl_auto_xfrmnl_ae_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct xfrmnl_ae *, _nl_auto_xfrmnl_ae_fcn,
			   xfrmnl_ae_put);

struct xfrmnl_user_tmpl;
void xfrmnl_user_tmpl_free(struct xfrmnl_user_tmpl *utmpl);
#define _nl_auto_xfrmnl_user_tmpl _nl_auto(_nl_auto_xfrmnl_user_tmpl_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct xfrmnl_user_tmpl *,
			   _nl_auto_xfrmnl_user_tmpl_fcn,
			   xfrmnl_user_tmpl_free);

#endif /* __NETLINK_NL_AUX_XFRM_NL_XFRM_H__ */
