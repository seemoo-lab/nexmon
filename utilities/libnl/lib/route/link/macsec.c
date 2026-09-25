/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Sabrina Dubroca <sd@queasysnail.net>
 */

/**
 * @ingroup link
 * @defgroup macsec MACsec
 * MACsec link module
 *
 * @details
 * \b Link Type Name: "macsec"
 *
 * @route_doc{link_macsec, MACsec Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_macsec.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/macsec.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-priv-dynamic-core/nl-core.h"

/** @cond SKIP */
#define MACSEC_ATTR_SCI			(1 << 0)
#define MACSEC_ATTR_ICV_LEN		(1 << 1)
#define MACSEC_ATTR_CIPHER_SUITE	(1 << 2)
#define MACSEC_ATTR_WINDOW		(1 << 3)
#define MACSEC_ATTR_ENCODING_SA		(1 << 4)
#define MACSEC_ATTR_ENCRYPT		(1 << 5)
#define MACSEC_ATTR_PROTECT		(1 << 6)
#define MACSEC_ATTR_INC_SCI		(1 << 7)
#define MACSEC_ATTR_ES			(1 << 8)
#define MACSEC_ATTR_SCB			(1 << 9)
#define MACSEC_ATTR_REPLAY_PROTECT	(1 << 10)
#define MACSEC_ATTR_VALIDATION		(1 << 11)
#define MACSEC_ATTR_PORT		(1 << 12)
#define MACSEC_ATTR_OFFLOAD		(1 << 13)

struct macsec_info {
	int ifindex;
	uint64_t sci;
	uint16_t port;
	uint64_t cipher_suite;
	uint16_t icv_len;
	uint32_t window;
	enum macsec_validation_type validate;
	uint8_t encoding_sa;

	uint8_t send_sci, end_station, scb, replay_protect, protect, encrypt, offload;

	uint32_t ce_mask;
};

#define DEFAULT_ICV_LEN 16

/** @endcond */

static struct nla_policy macsec_policy[IFLA_MACSEC_MAX+1] = {
	[IFLA_MACSEC_SCI] = { .type = NLA_U64 },
	[IFLA_MACSEC_ICV_LEN] = { .type = NLA_U8 },
	[IFLA_MACSEC_CIPHER_SUITE] = { .type = NLA_U64 },
	[IFLA_MACSEC_WINDOW] = { .type = NLA_U32 },
	[IFLA_MACSEC_ENCODING_SA] = { .type = NLA_U8 },
	[IFLA_MACSEC_ENCRYPT] = { .type = NLA_U8 },
	[IFLA_MACSEC_PROTECT] = { .type = NLA_U8 },
	[IFLA_MACSEC_INC_SCI] = { .type = NLA_U8 },
	[IFLA_MACSEC_ES] = { .type = NLA_U8 },
	[IFLA_MACSEC_SCB] = { .type = NLA_U8 },
	[IFLA_MACSEC_REPLAY_PROTECT] = { .type = NLA_U8 },
	[IFLA_MACSEC_VALIDATION] = { .type = NLA_U8 },
	[IFLA_MACSEC_OFFLOAD] = { .type = NLA_U8 },
};

/**
 * @name MACsec Object
 * @{
 */

/**
 * Allocate link object of type MACsec
 *
 * @return Allocated link object or NULL.
 */
static int macsec_alloc(struct rtnl_link *link)
{
	struct macsec_info *info;

	if (!link->l_info) {
		link->l_info = malloc(sizeof(struct macsec_info));
		if (!link->l_info)
			return -NLE_NOMEM;
	}

	memset(link->l_info, 0, sizeof(struct macsec_info));
	info = link->l_info;

	info->cipher_suite = MACSEC_DEFAULT_CIPHER_ID;
	info->icv_len = DEFAULT_ICV_LEN;
	info->ce_mask = MACSEC_ATTR_CIPHER_SUITE | MACSEC_ATTR_ICV_LEN;

	return 0;
}

