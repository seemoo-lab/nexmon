/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2018 Wang Jian <jianjian.wang1@gmail.com>
 */

/**
 * @ingroup link
 * @defgroup geneve Geneve
 * Generic Network Virtualization Encapsulation
 *
 * @details
 * \b Link Type Name: "geneve"
 *
 * @route_doc{link_geneve, Geneve Documentation}
 *
 * @{
 */
#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/geneve.h>

#include "nl-route.h"
#include "link-api.h"

/** @cond SKIP */
#define GENEVE_ATTR_ID          (1<<0)
#define GENEVE_ATTR_REMOTE      (1<<1)
#define GENEVE_ATTR_REMOTE6     (1<<2)
#define GENEVE_ATTR_TTL         (1<<3)
#define GENEVE_ATTR_TOS         (1<<4)
#define GENEVE_ATTR_LABEL       (1<<5)
#define GENEVE_ATTR_PORT        (1<<6)
#define GENEVE_ATTR_FLAGS       (1<<7)
#define GENEVE_ATTR_UDP_CSUM    (1<<8)
#define GENEVE_ATTR_UDP_ZERO_CSUM6_TX   (1<<9)
#define GENEVE_ATTR_UDP_ZERO_CSUM6_RX   (1<<10)

struct geneve_info
{
        uint32_t        id;
        uint32_t        remote;
        struct in6_addr remote6;
        uint8_t         ttl;
        uint8_t         tos;
        uint32_t        label;
        uint16_t        port;
        uint8_t         flags;
        uint8_t         udp_csum;
        uint8_t         udp_zero_csum6_tx;
        uint8_t         udp_zero_csum6_rx;
        uint32_t        mask;
};

/** @endcond */

static struct nla_policy geneve_policy[IFLA_GENEVE_MAX + 1] = {
        [IFLA_GENEVE_ID] = { .type = NLA_U32 },
        [IFLA_GENEVE_REMOTE]    = { .minlen = sizeof(uint32_t) },
        [IFLA_GENEVE_REMOTE6]   = { .minlen = sizeof(struct in6_addr) },
        [IFLA_GENEVE_TTL]       = { .type = NLA_U8 },
        [IFLA_GENEVE_TOS]       = { .type = NLA_U8 },
        [IFLA_GENEVE_LABEL]     = { .type = NLA_U32 },
        [IFLA_GENEVE_PORT]      = { .type = NLA_U16 },
        [IFLA_GENEVE_COLLECT_METADATA]  = { .type = NLA_FLAG },
        [IFLA_GENEVE_UDP_CSUM]  = { .type = NLA_U8 },
        [IFLA_GENEVE_UDP_ZERO_CSUM6_TX] = { .type = NLA_U8 },
        [IFLA_GENEVE_UDP_ZERO_CSUM6_RX] = { .type = NLA_U8 },
};

static int geneve_alloc(struct rtnl_link *link)
{
        struct geneve_info *geneve;

        if (link->l_info)
                memset(link->l_info, 0, sizeof(*geneve));
        else {
                if ((geneve = calloc(1, sizeof(*geneve))) == NULL)
                                return -NLE_NOMEM;
                link->l_info = geneve;
        }

        return 0;
}

static int geneve_parse(struct rtnl_link *link, struct nlattr *data,
                        struct nlattr *xstats)
{
        struct nlattr *tb[IFLA_GENEVE_MAX + 1];
        struct geneve_info *geneve;
        int err = 0;

        NL_DBG(3, "Parsing Geneve link info\n");

        err = nla_parse_nested(tb, IFLA_GENEVE_MAX, data, geneve_policy);
        if (err < 0)
                return err;

        err = geneve_alloc(link);
        if (err < 0)
                return err;

        geneve = link->l_info;

        if (tb[IFLA_GENEVE_ID]) {
                geneve->id = nla_get_u32(tb[IFLA_GENEVE_ID]);
                geneve->mask |= GENEVE_ATTR_ID;
        }

        if (tb[IFLA_GENEVE_REMOTE]) {
                nla_memcpy(&geneve->remote, tb[IFLA_GENEVE_REMOTE],
                                sizeof(geneve->remote));
                geneve->mask |= GENEVE_ATTR_REMOTE;
                geneve->mask &= ~GENEVE_ATTR_REMOTE6;
        }
        if (tb[IFLA_GENEVE_REMOTE6]) {
                nla_memcpy(&geneve->remote6, tb[IFLA_GENEVE_REMOTE6],
                                sizeof(geneve->remote6));
                geneve->mask |= GENEVE_ATTR_REMOTE6;
                geneve->mask &= ~GENEVE_ATTR_REMOTE;
        }

