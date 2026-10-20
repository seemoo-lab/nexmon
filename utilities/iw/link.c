#include <net/if.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "iw.h"

struct link_result {
	uint8_t sta_addr[8];
	bool link_found;
	bool anything_found;
	bool mld;
};

static struct link_result lr = { .link_found = false };

static int link_bss_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_FREQUENCY_OFFSET] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
	};
	struct link_result *result = arg;
	char mac_addr[20], dev[20], link_addr[20];
	int link_id = -1;
	const char *indent = "\t";
	int freq_offset = 0;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_BSS]) {
		fprintf(stderr, "bss info missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(bss, NL80211_BSS_MAX,
			     tb[NL80211_ATTR_BSS],
			     bss_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (!bss[NL80211_BSS_BSSID])
		return NL_SKIP;

	if (!bss[NL80211_BSS_STATUS])
		return NL_SKIP;

	mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_BSSID]));
	if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);

	if (bss[NL80211_BSS_MLO_LINK_ID])
		link_id = nla_get_u8(bss[NL80211_BSS_MLO_LINK_ID]);

	if (bss[NL80211_BSS_MLD_ADDR]) {
		mac_addr_n2a(link_addr, nla_data(bss[NL80211_BSS_BSSID]));
		indent = "\t\t";

		if (result->mld) {
			if (memcmp(result->sta_addr,
				   nla_data(bss[NL80211_BSS_MLD_ADDR]), 6)) {
				mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_MLD_ADDR]));
				printf("!! inconsistent MLD address information (%s)\n",
				       mac_addr);
			}
		} else {
			mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_MLD_ADDR]));
			result->mld = true;
			memcpy(result->sta_addr,
			       nla_data(bss[NL80211_BSS_MLD_ADDR]), 6);
			if (nla_get_u32(bss[NL80211_BSS_STATUS]) == NL80211_BSS_STATUS_ASSOCIATED) {
				printf("Connected to %s (on %s)\n", mac_addr, dev);
			}

			if (bss[NL80211_BSS_INFORMATION_ELEMENTS])
				print_ies(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
					  nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
					  false, PRINT_LINK_MLO_MLD, false);
		}
	} else {
		memcpy(result->sta_addr, nla_data(bss[NL80211_BSS_BSSID]), 6);
	}

	switch (nla_get_u32(bss[NL80211_BSS_STATUS])) {
	case NL80211_BSS_STATUS_ASSOCIATED:
		if (result->mld)
			printf("\tLink %d BSSID %s\n", link_id, link_addr);
		else
			printf("Connected to %s (on %s)\n", mac_addr, dev);
		break;
	case NL80211_BSS_STATUS_AUTHENTICATED:
		printf("Authenticated with %s (on %s)\n", mac_addr, dev);
		return NL_SKIP;
	case NL80211_BSS_STATUS_IBSS_JOINED:
		printf("Joined IBSS %s (on %s)\n", mac_addr, dev);
		break;
	default:
		return NL_SKIP;
	}

	result->anything_found = true;

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS])
		print_ies(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
			  nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
			  false, result->mld ? PRINT_LINK_MLO_LINK : PRINT_LINK,
			  false);

	if (bss[NL80211_BSS_FREQUENCY_OFFSET])
		freq_offset = nla_get_u32(bss[NL80211_BSS_FREQUENCY_OFFSET]);

	if (bss[NL80211_BSS_FREQUENCY])
		printf("%sfreq: %d.%d\n", indent,
			nla_get_u32(bss[NL80211_BSS_FREQUENCY]), freq_offset);

	if (nla_get_u32(bss[NL80211_BSS_STATUS]) != NL80211_BSS_STATUS_ASSOCIATED)
		return NL_SKIP;

	/* only in the assoc case do we want more info from station get */
	result->link_found = true;
	return NL_SKIP;
}

static int handle_scan_for_link(struct nl80211_state *state,
				struct nl_msg *msg,
				int argc, char **argv,
				enum id_input id)
{
	if (argc > 0)
		return 1;

	register_handler(link_bss_handler, &lr);
	return 0;
}

