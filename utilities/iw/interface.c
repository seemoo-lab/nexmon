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

#define VALID_FLAGS	"none:     no special flags\n"\
			"fcsfail:  show frames with FCS errors\n"\
			"control:  show control frames\n"\
			"otherbss: show frames from other BSSes\n"\
			"cook:     use cooked mode\n"\
			"active:   use active mode (ACK incoming unicast packets)\n"\
			"mumimo-groupid <GROUP_ID>: use MUMIMO according to a group id\n"\
			"mumimo-follow-mac <MAC_ADDRESS>: use MUMIMO according to a MAC address"

SECTION(interface);

static char *mntr_flags[NL80211_MNTR_FLAG_MAX + 1] = {
	"none",
	"fcsfail",
	"plcpfail",
	"control",
	"otherbss",
	"cook",
	"active",
};

static int parse_mumimo_options(int *_argc, char ***_argv, struct nl_msg *msg)
{
	uint8_t mumimo_group[VHT_MUMIMO_GROUP_LEN];
	unsigned char mac_addr[ETH_ALEN];
	char **argv = *_argv;
	int argc = *_argc;
	int i;
	unsigned int val;

	if (strcmp(*argv, "mumimo-groupid") == 0) {
		argc--;
		argv++;
		if (!argc || strlen(*argv) != VHT_MUMIMO_GROUP_LEN*2) {
			fprintf(stderr, "Invalid groupID: %s\n", *argv);
			return 1;
		}

		for (i = 0; i < VHT_MUMIMO_GROUP_LEN; i++) {
			if (sscanf((*argv) + i*2, "%2x", &val) != 1) {
				fprintf(stderr, "Failed reading groupID\n");
				return 1;
			}
			mumimo_group[i] = val;
		}

		NLA_PUT(msg,
			NL80211_ATTR_MU_MIMO_GROUP_DATA,
			VHT_MUMIMO_GROUP_LEN,
			mumimo_group);
		argc--;
		argv++;
	} else if (strcmp(*argv, "mumimo-follow-mac") == 0) {
		argc--;
		argv++;
		if (!argc || mac_addr_a2n(mac_addr, *argv)) {
			fprintf(stderr, "Invalid MAC address\n");
			return 1;
		}
		NLA_PUT(msg, NL80211_ATTR_MU_MIMO_FOLLOW_MAC_ADDR,
			ETH_ALEN, mac_addr);
		argc--;
		argv++;
	}
 nla_put_failure:
	*_argc = argc;
	*_argv = argv;
	return 0;
}

static int parse_mntr_flags(int *_argc, char ***_argv,
			    struct nl_msg *msg)
{
	struct nl_msg *flags;
	int err = -ENOBUFS;
	enum nl80211_mntr_flags flag;
	int argc = *_argc;
	char **argv = *_argv;

	flags = nlmsg_alloc();
	if (!flags)
		return -ENOMEM;

	while (argc) {
		int ok = 0;

		/* parse MU-MIMO options */
		err = parse_mumimo_options(&argc, &argv, msg);
		if (err)
			goto out;
		else if (!argc)
			break;

		/* parse monitor flags */
		for (flag = __NL80211_MNTR_FLAG_INVALID;
		     flag <= NL80211_MNTR_FLAG_MAX; flag++) {
			if (strcmp(*argv, mntr_flags[flag]) == 0) {
				ok = 1;
				/*
				 * This shouldn't be adding "flag" if that is
				 * zero, but due to a problem in the kernel's
				 * nl80211 code (using NLA_NESTED policy) it
				 * will reject an empty nested attribute but
				 * not one that contains an invalid attribute
				 */
				NLA_PUT_FLAG(flags, flag);
				break;
			}
		}
		if (!ok) {
			err = -EINVAL;
			goto out;
		}
		argc--;
		argv++;
	}

	nla_put_nested(msg, NL80211_ATTR_MNTR_FLAGS, flags);
	err = 0;
 nla_put_failure:
 out:
	nlmsg_free(flags);

	*_argc = argc;
	*_argv = argv;

	return err;
}

/* for help */
#define IFACE_TYPES "Valid interface types are: managed, ibss, monitor, mesh, wds."