        if (tb[IFLA_GENEVE_TTL]) {
                geneve->ttl = nla_get_u8(tb[IFLA_GENEVE_TTL]);
                geneve->mask |= GENEVE_ATTR_TTL;
        }

        if (tb[IFLA_GENEVE_TOS]) {
                geneve->tos = nla_get_u8(tb[IFLA_GENEVE_TOS]);
                geneve->mask |= GENEVE_ATTR_TOS;
        }

        if (tb[IFLA_GENEVE_LABEL]) {
                geneve->label = nla_get_u32(tb[IFLA_GENEVE_LABEL]);
                geneve->mask |= GENEVE_ATTR_LABEL;
        }

        if (tb[IFLA_GENEVE_PORT]) {
                geneve->port  = nla_get_u16(tb[IFLA_GENEVE_PORT]);
                geneve->mask |= GENEVE_ATTR_PORT;
        }

        if (tb[IFLA_GENEVE_COLLECT_METADATA])
                geneve->flags |= RTNL_LINK_GENEVE_F_COLLECT_METADATA;

        if (tb[IFLA_GENEVE_UDP_CSUM]) {
                geneve->udp_csum = nla_get_u8(tb[IFLA_GENEVE_UDP_CSUM]);
                geneve->mask |= GENEVE_ATTR_UDP_CSUM;
        }

        if (tb[IFLA_GENEVE_UDP_ZERO_CSUM6_TX]) {
                geneve->udp_zero_csum6_tx = nla_get_u8(tb[IFLA_GENEVE_UDP_ZERO_CSUM6_TX]);
                geneve->mask |= GENEVE_ATTR_UDP_ZERO_CSUM6_TX;
        }

        if (tb[IFLA_GENEVE_UDP_ZERO_CSUM6_RX]) {
                geneve->udp_zero_csum6_rx = nla_get_u8(tb[IFLA_GENEVE_UDP_ZERO_CSUM6_RX]);
                geneve->mask |= GENEVE_ATTR_UDP_ZERO_CSUM6_RX;
        }

        return err;
}

static void geneve_free(struct rtnl_link *link)
{
        struct geneve_info *geneve = link->l_info;

        free(geneve);
        link->l_info = NULL;
}

static void geneve_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
        struct geneve_info *geneve = link->l_info;

        nl_dump(p, "geneve-id %u", geneve->id);
}

static void geneve_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
        struct geneve_info *geneve = link->l_info;
        char addr[INET6_ADDRSTRLEN];

        nl_dump_line(p, "    geneve-id %u\n", geneve->id);

        if (geneve->mask & GENEVE_ATTR_REMOTE) {
                nl_dump(p, "     remote ");
                nl_dump_line(p, "%s\n",
                             _nl_inet_ntop(AF_INET, &geneve->remote, addr));
        } else if (geneve->mask & GENEVE_ATTR_REMOTE6) {
                nl_dump(p, "      remote ");
                nl_dump_line(p, "%s\n",
                             _nl_inet_ntop(AF_INET6, &geneve->remote6, addr));
        }

        if (geneve->mask & GENEVE_ATTR_TTL) {
                nl_dump(p, "      ttl ");
                nl_dump_line(p, "%u\n", geneve->ttl);
        }

        if (geneve->mask & GENEVE_ATTR_TOS) {
                nl_dump(p, "      tos ");
                nl_dump_line(p, "%u\n", geneve->tos);
        }

        if (geneve->mask & GENEVE_ATTR_PORT) {
                nl_dump(p, "      port ");
                nl_dump_line(p, "%u\n", ntohs(geneve->port));
        }

        if (geneve->mask & GENEVE_ATTR_LABEL) {
                nl_dump(p, "      label ");
                nl_dump_line(p, "%u\n", ntohl(geneve->label));
        }

        if (geneve->mask & GENEVE_ATTR_UDP_CSUM) {
                nl_dump(p, "      UDP checksum ");
                if (geneve->udp_csum)
                        nl_dump_line(p, "enabled (%#x)\n", geneve->udp_csum);
                else
                        nl_dump_line(p, "disabled\n");
        }

        if (geneve->mask & GENEVE_ATTR_UDP_ZERO_CSUM6_TX) {
                nl_dump(p, "      udp-zero-csum6-tx ");
                if (geneve->udp_zero_csum6_tx)
                        nl_dump_line(p, "enabled (%#x)\n", geneve->udp_zero_csum6_tx);
                else
                        nl_dump_line(p, "disabled\n");
        }

        if (geneve->mask & GENEVE_ATTR_UDP_ZERO_CSUM6_RX) {
                nl_dump(p, "      udp-zero-csum6-rx ");
                if (geneve->udp_zero_csum6_rx)
                        nl_dump_line(p, "enabled (%#x)\n", geneve->udp_zero_csum6_rx);
                else
                        nl_dump_line(p, "disabled\n");
        }

        if (geneve->flags & RTNL_LINK_GENEVE_F_COLLECT_METADATA)
                nl_dump(p, "      collect-metadata\n");
}