static int print_link_sta(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	struct nlattr *binfo[NL80211_STA_BSS_PARAM_MAX + 1];
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
		[NL80211_STA_INFO_RX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
	};
	static struct nla_policy bss_policy[NL80211_STA_BSS_PARAM_MAX + 1] = {
		[NL80211_STA_BSS_PARAM_CTS_PROT] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_DTIM_PERIOD] = { .type = NLA_U8 },
		[NL80211_STA_BSS_PARAM_BEACON_INTERVAL] = { .type = NLA_U16 },
	};

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (sinfo[NL80211_STA_INFO_RX_BYTES] && sinfo[NL80211_STA_INFO_RX_PACKETS])
		printf("\tRX: %u bytes (%u packets)\n",
			nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]),
			nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]));
	if (sinfo[NL80211_STA_INFO_TX_BYTES] && sinfo[NL80211_STA_INFO_TX_PACKETS])
		printf("\tTX: %u bytes (%u packets)\n",
			nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]),
			nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]));
	if (sinfo[NL80211_STA_INFO_SIGNAL])
		printf("\tsignal: %d dBm\n",
			(int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]));

	if (sinfo[NL80211_STA_INFO_RX_BITRATE]) {
		char buf[100];

		parse_bitrate(sinfo[NL80211_STA_INFO_RX_BITRATE], buf, sizeof(buf));
		printf("\trx bitrate: %s\n", buf);
	}
	if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
		char buf[100];

		parse_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
		printf("\ttx bitrate: %s\n", buf);
	}

	if (sinfo[NL80211_STA_INFO_BSS_PARAM]) {
		if (nla_parse_nested(binfo, NL80211_STA_BSS_PARAM_MAX,
				     sinfo[NL80211_STA_INFO_BSS_PARAM],
				     bss_policy)) {
			fprintf(stderr, "failed to parse nested bss parameters!\n");
		} else {
			char *delim = "";
			printf("\tbss flags: ");
			if (binfo[NL80211_STA_BSS_PARAM_CTS_PROT]) {
				printf("CTS-protection");
				delim = " ";
			}
			if (binfo[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE]) {
				printf("%sshort-preamble", delim);
				delim = " ";
			}
			if (binfo[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME])
				printf("%sshort-slot-time", delim);
			printf("\n\tdtim period: %d",
			       nla_get_u8(binfo[NL80211_STA_BSS_PARAM_DTIM_PERIOD]));
			printf("\n\tbeacon int: %d",
			       nla_get_u16(binfo[NL80211_STA_BSS_PARAM_BEACON_INTERVAL]));
			printf("\n");
		}
	}

	return NL_SKIP;
}

static int handle_link_sta(struct nl80211_state *state,
			   struct nl_msg *msg,
			   int argc, char **argv,
			   enum id_input id)
{
	unsigned char mac_addr[ETH_ALEN];

	if (argc < 1)
		return 1;

	if (mac_addr_a2n(mac_addr, argv[0])) {
		fprintf(stderr, "invalid mac address\n");
		return 2;
	}

	argc--;
	argv++;

	if (argc)
		return 1;

	NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, mac_addr);

	register_handler(print_link_sta, NULL);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}

static int handle_link(struct nl80211_state *state,
		       struct nl_msg *msg, int argc, char **argv,
		       enum id_input id)
{
	char *link_argv[] = {
		NULL,
		"link",
		"get_bss",
		NULL,
	};
	char *station_argv[] = {
		NULL,
		"link",
		"get_sta",
		NULL,
		NULL,
	};
	char addr_buf[3*6];
	int err;

	link_argv[0] = argv[0];
	err = handle_cmd(state, id, 3, link_argv);
	if (err)
		return err;

	if (!lr.link_found) {
		if (!lr.anything_found)
			printf("Not connected.\n");
		return 0;
	}

	mac_addr_n2a(addr_buf, lr.sta_addr);
	addr_buf[17] = '\0';

	if (lr.mld)
		printf("MLD %s stats:\n", addr_buf);

	station_argv[0] = argv[0];
	station_argv[3] = addr_buf;
	return handle_cmd(state, id, 4, station_argv);
}
TOPLEVEL(link, NULL, 0, 0, CIB_NETDEV, handle_link,
	 "Print information about the current connection, if any.");
HIDDEN(link, get_sta, "<mac-addr>", NL80211_CMD_GET_STATION, 0,
	CIB_NETDEV, handle_link_sta);
HIDDEN(link, get_bss, NULL, NL80211_CMD_GET_SCAN, NLM_F_DUMP,
	CIB_NETDEV, handle_scan_for_link);