/* return 0 if ok, internal error otherwise */
static int get_if_type(int *argc, char ***argv, enum nl80211_iftype *type,
		       bool need_type)
{
	char *tpstr;

	if (*argc < 1 + !!need_type)
		return 1;

	if (need_type && strcmp((*argv)[0], "type"))
		return 1;

	tpstr = (*argv)[!!need_type];
	*argc -= 1 + !!need_type;
	*argv += 1 + !!need_type;

	if (strcmp(tpstr, "adhoc") == 0 ||
	    strcmp(tpstr, "ibss") == 0) {
		*type = NL80211_IFTYPE_ADHOC;
		return 0;
	} else if (strcmp(tpstr, "ocb") == 0) {
		*type = NL80211_IFTYPE_OCB;
		return 0;
	} else if (strcmp(tpstr, "monitor") == 0) {
		*type = NL80211_IFTYPE_MONITOR;
		return 0;
	} else if (strcmp(tpstr, "master") == 0 ||
		   strcmp(tpstr, "ap") == 0) {
		*type = NL80211_IFTYPE_UNSPECIFIED;
		fprintf(stderr, "You need to run a management daemon, e.g. hostapd,\n");
		fprintf(stderr, "see http://wireless.kernel.org/en/users/Documentation/hostapd\n");
		fprintf(stderr, "for more information on how to do that.\n");
		return 2;
	} else if (strcmp(tpstr, "__ap") == 0) {
		*type = NL80211_IFTYPE_AP;
		return 0;
	} else if (strcmp(tpstr, "__ap_vlan") == 0) {
		*type = NL80211_IFTYPE_AP_VLAN;
		return 0;
	} else if (strcmp(tpstr, "wds") == 0) {
		*type = NL80211_IFTYPE_WDS;
		return 0;
	} else if (strcmp(tpstr, "managed") == 0 ||
		   strcmp(tpstr, "mgd") == 0 ||
		   strcmp(tpstr, "station") == 0) {
		*type = NL80211_IFTYPE_STATION;
		return 0;
	} else if (strcmp(tpstr, "mp") == 0 ||
		   strcmp(tpstr, "mesh") == 0) {
		*type = NL80211_IFTYPE_MESH_POINT;
		return 0;
	} else if (strcmp(tpstr, "__p2pcl") == 0) {
		*type = NL80211_IFTYPE_P2P_CLIENT;
		return 0;
	} else if (strcmp(tpstr, "__p2pdev") == 0) {
		*type = NL80211_IFTYPE_P2P_DEVICE;
		return 0;
	} else if (strcmp(tpstr, "__p2pgo") == 0) {
		*type = NL80211_IFTYPE_P2P_GO;
		return 0;
	} else if (strcmp(tpstr, "__nan") == 0) {
		*type = NL80211_IFTYPE_NAN;
		return 0;
	}

	fprintf(stderr, "invalid interface type %s\n", tpstr);
	return 2;
}

static int parse_4addr_flag(const char *value, struct nl_msg *msg)
{
	if (strcmp(value, "on") == 0)
		NLA_PUT_U8(msg, NL80211_ATTR_4ADDR, 1);
	else if (strcmp(value, "off") == 0)
		NLA_PUT_U8(msg, NL80211_ATTR_4ADDR, 0);
	else
		return 1;
	return 0;

nla_put_failure:
	return 1;
}

static int handle_interface_add(struct nl80211_state *state,
				struct nl_msg *msg,
				int argc, char **argv,
				enum id_input id)
{
	char *name;
	char *mesh_id = NULL;
	enum nl80211_iftype type;
	int tpset;
	unsigned char mac_addr[ETH_ALEN];
	int found_mac = 0;

	if (argc < 1)
		return 1;

	name = argv[0];
	argc--;
	argv++;