static int macsec_parse(struct rtnl_link *link, struct nlattr *data,
		      struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_MACSEC_MAX+1];
	struct macsec_info *info;
	int err;

	NL_DBG(3, "Parsing MACsec link info\n");

	if ((err = nla_parse_nested(tb, IFLA_MACSEC_MAX, data, macsec_policy)) < 0)
		goto errout;

	if ((err = macsec_alloc(link)) < 0)
		goto errout;

	info = link->l_info;

	if (tb[IFLA_MACSEC_SCI]) {
		info->sci = nla_get_u64(tb[IFLA_MACSEC_SCI]);
		info->ce_mask |= MACSEC_ATTR_SCI;
	}

	if (tb[IFLA_MACSEC_PROTECT]) {
		info->protect = nla_get_u8(tb[IFLA_MACSEC_PROTECT]);
		info->ce_mask |= MACSEC_ATTR_PROTECT;
	}

	if (tb[IFLA_MACSEC_CIPHER_SUITE]) {
		info->cipher_suite = nla_get_u64(tb[IFLA_MACSEC_CIPHER_SUITE]);
		info->ce_mask |= MACSEC_ATTR_CIPHER_SUITE;
	}

	if (tb[IFLA_MACSEC_ICV_LEN]) {
		info->icv_len = nla_get_u8(tb[IFLA_MACSEC_ICV_LEN]);
		info->ce_mask |= MACSEC_ATTR_ICV_LEN;
	}

	if (tb[IFLA_MACSEC_ENCODING_SA]) {
		info->encoding_sa = nla_get_u8(tb[IFLA_MACSEC_ENCODING_SA]);
		info->ce_mask |= MACSEC_ATTR_ENCODING_SA;
	}

	if (tb[IFLA_MACSEC_VALIDATION]) {
		info->validate = nla_get_u8(tb[IFLA_MACSEC_VALIDATION]);
		info->ce_mask |= MACSEC_ATTR_VALIDATION;
	}

	if (tb[IFLA_MACSEC_ENCRYPT]) {
		info->encrypt = nla_get_u8(tb[IFLA_MACSEC_ENCRYPT]);
		info->ce_mask |= MACSEC_ATTR_ENCRYPT;
	}

	if (tb[IFLA_MACSEC_OFFLOAD]) {
		info->offload = nla_get_u8(tb[IFLA_MACSEC_OFFLOAD]);
		info->ce_mask |= MACSEC_ATTR_OFFLOAD;
	}

	if (tb[IFLA_MACSEC_INC_SCI]) {
		info->send_sci = nla_get_u8(tb[IFLA_MACSEC_INC_SCI]);
		info->ce_mask |= MACSEC_ATTR_INC_SCI;
	}

	if (tb[IFLA_MACSEC_ES]) {
		info->end_station = nla_get_u8(tb[IFLA_MACSEC_ES]);
		info->ce_mask |= MACSEC_ATTR_ES;
	}

	if (tb[IFLA_MACSEC_SCB]) {
		info->scb = nla_get_u8(tb[IFLA_MACSEC_SCB]);
		info->ce_mask |= MACSEC_ATTR_SCB;
	}

	if (tb[IFLA_MACSEC_REPLAY_PROTECT]) {
		info->replay_protect = nla_get_u8(tb[IFLA_MACSEC_REPLAY_PROTECT]);
		info->ce_mask |= MACSEC_ATTR_REPLAY_PROTECT;
	}

	if (tb[IFLA_MACSEC_WINDOW]) {
		info->window = nla_get_u32(tb[IFLA_MACSEC_WINDOW]);
		info->ce_mask |= MACSEC_ATTR_WINDOW;
	}

	err = 0;
errout:
	return err;
}

static void macsec_free(struct rtnl_link *link)
{
	free(link->l_info);
	link->l_info = NULL;
}

static const char *values_on_off[] = {	"off", "on" };

static const char *VALIDATE_STR[] = {
	[MACSEC_VALIDATE_DISABLED] = "disabled",
	[MACSEC_VALIDATE_CHECK] = "check",
	[MACSEC_VALIDATE_STRICT] = "strict",
};

static char *replay_protect_str(char *buf, uint8_t replay_protect, uint8_t window)
{
	if (replay_protect == 1) {
		sprintf(buf, "replay_protect on window %d", window);
	} else if (replay_protect == 0) {
		sprintf(buf, "replay_protect off");
	} else {
		buf[0] = '\0';
	}

	return buf;
}

