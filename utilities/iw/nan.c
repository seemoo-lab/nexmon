#include <net/if.h>
#include <errno.h>
#include <string.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <ctype.h>
#include <inttypes.h>
#include "nl80211.h"
#include "iw.h"
#include "sha256.h"

SECTION(nan);

static int parse_bands(int argc, char **argv)
{
	int i = 0, bands = 0;

	for (i = 0; i < argc; i++) {
		if (!strcasecmp("2ghz", argv[i]))
			bands |= BIT(NL80211_BAND_2GHZ);
		else if (!strcasecmp("5ghz", argv[i]))
			bands |= BIT(NL80211_BAND_5GHZ);
		else
			return -EINVAL;
	}

	return bands;
}

static int handle_nan_start(struct nl80211_state *state,
			    struct nl_msg *msg, int argc, char **argv,
			    enum id_input id)
{
	int bands = 0;

	if (argc < 2)
		return -EINVAL;

	if (strcmp(argv[0], "pref") == 0) {
		argv++;
		argc--;
		NLA_PUT_U8(msg, NL80211_ATTR_NAN_MASTER_PREF, atoi(argv[0]));
		argv++;
		argc--;
	} else {
		/* Master preference is mandatory */
		return -EINVAL;
	}

	if (argc > 1 && !strcmp(argv[0], "bands")) {
		argv++;
		argc--;

		bands = parse_bands(argc, argv);
		if (bands < 0)
			return bands;

		NLA_PUT_U32(msg, NL80211_ATTR_BANDS, bands);
	} else if (argc != 0)
		return -EINVAL;

	return 0;
nla_put_failure:
	return -ENOBUFS;
}
COMMAND(nan, start, "pref <pref> [bands [2GHz] [5GHz]]",
	NL80211_CMD_START_NAN, 0, CIB_WDEV, handle_nan_start, "");

static int handle_nan_stop(struct nl80211_state *state,
			   struct nl_msg *msg, int argc, char **argv,
			   enum id_input id)
{
	return 0;
}
COMMAND(nan, stop, "", NL80211_CMD_STOP_NAN, 0, CIB_WDEV, handle_nan_stop, "");

static int handle_nan_config(struct nl80211_state *state,
			     struct nl_msg *msg, int argc, char **argv,
			     enum id_input id)
{
	int bands = 0;

	if (argc < 2)
		return -EINVAL;

	if (strcmp(argv[0], "pref") == 0) {
		argv++;
		argc--;
		NLA_PUT_U8(msg, NL80211_ATTR_NAN_MASTER_PREF, atoi(argv[0]));
		argv++;
		argc--;
	}

	if (argc > 1 && !strcmp(argv[0], "bands")) {
		argv++;
		argc--;

		bands = parse_bands(argc, argv);
		if (bands < 0)
			return bands;

		NLA_PUT_U32(msg, NL80211_ATTR_BANDS, bands);
		argv++;
		argc--;
	} else if (argc != 0)
		return -EINVAL;

	return 0;
nla_put_failure:
	return -ENOBUFS;
}
COMMAND(nan, config, "[pref <pref>] [bands [2GHz] [5GHz]]",
	NL80211_CMD_CHANGE_NAN_CONFIG, 0, CIB_WDEV, handle_nan_config, "");

static int handle_nan_rm_func(struct nl80211_state *state,
			      struct nl_msg *msg, int argc, char **argv,
			      enum id_input id)
{
	if (argc != 2)
		return -EINVAL;

	if (strcmp(argv[0], "cookie") == 0) {
		argv++;
		argc--;
		NLA_PUT_U64(msg, NL80211_ATTR_COOKIE, atoi(argv[0]));
		argv++;
		argc--;
	}

	if (argc != 0)
		return -EINVAL;

	return 0;
nla_put_failure:
	return -ENOBUFS;
}
COMMAND(nan, rm_func, "cookie <cookie>", NL80211_CMD_DEL_NAN_FUNCTION, 0,
	CIB_WDEV, handle_nan_rm_func, "");

