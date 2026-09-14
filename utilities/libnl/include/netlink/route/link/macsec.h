/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Sabrina Dubroca <sd@queasysnail.net>
 */

#ifndef NETLINK_LINK_MACSEC_H_
#define NETLINK_LINK_MACSEC_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <linux/if_link.h>
#include <linux/if_macsec.h>

#ifdef __cplusplus
extern "C" {
#endif

enum macsec_validation_type;

struct rtnl_link *rtnl_link_macsec_alloc(void);

int rtnl_link_macsec_set_sci(struct rtnl_link *, uint64_t);
int rtnl_link_macsec_get_sci(struct rtnl_link *, uint64_t *);

int rtnl_link_macsec_set_port(struct rtnl_link *, uint16_t);
int rtnl_link_macsec_get_port(struct rtnl_link *, uint16_t *);

int rtnl_link_macsec_set_cipher_suite(struct rtnl_link *, uint64_t);
int rtnl_link_macsec_get_cipher_suite(struct rtnl_link *, uint64_t *);

int rtnl_link_macsec_set_icv_len(struct rtnl_link *, uint16_t);
int rtnl_link_macsec_get_icv_len(struct rtnl_link *, uint16_t *);

int rtnl_link_macsec_set_protect(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_protect(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_encrypt(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_encrypt(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_offload(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_offload(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_encoding_sa(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_encoding_sa(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_validation_type(struct rtnl_link *,
					 enum macsec_validation_type);
int rtnl_link_macsec_get_validation_type(struct rtnl_link *,
					 enum macsec_validation_type *);

int rtnl_link_macsec_set_replay_protect(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_replay_protect(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_window(struct rtnl_link *, uint32_t);
int rtnl_link_macsec_get_window(struct rtnl_link *, uint32_t *);

int rtnl_link_macsec_set_send_sci(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_send_sci(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_end_station(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_end_station(struct rtnl_link *, uint8_t *);

int rtnl_link_macsec_set_scb(struct rtnl_link *, uint8_t);
int rtnl_link_macsec_get_scb(struct rtnl_link *, uint8_t *);



#ifdef __cplusplus
}
#endif

#endif