	tpset = get_if_type(&argc, &argv, &type, true);
	if (tpset)
		return tpset;

try_another:
	if (argc) {
		if (strcmp(argv[0], "mesh_id") == 0) {
			argc--;
			argv++;

			if (!argc)
				return 1;
			mesh_id = argv[0];
			argc--;
			argv++;
		} else if (strcmp(argv[0], "addr") == 0) {
			argc--;
			argv++;
			if (mac_addr_a2n(mac_addr, argv[0])) {
				fprintf(stderr, "Invalid MAC address\n");
				return 2;
			}
			argc--;
			argv++;
			found_mac = 1;
			goto try_another;
		} else if (strcmp(argv[0], "4addr") == 0) {
			argc--;
			argv++;
			if (parse_4addr_flag(argv[0], msg)) {
				fprintf(stderr, "4addr error\n");
				return 2;
			}
			argc--;
			argv++;
		} else if (strcmp(argv[0], "flags") == 0) {
			argc--;
			argv++;
			if (parse_mntr_flags(&argc, &argv, msg)) {
				fprintf(stderr, "flags error\n");
				return 2;
			}
		} else {
			return 1;
		}
	}

	if (argc)
		return 1;

	NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, name);
	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, type);
	if (mesh_id)
		NLA_PUT(msg, NL80211_ATTR_MESH_ID, strlen(mesh_id), mesh_id);
	if (found_mac)
		NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, mac_addr);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(interface, add, "<name> type <type> [mesh_id <meshid>] [4addr on|off] [flags <flag>*] [addr <mac-addr>]",
	NL80211_CMD_NEW_INTERFACE, 0, CIB_PHY, handle_interface_add,
	"Add a new virtual interface with the given configuration.\n"
	IFACE_TYPES "\n\n"
	"The flags are only used for monitor interfaces, valid flags are:\n"
	VALID_FLAGS "\n\n"
	"The mesh_id is used only for mesh mode.");
COMMAND(interface, add, "<name> type <type> [mesh_id <meshid>] [4addr on|off] [flags <flag>*] [addr <mac-addr>]",
	NL80211_CMD_NEW_INTERFACE, 0, CIB_NETDEV, handle_interface_add, NULL);

static int handle_interface_del(struct nl80211_state *state,
				struct nl_msg *msg,
				int argc, char **argv,
				enum id_input id)
{
	return 0;
}
TOPLEVEL(del, NULL, NL80211_CMD_DEL_INTERFACE, 0, CIB_NETDEV, handle_interface_del,
	 "Remove this virtual interface");
HIDDEN(interface, del, NULL, NL80211_CMD_DEL_INTERFACE, 0, CIB_NETDEV, handle_interface_del);

static char *channel_type_name(enum nl80211_channel_type channel_type)
{
	switch (channel_type) {
	case NL80211_CHAN_NO_HT:
		return "NO HT";
	case NL80211_CHAN_HT20:
		return "HT20";
	case NL80211_CHAN_HT40MINUS:
		return "HT40-";
	case NL80211_CHAN_HT40PLUS:
		return "HT40+";
	default:
		return "unknown";
	}
}

char *channel_width_name(enum nl80211_chan_width width)
{
	switch (width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
		return "20 MHz (no HT)";
	case NL80211_CHAN_WIDTH_20:
		return "20 MHz";
	case NL80211_CHAN_WIDTH_40:
		return "40 MHz";
	case NL80211_CHAN_WIDTH_80:
		return "80 MHz";
	case NL80211_CHAN_WIDTH_80P80:
		return "80+80 MHz";
	case NL80211_CHAN_WIDTH_160:
		return "160 MHz";
	case NL80211_CHAN_WIDTH_5:
		return "5 MHz";
	case NL80211_CHAN_WIDTH_10:
		return "10 MHz";
	case NL80211_CHAN_WIDTH_320:
		return "320 MHz";
	default:
		return "unknown";
	}
}

static void print_channel(struct nlattr **tb)
{
	uint32_t freq = nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]);

	printf("channel %d (%d MHz)",
	       ieee80211_frequency_to_channel(freq), freq);

	if (tb[NL80211_ATTR_CHANNEL_WIDTH]) {
		printf(", width: %s",
			channel_width_name(nla_get_u32(tb[NL80211_ATTR_CHANNEL_WIDTH])));
		if (tb[NL80211_ATTR_CENTER_FREQ1])
			printf(", center1: %d MHz",
				nla_get_u32(tb[NL80211_ATTR_CENTER_FREQ1]));
		if (tb[NL80211_ATTR_CENTER_FREQ2])
			printf(", center2: %d MHz",
				nla_get_u32(tb[NL80211_ATTR_CENTER_FREQ2]));

		if (tb[NL80211_ATTR_PUNCT_BITMAP]) {
			uint32_t punct = nla_get_u32(tb[NL80211_ATTR_PUNCT_BITMAP]);

			if (punct)
				printf(", punctured: 0x%x", punct);
		}
	} else if (tb[NL80211_ATTR_WIPHY_CHANNEL_TYPE]) {
		enum nl80211_channel_type channel_type;

		channel_type = nla_get_u32(tb[NL80211_ATTR_WIPHY_CHANNEL_TYPE]);
		printf(" %s", channel_type_name(channel_type));
	}
}

