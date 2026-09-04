#include <string.h>
#include <errno.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "iw.h"

SECTION(mgmt);

static int seq_handler(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

static int dump_mgmt_frame(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
		uint32_t freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
		printf("freq %u MHz\n", freq);
	}

	if (tb_msg[NL80211_ATTR_RX_SIGNAL_DBM]) {
		/* nl80211_send_mgmt sends signed dBm value as u32 */
		int dbm = nla_get_u32(tb_msg[NL80211_ATTR_RX_SIGNAL_DBM]);
		printf("rssi %d dBm\n", dbm);
	}

	if (tb_msg[NL80211_ATTR_FRAME]) {
		int len = nla_len(tb_msg[NL80211_ATTR_FRAME]);
		uint8_t *data = nla_data(tb_msg[NL80211_ATTR_FRAME]);
		iw_hexdump("mgmt", data, len);
	}

	return 0;
}

static int register_mgmt_frame(struct nl80211_state *state,
			       struct nl_msg *msg, int argc, char **argv,
			       enum id_input id)
{
	unsigned int type;
	unsigned char *match;
	size_t match_len;
	int ret;

	if (argc < 2)
		return HANDLER_RET_USAGE;

	ret = sscanf(argv[0], "%x", &type);
	if (ret != 1) {
		printf("invalid frame type: %s\n", argv[0]);
		return 2;
	}

	match = parse_hex(argv[1], &match_len);
	if (!match) {
		printf("invalid frame pattern: %s\n", argv[1]);
		return 2;
	}

	NLA_PUT_U16(msg, NL80211_ATTR_FRAME_TYPE, type);
	NLA_PUT(msg, NL80211_ATTR_FRAME_MATCH, match_len, match);

	free(match);
	return 0;

nla_put_failure:
	free(match);
	return -ENOBUFS;
}

static int handle_mgmt_reg(struct nl80211_state *state,
				    struct nl_msg *msg, int argc,
				    char **argv, enum id_input id)
{
	return register_mgmt_frame(state, msg, argc, argv, id);
}

HIDDEN(mgmt, reg, "", NL80211_CMD_REGISTER_FRAME, 0, CIB_NETDEV, handle_mgmt_reg);

static int handle_mgmt_dump(struct nl80211_state *state,
			       struct nl_msg *msg, int argc,
			       char **argv, enum id_input id)
{
	struct nl_cb *mgmt_cb;
	char *ndev = argv[0];
	int mgmt_argc = 5;
	char **mgmt_argv;
	unsigned int count = 0;
	int err = 0;

	mgmt_argv = calloc(mgmt_argc, sizeof(char*));
	if (!mgmt_argv)
		return -ENOMEM;

	mgmt_argv[0] = ndev;
	mgmt_argv[1] = "mgmt";
	mgmt_argv[2] = "reg";

	if (argc < 6) {
		err = HANDLER_RET_USAGE;
		goto out;
	}

	argc -= 3;
	argv += 3;
	while (argc >= 3) {
		if (strcmp(argv[0], "frame") != 0) {
			err = HANDLER_RET_USAGE;
			goto out;
		}

		mgmt_argv[3] = argv[1];
		mgmt_argv[4] = argv[2];

		argc -= 3;
		argv += 3;

		err = handle_cmd(state, II_NETDEV, mgmt_argc, mgmt_argv);
		if (err)
			goto out;
	}

	if (argc == 2 && strcmp(argv[0], "count") == 0) {
		count = 1 + atoi(argv[1]);
		if (count < 1)
			count = 1;

		argc -= 2;
		argv += 2;
	} else if (argc) {
		err = HANDLER_RET_USAGE;
		goto out;
	}

	mgmt_cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	if (!mgmt_cb) {
		err = 1;
		goto out;
	}

	/* need to turn off sequence number checking */
	nl_cb_set(mgmt_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, seq_handler, NULL);
	nl_cb_set(mgmt_cb, NL_CB_VALID, NL_CB_CUSTOM, dump_mgmt_frame, NULL);

	while (--count)
		nl_recvmsgs(state->nl_sock, mgmt_cb);

	nl_cb_put(mgmt_cb);
out:
	free(mgmt_argv);
	return err;
}

COMMAND(mgmt, dump, "frame <type as hex ab> <pattern as hex ab:cd:..> [frame <type> <pattern>]* [count <frames>]",
	0, 0, CIB_NETDEV, handle_mgmt_dump,
	"Register for receiving certain mgmt frames and print them.\n"
	"Frames are selected by their type and pattern containing\n"
	"the first several bytes of the frame that should match.\n\n"
	"Example: iw dev wlan0 mgmt dump frame 40 00 frame 40 01:02 count 10\n");
