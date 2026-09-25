#include <errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "nl80211.h"
#include "iw.h"

SECTION(key);

static int print_keys(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_KEY_IDX]) {
		fprintf(stderr, "KEY_IDX missing!\n");
		return NL_SKIP;
	}

	if (!tb[NL80211_ATTR_KEY_DATA]) {
		fprintf(stderr, "ATTR_KEY_DATA missing!\n");
		return NL_SKIP;
	}

	iw_hexdump("Key", nla_data(tb[NL80211_ATTR_KEY_DATA]),
		   nla_len(tb[NL80211_ATTR_KEY_DATA]));

	if (!tb[NL80211_ATTR_KEY_SEQ]) {
		fprintf(stderr, "ATTR_KEY_SEQ missing!\n");
		return NL_SKIP;
	}

	iw_hexdump("Key seq", nla_data(tb[NL80211_ATTR_KEY_SEQ]),
		   nla_len(tb[NL80211_ATTR_KEY_SEQ]));

	return NL_OK;
}

static int handle_get_key(struct nl80211_state *state,
			  struct nl_msg *msg, int argc, char **argv,
			  enum id_input id)
{
	char *end;
	unsigned char mac[6];

	/* key index */
	if (argc) {
		nla_put_u8(msg, NL80211_ATTR_KEY_IDX, strtoul(argv[0], &end, 10));
		if (*end != '\0')
			return -EINVAL;
		argv++;
		argc--;
	}

	/* mac */
	if (argc) {
		if (mac_addr_a2n(mac, argv[0]) == 0) {
			NLA_PUT(msg, NL80211_ATTR_MAC, 6, mac);
			nla_put_u32(msg, NL80211_ATTR_KEY_TYPE,
				    NL80211_KEYTYPE_PAIRWISE);
			argv++;
			argc--;
		} else {
			return -EINVAL;
		}
	} else {
		nla_put_u32(msg, NL80211_ATTR_KEY_TYPE, NL80211_KEYTYPE_GROUP);
	}

	register_handler(print_keys, NULL);
	return 0;

 nla_put_failure:
	return -ENOSPC;
}
COMMAND(key, get, "<key index> <MAC address>",
	NL80211_CMD_GET_KEY, 0, CIB_NETDEV, handle_get_key,
	"Retrieve a key and key sequence.\n");