static int print_iface_handler(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	unsigned int *wiphy = arg;
	const char *indent = "";

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (wiphy && tb_msg[NL80211_ATTR_WIPHY]) {
		unsigned int thiswiphy = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
		indent = "\t";
		if (*wiphy != thiswiphy)
			printf("phy#%d\n", thiswiphy);
		*wiphy = thiswiphy;
	}

	if (tb_msg[NL80211_ATTR_IFNAME])
		printf("%sInterface %s\n", indent, nla_get_string(tb_msg[NL80211_ATTR_IFNAME]));
	else
		printf("%sUnnamed/non-netdev interface\n", indent);
	if (tb_msg[NL80211_ATTR_IFINDEX])
		printf("%s\tifindex %d\n", indent, nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]));
	if (tb_msg[NL80211_ATTR_WDEV])
		printf("%s\twdev 0x%llx\n", indent,
		       (unsigned long long)nla_get_u64(tb_msg[NL80211_ATTR_WDEV]));
	if (tb_msg[NL80211_ATTR_MAC]) {
		char mac_addr[20];
		mac_addr_n2a(mac_addr, nla_data(tb_msg[NL80211_ATTR_MAC]));
		printf("%s\taddr %s\n", indent, mac_addr);
	}
	if (tb_msg[NL80211_ATTR_SSID]) {
		printf("%s\tssid ", indent);
		print_ssid_escaped(nla_len(tb_msg[NL80211_ATTR_SSID]),
				   nla_data(tb_msg[NL80211_ATTR_SSID]));
		printf("\n");
	}
	if (tb_msg[NL80211_ATTR_IFTYPE])
		printf("%s\ttype %s\n", indent, iftype_name(nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE])));
	if (!wiphy && tb_msg[NL80211_ATTR_WIPHY])
		printf("%s\twiphy %d\n", indent, nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]));
	if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
		printf("%s\t", indent);
		print_channel(tb_msg);
		printf("\n");
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]) {
		int32_t txp = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);

		printf("%s\ttxpower %d.%.2d dBm\n",
		       indent, txp / 100, txp % 100);
	}

	if (tb_msg[NL80211_ATTR_TXQ_STATS]) {
		char buf[150];
		parse_txq_stats(buf, sizeof(buf), tb_msg[NL80211_ATTR_TXQ_STATS], 1, -1, indent);
		printf("%s\tmulticast TXQ:%s\n", indent, buf);
	}

	if (tb_msg[NL80211_ATTR_4ADDR]) {
		uint8_t use_4addr = nla_get_u8(tb_msg[NL80211_ATTR_4ADDR]);
		if (use_4addr)
			printf("%s\t4addr: on\n", indent);
	}

	if (tb_msg[NL80211_ATTR_MLO_LINKS]) {
		struct nlattr *link;
		int n;

		printf("%s\tMLD with links:\n", indent);

		nla_for_each_nested(link, tb_msg[NL80211_ATTR_MLO_LINKS], n) {
			struct nlattr *tb[NL80211_ATTR_MAX + 1];

			nla_parse_nested(tb, NL80211_ATTR_MAX, link, NULL);
			printf("%s\t - link", indent);
			if (tb[NL80211_ATTR_MLO_LINK_ID])
				printf(" ID %2d", nla_get_u32(tb[NL80211_ATTR_MLO_LINK_ID]));
			if (tb[NL80211_ATTR_MAC]) {
				char buf[20];

				mac_addr_n2a(buf, nla_data(tb[NL80211_ATTR_MAC]));
				printf(" link addr %s", buf);
			}
			if (tb[NL80211_ATTR_WIPHY_FREQ]) {
				printf("\n%s\t   ", indent);
				print_channel(tb);
			}
			if (tb[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]) {
				int32_t txp = nla_get_u32(tb[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);

				printf("\n%s\t   txpower %d.%.2d dBm", indent, txp / 100, txp % 100);
			}
			printf("\n");
		}
	}

	return NL_SKIP;
}