static int compute_service_id(const unsigned char *serv_name,
			      unsigned int len, unsigned char *res)
{
	size_t size = len;
	unsigned char md_value[32];
	int retcode = sha256(serv_name, size, md_value);

	if (retcode)
		return retcode;
	memcpy(res, md_value, 6);

	return 0;
}

static int print_instance_id_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *func[NL80211_NAN_FUNC_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_COOKIE]) {
		fprintf(stderr, "cookie is missing!\n");
		return NL_SKIP;
	}

	nla_parse_nested(func, NL80211_NAN_FUNC_ATTR_MAX,
			 tb[NL80211_ATTR_NAN_FUNC],
			 NULL);
	if (!func[NL80211_NAN_FUNC_INSTANCE_ID]) {
		fprintf(stderr, "instance id is missing!\n");
		return NL_SKIP;
	}

	printf("instance_id: %d, cookie: %" PRIu64 "\n",
	       nla_get_u8(func[NL80211_NAN_FUNC_INSTANCE_ID]),
	       nla_get_u64(tb[NL80211_ATTR_COOKIE]));

	return NL_SKIP;
}

static int parse_srf(char **argv, int argc, struct nl_msg *func_attrs)
{
	struct nl_msg *srf_attrs;
	int old_argc = argc;
	unsigned char mac_addr[ETH_ALEN];
	char *cur_mac, *sptr = NULL;

	srf_attrs = nlmsg_alloc();
	if (strcmp(argv[0], "include") == 0)
		NLA_PUT_FLAG(srf_attrs, NL80211_NAN_SRF_INCLUDE);
	else if (strcmp(argv[0], "exclude") != 0)
		return -EINVAL;

	argc--;
	argv++;
	if (strcmp(argv[0], "bf") == 0) {
		unsigned char *srf;
		size_t srf_len;
		__u8 bf_idx;
		int err;

		argc--;
		argv++;

		if (argc < 3)
			return -EINVAL;

		bf_idx = atoi(argv[0]);
		NLA_PUT_U8(srf_attrs, NL80211_NAN_SRF_BF_IDX, bf_idx);

		argc--;
		argv++;
		srf_len = atoi(argv[0]);
		if (srf_len == 0 || srf_len > NL80211_NAN_FUNC_SRF_MAX_LEN)
			return -EINVAL;

		argc--;
		argv++;
		srf = malloc(srf_len);
		if (!srf)
			return -ENOBUFS;

		memset(srf, 0, srf_len);
		cur_mac = strtok_r(argv[0], ";", &sptr);
		while (cur_mac) {
			if (mac_addr_a2n(mac_addr, cur_mac)) {
				printf("mac format error %s\n", cur_mac);
				free(srf);
				return -EINVAL;
			}

			nan_bf(bf_idx, srf, srf_len, mac_addr, ETH_ALEN);
			cur_mac = strtok_r(NULL, ";", &sptr);
		}

		err = nla_put(srf_attrs, NL80211_NAN_SRF_BF, srf_len, srf);
		free(srf);
		if (err)
			goto nla_put_failure;
		argv++;
		argc--;
	} else if  (strcmp(argv[0], "list") == 0) {
		struct nlattr *nl_macs = nla_nest_start(srf_attrs,
						NL80211_NAN_SRF_MAC_ADDRS);
		int i = 0;

		argc--;
		argv++;
		cur_mac = strtok_r(argv[0], ";", &sptr);
		while (cur_mac) {
			if (mac_addr_a2n(mac_addr, cur_mac))
				return -EINVAL;

			nla_put(srf_attrs, ++i, ETH_ALEN, mac_addr);
			cur_mac = strtok_r(NULL, ";", &sptr);
		}

		nla_nest_end(srf_attrs, nl_macs);
		argv++;
		argc--;
	} else {
		return -EINVAL;
	}

	nla_put_nested(func_attrs, NL80211_NAN_FUNC_SRF, srf_attrs);
	return old_argc - argc;
nla_put_failure:
	return -ENOBUFS;
}

