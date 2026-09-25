/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef NETLINK_NL_AUTO_H_
#define NETLINK_NL_AUTO_H_

#include "base/nl-base-utils.h"

#define NL_DBG(LVL, FMT, ARG...)                                           \
	do {                                                               \
		if ((NL_DEBUG) && (LVL) <= nl_debug) {                     \
			const int _errsv = errno;                          \
                                                                           \
			fprintf(stderr, "DBG<" #LVL ">%20s:%-4u %s: " FMT, \
				__FILE__, __LINE__, __func__, ##ARG);      \
			errno = _errsv;                                    \
		}                                                          \
	} while (0)

struct nl_addr;
void nl_addr_put(struct nl_addr *);
#define _nl_auto_nl_addr _nl_auto(_nl_auto_nl_addr_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_addr *, _nl_auto_nl_addr_fcn, nl_addr_put);

struct nl_data;
void nl_data_free(struct nl_data *data);
#define _nl_auto_nl_data _nl_auto(_nl_auto_nl_data_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_data *, _nl_auto_nl_data_fcn,
			   nl_data_free);

struct nl_msg;
void nlmsg_free(struct nl_msg *);
#define _nl_auto_nl_msg _nl_auto(_nl_auto_nl_msg_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_msg *, _nl_auto_nl_msg_fcn, nlmsg_free);

struct nl_cache;
void nl_cache_put(struct nl_cache *);
#define _nl_auto_nl_cache _nl_auto(_nl_auto_nl_cache_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_cache *, _nl_auto_nl_cache_fcn,
			   nl_cache_put);

struct nl_sock;
void nl_socket_free(struct nl_sock *);
#define _nl_auto_nl_socket _nl_auto(_nl_auto_nl_socket_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_sock *, _nl_auto_nl_socket_fcn,
			   nl_socket_free);

struct nl_cache_mngr;
void nl_cache_mngr_free(struct nl_cache_mngr *mngr);
#define _nl_auto_nl_cache_mngr _nl_auto(_nl_auto_nl_cache_mngr_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_cache_mngr *, _nl_auto_nl_cache_mngr_fcn,
			   nl_cache_mngr_free);

struct nl_object;
void nl_object_put(struct nl_object *);
#define _nl_auto_nl_object _nl_auto(_nl_auto_nl_object_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct nl_object *, _nl_auto_nl_object_fcn,
			   nl_object_put);

struct nl_addr *nl_addr_build(int, const void *, size_t);

static inline struct nl_addr *_nl_addr_build(int family, const void *buf)
{
	return nl_addr_build(family, buf, _nl_addr_family_to_size(family));
}

static inline uint16_t _nla_len(const struct nlattr *nla)
{
	_nl_assert(nla);
	_nl_assert(nla->nla_len >= (uint16_t)NLA_HDRLEN);

	return nla->nla_len - (uint16_t)NLA_HDRLEN;
}

#endif /* NETLINK_NL_AUTO_H_ */