static int handle_interface_info(struct nl80211_state *state,
				 struct nl_msg *msg,
				 int argc, char **argv,
				 enum id_input id)
{
	register_handler(print_iface_handler, NULL);
	return 0;
}
TOPLEVEL(info, NULL, NL80211_CMD_GET_INTERFACE, 0, CIB_NETDEV, handle_interface_info,
	 "Show information for this interface.");

static int handle_interface_set(struct nl80211_state *state,
				struct nl_msg *msg,
				int argc, char **argv,
				enum id_input id)
{
	if (!argc)
		return 1;

	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);

	switch (parse_mntr_flags(&argc, &argv, msg)) {
	case 0:
		return 0;
	case 1:
		return 1;
	case -ENOMEM:
		fprintf(stderr, "failed to allocate flags\n");
		return 2;
	case -EINVAL:
		fprintf(stderr, "unknown flag %s\n", *argv);
		return 2;
	default:
		return 2;
	}
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, monitor, "<flag>*",
	NL80211_CMD_SET_INTERFACE, 0, CIB_NETDEV, handle_interface_set,
	"Set monitor flags. Valid flags are:\n"
	VALID_FLAGS);

static int handle_interface_meshid(struct nl80211_state *state,
				   struct nl_msg *msg,
				   int argc, char **argv,
				   enum id_input id)
{
	char *mesh_id = NULL;

	if (argc != 1)
		return 1;

	mesh_id = argv[0];

	NLA_PUT(msg, NL80211_ATTR_MESH_ID, strlen(mesh_id), mesh_id);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, meshid, "<meshid>",
	NL80211_CMD_SET_INTERFACE, 0, CIB_NETDEV, handle_interface_meshid, NULL);

static unsigned int dev_dump_wiphy;

static int handle_dev_dump(struct nl80211_state *state,
			   struct nl_msg *msg,
			   int argc, char **argv,
			   enum id_input id)
{
	dev_dump_wiphy = -1;
	register_handler(print_iface_handler, &dev_dump_wiphy);
	return 0;
}
TOPLEVEL(dev, NULL, NL80211_CMD_GET_INTERFACE, NLM_F_DUMP, CIB_NONE, handle_dev_dump,
	 "List all network interfaces for wireless hardware.");

static int handle_interface_type(struct nl80211_state *state,
				 struct nl_msg *msg,
				 int argc, char **argv,
				 enum id_input id)
{
	enum nl80211_iftype type;
	int tpset;

	tpset = get_if_type(&argc, &argv, &type, false);
	if (tpset)
		return tpset;

	if (argc)
		return 1;

	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, type);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, type, "<type>",
	NL80211_CMD_SET_INTERFACE, 0, CIB_NETDEV, handle_interface_type,
	"Set interface type/mode.\n"
	IFACE_TYPES);

static int handle_interface_4addr(struct nl80211_state *state,
				  struct nl_msg *msg,
				  int argc, char **argv,
				  enum id_input id)
{
	if (argc != 1)
		return 1;
	return parse_4addr_flag(argv[0], msg);
}
COMMAND(set, 4addr, "<on|off>",
	NL80211_CMD_SET_INTERFACE, 0, CIB_NETDEV, handle_interface_4addr,
	"Set interface 4addr (WDS) mode.");

static int handle_interface_noack_map(struct nl80211_state *state,
				      struct nl_msg *msg,
				      int argc, char **argv,
				      enum id_input id)
{
	uint16_t noack_map;
	char *end;

	if (argc != 1)
		return 1;

	noack_map = strtoul(argv[0], &end, 16);
	if (*end)
		return 1;

	NLA_PUT_U16(msg, NL80211_ATTR_NOACK_MAP, noack_map);

	return 0;
 nla_put_failure:
	return -ENOBUFS;

}
COMMAND(set, noack_map, "<map>",
	NL80211_CMD_SET_NOACK_MAP, 0, CIB_NETDEV, handle_interface_noack_map,
	"Set the NoAck map for the TIDs. (0x0009 = BE, 0x0006 = BK, 0x0030 = VI, 0x00C0 = VO)");