static int geneve_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
        struct geneve_info *gdst, *gsrc;
        int err;

        gsrc = src->l_info;
        dst->l_info = NULL;
        err = rtnl_link_set_type(dst, "geneve");
        if (err < 0)
                return err;

        gdst = dst->l_info;

        if (!gsrc || !gdst)
                return -NLE_NOMEM;

        memcpy(gdst, gsrc, sizeof(struct geneve_info));

        return 0;
}

static int geneve_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
        struct geneve_info  *geneve = link->l_info;
        struct nlattr *data;

        if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
                return -NLE_MSGSIZE;

        if (geneve->mask & GENEVE_ATTR_ID)
                NLA_PUT_U32(msg, IFLA_GENEVE_ID, geneve->id);

        if (geneve->mask & GENEVE_ATTR_REMOTE)
                NLA_PUT(msg, IFLA_GENEVE_REMOTE,
                                sizeof(geneve->remote), &geneve->remote);

        if (geneve->mask & GENEVE_ATTR_REMOTE6)
                NLA_PUT(msg, IFLA_GENEVE_REMOTE6,
                                sizeof(geneve->remote6), &geneve->remote6);

        if (geneve->mask & GENEVE_ATTR_TTL)
                NLA_PUT_U8(msg, IFLA_GENEVE_TTL, geneve->ttl);

        if (geneve->mask & GENEVE_ATTR_TOS)
                NLA_PUT_U8(msg, IFLA_GENEVE_TOS, geneve->tos);

        if (geneve->mask & GENEVE_ATTR_LABEL)
                NLA_PUT_U32(msg, IFLA_GENEVE_LABEL, geneve->label);

        if (geneve->mask & GENEVE_ATTR_PORT)
                NLA_PUT_U16(msg, IFLA_GENEVE_PORT, geneve->port);

        if (geneve->mask & GENEVE_ATTR_UDP_CSUM)
                NLA_PUT_U8(msg, IFLA_GENEVE_UDP_CSUM, geneve->udp_csum);

        if (geneve->mask & GENEVE_ATTR_UDP_ZERO_CSUM6_TX)
                NLA_PUT_U8(msg, IFLA_GENEVE_UDP_ZERO_CSUM6_TX, geneve->udp_zero_csum6_tx);

        if (geneve->mask & GENEVE_ATTR_UDP_ZERO_CSUM6_RX)
                NLA_PUT_U8(msg, IFLA_GENEVE_UDP_ZERO_CSUM6_RX, geneve->udp_zero_csum6_rx);

        if (geneve->flags & RTNL_LINK_GENEVE_F_COLLECT_METADATA)
                NLA_PUT_FLAG(msg, IFLA_GENEVE_COLLECT_METADATA);

        nla_nest_end(msg, data);

nla_put_failure:

        return 0;
}

static struct rtnl_link_info_ops geneve_info_ops = {
        .io_name        = "geneve",
        .io_alloc       = geneve_alloc,
        .io_parse       = geneve_parse,
        .io_dump        = {
                [NL_DUMP_LINE]          = geneve_dump_line,
                [NL_DUMP_DETAILS]       = geneve_dump_details,
        },
        .io_clone       = geneve_clone,
        .io_put_attrs   = geneve_put_attrs,
        .io_free        = geneve_free,
};


/** @cond SKIP */
#define IS_GENEVE_LINK_ASSERT(link) \
        if ((link)->l_info_ops != &geneve_info_ops) { \
                APPBUG("Link is not a geneve link. set type \"geneve\" first."); \
                return -NLE_OPNOTSUPP; \
        }
/** @endcond */

/**
 * @name Geneve Object
 * @{
 */