static void parse_match_filter(char *filter, struct nl_msg *func_attrs, int tx)
{
	struct nlattr *nl_filt;
	char *cur_filt, *sptr = NULL;
	int i = 0;

	if (tx)
		nl_filt = nla_nest_start(func_attrs,
					 NL80211_NAN_FUNC_TX_MATCH_FILTER);
	else
		nl_filt = nla_nest_start(func_attrs,
					 NL80211_NAN_FUNC_RX_MATCH_FILTER);

	cur_filt = strtok_r(filter, ":", &sptr);
	while (cur_filt) {
		if (strcmp(cur_filt, "*") != 0)
			nla_put(func_attrs, ++i, strlen(cur_filt), cur_filt);
		else
			nla_put(func_attrs, ++i, 0, NULL);

		cur_filt = strtok_r(NULL, ":", &sptr);
	}

	nla_nest_end(func_attrs, nl_filt);
}

static int handle_nan_add_func(struct nl80211_state *state,
			       struct nl_msg *msg, int argc, char **argv,
			       enum id_input id)
{
	struct nl_msg *func_attrs = NULL;
	int err = 0;
	__u8 type;

	func_attrs = nlmsg_alloc();
	if (!func_attrs) {
		err = -ENOBUFS;
		goto out;
	}

	if (argc > 1 && strcmp(argv[0], "type") == 0) {
		argv++;
		argc--;
		if (strcmp(argv[0], "publish") == 0)
			type = NL80211_NAN_FUNC_PUBLISH;
		else if (strcmp(argv[0], "subscribe") == 0)
			type = NL80211_NAN_FUNC_SUBSCRIBE;
		else if (strcmp(argv[0], "followup") == 0)
			type = NL80211_NAN_FUNC_FOLLOW_UP;
		else
			return -EINVAL;
		argv++;
		argc--;

		NLA_PUT_U8(func_attrs, NL80211_NAN_FUNC_TYPE, type);
	} else {
		return -EINVAL;
	}

	if (type == NL80211_NAN_FUNC_SUBSCRIBE) {
		if (argc > 1 && strcmp(argv[0], "active") == 0) {
			argv++;
			argc--;
			NLA_PUT_FLAG(func_attrs,
				     NL80211_NAN_FUNC_SUBSCRIBE_ACTIVE);
		}
	}

	if (type == NL80211_NAN_FUNC_PUBLISH) {
		__u8 publish_type = 0;

		if (argc > 1 && strcmp(argv[0], "solicited") == 0) {
			argv++;
			argc--;
			publish_type |= NL80211_NAN_SOLICITED_PUBLISH;
		}

		if (argc > 1 && strcmp(argv[0], "unsolicited") == 0) {
			argv++;
			argc--;
			publish_type |= NL80211_NAN_UNSOLICITED_PUBLISH;
		}

		NLA_PUT_U8(func_attrs, NL80211_NAN_FUNC_PUBLISH_TYPE,
			   publish_type);

		/* only allow for solicited publish */
		if (argc > 1 && strcmp(argv[0], "bcast") == 0) {
			argv++;
			argc--;
			if (!(publish_type & NL80211_NAN_SOLICITED_PUBLISH))
				return -EINVAL;

			NLA_PUT_FLAG(func_attrs,
				     NL80211_NAN_FUNC_PUBLISH_BCAST);
		}
	}

	if (argc > 1 && strcmp(argv[0], "close_range") == 0) {
		argv++;
		argc--;
		NLA_PUT_FLAG(func_attrs, NL80211_NAN_FUNC_CLOSE_RANGE);
	}