static int handle_interface_wds_peer(struct nl80211_state *state,
				     struct nl_msg *msg,
				     int argc, char **argv,
				     enum id_input id)
{
	unsigned char mac_addr[ETH_ALEN];

	if (argc < 1)
		return 1;

	if (mac_addr_a2n(mac_addr, argv[0])) {
		fprintf(stderr, "Invalid MAC address\n");
		return 2;
	}

	argc--;
	argv++;

	if (argc)
		return 1;

	NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, mac_addr);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, peer, "<MAC address>",
	NL80211_CMD_SET_WDS_PEER, 0, CIB_NETDEV, handle_interface_wds_peer,
	"Set interface WDS peer.");

static int set_mcast_rate(struct nl80211_state *state,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	float rate;
	char *end;

	if (argc != 1)
		return 1;

	rate = strtod(argv[0], &end);
	if (*end != '\0')
		return 1;

	NLA_PUT_U32(msg, NL80211_ATTR_MCAST_RATE, (int)(rate * 10));

	return 0;
nla_put_failure:
	return -ENOBUFS;
}

COMMAND(set, mcast_rate, "<rate in Mbps>",
	NL80211_CMD_SET_MCAST_RATE, 0, CIB_NETDEV, set_mcast_rate,
	"Set the multicast bitrate.");


static int handle_chanfreq(struct nl80211_state *state, struct nl_msg *msg,
			   bool chan, int argc, char **argv,
			   enum id_input id)
{
	struct chandef chandef;
	int res;
	int parsed;
	char *end;

	res = parse_freqchan(&chandef, chan, argc, argv, &parsed, false);
	if (res)
		return res;

	argc -= parsed;
	argv += parsed;

	while (argc) {
		unsigned int beacons = 10;

		if (strcmp(argv[0], "beacons") == 0) {
			if (argc < 2)
				return 1;

			beacons = strtol(argv[1], &end, 10);
			if (*end)
				return 1;

			argc -= 2;
			argv += 2;

			NLA_PUT_U32(msg, NL80211_ATTR_CH_SWITCH_COUNT, beacons);
		} else if (strcmp(argv[0], "block-tx") == 0) {
			argc -= 1;
			argv += 1;

			NLA_PUT_FLAG(msg, NL80211_ATTR_CH_SWITCH_BLOCK_TX);
		} else {
			return 1;
		}
	}

	return put_chandef(msg, &chandef);

 nla_put_failure:
	return -ENOBUFS;
}

static int handle_freq(struct nl80211_state *state, struct nl_msg *msg,
		       int argc, char **argv,
		       enum id_input id)
{
	return handle_chanfreq(state, msg, false, argc, argv, id);
}

static int handle_chan(struct nl80211_state *state, struct nl_msg *msg,
		       int argc, char **argv,
		       enum id_input id)
{
	return handle_chanfreq(state, msg, true, argc, argv, id);
}

SECTION(switch);
COMMAND(switch, freq,
	"<freq> [NOHT|HT20|HT40+|HT40-|5MHz|10MHz|80MHz] [beacons <count>] [block-tx]\n"
	"<control freq> [5|10|20|40|80|80+80|160] [<center1_freq> [<center2_freq>]] [beacons <count>] [block-tx]",
	NL80211_CMD_CHANNEL_SWITCH, 0, CIB_NETDEV, handle_freq,
	"Switch the operating channel by sending a channel switch announcement (CSA).");
COMMAND(switch, channel, "<channel> [NOHT|HT20|HT40+|HT40-|5MHz|10MHz|80MHz] [beacons <count>] [block-tx]",
	NL80211_CMD_CHANNEL_SWITCH, 0, CIB_NETDEV, handle_chan, NULL);


static int toggle_tid_param(const char *argv0, const char *argv1,
			    struct nl_msg *msg, uint32_t attr)
{
	uint8_t val;

	if (strcmp(argv1, "on") == 0) {
		val = NL80211_TID_CONFIG_ENABLE;
	} else if (strcmp(argv1, "off") == 0) {
		val = NL80211_TID_CONFIG_DISABLE;
	} else {
		fprintf(stderr, "Invalid %s parameter: %s\n", argv0, argv1);
		return 2;
	}

	NLA_PUT_U8(msg, attr, val);
	return 0;

