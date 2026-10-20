#include <errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "nl80211.h"
#include "iw.h"

SECTION(ap);

static int handle_start_ap(struct nl80211_state *state,
			   struct nl_msg *msg, int argc, char **argv,
			   enum id_input id)
{
	struct chandef chandef;
	int res, parsed;
	char *end;
	int val, len;
	char buf[2304];

	if (argc < 6)
		return 1;

	/* SSID */
	NLA_PUT(msg, NL80211_ATTR_SSID, strlen(argv[0]), argv[0]);
	argv++;
	argc--;

	/* chandef */
	res = parse_freqchan(&chandef, false, argc, argv, &parsed, false);
	if (res)
		return res;
	argc -= parsed;
	argv += parsed;
	res = put_chandef(msg, &chandef);
	if (res)
		return res;

	/* beacon interval */
	val = strtoul(argv[0], &end, 10);
	if (*end != '\0')
		return -EINVAL;

	NLA_PUT_U32(msg, NL80211_ATTR_BEACON_INTERVAL, val);
	argv++;
	argc--;

	/* dtim */
	val = strtoul(argv[0], &end, 10);
	if (*end != '\0')
		return -EINVAL;

	NLA_PUT_U32(msg, NL80211_ATTR_DTIM_PERIOD, val);
	argv++;
	argc--;

	if (strcmp(argv[0], "hidden-ssid") == 0) {
		argc--;
		argv++;
		NLA_PUT_U32(msg, NL80211_ATTR_HIDDEN_SSID,
			    NL80211_HIDDEN_SSID_ZERO_LEN);
	} else if (strcmp(argv[0], "zeroed-ssid") == 0) {
		argc--;
		argv++;
		NLA_PUT_U32(msg, NL80211_ATTR_HIDDEN_SSID,
			    NL80211_HIDDEN_SSID_ZERO_CONTENTS);
	}

	/* beacon head must be provided */
	if (strcmp(argv[0], "head") != 0)
		return 1;
	argv++;
	argc--;

	len = strlen(argv[0]);
	if (!len || (len % 2))
		return -EINVAL;

	if (!hex2bin(&argv[0][0], buf))
		return -EINVAL;

	NLA_PUT(msg, NL80211_ATTR_BEACON_HEAD, (len / 2), &buf);
	argv++;
	argc--;

	if (!argc)
		return 0;

	/* tail is optional */
	if (strcmp(argv[0], "tail") == 0) {
		argv++;
		argc--;

		if (!argc)
			return -EINVAL;

		len = strlen(argv[0]);
		if (!len || (len % 2))
			return -EINVAL;

		if (!hex2bin(&argv[0][0], buf))
			return -EINVAL;

		NLA_PUT(msg, NL80211_ATTR_BEACON_TAIL, (len / 2), &buf);
		argv++;
		argc--;
	}

	if (!argc)
		return 0;

	/* inactivity time (optional) */
	if (strcmp(argv[0], "inactivity-time") == 0) {
		argv++;
		argc--;

		if (!argc)
			return -EINVAL;
		len = strlen(argv[0]);
		if (!len)
			return -EINVAL;

		val = strtoul(argv[0], &end, 10);
		if (*end != '\0')
			return -EINVAL;

		NLA_PUT_U16(msg, NL80211_ATTR_INACTIVITY_TIMEOUT, val);
		argv++;
		argc--;
	}

	if (!argc) {
		return 0;
	}

	if (strcmp(*argv, "key") != 0 && strcmp(*argv, "keys") != 0)
		return 1;

	argv++;
	argc--;

	return parse_keys(msg, &argv, &argc);
 nla_put_failure:
	return -ENOSPC;
}
COMMAND(ap, start,
	"<SSID> "
	PARSE_FREQ_ARGS("<SSID> ",
	" <beacon interval in TU> <DTIM period> [hidden-ssid|zeroed-ssid] head"
	" <beacon head in hexadecimal> [tail <beacon tail in hexadecimal>]"
	" [inactivity-time <inactivity time in seconds>] [key0:abcde d:1:6162636465]"),
	NL80211_CMD_NEW_BEACON, 0, CIB_NETDEV, handle_start_ap,
	"Start an AP. Note that this usually requires hostapd or similar.\n");

static int handle_stop_ap(struct nl80211_state *state,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	return 0;
}
COMMAND(ap, stop, "",
	NL80211_CMD_DEL_BEACON, 0, CIB_NETDEV, handle_stop_ap,
	"Stop AP functionality\n");