	if (argc > 1 && strcmp(argv[0], "name") == 0) {
		unsigned char serv_id_c[6] = {0};
		__u64 service_id;

		argv++;
		argc--;
		compute_service_id((const unsigned char *)argv[0],
				   strlen(argv[0]), serv_id_c);
		service_id = (__u64)serv_id_c[0] << 0  |
			     (__u64)serv_id_c[1] << 8  |
			     (__u64)serv_id_c[2] << 16 |
			     (__u64)serv_id_c[3] << 24 |
			     (__u64)serv_id_c[4] << 32 |
			     (__u64)serv_id_c[5] << 40;

		NLA_PUT(func_attrs, NL80211_NAN_FUNC_SERVICE_ID, 6,
			&service_id);
		argv++;
		argc--;
	} else {
		return -EINVAL;
	}

	if (argc > 1 && strcmp(argv[0], "info") == 0) {
		argv++;
		argc--;
		NLA_PUT(func_attrs, NL80211_NAN_FUNC_SERVICE_INFO,
			strlen(argv[0]), argv[0]);
		argv++;
		argc--;
	}

	if (type == NL80211_NAN_FUNC_FOLLOW_UP) {
		if (argc > 1 && strcmp(argv[0], "flw_up_id") == 0) {
			argv++;
			argc--;
			NLA_PUT_U8(func_attrs, NL80211_NAN_FUNC_FOLLOW_UP_ID,
				   atoi(argv[0]));
			argv++;
			argc--;
		}

		if (argc > 1 && strcmp(argv[0], "flw_up_req_id") == 0) {
			argv++;
			argc--;
			NLA_PUT_U8(func_attrs,
				   NL80211_NAN_FUNC_FOLLOW_UP_REQ_ID,
				   atoi(argv[0]));
			argv++;
			argc--;
		}

		if (argc > 1 && strcmp(argv[0], "flw_up_dest") == 0) {
			unsigned char addr[6];

			argv++;
			argc--;
			if (mac_addr_a2n(addr, argv[0]))
				goto nla_put_failure;
			nla_put(func_attrs, NL80211_NAN_FUNC_FOLLOW_UP_DEST,
				ETH_ALEN, addr);
			argv++;
			argc--;
		}
	}

	if (type != NL80211_NAN_FUNC_FOLLOW_UP &&
	    argc > 1 && strcmp(argv[0], "ttl") == 0) {
		argv++;
		argc--;
		NLA_PUT_U32(func_attrs, NL80211_NAN_FUNC_TTL, atoi(argv[0]));
		argv++;
		argc--;
	}

	if (type != NL80211_NAN_FUNC_FOLLOW_UP &&
	    argc >= 4 && strcmp(argv[0], "srf") == 0) {
		int res;

		argv++;
		argc--;
		res = parse_srf(argv, argc, func_attrs);
		if (res < 0)
			return -EINVAL;

		argc -= res;
		argv += res;
	}

	if (type != NL80211_NAN_FUNC_FOLLOW_UP &&
	    argc > 1 && strcmp(argv[0], "rx_filter") == 0) {
		argv++;
		argc--;
		parse_match_filter(argv[0], func_attrs, 0);

		argv++;
		argc--;
	}

	if (type != NL80211_NAN_FUNC_FOLLOW_UP &&
	    argc > 1 && strcmp(argv[0], "tx_filter") == 0) {
		argv++;
		argc--;
		parse_match_filter(argv[0], func_attrs, 1);

		argv++;
		argc--;
	}

	if (argc != 0)
		return -EINVAL;

	nla_put_nested(msg, NL80211_ATTR_NAN_FUNC, func_attrs);
	register_handler(print_instance_id_handler, NULL);

	return err;
nla_put_failure:
	return -ENOBUFS;
out:
	return err;
}
COMMAND(nan, add_func,
	"type <publish|subscribe|followup> [active] [solicited] [unsolicited] [bcast] [close_range] name <name> [info <info>] [flw_up_id <id> flw_up_req_id <id> flw_up_dest <mac>] [ttl <ttl>] [srf <include|exclude> <bf|list> [bf_idx] [bf_len] <mac1;mac2...>] [rx_filter <str1:str2...>] [tx_filter <str1:str2...>]",
	NL80211_CMD_ADD_NAN_FUNCTION, 0, CIB_WDEV,
	handle_nan_add_func, "");