/**
 * Allocate link object of type Geneve
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_geneve_alloc(void)
{
        struct rtnl_link *link;

        if (!(link = rtnl_link_alloc()))
                return NULL;

        if (rtnl_link_set_type(link, "geneve") < 0) {
                rtnl_link_put(link);
                return NULL;
        }

        return link;
}

/**
 * Check if link is a Geneve link
 * @arg link    Link object
 *
 * @return True if link is a Geneve link, otherwisee false is returned.
 */
int rtnl_link_is_geneve(struct rtnl_link *link)
{
        return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "geneve");
}

/**
 * Set Geneve Network Indentifier
 * @arg link    Link object
 * @arg id      Geneve network identifier
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_id(struct rtnl_link *link, uint32_t id)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (id > RTNL_GENEVE_ID_MAX)
                return -NLE_INVAL;

        geneve->id = id;
        geneve->mask |= GENEVE_ATTR_ID;

        return 0;
}

/**
 * Get Geneve Network Identifier
 * @arg link    Link object
 * @arg id      Pointer to store network identifier
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_get_id(struct rtnl_link *link, uint32_t *id)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!id)
                return -NLE_INVAL;

        if (geneve->mask & GENEVE_ATTR_ID)
                *id = geneve->id;
        else
                return -NLE_AGAIN;

        return 0;
}

/**
 * Set Geneve unicast destination IP address
 * @arg link    Link object
 * @arg addr    The unicast destination IP address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_remote(struct rtnl_link *link, struct nl_addr *addr)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if ((nl_addr_get_family(addr) == AF_INET) &&
                (nl_addr_get_len(addr) == sizeof(geneve->remote))) {
                memcpy(&geneve->remote, nl_addr_get_binary_addr(addr),
                                sizeof(geneve->remote));
                geneve->mask |= GENEVE_ATTR_REMOTE;
                geneve->mask &= ~GENEVE_ATTR_REMOTE6;
        } else if ((nl_addr_get_family(addr) == AF_INET6) &&
                        (nl_addr_get_len(addr) == sizeof(geneve->remote6))) {
                memcpy(&geneve->remote6, nl_addr_get_binary_addr(addr),
                                sizeof(geneve->remote6));
                geneve->mask |= GENEVE_ATTR_REMOTE6;
                geneve->mask &= ~GENEVE_ATTR_REMOTE;
        } else
                return -NLE_INVAL;

        return 0;
}

/**
 * Get Geneve unicast destination IP address
 * @arg link    Link object
 * @arg addr    Pointer to store unicast destination IP addree
 *
 * @return 0 on success or a a negative error code
 */
int rtnl_link_geneve_get_remote(struct rtnl_link *link, struct nl_addr **addr)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!addr)
                return -NLE_INVAL;

        if (geneve->mask & GENEVE_ATTR_REMOTE)
                *addr = nl_addr_build(AF_INET, &geneve->remote, sizeof(geneve->remote));
        else if (geneve->mask & GENEVE_ATTR_REMOTE6)
                *addr = nl_addr_build(AF_INET6, &geneve->remote6, sizeof(geneve->remote6));
        else
                return -NLE_AGAIN;

        return 0;
}

/**
 * Set IP TTL value to use for Geneve
 * @arg link    Link object
 * @arg ttl     TTL value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_ttl(struct rtnl_link *link, uint8_t ttl)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->ttl = ttl;
        geneve->mask |= GENEVE_ATTR_TTL;

        return 0;
}

/**
 * Get IP TTL value to use for Geneve
 * @arg link    Link object
 *
 * @return TTL value on success or a negative error code
 */
int rtnl_link_geneve_get_ttl(struct rtnl_link *link)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!(geneve->mask & GENEVE_ATTR_TTL))
                return -NLE_AGAIN;

        return geneve->ttl;
}

/**
 * Set IP ToS value to use for Geneve
 * @arg link    Link object
 * @arg tos     ToS value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_tos(struct rtnl_link *link, uint8_t tos)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->tos = tos;
        geneve->mask |= GENEVE_ATTR_TOS;

        return 0;
}

/**
 * Get IP ToS value to use for Geneve
 * @arg link    Link object
 *
 * @return ToS value on success or a negative error code
 */
int rtnl_link_geneve_get_tos(struct rtnl_link *link)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!(geneve->mask & GENEVE_ATTR_TOS))
                return -NLE_AGAIN;

        return geneve->tos;
}

/**
 * Set UDP destination port to use for Geneve
 * @arg link    Link object
 * @arg port    Destination port
 *
 * @return 0 on success or a negative error code
 */