 nla_put_failure:
	return -ENOBUFS;
}

static int handle_tid_config(struct nl80211_state *state,
			     struct nl_msg *msg,
			     int argc, char **argv,
			     enum id_input id)
{
	struct nlattr *tids_array = NULL;
	struct nlattr *tids_entry = NULL;
	enum nl80211_tx_rate_setting txrate_type;
	unsigned char peer[ETH_ALEN];
	int tids_num = 0;
	char *end;
	int ret;
	enum {
		PS_ADDR,
		PS_TIDS,
		PS_CONF,
	} parse_state = PS_ADDR;
	unsigned int attr;

	while (argc) {
		switch (parse_state) {
		case PS_ADDR:
			if (strcmp(argv[0], "peer") == 0) {
				if (argc < 2) {
					fprintf(stderr, "Not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				if (mac_addr_a2n(peer, argv[1])) {
					fprintf(stderr, "Invalid MAC address\n");
					return 2;
				}

				NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, peer);

				argc -= 2;
				argv += 2;
				parse_state = PS_TIDS;

			} else if (strcmp(argv[0], "tids") == 0) {
				parse_state = PS_TIDS;
			} else {
				fprintf(stderr, "Peer MAC address expected\n");
				return HANDLER_RET_USAGE;
			}

			break;
		case PS_TIDS:
			if (strcmp(argv[0], "tids") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				if (!tids_array) {
					tids_array = nla_nest_start(msg, NL80211_ATTR_TID_CONFIG);
					if (!tids_array)
						return -ENOBUFS;
				}

				if (tids_entry) {
					nla_nest_end(msg, tids_entry);
					tids_num++;
				}

				tids_entry = nla_nest_start(msg, tids_num);
				if (!tids_entry)
					return -ENOBUFS;

				NLA_PUT_U16(msg, NL80211_TID_CONFIG_ATTR_TIDS, strtol(argv[1], &end, 0));
				if (*end) {
					fprintf(stderr, "Invalid TID mask value: %s\n", argv[1]);
					return 2;
				}

				argc -= 2;
				argv += 2;
				parse_state = PS_CONF;
			} else {
				fprintf(stderr, "TID mask expected\n");
				return HANDLER_RET_USAGE;
			}

			break;
		case PS_CONF:
			if (strcmp(argv[0], "tids") == 0) {
				parse_state = PS_TIDS;
			} else if (strcmp(argv[0], "override") == 0) {
				NLA_PUT_FLAG(msg, NL80211_TID_CONFIG_ATTR_OVERRIDE);

				argc -= 1;
				argv += 1;
			} else if (strcmp(argv[0], "ampdu") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				ret = toggle_tid_param(argv[0], argv[1], msg,
						       NL80211_TID_CONFIG_ATTR_AMPDU_CTRL);
				if (ret)
					return ret;

				argc -= 2;
				argv += 2;
			} else if (strcmp(argv[0], "amsdu") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				ret = toggle_tid_param(argv[0], argv[1], msg,
						       NL80211_TID_CONFIG_ATTR_AMSDU_CTRL);
				if (ret)
					return ret;

				argc -= 2;
				argv += 2;
			} else if (strcmp(argv[0], "noack") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				ret = toggle_tid_param(argv[0], argv[1], msg,
						       NL80211_TID_CONFIG_ATTR_NOACK);
				if (ret)
					return ret;

				argc -= 2;
				argv += 2;
			} else if (strcmp(argv[0], "rtscts") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				ret = toggle_tid_param(argv[0], argv[1], msg,
						       NL80211_TID_CONFIG_ATTR_RTSCTS_CTRL);
				if (ret)
					return ret;

				argc -= 2;
				argv += 2;
			} else if (strcmp(argv[0], "sretry") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				NLA_PUT_U8(msg, NL80211_TID_CONFIG_ATTR_RETRY_SHORT, strtol(argv[1], &end, 0));
				if (*end) {
					fprintf(stderr, "Invalid short_retry value: %s\n", argv[1]);
					return 2;
				}

				argc -= 2;
				argv += 2;
			} else if (strcmp(argv[0], "lretry") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}

				NLA_PUT_U8(msg, NL80211_TID_CONFIG_ATTR_RETRY_LONG, strtol(argv[1], &end, 0));
				if (*end) {
					fprintf(stderr, "Invalid long_retry value: %s\n", argv[1]);
					return 2;
				}

				argc -= 2;
				argv += 2;
			} else if (strcmp(argv[0], "bitrates") == 0) {
				if (argc < 2) {
					fprintf(stderr, "not enough args for %s\n", argv[0]);
					return HANDLER_RET_USAGE;
				}
				if (!strcmp(argv[1], "auto"))
					txrate_type = NL80211_TX_RATE_AUTOMATIC;
				else if (!strcmp(argv[1], "fixed"))
					txrate_type = NL80211_TX_RATE_FIXED;
				else if (!strcmp(argv[1], "limit"))
					txrate_type = NL80211_TX_RATE_LIMITED;
				else {
					printf("Invalid parameter: %s\n", argv[0]);
					return 2;
				}
				NLA_PUT_U8(msg, NL80211_TID_CONFIG_ATTR_TX_RATE_TYPE, txrate_type);
				argc -= 2;
				argv += 2;
				if (txrate_type != NL80211_TX_RATE_AUTOMATIC) {
					attr = NL80211_TID_CONFIG_ATTR_TX_RATE;
					ret = set_bitrates(msg, argc, argv,
							   attr);
					if (ret < 2)
						return 1;

					argc -= ret;
					argv += ret;
				}
			} else {
				fprintf(stderr, "Unknown parameter: %s\n", argv[0]);
				return HANDLER_RET_USAGE;
			}

			break;
		default:
			fprintf(stderr, "Failed to parse: internal failure\n");
			return HANDLER_RET_USAGE;
		}
	}

	if (tids_entry)
		nla_nest_end(msg, tids_entry);

	if (tids_array)
		nla_nest_end(msg, tids_array);

	return 0;