/** @cond SKIP */
#define PRINT_FLAG(buf, i, field, c) ({ if (i->field == 1) *buf++ = c; })
/** @endcond */
static char *flags_str(char *buf, unsigned char len, struct macsec_info *info)
{
	char *tmp = buf;
	memset(tmp, 0, len);

	PRINT_FLAG(tmp, info, protect, 'P');
	PRINT_FLAG(tmp, info, encrypt, 'E');
	PRINT_FLAG(tmp, info, send_sci, 'S');
	PRINT_FLAG(tmp, info, end_station, 'e');
	PRINT_FLAG(tmp, info, scb, 's');
	PRINT_FLAG(tmp, info, replay_protect, 'R');

	*tmp++ = ' ';
	*tmp++ = 'v';
	switch (info->validate) {
	case MACSEC_VALIDATE_DISABLED:
		*tmp++ = 'd';
		break;
	case MACSEC_VALIDATE_CHECK:
		*tmp++ = 'c';
		break;
	case MACSEC_VALIDATE_STRICT:
		*tmp++ = 's';
		break;
	default:
		break;
	}

	sprintf(tmp, " %d", info->encoding_sa);

	return buf;
}

static void macsec_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct macsec_info *info = link->l_info;
	char tmp[128];

	nl_dump(p, "sci %016llx <%s>", (long long unsigned)ntohll(info->sci),
		flags_str(tmp, sizeof(tmp), info));
}

static void macsec_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct macsec_info *info = link->l_info;
	char tmp[128];

	nl_dump(p,
		"    sci %016llx protect %s encoding_sa %d encrypt %s send_sci %s validate %s %s\n",
		(long long unsigned)ntohll(info->sci),
		values_on_off[info->protect], info->encoding_sa,
		values_on_off[info->encrypt], values_on_off[info->send_sci],
		VALIDATE_STR[info->validate],
		replay_protect_str(tmp, info->replay_protect, info->window));
	nl_dump(p, "    cipher suite: %016llx, icv_len %d\n",
		(long long unsigned)info->cipher_suite, info->icv_len);
}

static int macsec_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct macsec_info *copy, *info = src->l_info;
	int err;

	dst->l_info = NULL;
	if ((err = rtnl_link_set_type(dst, "macsec")) < 0)
		return err;
	copy = dst->l_info;

	if (!info || !copy)
		return -NLE_NOMEM;

	memcpy(copy, info, sizeof(struct macsec_info));

	return 0;
}