int rtnl_link_geneve_set_port(struct rtnl_link *link,  uint32_t port)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->port = htons(port);
        geneve->mask |= GENEVE_ATTR_PORT;

        return 0;
}

/**
 * Get UDP destination port to use for Geneve
 * @arg link    Link object
 * @arg port    Pointer to store destination port
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_get_port(struct rtnl_link *link, uint32_t *port)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!port)
                return -NLE_INVAL;

        if (!(geneve->mask & GENEVE_ATTR_PORT))
                return -NLE_NOATTR;

        *port = ntohs(geneve->port);

        return 0;
}

/**
 * Set flow label to use for Geneve
 * @arg link    Link object
 * @arg label   Destination label
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_label(struct rtnl_link *link, uint32_t label)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->label = htonl(label);
        geneve->mask |= GENEVE_ATTR_LABEL;

        return 0;
}

/**
 * Get flow label to use for Geneve
 * @arg link    Link object
 * @arg label   Pointer to store destination label
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_get_label(struct rtnl_link *link, uint32_t *label)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!label)
                return -NLE_INVAL;
        if (!(geneve->mask & GENEVE_ATTR_LABEL))
                return -NLE_NOATTR;

        *label = ntohl(geneve->label);

        return 0;
}

/**
 * Set UDP checksum status to use for Geneve
 * @arg link    Link object
 * @arg csum    Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_udp_csum(struct rtnl_link *link, uint8_t csum)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->udp_csum = csum;
        geneve->mask |= GENEVE_ATTR_UDP_CSUM;

        return 0;
}

/**
 * Get UDP checksum status to use for Geneve
 * @arg link    Link object
 *
 * @return status value on success or a negative error code
 */
int rtnl_link_geneve_get_udp_csum(struct rtnl_link *link)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!(geneve->mask & GENEVE_ATTR_UDP_CSUM))
                return -NLE_NOATTR;

        return geneve->udp_csum;
}

/**
 * Set skip UDP checksum transmitted over IPv6 status to use for Geneve
 * @arg link    Link object
 * @arg csum    Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_udp_zero_csum6_tx(struct rtnl_link *link, uint8_t csum)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->udp_zero_csum6_tx = csum;
        geneve->mask |= GENEVE_ATTR_UDP_ZERO_CSUM6_TX;

        return 0;
}

/**
 * Get skip UDP checksum transmitted over IPv6 status to use for Geneve
 * @arg link    Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_geneve_get_udp_zero_csum6_tx(struct rtnl_link *link)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!(geneve->mask & GENEVE_ATTR_UDP_ZERO_CSUM6_TX))
                return -NLE_NOATTR;

        return geneve->udp_zero_csum6_tx;
}

/**
 * Set skip UDP checksum received over IPv6 status to use for Geneve
 * @arg link    Link object
 * @arg csum    Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_udp_zero_csum6_rx(struct rtnl_link *link, uint8_t csum)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        geneve->udp_zero_csum6_rx = csum;
        geneve->mask |= GENEVE_ATTR_UDP_ZERO_CSUM6_RX;

        return 0;
}

/**
 * Get skip UDP checksum received over IPv6  status to use for Geneve
 * @arg link    Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_geneve_get_udp_zero_csum6_rx(struct rtnl_link *link)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (!(geneve->mask & GENEVE_ATTR_UDP_ZERO_CSUM6_RX))
                return -NLE_NOATTR;

        return geneve->udp_zero_csum6_rx;
}

/**
 * Set Geneve flags
 * @arg link    Link object
 * @arg flags   Which flags to set
 * @arg enable  Boolean enabling or disabling flag
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_set_flags(struct rtnl_link *link, uint8_t flags, int enable)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        if (flags & ~RTNL_LINK_GENEVE_F_COLLECT_METADATA)
                return -NLE_INVAL;

        if (enable)
                geneve->flags = flags;
        else
                geneve->flags &= ~flags;

        return 0;
}

/**
 * Get Geneve flags
 * @arg link    Link object
 * @arg flags   Pointer to store flags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_geneve_get_flags(struct rtnl_link *link, uint8_t *flags)
{
        struct geneve_info *geneve = link->l_info;

        IS_GENEVE_LINK_ASSERT(link);

        *flags = geneve->flags;
        return 0;
}

/** @} */
static void _nl_init geneve_init(void)
{
        rtnl_link_register_info(&geneve_info_ops);
}

static void _nl_exit geneve_exit(void)
{
        rtnl_link_unregister_info(&geneve_info_ops);
}

/** @} */
