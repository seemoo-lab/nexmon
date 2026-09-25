#include <errno.h>

#include "nl80211.h"
#include "iw.h"
#include <unistd.h>

SECTION(measurement);

static int put_preamble(struct nl_msg *msg, char *s)
{
	static const struct {
		const char *name;
		unsigned int val;
	} preamble_map[] = {
		{ .name = "legacy", .val = NL80211_PREAMBLE_LEGACY, },
		{ .name = "ht", .val = NL80211_PREAMBLE_HT, },
		{ .name = "vht", .val = NL80211_PREAMBLE_VHT, },
		{ .name = "dmg", .val = NL80211_PREAMBLE_DMG, },
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(preamble_map); i++) {
		if (strcasecmp(preamble_map[i].name, s) == 0) {
			NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE,
				    preamble_map[i].val);
			return 0;
		}
	}

nla_put_failure:
	return -1;
}

static int parse_ftm_target(struct nl_msg *msg, char *str, int peer_index)
{
	unsigned char addr[ETH_ALEN];
	int res, consumed;
	char *bw = NULL, *pos, *tmp, *save_ptr, *delims = " \t\n";
	struct nlattr *peer, *req, *reqdata, *ftm, *chan;
	bool report_ap_tsf = false, preamble = false;
	unsigned int freq = 0, cf1 = 0, cf2 = 0;

	res = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
		     &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5],
		     &consumed);

	if (res != ETH_ALEN) {
		printf("Invalid MAC address\n");
		return HANDLER_RET_USAGE;
	}

	peer = nla_nest_start(msg, peer_index);

	NLA_PUT(msg, NL80211_PMSR_PEER_ATTR_ADDR, ETH_ALEN, addr);

	req = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_REQ);
	if (!req)
		goto nla_put_failure;
	reqdata = nla_nest_start(msg, NL80211_PMSR_REQ_ATTR_DATA);
	if (!reqdata)
		goto nla_put_failure;
	ftm = nla_nest_start(msg, NL80211_PMSR_TYPE_FTM);
	if (!ftm)
		goto nla_put_failure;

	str += consumed;
	pos = strtok_r(str, delims, &save_ptr);

	while (pos) {
		if (strncmp(pos, "cf=", 3) == 0) {
			freq = strtol(pos + 3, &tmp, 0);
			if (*tmp) {
				printf("Invalid cf value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "bw=", 3) == 0) {
			bw = pos + 3;
		} else if (strncmp(pos, "cf1=", 4) == 0) {
			cf1 = strtol(pos + 4, &tmp, 0);
			if (*tmp) {
				printf("Invalid cf1 value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "cf2=", 4) == 0) {
			cf2 = strtol(pos + 4, &tmp, 0);
			if (*tmp) {
				printf("Invalid cf2 value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "bursts_exp=", 11) == 0) {
			NLA_PUT_U8(msg,
				   NL80211_PMSR_FTM_REQ_ATTR_NUM_BURSTS_EXP,
				   strtol(pos + 11, &tmp, 0));
			if (*tmp) {
				printf("Invalid bursts_exp value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "burst_period=", 13) == 0) {
			NLA_PUT_U16(msg, NL80211_PMSR_FTM_REQ_ATTR_BURST_PERIOD,
				    strtol(pos + 13, &tmp, 0));
			if (*tmp) {
				printf("Invalid burst_period value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "retries=", 8) == 0) {
			NLA_PUT_U8(msg,
				   NL80211_PMSR_FTM_REQ_ATTR_NUM_FTMR_RETRIES,
				   strtol(pos + 8, &tmp, 0));
			if (*tmp) {
				printf("Invalid retries value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "burst_duration=", 15) == 0) {
			NLA_PUT_U8(msg,
				   NL80211_PMSR_FTM_REQ_ATTR_BURST_DURATION,
				   strtol(pos + 15, &tmp, 0));
			if (*tmp) {
				printf("Invalid burst_duration value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strncmp(pos, "ftms_per_burst=", 15) == 0) {
			NLA_PUT_U8(msg,
				   NL80211_PMSR_FTM_REQ_ATTR_FTMS_PER_BURST,
				   strtol(pos + 15, &tmp, 0));
			if (*tmp) {
				printf("Invalid ftms_per_burst value!\n");
				return HANDLER_RET_USAGE;
			}
		} else if (strcmp(pos, "asap") == 0) {
			NLA_PUT_FLAG(msg, NL80211_PMSR_FTM_REQ_ATTR_ASAP);
		} else if (strcmp(pos, "ap-tsf") == 0) {
			report_ap_tsf = true;
		} else if (strcmp(pos, "civic") == 0) {
			NLA_PUT_FLAG(msg, NL80211_PMSR_FTM_REQ_ATTR_REQUEST_CIVICLOC);
		} else if (strcmp(pos, "lci") == 0) {
			NLA_PUT_FLAG(msg, NL80211_PMSR_FTM_REQ_ATTR_REQUEST_LCI);
		} else if (strncmp(pos, "preamble=", 9) == 0) {
			if (put_preamble(msg, pos + 9)) {
				printf("Invalid preamble %s\n", pos + 9);
				return HANDLER_RET_USAGE;
			}
			preamble = true;
		} else if (strncmp(pos, "tb", 2) == 0) {
			NLA_PUT_FLAG(msg,
				     NL80211_PMSR_FTM_REQ_ATTR_TRIGGER_BASED);
			NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE,
				    NL80211_PREAMBLE_HE);
			preamble = true;
		} else if (strncmp(pos, "non_tb", 6) == 0) {
			NLA_PUT_FLAG(msg,
				     NL80211_PMSR_FTM_REQ_ATTR_NON_TRIGGER_BASED);
			NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE,
				    NL80211_PREAMBLE_HE);
			preamble = true;
		} else if (strncmp(pos, "lmr_feedback", 12) == 0) {
			NLA_PUT_FLAG(msg,
				     NL80211_PMSR_FTM_REQ_ATTR_LMR_FEEDBACK);
		} else if (strncmp(pos, "bss_color=", 10) == 0) {
			NLA_PUT_U8(msg,
				   NL80211_PMSR_FTM_REQ_ATTR_BSS_COLOR,
				   strtol(pos + 10, &tmp, 0));
			if (*tmp) {
				printf("Invalid bss_color value!\n");
				return HANDLER_RET_USAGE;
			}
		} else {
			printf("Unknown parameter %s\n", pos);
			return HANDLER_RET_USAGE;
		}

		pos = strtok_r(NULL, delims, &save_ptr);
	}

	if (!preamble) {
		int preamble = -1;

		switch (str_to_bw(bw)) {
		case NL80211_CHAN_WIDTH_20_NOHT:
		case NL80211_CHAN_WIDTH_5:
		case NL80211_CHAN_WIDTH_10:
			preamble = NL80211_PREAMBLE_LEGACY;
			break;
		case NL80211_CHAN_WIDTH_20:
		case NL80211_CHAN_WIDTH_40:
			preamble = NL80211_PREAMBLE_HT;
			break;
		case NL80211_CHAN_WIDTH_80:
		case NL80211_CHAN_WIDTH_80P80:
		case NL80211_CHAN_WIDTH_160:
			preamble = NL80211_PREAMBLE_VHT;
			break;
		default:
			return HANDLER_RET_USAGE;
		}

		NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE, preamble);
	}

	nla_nest_end(msg, ftm);
	if (report_ap_tsf)
		NLA_PUT_FLAG(msg, NL80211_PMSR_REQ_ATTR_GET_AP_TSF);
	nla_nest_end(msg, reqdata);
	nla_nest_end(msg, req);

	/* set the channel */
	chan = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_CHAN);
	if (!chan)
		goto nla_put_failure;
	if (freq)
		NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
	if (cf1)
		NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ1, cf1);
	if (cf2)
		NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ2, cf2);
	if (bw)
		NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH,
			    str_to_bw(bw));
	nla_nest_end(msg, chan);

	nla_nest_end(msg, peer);
	return 0;
nla_put_failure:
	return -ENOBUFS;
}

static int parse_ftm_config(struct nl_msg *msg, const char *file)
{
	FILE *input;
	char line[256];
	int line_num;

	input = fopen(file, "r");
	if (!input) {
		int err = errno;

		printf("Failed to open file: %s\n", strerror(err));
		return -err;
	}

	for (line_num = 1; fgets(line, sizeof(line), input); line_num++) {
		if (line[0] == '#')
			continue;

		if (parse_ftm_target(msg, line, line_num)) {
			printf("Invalid FTM configuration at line %d!\n",
			       line_num);
			return HANDLER_RET_USAGE;
		}
	}

	return 0;
}

static int handle_ftm_req(struct nl80211_state *state, struct nl_msg *msg,
			  int argc, char **argv, enum id_input id)
{
	int err, i;
	static char **req_argv;
	static const __u32 wait[] = {
		NL80211_CMD_PEER_MEASUREMENT_COMPLETE,
	};
	static const __u32 print[] = {
		NL80211_CMD_PEER_MEASUREMENT_RESULT,
		NL80211_CMD_PEER_MEASUREMENT_COMPLETE,
	};
	struct print_event_args printargs = { };

	req_argv = calloc(argc + 1, sizeof(req_argv[0]));
	req_argv[0] = argv[0];
	req_argv[1] = "measurement";
	req_argv[2] = "ftm_request_send";
	for (i = 3; i < argc; i++)
		req_argv[i] = argv[i];

	err = handle_cmd(state, id, argc, req_argv);

	free(req_argv);

	if (err)
		return err;

	__do_listen_events(state,
			   ARRAY_SIZE(wait), wait,
			   ARRAY_SIZE(print), print,
			   &printargs);
	return 0;
}

static int handle_ftm_req_send(struct nl80211_state *state, struct nl_msg *msg,
			       int argc, char **argv, enum id_input id)
{
	struct nlattr *pmsr, *peers;
	const char *file;
	int err;

	if (argc < 1)
		return HANDLER_RET_USAGE;

	file = argv[0];
	argc--;
	argv++;
	while (argc) {
		if (strncmp(argv[0], "randomise", 9) == 0 ||
		    strncmp(argv[0], "randomize", 9) == 0) {
			err = parse_random_mac_addr(msg, argv[0] + 9);
			if (err)
				return err;
		} else if (strncmp(argv[0], "timeout=", 8) == 0) {
			char *end;

			NLA_PUT_U32(msg, NL80211_ATTR_TIMEOUT,
				    strtoul(argv[0] + 8, &end, 0));
			if (*end)
				return HANDLER_RET_USAGE;
		} else {
			return HANDLER_RET_USAGE;
		}

		argc--;
		argv++;
	}

	pmsr = nla_nest_start(msg, NL80211_ATTR_PEER_MEASUREMENTS);
	if (!pmsr)
		goto nla_put_failure;
	peers = nla_nest_start(msg, NL80211_PMSR_ATTR_PEERS);
	if (!peers)
		goto nla_put_failure;

	err = parse_ftm_config(msg, file);
	if (err)
		return err;

	nla_nest_end(msg, peers);
	nla_nest_end(msg, pmsr);

	return 0;

nla_put_failure:
	return -ENOBUFS;
}
COMMAND(measurement, ftm_request, "<config-file> [timeout=<seconds>] [randomise[=<addr>/<mask>]]", 0, 0,
	CIB_NETDEV, handle_ftm_req,
	"Send an FTM request to the targets supplied in the config file.\n"
	"Each line in the file represents a target, with the following format:\n"
	"<addr> bw=<[20|40|80|80+80|160]> cf=<center_freq> [cf1=<center_freq1>] [cf2=<center_freq2>] [ftms_per_burst=<samples per burst>] [ap-tsf] [asap] [bursts_exp=<num of bursts exponent>] [burst_period=<burst period>] [retries=<num of retries>] [burst_duration=<burst duration>] [preamble=<legacy,ht,vht,dmg>] [lci] [civic] [tb] [non_tb]");
HIDDEN(measurement, ftm_request_send, "", NL80211_CMD_PEER_MEASUREMENT_START,
       0, CIB_NETDEV, handle_ftm_req_send);