static int macsec_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct macsec_info *info = link->l_info;
	struct nlattr *data;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_MSGSIZE;

	if (info->ce_mask & MACSEC_ATTR_SCI)
		NLA_PUT_U64(msg, IFLA_MACSEC_SCI, info->sci);
	else if (info->ce_mask & MACSEC_ATTR_PORT)
		NLA_PUT_U16(msg, IFLA_MACSEC_PORT, htons(info->port));

	if ((info->ce_mask & MACSEC_ATTR_ENCRYPT))
		NLA_PUT_U8(msg, IFLA_MACSEC_ENCRYPT, info->encrypt);

	if ((info->ce_mask & MACSEC_ATTR_OFFLOAD))
		NLA_PUT_U8(msg, IFLA_MACSEC_OFFLOAD, info->offload);

	if (info->cipher_suite != MACSEC_DEFAULT_CIPHER_ID || info->icv_len != DEFAULT_ICV_LEN) {
		NLA_PUT_U64(msg, IFLA_MACSEC_CIPHER_SUITE, info->cipher_suite);
		NLA_PUT_U8(msg, IFLA_MACSEC_ICV_LEN, info->icv_len);
	}

	if ((info->ce_mask & MACSEC_ATTR_INC_SCI))
		NLA_PUT_U8(msg, IFLA_MACSEC_INC_SCI, info->send_sci);

	if ((info->ce_mask & MACSEC_ATTR_ES))
		NLA_PUT_U8(msg, IFLA_MACSEC_ES, info->end_station);

	if ((info->ce_mask & MACSEC_ATTR_SCB))
		NLA_PUT_U8(msg, IFLA_MACSEC_SCB, info->scb);

	if ((info->ce_mask & MACSEC_ATTR_PROTECT))
		NLA_PUT_U8(msg, IFLA_MACSEC_PROTECT, info->protect);

	if ((info->ce_mask & MACSEC_ATTR_REPLAY_PROTECT)) {
		if (info->replay_protect && !(info->ce_mask & MACSEC_ATTR_WINDOW))
			return -NLE_INVAL;

		NLA_PUT_U8(msg, IFLA_MACSEC_REPLAY_PROTECT, info->replay_protect);
		NLA_PUT_U32(msg, IFLA_MACSEC_WINDOW, info->window);
	}

	if ((info->ce_mask & MACSEC_ATTR_VALIDATION))
		NLA_PUT_U8(msg, IFLA_MACSEC_VALIDATION, info->validate);

	if ((info->ce_mask & MACSEC_ATTR_ENCODING_SA))
		NLA_PUT_U8(msg, IFLA_MACSEC_ENCODING_SA, info->encoding_sa);

	nla_nest_end(msg, data);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static int macsec_compare(struct rtnl_link *link_a, struct rtnl_link *link_b,
			  int flags)
{
	struct macsec_info *a = link_a->l_info;
	struct macsec_info *b = link_b->l_info;
	int diff = 0;
	uint32_t attrs = flags & LOOSE_COMPARISON ? b->ce_mask :
						    ~((uint32_t)0u);

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	if (a->ce_mask & MACSEC_ATTR_SCI && b->ce_mask & MACSEC_ATTR_SCI)
		diff |= _DIFF(MACSEC_ATTR_SCI, a->sci != b->sci);
	else if (a->ce_mask & MACSEC_ATTR_PORT && b->ce_mask & MACSEC_ATTR_PORT)
		diff |= _DIFF(MACSEC_ATTR_PORT, a->port != b->port);

	if (a->ce_mask & MACSEC_ATTR_CIPHER_SUITE &&
	    b->ce_mask & MACSEC_ATTR_CIPHER_SUITE) {
		diff |= _DIFF(MACSEC_ATTR_ICV_LEN, a->icv_len != b->icv_len);
		diff |= _DIFF(MACSEC_ATTR_CIPHER_SUITE,
			      a->cipher_suite != b->cipher_suite);
	}

	if (a->ce_mask & MACSEC_ATTR_REPLAY_PROTECT &&
	    b->ce_mask & MACSEC_ATTR_REPLAY_PROTECT) {
		int d = _DIFF(MACSEC_ATTR_REPLAY_PROTECT,
			      a->replay_protect != b->replay_protect);
		if (a->replay_protect && b->replay_protect) {
			d |= _DIFF(MACSEC_ATTR_WINDOW, a->window != b->window);
		}
		diff |= d;
	}

	diff |= _DIFF(MACSEC_ATTR_ENCODING_SA,
		      a->encoding_sa != b->encoding_sa);
	diff |= _DIFF(MACSEC_ATTR_ENCRYPT, a->encrypt != b->encrypt);
	diff |= _DIFF(MACSEC_ATTR_PROTECT, a->protect != b->protect);
	diff |= _DIFF(MACSEC_ATTR_INC_SCI, a->send_sci != b->send_sci);
	diff |= _DIFF(MACSEC_ATTR_ES, a->end_station != b->end_station);
	diff |= _DIFF(MACSEC_ATTR_SCB, a->scb != b->scb);
	diff |= _DIFF(MACSEC_ATTR_VALIDATION, a->validate != b->validate);
#undef _DIFF

	return diff;
}


static struct rtnl_link_info_ops macsec_info_ops = {
	.io_name		= "macsec",
	.io_alloc		= macsec_alloc,
	.io_parse		= macsec_parse,
	.io_dump = {
		[NL_DUMP_LINE] = macsec_dump_line,
		[NL_DUMP_DETAILS]	= macsec_dump_details,
	},
	.io_clone		= macsec_clone,
	.io_put_attrs		= macsec_put_attrs,
	.io_free		= macsec_free,
	.io_compare		= macsec_compare,
};