nla_put_failure:
	return -ENOBUFS;
}

COMMAND(set, tidconf, "[peer <MAC address>] tids <mask> [override] [sretry <num>] [lretry <num>] "
	"[ampdu [on|off]] [amsdu [on|off]] [noack [on|off]] [rtscts [on|off]]"
	"[bitrates <type [auto|fixed|limit]> [legacy-<2.4|5> <legacy rate in Mbps>*] [ht-mcs-<2.4|5> <MCS index>*]"
	" [vht-mcs-<2.4|5> <NSS:MCSx,MCSy... | NSS:MCSx-MCSy>*] [sgi-2.4|lgi-2.4] [sgi-5|lgi-5]]",
	NL80211_CMD_SET_TID_CONFIG, 0, CIB_NETDEV, handle_tid_config,
	"Setup per-node TID specific configuration for TIDs selected by bitmask.\n"
	"If MAC address is not specified, then supplied TID configuration\n"
	"applied to all the peers.\n"
	"Examples:\n"
	"  $ iw dev wlan0 set tidconf tids 0x1 ampdu off\n"
	"  $ iw dev wlan0 set tidconf tids 0x5 ampdu off amsdu off rtscts on\n"
	"  $ iw dev wlan0 set tidconf tids 0x3 override ampdu on noack on rtscts on\n"
	"  $ iw dev wlan0 set tidconf peer xx:xx:xx:xx:xx:xx tids 0x1 ampdu off tids 0x3 amsdu off rtscts on\n"
	"  $ iw dev wlan0 set tidconf peer xx:xx:xx:xx:xx:xx tids 0x2 bitrates auto\n"
	"  $ iw dev wlan0 set tidconf peer xx:xx:xx:xx:xx:xx tids 0x2 bitrates limit vht-mcs-5 4:9\n"
	);

static int handle_set_epcs(struct nl80211_state *state,
			   struct nl_msg *msg,
			   int argc, char **argv,
			   enum id_input id)
{
	if (argc != 1)
		return 1;

	if (strcmp(argv[0], "enable") == 0)
		NLA_PUT_FLAG(msg, NL80211_ATTR_EPCS);
	else if (strcmp(argv[0], "disable") != 0)
		return 1;

	return 0;

nla_put_failure:
	return 1;
}

COMMAND(set, epcs, "<enable|disable>",
	NL80211_CMD_EPCS_CFG, 0, CIB_NETDEV, handle_set_epcs,
	"Enable/Disable EPCS support");