static void _nl_init macsec_init(void)
{
	rtnl_link_register_info(&macsec_info_ops);
}

static void _nl_exit macsec_exit(void)
{
	rtnl_link_unregister_info(&macsec_info_ops);
}

/** @cond SKIP */
#define IS_MACSEC_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &macsec_info_ops) { \
		APPBUG("Link is not a MACsec link. set type \"macsec\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

struct rtnl_link *rtnl_link_macsec_alloc(void)
{
	struct rtnl_link *link = rtnl_link_alloc();

	if (!link)
		return NULL;

	if (rtnl_link_set_type(link, "macsec") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Set SCI
 * @arg link		Link object
 * @arg sci		Secure Channel Identifier in network byte order
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macsec_set_sci(struct rtnl_link *link, uint64_t sci)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	info->sci = sci;
	info->ce_mask |= MACSEC_ATTR_SCI;

	return 0;
}

/**
 * Get SCI
 * @arg link		Link object
 * @arg sci		On return points to the Secure Channel Identifier
 *			in network byte order
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macsec_get_sci(struct rtnl_link *link, uint64_t *sci)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_SCI))
		return -NLE_NOATTR;

	if (sci)
		*sci = info->sci;

	return 0;
}

/**
 * Set port identifier
 * @arg link		Link object
 * @arg port		Port identifier in host byte order
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macsec_set_port(struct rtnl_link *link, uint16_t port)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	info->port = port;
	info->ce_mask |= MACSEC_ATTR_PORT;

	return 0;
}

/**
 * Get port identifier
 * @arg link		Link object
 * @arg port		On return points to the port identifier in host byte order
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macsec_get_port(struct rtnl_link *link, uint16_t *port)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_PORT))
		return -NLE_NOATTR;

	if (port)
		*port = info->port;

	return 0;
}

int rtnl_link_macsec_set_cipher_suite(struct rtnl_link *link, uint64_t cipher_suite)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	info->cipher_suite = cipher_suite;
	info->ce_mask |= MACSEC_ATTR_CIPHER_SUITE;

	return 0;
}

int rtnl_link_macsec_get_cipher_suite(struct rtnl_link *link, uint64_t *cs)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_CIPHER_SUITE))
		return -NLE_NOATTR;

	if (cs)
		*cs = info->cipher_suite;

	return 0;
}

int rtnl_link_macsec_set_icv_len(struct rtnl_link *link, uint16_t icv_len)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (icv_len > MACSEC_STD_ICV_LEN)
		return -NLE_INVAL;

	info->icv_len = icv_len;
	info->ce_mask |= MACSEC_ATTR_ICV_LEN;

	return 0;
}

int rtnl_link_macsec_get_icv_len(struct rtnl_link *link, uint16_t *icv_len)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_ICV_LEN))
		return -NLE_NOATTR;

	if (icv_len)
		*icv_len = info->icv_len;

	return 0;
}

int rtnl_link_macsec_set_protect(struct rtnl_link *link, uint8_t protect)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (protect > 1)
		return -NLE_INVAL;

	info->protect = protect;
	info->ce_mask |= MACSEC_ATTR_PROTECT;

	return 0;
}

int rtnl_link_macsec_get_protect(struct rtnl_link *link, uint8_t *protect)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_PROTECT))
		return -NLE_NOATTR;

	if (protect)
		*protect = info->protect;

	return 0;
}

int rtnl_link_macsec_set_encrypt(struct rtnl_link *link, uint8_t encrypt)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (encrypt > 1)
		return -NLE_INVAL;

	info->encrypt = encrypt;
	info->ce_mask |= MACSEC_ATTR_ENCRYPT;

	return 0;
}

int rtnl_link_macsec_get_encrypt(struct rtnl_link *link, uint8_t *encrypt)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_ENCRYPT))
		return -NLE_NOATTR;

	if (encrypt)
		*encrypt = info->encrypt;

	return 0;
}

int rtnl_link_macsec_set_offload(struct rtnl_link *link, uint8_t offload)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	info->offload = offload;
	info->ce_mask |= MACSEC_ATTR_OFFLOAD;

	return 0;
}

int rtnl_link_macsec_get_offload(struct rtnl_link *link, uint8_t *offload)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_OFFLOAD))
		return -NLE_NOATTR;

	if (offload)
		*offload = info->offload;

	return 0;
}

int rtnl_link_macsec_set_encoding_sa(struct rtnl_link *link, uint8_t encoding_sa)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (encoding_sa > 3)
		return -NLE_INVAL;

	info->encoding_sa = encoding_sa;
	info->ce_mask |= MACSEC_ATTR_ENCODING_SA;

	return 0;
}

int rtnl_link_macsec_get_encoding_sa(struct rtnl_link *link, uint8_t *encoding_sa)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_ENCODING_SA))
		return -NLE_NOATTR;

	if (encoding_sa)
		*encoding_sa = info->encoding_sa;

	return 0;
}

int rtnl_link_macsec_set_validation_type(struct rtnl_link *link, enum macsec_validation_type validate)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (validate > MACSEC_VALIDATE_MAX)
		return -NLE_INVAL;

	info->validate = validate;
	info->ce_mask |= MACSEC_ATTR_VALIDATION;

	return 0;
}

int rtnl_link_macsec_get_validation_type(struct rtnl_link *link, enum macsec_validation_type *validate)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_VALIDATION))
		return -NLE_NOATTR;

	if (validate)
		*validate = info->validate;

	return 0;
}

int rtnl_link_macsec_set_replay_protect(struct rtnl_link *link, uint8_t replay_protect)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (replay_protect > 1)
		return -NLE_INVAL;

	info->replay_protect = replay_protect;
	info->ce_mask |= MACSEC_ATTR_REPLAY_PROTECT;

	return 0;
}

int rtnl_link_macsec_get_replay_protect(struct rtnl_link *link, uint8_t *replay_protect)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_REPLAY_PROTECT))
		return -NLE_NOATTR;

	if (replay_protect)
		*replay_protect = info->replay_protect;

	return 0;
}

int rtnl_link_macsec_set_window(struct rtnl_link *link, uint32_t window)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	info->window = window;
	info->ce_mask |= MACSEC_ATTR_WINDOW;

	return 0;
}

int rtnl_link_macsec_get_window(struct rtnl_link *link, uint32_t *window)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_WINDOW))
		return -NLE_NOATTR;

	if (window)
		*window = info->window;

	return 0;
}

int rtnl_link_macsec_set_send_sci(struct rtnl_link *link, uint8_t send_sci)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (send_sci > 1)
		return -NLE_INVAL;

	info->send_sci = send_sci;
	info->ce_mask |= MACSEC_ATTR_INC_SCI;

	return 0;
}

int rtnl_link_macsec_get_send_sci(struct rtnl_link *link, uint8_t *send_sci)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_INC_SCI))
		return -NLE_NOATTR;

	if (send_sci)
		*send_sci = info->send_sci;

	return 0;
}

int rtnl_link_macsec_set_end_station(struct rtnl_link *link, uint8_t end_station)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (end_station > 1)
		return -NLE_INVAL;

	info->end_station = end_station;
	info->ce_mask |= MACSEC_ATTR_ES;

	return 0;
}

int rtnl_link_macsec_get_end_station(struct rtnl_link *link, uint8_t *es)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_ES))
		return -NLE_NOATTR;

	if (es)
		*es = info->end_station;

	return 0;
}

int rtnl_link_macsec_set_scb(struct rtnl_link *link, uint8_t scb)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (scb > 1)
		return -NLE_INVAL;

	info->scb = scb;
	info->ce_mask |= MACSEC_ATTR_SCB;

	return 0;
}

int rtnl_link_macsec_get_scb(struct rtnl_link *link, uint8_t *scb)
{
	struct macsec_info *info = link->l_info;

	IS_MACSEC_LINK_ASSERT(link);

	if (!(info->ce_mask & MACSEC_ATTR_SCB))
		return -NLE_NOATTR;

	if (scb)
		*scb = info->scb;

	return 0;
}

/** @} */

/** @} */
