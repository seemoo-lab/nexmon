#include <stdint.h>
#include <stdbool.h>
#include <net/if.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include "iw.h"

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

struct ieee80211_beacon_channel {
	__u16 center_freq;
	bool no_ir;
	bool no_ibss;
};

static int parse_beacon_hint_chan(struct nlattr *tb,
				  struct ieee80211_beacon_channel *chan)
{
	struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
	static struct nla_policy beacon_freq_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
		[NL80211_FREQUENCY_ATTR_FREQ] = { .type = NLA_U32 },
		[NL80211_FREQUENCY_ATTR_NO_IR] = { .type = NLA_FLAG },
		[__NL80211_FREQUENCY_ATTR_NO_IBSS] = { .type = NLA_FLAG },
	};

	if (nla_parse_nested(tb_freq,
			     NL80211_FREQUENCY_ATTR_MAX,
			     tb,
			     beacon_freq_policy))
		return -EINVAL;

	chan->center_freq = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);

	if (tb_freq[NL80211_FREQUENCY_ATTR_NO_IR])
		chan->no_ir = true;
	if (tb_freq[__NL80211_FREQUENCY_ATTR_NO_IBSS])
		chan->no_ibss = true;

	return 0;
}

static void print_frame(struct print_event_args *args, struct nlattr *attr)
{
	uint8_t *frame;
	size_t len;
	unsigned int i;
	char macbuf[6*3];
	uint16_t tmp;

	if (!attr) {
		printf(" [no frame]");
		return;
	}

	frame = nla_data(attr);
	len = nla_len(attr);

	if (len < 26) {
		printf(" [invalid frame: ");
		goto print_frame;
	}

	mac_addr_n2a(macbuf, frame + 10);
	printf(" %s -> ", macbuf);
	mac_addr_n2a(macbuf, frame + 4);
	printf("%s", macbuf);

	switch (frame[0] & 0xfc) {
	case 0x10: /* assoc resp */
	case 0x30: /* reassoc resp */
		/* status */
		tmp = (frame[27] << 8) + frame[26];
		printf(" status: %d: %s", tmp, get_status_str(tmp));
		break;
	case 0x00: /* assoc req */
	case 0x20: /* reassoc req */
		break;
	case 0xb0: /* auth */
		/* status */
		tmp = (frame[29] << 8) + frame[28];
		printf(" status: %d: %s", tmp, get_status_str(tmp));
		break;
	case 0xa0: /* disassoc */
	case 0xc0: /* deauth */
		/* reason */
		tmp = (frame[25] << 8) + frame[24];
		printf(" reason %d: %s", tmp, get_reason_str(tmp));
		break;
	}

	if (!args->frame)
		return;

	printf(" [frame:");

 print_frame:
	for (i = 0; i < len; i++)
		printf(" %.02x", frame[i]);
	printf("]");
}

static void parse_cqm_event(struct nlattr **attrs)
{
	static struct nla_policy cqm_policy[NL80211_ATTR_CQM_MAX + 1] = {
		[NL80211_ATTR_CQM_RSSI_THOLD] = { .type = NLA_U32 },
		[NL80211_ATTR_CQM_RSSI_HYST] = { .type = NLA_U32 },
		[NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT] = { .type = NLA_U32 },
	};
	struct nlattr *cqm[NL80211_ATTR_CQM_MAX + 1];
	struct nlattr *cqm_attr = attrs[NL80211_ATTR_CQM];

	printf("CQM event: ");

	if (!cqm_attr ||
	    nla_parse_nested(cqm, NL80211_ATTR_CQM_MAX, cqm_attr, cqm_policy)) {
		printf("missing data!\n");
		return;
	}

	if (cqm[NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT]) {
		enum nl80211_cqm_rssi_threshold_event rssi_event;
		int32_t rssi_level = -1;
		bool found_one = false;

		rssi_event = nla_get_u32(cqm[NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT]);
		if (cqm[NL80211_ATTR_CQM_RSSI_LEVEL])
			rssi_level = nla_get_u32(cqm[NL80211_ATTR_CQM_RSSI_LEVEL]);

		switch (rssi_event) {
		case NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH:
			printf("RSSI (%i dBm) went above threshold\n", rssi_level);
			found_one = true;
			break;
		case NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW:
			printf("RSSI (%i dBm) went below threshold\n", rssi_level);
			found_one = true;
			break;
		case NL80211_CQM_RSSI_BEACON_LOSS_EVENT:
			printf("Beacon loss detected\n");
			found_one = true;
			break;
		}

		if (!found_one)
			printf("Unknown event type: %i\n", rssi_event);
	} else if (cqm[NL80211_ATTR_CQM_PKT_LOSS_EVENT]) {
		if (attrs[NL80211_ATTR_MAC]) {
			uint32_t frames;
			char buf[3*6];

			frames = nla_get_u32(cqm[NL80211_ATTR_CQM_PKT_LOSS_EVENT]);
			mac_addr_n2a(buf, nla_data(attrs[NL80211_ATTR_MAC]));
			printf("peer %s didn't ACK %d packets\n", buf, frames);
		} else {
			printf("PKT-LOSS-EVENT did not have MAC attribute!\n");
		}
	} else if (cqm[NL80211_ATTR_CQM_BEACON_LOSS_EVENT]) {
		printf("beacon loss\n");
	} else {
		printf("unknown event\n");
	}
}

static const char * key_type_str(enum nl80211_key_type key_type)
{
	static char buf[30];
	switch (key_type) {
	case NL80211_KEYTYPE_GROUP:
		return "Group";
	case NL80211_KEYTYPE_PAIRWISE:
		return "Pairwise";
	case NL80211_KEYTYPE_PEERKEY:
		return "PeerKey";
	default:
		snprintf(buf, sizeof(buf), "unknown(%d)", key_type);
		return buf;
	}
}

static void parse_mic_failure(struct nlattr **attrs)
{
	printf("Michael MIC failure event:");

	if (attrs[NL80211_ATTR_MAC]) {
		char addr[3 * ETH_ALEN];
		mac_addr_n2a(addr, nla_data(attrs[NL80211_ATTR_MAC]));
		printf(" source MAC address %s", addr);
	}

	if (attrs[NL80211_ATTR_KEY_SEQ] &&
	    nla_len(attrs[NL80211_ATTR_KEY_SEQ]) == 6) {
		unsigned char *seq = nla_data(attrs[NL80211_ATTR_KEY_SEQ]);
		printf(" seq=%02x%02x%02x%02x%02x%02x",
		       seq[0], seq[1], seq[2], seq[3], seq[4], seq[5]);
	}
	if (attrs[NL80211_ATTR_KEY_TYPE]) {
		enum nl80211_key_type key_type =
			nla_get_u32(attrs[NL80211_ATTR_KEY_TYPE]);
		printf(" Key Type %s", key_type_str(key_type));
	}

	if (attrs[NL80211_ATTR_KEY_IDX]) {
		__u8 key_id = nla_get_u8(attrs[NL80211_ATTR_KEY_IDX]);
		printf(" Key Id %d", key_id);
	}

	printf("\n");
}

static void parse_wowlan_wake_event(struct nlattr **attrs)
{
	struct nlattr *tb[NUM_NL80211_WOWLAN_TRIG],
		*tb_match[NUM_NL80211_ATTR];

	printf("WoWLAN wakeup\n");
	if (!attrs[NL80211_ATTR_WOWLAN_TRIGGERS]) {
		printf("\twakeup not due to WoWLAN\n");
		return;
	}

	nla_parse(tb, MAX_NL80211_WOWLAN_TRIG,
		  nla_data(attrs[NL80211_ATTR_WOWLAN_TRIGGERS]),
		  nla_len(attrs[NL80211_ATTR_WOWLAN_TRIGGERS]), NULL);

	if (tb[NL80211_WOWLAN_TRIG_DISCONNECT])
		printf("\t* was disconnected\n");
	if (tb[NL80211_WOWLAN_TRIG_MAGIC_PKT])
		printf("\t* magic packet received\n");
	if (tb[NL80211_WOWLAN_TRIG_PKT_PATTERN])
		printf("\t* pattern index: %u\n",
		       nla_get_u32(tb[NL80211_WOWLAN_TRIG_PKT_PATTERN]));
	if (tb[NL80211_WOWLAN_TRIG_GTK_REKEY_FAILURE])
		printf("\t* GTK rekey failure\n");
	if (tb[NL80211_WOWLAN_TRIG_EAP_IDENT_REQUEST])
		printf("\t* EAP identity request\n");
	if (tb[NL80211_WOWLAN_TRIG_4WAY_HANDSHAKE])
		printf("\t* 4-way handshake\n");
	if (tb[NL80211_WOWLAN_TRIG_RFKILL_RELEASE])
		printf("\t* RF-kill released\n");
	if (tb[NL80211_WOWLAN_TRIG_NET_DETECT_RESULTS]) {
		struct nlattr *match, *freq;
		int rem_nst, rem_nst2;

		printf("\t* network detected\n");
		nla_for_each_nested(match,
				    tb[NL80211_WOWLAN_TRIG_NET_DETECT_RESULTS],
				    rem_nst) {
			nla_parse_nested(tb_match, NL80211_ATTR_MAX, match,
					 NULL);
			printf("\t\tSSID: \"");
			print_ssid_escaped(nla_len(tb_match[NL80211_ATTR_SSID]),
					   nla_data(tb_match[NL80211_ATTR_SSID]));
			printf("\"");
			if (tb_match[NL80211_ATTR_SCAN_FREQUENCIES]) {
				printf(" freq(s):");
				nla_for_each_nested(freq,
						    tb_match[NL80211_ATTR_SCAN_FREQUENCIES],
						    rem_nst2)
					printf(" %d", nla_get_u32(freq));
			}
			printf("\n");
		}
	}
	if (tb[NL80211_WOWLAN_TRIG_WAKEUP_PKT_80211]) {
		uint8_t *d = nla_data(tb[NL80211_WOWLAN_TRIG_WAKEUP_PKT_80211]);
		int l = nla_len(tb[NL80211_WOWLAN_TRIG_WAKEUP_PKT_80211]);
		int i;
		printf("\t* packet (might be truncated): ");
		for (i = 0; i < l; i++) {
			if (i > 0)
				printf(":");
			printf("%.2x", d[i]);
		}
		printf("\n");
	}
	if (tb[NL80211_WOWLAN_TRIG_WAKEUP_PKT_8023]) {
		uint8_t *d = nla_data(tb[NL80211_WOWLAN_TRIG_WAKEUP_PKT_8023]);
		int l = nla_len(tb[NL80211_WOWLAN_TRIG_WAKEUP_PKT_8023]);
		int i;
		printf("\t* packet (might be truncated): ");
		for (i = 0; i < l; i++) {
			if (i > 0)
				printf(":");
			printf("%.2x", d[i]);
		}
		printf("\n");
	}
	if (tb[NL80211_WOWLAN_TRIG_WAKEUP_TCP_MATCH])
		printf("\t* TCP connection wakeup received\n");
	if (tb[NL80211_WOWLAN_TRIG_WAKEUP_TCP_CONNLOST])
		printf("\t* TCP connection lost\n");
	if (tb[NL80211_WOWLAN_TRIG_WAKEUP_TCP_NOMORETOKENS])
		printf("\t* TCP connection ran out of tokens\n");
	if (tb[NL80211_WOWLAN_TRIG_UNPROTECTED_DEAUTH_DISASSOC])
		printf("\t* unprotected deauth/disassoc\n");
}

extern struct vendor_event *__start_vendor_event[];
extern struct vendor_event *__stop_vendor_event;

// Dummy to force the section to exist
VENDOR_EVENT(0xffffffff, 0xffffffff, NULL);

static void parse_vendor_event(struct nlattr **attrs, bool dump)
{
	__u32 vendor_id, subcmd;
	unsigned int i;

	if (!attrs[NL80211_ATTR_VENDOR_ID] ||
	    !attrs[NL80211_ATTR_VENDOR_SUBCMD])
		return;

	vendor_id = nla_get_u32(attrs[NL80211_ATTR_VENDOR_ID]);
	subcmd = nla_get_u32(attrs[NL80211_ATTR_VENDOR_SUBCMD]);

	printf("vendor event %.6x:%d", vendor_id, subcmd);

	for (i = 0; i < &__stop_vendor_event - __start_vendor_event; i++) {
		struct vendor_event *ev = __start_vendor_event[i];

		if (!ev)
			continue;

		if (ev->vendor_id != vendor_id)
			continue;
		if (ev->subcmd != subcmd)
			continue;
		if (!ev->callback)
			continue;

		ev->callback(vendor_id, subcmd, attrs[NL80211_ATTR_VENDOR_DATA]);
		goto out;
	}

	if (dump && attrs[NL80211_ATTR_VENDOR_DATA])
		iw_hexdump("vendor event",
			   nla_data(attrs[NL80211_ATTR_VENDOR_DATA]),
			   nla_len(attrs[NL80211_ATTR_VENDOR_DATA]));
out:
	printf("\n");
}

static void parse_nan_term(struct nlattr **attrs)
{
	struct nlattr *func[NL80211_NAN_FUNC_ATTR_MAX + 1];

	static struct nla_policy
		nan_func_policy[NL80211_NAN_FUNC_ATTR_MAX + 1] = {
		[NL80211_NAN_FUNC_TYPE] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_SERVICE_ID] = { },
		[NL80211_NAN_FUNC_PUBLISH_TYPE] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_PUBLISH_BCAST] = { .type = NLA_FLAG },
		[NL80211_NAN_FUNC_SUBSCRIBE_ACTIVE] = { .type = NLA_FLAG },
		[NL80211_NAN_FUNC_FOLLOW_UP_ID] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_FOLLOW_UP_REQ_ID] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_FOLLOW_UP_DEST] = { },
		[NL80211_NAN_FUNC_CLOSE_RANGE] = { .type = NLA_FLAG },
		[NL80211_NAN_FUNC_TTL] = { .type = NLA_U32 },
		[NL80211_NAN_FUNC_SERVICE_INFO] = { },
		[NL80211_NAN_FUNC_SRF] = { .type = NLA_NESTED },
		[NL80211_NAN_FUNC_RX_MATCH_FILTER] = { .type = NLA_NESTED },
		[NL80211_NAN_FUNC_TX_MATCH_FILTER] = { .type = NLA_NESTED },
		[NL80211_NAN_FUNC_INSTANCE_ID] = { .type = NLA_U8},
	};

	if (!attrs[NL80211_ATTR_COOKIE]) {
		printf("Bad NAN func termination format - cookie is missing\n");
		return;
	}

	if (nla_parse_nested(func, NL80211_NAN_FUNC_ATTR_MAX,
			     attrs[NL80211_ATTR_NAN_FUNC],
			     nan_func_policy)) {
		printf("NAN: failed to parse nan func\n");
		return;
	}

	if (!func[NL80211_NAN_FUNC_INSTANCE_ID]) {
		printf("Bad NAN func termination format-instance id missing\n");
		return;
	}

	if (!func[NL80211_NAN_FUNC_TERM_REASON]) {
		printf("Bad NAN func termination format - reason is missing\n");
		return;
	}
	printf("NAN(cookie=0x%llx): Termination event: id = %d, reason = ",
	       (long long int)nla_get_u64(attrs[NL80211_ATTR_COOKIE]),
	       nla_get_u8(func[NL80211_NAN_FUNC_INSTANCE_ID]));
	switch (nla_get_u8(func[NL80211_NAN_FUNC_TERM_REASON])) {
	case NL80211_NAN_FUNC_TERM_REASON_USER_REQUEST:
		printf("user request\n");
		break;
	case NL80211_NAN_FUNC_TERM_REASON_TTL_EXPIRED:
		printf("expired\n");
		break;
	case NL80211_NAN_FUNC_TERM_REASON_ERROR:
		printf("error\n");
		break;
	default:
		printf("unknown\n");
	}
}

static const char *ftm_fail_reason(unsigned int reason)
{
#define FTM_FAIL_REASON(x) case NL80211_PMSR_FTM_FAILURE_##x: return #x
	switch (reason) {
	FTM_FAIL_REASON(UNSPECIFIED);
	FTM_FAIL_REASON(NO_RESPONSE);
	FTM_FAIL_REASON(REJECTED);
	FTM_FAIL_REASON(WRONG_CHANNEL);
	FTM_FAIL_REASON(PEER_NOT_CAPABLE);
	FTM_FAIL_REASON(INVALID_TIMESTAMP);
	FTM_FAIL_REASON(PEER_BUSY);
	FTM_FAIL_REASON(BAD_CHANGED_PARAMS);
	default:
		return "unknown";
	}
}

static void parse_pmsr_ftm_data(struct nlattr *data)
{
	struct nlattr *ftm[NL80211_PMSR_FTM_RESP_ATTR_MAX + 1];

	printf("    FTM");
	nla_parse_nested(ftm, NL80211_PMSR_FTM_RESP_ATTR_MAX, data, NULL);

	if (ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON]) {
		printf(" failed: %s (%d)",
		       ftm_fail_reason(nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON])),
		       nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON]));
		if (ftm[NL80211_PMSR_FTM_RESP_ATTR_BUSY_RETRY_TIME])
			printf(" retry after %us",
			       nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_BUSY_RETRY_TIME]));
		printf("\n");
		return;
	}

	printf("\n");

#define PFTM(tp, attr, sign)							\
	do {									\
		if (ftm[NL80211_PMSR_FTM_RESP_ATTR_##attr])			\
			printf("      " #attr ": %lld\n",			\
			       (sign long long)nla_get_##tp(			\
				ftm[NL80211_PMSR_FTM_RESP_ATTR_##attr]));	\
	} while (0)

	PFTM(u32, BURST_INDEX, unsigned);
	PFTM(u32, NUM_FTMR_ATTEMPTS, unsigned);
	PFTM(u32, NUM_FTMR_SUCCESSES, unsigned);
	PFTM(u8, NUM_BURSTS_EXP, unsigned);
	PFTM(u8, BURST_DURATION, unsigned);
	PFTM(u8, FTMS_PER_BURST, unsigned);
	PFTM(u32, RSSI_AVG, signed);
	PFTM(u32, RSSI_SPREAD, unsigned);
	PFTM(u64, RTT_AVG, signed);
	PFTM(u64, RTT_VARIANCE, unsigned);
	PFTM(u64, RTT_SPREAD, unsigned);
	PFTM(u64, DIST_AVG, signed);
	PFTM(u64, DIST_VARIANCE, unsigned);
	PFTM(u64, DIST_SPREAD, unsigned);

	if (ftm[NL80211_PMSR_FTM_RESP_ATTR_TX_RATE]) {
		char buf[100];

		parse_bitrate(ftm[NL80211_PMSR_FTM_RESP_ATTR_TX_RATE],
			      buf, sizeof(buf));
		printf("      TX bitrate: %s\n", buf);
	}

	if (ftm[NL80211_PMSR_FTM_RESP_ATTR_RX_RATE]) {
		char buf[100];

		parse_bitrate(ftm[NL80211_PMSR_FTM_RESP_ATTR_RX_RATE],
			      buf, sizeof(buf));
		printf("      RX bitrate: %s\n", buf);
	}

	if (ftm[NL80211_PMSR_FTM_RESP_ATTR_LCI])
		iw_hexdump("      LCI",
			   nla_data(ftm[NL80211_PMSR_FTM_RESP_ATTR_LCI]),
			   nla_len(ftm[NL80211_PMSR_FTM_RESP_ATTR_LCI]));

	if (ftm[NL80211_PMSR_FTM_RESP_ATTR_CIVICLOC])
		iw_hexdump("      civic location",
			   nla_data(ftm[NL80211_PMSR_FTM_RESP_ATTR_CIVICLOC]),
			   nla_len(ftm[NL80211_PMSR_FTM_RESP_ATTR_CIVICLOC]));
}

static const char *pmsr_status(unsigned int status)
{
#define PMSR_STATUS(x) case NL80211_PMSR_STATUS_##x: return #x
	switch (status) {
	PMSR_STATUS(SUCCESS);
	PMSR_STATUS(REFUSED);
	PMSR_STATUS(TIMEOUT);
	PMSR_STATUS(FAILURE);
	default:
		return "unknown";
	}
#undef PMSR_STATUS
}

static void parse_pmsr_peer(struct nlattr *peer)
{
	struct nlattr *tb[NL80211_PMSR_PEER_ATTR_MAX + 1];
	struct nlattr *resp[NL80211_PMSR_RESP_ATTR_MAX + 1];
	struct nlattr *data[NL80211_PMSR_TYPE_MAX + 1];
	char macbuf[6*3];
	int err;

	err = nla_parse_nested(tb, NL80211_PMSR_PEER_ATTR_MAX, peer, NULL);
	if (err) {
		printf("  Peer: failed to parse!\n");
		return;
	}

	if (!tb[NL80211_PMSR_PEER_ATTR_ADDR]) {
		printf("  Peer: no MAC address\n");
		return;
	}

	mac_addr_n2a(macbuf, nla_data(tb[NL80211_PMSR_PEER_ATTR_ADDR]));
	printf("  Peer %s:", macbuf);

	if (!tb[NL80211_PMSR_PEER_ATTR_RESP]) {
		printf(" no response!\n");
		return;
	}

	err = nla_parse_nested(resp, NL80211_PMSR_RESP_ATTR_MAX,
			       tb[NL80211_PMSR_PEER_ATTR_RESP], NULL);
	if (err) {
		printf(" failed to parse response!\n");
		return;
	}

	if (resp[NL80211_PMSR_RESP_ATTR_STATUS])
		printf(" status=%d (%s)",
		       nla_get_u32(resp[NL80211_PMSR_RESP_ATTR_STATUS]),
		       pmsr_status(nla_get_u32(resp[NL80211_PMSR_RESP_ATTR_STATUS])));
	if (resp[NL80211_PMSR_RESP_ATTR_HOST_TIME])
		printf(" @%llu",
		       (unsigned long long)nla_get_u64(resp[NL80211_PMSR_RESP_ATTR_HOST_TIME]));
	if (resp[NL80211_PMSR_RESP_ATTR_AP_TSF])
		printf(" tsf=%llu",
		       (unsigned long long)nla_get_u64(resp[NL80211_PMSR_RESP_ATTR_AP_TSF]));
	if (resp[NL80211_PMSR_RESP_ATTR_FINAL])
		printf(" (final)");

	if (!resp[NL80211_PMSR_RESP_ATTR_DATA]) {
		printf(" - no data!\n");
		return;
	}

	printf("\n");

	nla_parse_nested(data, NL80211_PMSR_TYPE_MAX,
			 resp[NL80211_PMSR_RESP_ATTR_DATA], NULL);

	if (data[NL80211_PMSR_TYPE_FTM])
		parse_pmsr_ftm_data(data[NL80211_PMSR_TYPE_FTM]);
}

static void parse_pmsr_result(struct nlattr **tb,
			      struct print_event_args *pargs)
{
	struct nlattr *pmsr[NL80211_PMSR_ATTR_MAX + 1];
	struct nlattr *peer;
	unsigned long long cookie;
	int err, i;

	if (!tb[NL80211_ATTR_COOKIE]) {
		printf("Peer measurements: no cookie!\n");
		return;
	}
	cookie = nla_get_u64(tb[NL80211_ATTR_COOKIE]);

	if (!tb[NL80211_ATTR_PEER_MEASUREMENTS]) {
		printf("Peer measurements: no measurement data!\n");
		return;
	}

	err = nla_parse_nested(pmsr, NL80211_PMSR_ATTR_MAX,
			       tb[NL80211_ATTR_PEER_MEASUREMENTS], NULL);
	if (err) {
		printf("Peer measurements: failed to parse measurement data!\n");
		return;
	}

	if (!pmsr[NL80211_PMSR_ATTR_PEERS]) {
		printf("Peer measurements: no peer data!\n");
		return;
	}

	printf("Peer measurements (cookie %llu):\n", cookie);

	nla_for_each_nested(peer, pmsr[NL80211_PMSR_ATTR_PEERS], i)
		parse_pmsr_peer(peer);
}

static void parse_nan_match(struct nlattr **attrs)
{
	char macbuf[6*3];
	__u64 cookie;
	struct nlattr *match[NL80211_NAN_MATCH_ATTR_MAX + 1];
	struct nlattr *local_func[NL80211_NAN_FUNC_ATTR_MAX + 1];
	struct nlattr *peer_func[NL80211_NAN_FUNC_ATTR_MAX + 1];

	static struct nla_policy
		nan_match_policy[NL80211_NAN_MATCH_ATTR_MAX + 1] = {
		[NL80211_NAN_MATCH_FUNC_LOCAL] = { .type = NLA_NESTED },
		[NL80211_NAN_MATCH_FUNC_PEER] = { .type = NLA_NESTED },
	};

	static struct nla_policy
		nan_func_policy[NL80211_NAN_FUNC_ATTR_MAX + 1] = {
		[NL80211_NAN_FUNC_TYPE] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_SERVICE_ID] = { },
		[NL80211_NAN_FUNC_PUBLISH_TYPE] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_PUBLISH_BCAST] = { .type = NLA_FLAG },
		[NL80211_NAN_FUNC_SUBSCRIBE_ACTIVE] = { .type = NLA_FLAG },
		[NL80211_NAN_FUNC_FOLLOW_UP_ID] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_FOLLOW_UP_REQ_ID] = { .type = NLA_U8 },
		[NL80211_NAN_FUNC_FOLLOW_UP_DEST] = { },
		[NL80211_NAN_FUNC_CLOSE_RANGE] = { .type = NLA_FLAG },
		[NL80211_NAN_FUNC_TTL] = { .type = NLA_U32 },
		[NL80211_NAN_FUNC_SERVICE_INFO] = { },
		[NL80211_NAN_FUNC_SRF] = { .type = NLA_NESTED },
		[NL80211_NAN_FUNC_RX_MATCH_FILTER] = { .type = NLA_NESTED },
		[NL80211_NAN_FUNC_TX_MATCH_FILTER] = { .type = NLA_NESTED },
		[NL80211_NAN_FUNC_INSTANCE_ID] = { .type = NLA_U8},
	};

	cookie = nla_get_u64(attrs[NL80211_ATTR_COOKIE]);
	mac_addr_n2a(macbuf, nla_data(attrs[NL80211_ATTR_MAC]));

	if (nla_parse_nested(match, NL80211_NAN_MATCH_ATTR_MAX,
			     attrs[NL80211_ATTR_NAN_MATCH],
			     nan_match_policy)) {
		printf("NAN: failed to parse nan match event\n");
		return;
	}

	if (nla_parse_nested(local_func, NL80211_NAN_FUNC_ATTR_MAX,
			     match[NL80211_NAN_MATCH_FUNC_LOCAL],
			     nan_func_policy)) {
		printf("NAN: failed to parse nan local func\n");
		return;
	}

	if (nla_parse_nested(peer_func, NL80211_NAN_FUNC_ATTR_MAX,
			      match[NL80211_NAN_MATCH_FUNC_PEER],
			      nan_func_policy)) {
		printf("NAN: failed to parse nan local func\n");
		return;
	}

	if (nla_get_u8(peer_func[NL80211_NAN_FUNC_TYPE]) ==
	    NL80211_NAN_FUNC_PUBLISH) {
		printf(
		       "NAN(cookie=0x%llx): DiscoveryResult, peer_id=%d, local_id=%d, peer_mac=%s",
		       cookie,
		       nla_get_u8(peer_func[NL80211_NAN_FUNC_INSTANCE_ID]),
		       nla_get_u8(local_func[NL80211_NAN_FUNC_INSTANCE_ID]),
		       macbuf);
		if (peer_func[NL80211_NAN_FUNC_SERVICE_INFO])
			printf(", info=%.*s",
				   nla_len(peer_func[NL80211_NAN_FUNC_SERVICE_INFO]),
			       (char *)nla_data(peer_func[NL80211_NAN_FUNC_SERVICE_INFO]));
	} else if (nla_get_u8(peer_func[NL80211_NAN_FUNC_TYPE]) ==
		   NL80211_NAN_FUNC_SUBSCRIBE) {
		printf(
		       "NAN(cookie=0x%llx): Replied, peer_id=%d, local_id=%d, peer_mac=%s",
		       cookie,
		       nla_get_u8(peer_func[NL80211_NAN_FUNC_INSTANCE_ID]),
		       nla_get_u8(local_func[NL80211_NAN_FUNC_INSTANCE_ID]),
		       macbuf);
	} else if (nla_get_u8(peer_func[NL80211_NAN_FUNC_TYPE]) ==
		   NL80211_NAN_FUNC_FOLLOW_UP) {
		printf(
		       "NAN(cookie=0x%llx): FollowUpReceive, peer_id=%d, local_id=%d, peer_mac=%s",
		       cookie,
		       nla_get_u8(peer_func[NL80211_NAN_FUNC_INSTANCE_ID]),
		       nla_get_u8(local_func[NL80211_NAN_FUNC_INSTANCE_ID]),
		       macbuf);
		if (peer_func[NL80211_NAN_FUNC_SERVICE_INFO])
			printf(", info=%.*s",
			       nla_len(peer_func[NL80211_NAN_FUNC_SERVICE_INFO]),
			       (char *)nla_data(peer_func[NL80211_NAN_FUNC_SERVICE_INFO]));
	} else {
		printf("NaN: Malformed event");
	}

	printf("\n");
}

static void parse_new_peer_candidate(struct nlattr **attrs)
{
	char macbuf[ETH_ALEN * 3];
	int32_t sig_dbm;

	printf("new peer candidate");
	if (attrs[NL80211_ATTR_MAC]) {
		mac_addr_n2a(macbuf, nla_data(attrs[NL80211_ATTR_MAC]));
		printf(" %s", macbuf);
	}
	if (attrs[NL80211_ATTR_RX_SIGNAL_DBM]) {
		sig_dbm = nla_get_u32(attrs[NL80211_ATTR_RX_SIGNAL_DBM]);
		printf(" %d dBm", sig_dbm);
	}

	printf("\n");
}

static void parse_recv_interface(struct nlattr **attrs, int command)
{
	switch (command) {
	case NL80211_CMD_NEW_INTERFACE:
		printf("new interface");
		break;
	case NL80211_CMD_DEL_INTERFACE:
		printf("del interface");
		break;
	case NL80211_CMD_SET_INTERFACE:
		printf("set interface");
		break;
	default:
		printf("unknown interface command (%i) received\n", command);
		return;
	}

	if (attrs[NL80211_ATTR_IFTYPE]) {
		printf(" type ");
		switch (nla_get_u32(attrs[NL80211_ATTR_IFTYPE])) {
		case NL80211_IFTYPE_STATION:
			printf("station");
			break;
		case NL80211_IFTYPE_AP:
			printf("access point");
			break;
		case NL80211_IFTYPE_MESH_POINT:
			printf("mesh point");
			break;
		case NL80211_IFTYPE_ADHOC:
			printf("IBSS");
			break;
		case NL80211_IFTYPE_MONITOR:
			printf("monitor");
			break;
		case NL80211_IFTYPE_AP_VLAN:
			printf("AP-VLAN");
			break;
		case NL80211_IFTYPE_WDS:
			printf("WDS");
			break;
		case NL80211_IFTYPE_P2P_CLIENT:
			printf("P2P-client");
			break;
		case NL80211_IFTYPE_P2P_GO:
			printf("P2P-GO");
			break;
		case NL80211_IFTYPE_P2P_DEVICE:
			printf("P2P-Device");
			break;
		case NL80211_IFTYPE_OCB:
			printf("OCB");
			break;
		case NL80211_IFTYPE_NAN:
			printf("NAN");
			break;
		default:
			printf("unknown (%d)",
			       nla_get_u32(attrs[NL80211_ATTR_IFTYPE]));
			break;
		}
	}

	if (attrs[NL80211_ATTR_MESH_ID]) {
		printf(" meshid ");
		print_ssid_escaped(nla_len(attrs[NL80211_ATTR_MESH_ID]),
				   nla_data(attrs[NL80211_ATTR_MESH_ID]));
	}

	if (attrs[NL80211_ATTR_4ADDR]) {
		printf(" use 4addr %d", nla_get_u8(attrs[NL80211_ATTR_4ADDR]));
	}

	printf("\n");
}

static void parse_sta_opmode_changed(struct nlattr **attrs)
{
	char macbuf[ETH_ALEN*3];

	printf("sta opmode changed");

	if (attrs[NL80211_ATTR_MAC]) {
		mac_addr_n2a(macbuf, nla_data(attrs[NL80211_ATTR_MAC]));
		printf(" %s", macbuf);
	}

	if (attrs[NL80211_ATTR_SMPS_MODE])
		printf(" smps mode %d", nla_get_u8(attrs[NL80211_ATTR_SMPS_MODE]));

	if (attrs[NL80211_ATTR_CHANNEL_WIDTH])
		printf(" chan width %d", nla_get_u8(attrs[NL80211_ATTR_CHANNEL_WIDTH]));

	if (attrs[NL80211_ATTR_NSS])
		printf(" nss %d", nla_get_u8(attrs[NL80211_ATTR_NSS]));

	printf("\n");
}

static void parse_ch_switch_notify(struct nlattr **attrs, int command)
{
	switch (command) {
	case NL80211_CMD_CH_SWITCH_STARTED_NOTIFY:
		printf("channel switch started");
		break;
	case NL80211_CMD_CH_SWITCH_NOTIFY:
		printf("channel switch");
		break;
	default:
		printf("unknown channel switch command (%i) received\n", command);
		return;
	}

	if (attrs[NL80211_ATTR_CH_SWITCH_COUNT])
		printf(" (count=%d)", nla_get_u32(attrs[NL80211_ATTR_CH_SWITCH_COUNT]));

	if (attrs[NL80211_ATTR_WIPHY_FREQ])
		printf(" freq=%d", nla_get_u32(attrs[NL80211_ATTR_WIPHY_FREQ]));

	if (attrs[NL80211_ATTR_CHANNEL_WIDTH]) {
		printf(" width=");
		switch(nla_get_u32(attrs[NL80211_ATTR_CHANNEL_WIDTH])) {
		case NL80211_CHAN_WIDTH_20_NOHT:
		case NL80211_CHAN_WIDTH_20:
			printf("\"20 MHz\"");
			break;
		case NL80211_CHAN_WIDTH_40:
			printf("\"40 MHz\"");
			break;
		case NL80211_CHAN_WIDTH_80:
			printf("\"80 MHz\"");
			break;
		case NL80211_CHAN_WIDTH_80P80:
			printf("\"80+80 MHz\"");
			break;
		case NL80211_CHAN_WIDTH_160:
			printf("\"160 MHz\"");
			break;
		case NL80211_CHAN_WIDTH_5:
			printf("\"5 MHz\"");
			break;
		case NL80211_CHAN_WIDTH_10:
			printf("\"10 MHz\"");
			break;
		default:
			printf("\"unknown\"");
		}
	}

	if (attrs[NL80211_ATTR_WIPHY_CHANNEL_TYPE]) {
		printf(" type=");
		switch(nla_get_u32(attrs[NL80211_ATTR_WIPHY_CHANNEL_TYPE])) {
		case NL80211_CHAN_NO_HT:
			printf("\"No HT\"");
			break;
		case NL80211_CHAN_HT20:
			printf("\"HT20\"");
			break;
		case NL80211_CHAN_HT40MINUS:
			printf("\"HT40-\"");
			break;
		case NL80211_CHAN_HT40PLUS:
			printf("\"HT40+\"");
			break;
		}
	}

	if (attrs[NL80211_ATTR_CENTER_FREQ1])
		printf(" freq1=%d", nla_get_u32(attrs[NL80211_ATTR_CENTER_FREQ1]));

	if (attrs[NL80211_ATTR_CENTER_FREQ2])
		printf(" freq2=%d", nla_get_u32(attrs[NL80211_ATTR_CENTER_FREQ2]));

	printf("\n");
}

static void parse_assoc_comeback(struct nlattr **attrs, int command)
{
	__u32 timeout = 0;
	char macbuf[6 * 3] = "<unset>";

	if (attrs[NL80211_ATTR_MAC])
		mac_addr_n2a(macbuf, nla_data(attrs[NL80211_ATTR_MAC]));

	if (attrs[NL80211_ATTR_TIMEOUT])
		timeout = nla_get_u32(attrs[NL80211_ATTR_TIMEOUT]);

	printf("assoc comeback bssid %s timeout %d\n",
	       macbuf, timeout);
}

static int print_event(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1], *nst;
	struct print_event_args *args = arg;
	char ifname[100];
	char macbuf[6*3];
	__u8 reg_type;
	struct ieee80211_beacon_channel chan_before_beacon,  chan_after_beacon;
	__u32 wiphy_idx = 0;
	int rem_nst;
	__u16 status;

	if (args->time || args->reltime || args->ctime) {
		unsigned long long usecs, previous;

		previous = 1000000ULL * args->ts.tv_sec + args->ts.tv_usec;
		gettimeofday(&args->ts, NULL);
		usecs = 1000000ULL * args->ts.tv_sec + args->ts.tv_usec;

		if (args->reltime) {
			if (!args->have_ts) {
				usecs = 0;
				args->have_ts = true;
			} else
				usecs -= previous;
		}

		if (args->ctime) {
			struct tm *tm = localtime(&args->ts.tv_sec);
			char buf[255];

			memset(buf, 0, 255);
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
			printf("[%s.%06lu]: ", buf, (unsigned long )args->ts.tv_usec);
		} else {
			printf("%llu.%06llu: ", usecs/1000000, usecs % 1000000);
		}
	}

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_IFINDEX] && tb[NL80211_ATTR_WIPHY]) {
		/* if_indextoname may fails on delete interface/wiphy event */
		if (if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), ifname))
			printf("%s (phy #%d): ", ifname, nla_get_u32(tb[NL80211_ATTR_WIPHY]));
		else
			printf("phy #%d: ", nla_get_u32(tb[NL80211_ATTR_WIPHY]));
	} else if (tb[NL80211_ATTR_WDEV] && tb[NL80211_ATTR_WIPHY]) {
		printf("wdev 0x%llx (phy #%d): ",
			(unsigned long long)nla_get_u64(tb[NL80211_ATTR_WDEV]),
			nla_get_u32(tb[NL80211_ATTR_WIPHY]));
	} else if (tb[NL80211_ATTR_IFINDEX]) {
		if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), ifname);
		printf("%s: ", ifname);
	} else if (tb[NL80211_ATTR_WDEV]) {
		printf("wdev 0x%llx: ", (unsigned long long)nla_get_u64(tb[NL80211_ATTR_WDEV]));
	} else if (tb[NL80211_ATTR_WIPHY]) {
		printf("phy #%d: ", nla_get_u32(tb[NL80211_ATTR_WIPHY]));
	}

	switch (gnlh->cmd) {
	case NL80211_CMD_NEW_WIPHY:
		printf("renamed to %s\n", nla_get_string(tb[NL80211_ATTR_WIPHY_NAME]));
		break;
	case NL80211_CMD_TRIGGER_SCAN:
		printf("scan started\n");
		break;
	case NL80211_CMD_NEW_SCAN_RESULTS:
		printf("scan finished:");
		/* fall through */
	case NL80211_CMD_SCAN_ABORTED:
		if (gnlh->cmd == NL80211_CMD_SCAN_ABORTED)
			printf("scan aborted:");
		if (tb[NL80211_ATTR_SCAN_FREQUENCIES]) {
			nla_for_each_nested(nst, tb[NL80211_ATTR_SCAN_FREQUENCIES], rem_nst)
				printf(" %d", nla_get_u32(nst));
			printf(",");
		}
		if (tb[NL80211_ATTR_SCAN_SSIDS]) {
			nla_for_each_nested(nst, tb[NL80211_ATTR_SCAN_SSIDS], rem_nst) {
				printf(" \"");
				print_ssid_escaped(nla_len(nst), nla_data(nst));
				printf("\"");
			}
		}
		printf("\n");
		break;
	case NL80211_CMD_START_SCHED_SCAN:
		printf("scheduled scan started\n");
		break;
	case NL80211_CMD_SCHED_SCAN_STOPPED:
		printf("sched scan stopped\n");
		break;
	case NL80211_CMD_SCHED_SCAN_RESULTS:
		printf("got scheduled scan results\n");
		break;
	case NL80211_CMD_WIPHY_REG_CHANGE:
	case NL80211_CMD_REG_CHANGE:
		if (gnlh->cmd == NL80211_CMD_WIPHY_REG_CHANGE)
			printf("regulatory domain change (phy): ");
		else
			printf("regulatory domain change: ");

		reg_type = nla_get_u8(tb[NL80211_ATTR_REG_TYPE]);

		switch (reg_type) {
		case NL80211_REGDOM_TYPE_COUNTRY:
			printf("set to %s by %s request",
			       nla_get_string(tb[NL80211_ATTR_REG_ALPHA2]),
			       reg_initiator_to_string(nla_get_u8(tb[NL80211_ATTR_REG_INITIATOR])));
			if (tb[NL80211_ATTR_WIPHY])
				printf(" on phy%d", nla_get_u32(tb[NL80211_ATTR_WIPHY]));
			break;
		case NL80211_REGDOM_TYPE_WORLD:
			printf("set to world roaming by %s request",
			       reg_initiator_to_string(nla_get_u8(tb[NL80211_ATTR_REG_INITIATOR])));
			break;
		case NL80211_REGDOM_TYPE_CUSTOM_WORLD:
			printf("custom world roaming rules in place on phy%d by %s request",
			       nla_get_u32(tb[NL80211_ATTR_WIPHY]),
			       reg_initiator_to_string(nla_get_u32(tb[NL80211_ATTR_REG_INITIATOR])));
			break;
		case NL80211_REGDOM_TYPE_INTERSECTION:
			printf("intersection used due to a request made by %s",
			       reg_initiator_to_string(nla_get_u32(tb[NL80211_ATTR_REG_INITIATOR])));
			if (tb[NL80211_ATTR_WIPHY])
				printf(" on phy%d", nla_get_u32(tb[NL80211_ATTR_WIPHY]));
			break;
		default:
			printf("unknown source (upgrade this utility)");
			break;
		}

		printf("\n");
		break;
	case NL80211_CMD_REG_BEACON_HINT:

		wiphy_idx = nla_get_u32(tb[NL80211_ATTR_WIPHY]);

		memset(&chan_before_beacon, 0, sizeof(chan_before_beacon));
		memset(&chan_after_beacon, 0, sizeof(chan_after_beacon));

		if (parse_beacon_hint_chan(tb[NL80211_ATTR_FREQ_BEFORE],
					   &chan_before_beacon))
			break;
		if (parse_beacon_hint_chan(tb[NL80211_ATTR_FREQ_AFTER],
					   &chan_after_beacon))
			break;

		if (chan_before_beacon.center_freq != chan_after_beacon.center_freq)
			break;

		/* A beacon hint is sent _only_ if something _did_ change */
		printf("beacon hint:\n");

		printf("phy%d %d MHz [%d]:\n",
		       wiphy_idx,
		       chan_before_beacon.center_freq,
		       ieee80211_frequency_to_channel(chan_before_beacon.center_freq));

		if (chan_before_beacon.no_ir && !chan_after_beacon.no_ir) {
			if (chan_before_beacon.no_ibss && !chan_after_beacon.no_ibss)
				printf("\to Initiating radiation enabled\n");
			else
				printf("\to active scan enabled\n");
		} else if (chan_before_beacon.no_ibss && !chan_after_beacon.no_ibss) {
			printf("\to ibss enabled\n");
		}

		break;
	case NL80211_CMD_NEW_STATION:
		mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
		printf("new station %s\n", macbuf);
		break;
	case NL80211_CMD_DEL_STATION:
		mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
		printf("del station %s\n", macbuf);
		break;
	case NL80211_CMD_JOIN_IBSS:
		mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
		printf("IBSS %s joined\n", macbuf);
		break;
	case NL80211_CMD_AUTHENTICATE:
		printf("auth");
		if (tb[NL80211_ATTR_FRAME])
			print_frame(args, tb[NL80211_ATTR_FRAME]);
		else if (tb[NL80211_ATTR_TIMED_OUT])
			printf(": timed out");
		else
			printf(": unknown event");
		printf("\n");
		break;
	case NL80211_CMD_ASSOCIATE:
		printf("assoc");
		if (tb[NL80211_ATTR_FRAME])
			print_frame(args, tb[NL80211_ATTR_FRAME]);
		else if (tb[NL80211_ATTR_TIMED_OUT])
			printf(": timed out");
		else
			printf(": unknown event");
		printf("\n");
		break;
	case NL80211_CMD_DEAUTHENTICATE:
		printf("deauth");
		print_frame(args, tb[NL80211_ATTR_FRAME]);
		printf("\n");
		break;
	case NL80211_CMD_DISASSOCIATE:
		printf("disassoc");
		print_frame(args, tb[NL80211_ATTR_FRAME]);
		printf("\n");
		break;
	case NL80211_CMD_UNPROT_DEAUTHENTICATE:
		printf("unprotected deauth");
		print_frame(args, tb[NL80211_ATTR_FRAME]);
		printf("\n");
		break;
	case NL80211_CMD_UNPROT_DISASSOCIATE:
		printf("unprotected disassoc");
		print_frame(args, tb[NL80211_ATTR_FRAME]);
		printf("\n");
		break;
	case NL80211_CMD_CONNECT:
		status = 0;
		if (tb[NL80211_ATTR_TIMED_OUT])
			printf("timed out");
		else if (!tb[NL80211_ATTR_STATUS_CODE])
			printf("unknown connect status");
		else if (nla_get_u16(tb[NL80211_ATTR_STATUS_CODE]) == 0)
			printf("connected");
		else {
			status = nla_get_u16(tb[NL80211_ATTR_STATUS_CODE]);
			printf("failed to connect");
		}
		if (tb[NL80211_ATTR_MAC]) {
			mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
			printf(" to %s", macbuf);
		}
		if (status)
			printf(", status: %d: %s", status, get_status_str(status));
		printf("\n");
		break;
	case NL80211_CMD_ROAM:
		printf("roamed");
		if (tb[NL80211_ATTR_MAC]) {
			mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
			printf(" to %s", macbuf);
		}
		printf("\n");
		break;
	case NL80211_CMD_DISCONNECT:
		printf("disconnected");
		if (tb[NL80211_ATTR_DISCONNECTED_BY_AP])
			printf(" (by AP)");
		else
			printf(" (local request)");
		if (tb[NL80211_ATTR_REASON_CODE])
			printf(" reason: %d: %s", nla_get_u16(tb[NL80211_ATTR_REASON_CODE]),
				get_reason_str(nla_get_u16(tb[NL80211_ATTR_REASON_CODE])));
		printf("\n");
		break;
	case NL80211_CMD_REMAIN_ON_CHANNEL:
		printf("remain on freq %d (%dms, cookie %llx)\n",
			nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]),
			nla_get_u32(tb[NL80211_ATTR_DURATION]),
			(unsigned long long)nla_get_u64(tb[NL80211_ATTR_COOKIE]));
		break;
	case NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL:
		printf("done with remain on freq %d (cookie %llx)\n",
			nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]),
			(unsigned long long)nla_get_u64(tb[NL80211_ATTR_COOKIE]));
		break;
	case NL80211_CMD_FRAME_WAIT_CANCEL:
		printf("frame wait cancel on freq %d (cookie %llx)\n",
			nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]),
			(unsigned long long)nla_get_u64(tb[NL80211_ATTR_COOKIE]));
		break;
	case NL80211_CMD_NOTIFY_CQM:
		parse_cqm_event(tb);
		break;
	case NL80211_CMD_MICHAEL_MIC_FAILURE:
		parse_mic_failure(tb);
		break;
	case NL80211_CMD_FRAME_TX_STATUS:
		printf("mgmt TX status (cookie %llx): %s\n",
			(unsigned long long)nla_get_u64(tb[NL80211_ATTR_COOKIE]),
			tb[NL80211_ATTR_ACK] ? "acked" : "no ack");
		break;
	case NL80211_CMD_CONTROL_PORT_FRAME_TX_STATUS:
		printf("ctrl. port TX status (cookie %llx): %s\n",
			(unsigned long long)nla_get_u64(tb[NL80211_ATTR_COOKIE]),
			tb[NL80211_ATTR_ACK] ? "acked" : "no ack");
		break;
	case NL80211_CMD_PMKSA_CANDIDATE:
		printf("PMKSA candidate found\n");
		break;
	case NL80211_CMD_SET_WOWLAN:
		parse_wowlan_wake_event(tb);
		break;
	case NL80211_CMD_PROBE_CLIENT:
		if (tb[NL80211_ATTR_MAC])
			mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
		else
			strcpy(macbuf, "??");
		printf("probe client %s (cookie %llx): %s\n",
		       macbuf,
		       (unsigned long long)nla_get_u64(tb[NL80211_ATTR_COOKIE]),
		       tb[NL80211_ATTR_ACK] ? "acked" : "no ack");
		break;
	case NL80211_CMD_VENDOR:
		parse_vendor_event(tb, args->frame);
		break;
	case NL80211_CMD_RADAR_DETECT: {
		enum nl80211_radar_event event_type;
		uint32_t freq;

		if (!tb[NL80211_ATTR_RADAR_EVENT] ||
		    !tb[NL80211_ATTR_WIPHY_FREQ]) {
			printf("BAD radar event\n");
			break;
		}

		freq = nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]);
		event_type = nla_get_u32(tb[NL80211_ATTR_RADAR_EVENT]);

		switch (event_type) {
		case NL80211_RADAR_DETECTED:
			printf("%d MHz: radar detected\n", freq);
			break;
		case NL80211_RADAR_CAC_FINISHED:
			printf("%d MHz: CAC finished\n", freq);
			break;
		case NL80211_RADAR_CAC_ABORTED:
			printf("%d MHz: CAC was aborted\n", freq);
			break;
		case NL80211_RADAR_NOP_FINISHED:
			printf("%d MHz: NOP finished\n", freq);
			break;
		case NL80211_RADAR_PRE_CAC_EXPIRED:
			printf("%d MHz: PRE-CAC expired\n", freq);
			break;
		case NL80211_RADAR_CAC_STARTED:
			printf("%d MHz: CAC started\n", freq);
			break;
		default:
			printf("%d MHz: unknown radar event\n", freq);
		}
		}
		break;
	case NL80211_CMD_DEL_WIPHY:
		printf("delete wiphy\n");
		break;
	case NL80211_CMD_PEER_MEASUREMENT_RESULT:
		parse_pmsr_result(tb, args);
		break;
	case NL80211_CMD_PEER_MEASUREMENT_COMPLETE:
		printf("peer measurement complete\n");
		break;
	case NL80211_CMD_DEL_NAN_FUNCTION:
		parse_nan_term(tb);
		break;
	case NL80211_CMD_NAN_MATCH:
		parse_nan_match(tb);
		break;
	case NL80211_CMD_NEW_PEER_CANDIDATE:
		parse_new_peer_candidate(tb);
		break;
	case NL80211_CMD_NEW_INTERFACE:
	case NL80211_CMD_SET_INTERFACE:
	case NL80211_CMD_DEL_INTERFACE:
		parse_recv_interface(tb, gnlh->cmd);
		break;
	case NL80211_CMD_STA_OPMODE_CHANGED:
		parse_sta_opmode_changed(tb);
		break;
	case NL80211_CMD_STOP_AP:
		printf("stop ap\n");
		break;
	case NL80211_CMD_CH_SWITCH_STARTED_NOTIFY:
	case NL80211_CMD_CH_SWITCH_NOTIFY:
		parse_ch_switch_notify(tb, gnlh->cmd);
		break;
	case NL80211_CMD_ASSOC_COMEBACK: /* 147 */
		parse_assoc_comeback(tb, gnlh->cmd);
		break;
	default:
		printf("unknown event %d (%s)\n",
		       gnlh->cmd, command_name(gnlh->cmd));
		break;
	}

	fflush(stdout);
	return NL_SKIP;
}

struct wait_event {
	int n_cmds, n_prints;
	const __u32 *cmds;
	const __u32 *prints;
	__u32 cmd;
	struct print_event_args *pargs;
};

static int wait_event(struct nl_msg *msg, void *arg)
{
	struct wait_event *wait = arg;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	int i;

	if (wait->pargs) {
		for (i = 0; i < wait->n_prints; i++) {
			if (gnlh->cmd == wait->prints[i])
				print_event(msg, wait->pargs);
		}
	}

	for (i = 0; i < wait->n_cmds; i++) {
		if (gnlh->cmd == wait->cmds[i])
			wait->cmd = gnlh->cmd;
	}

	return NL_SKIP;
}

int __prepare_listen_events(struct nl80211_state *state)
{
	int mcid, ret;

	/* Configuration multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "config");
	if (mcid < 0)
		return mcid;

	ret = nl_socket_add_membership(state->nl_sock, mcid);
	if (ret)
		return ret;

	/* Scan multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "scan");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	/* Regulatory multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "regulatory");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	/* MLME multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "mlme");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "vendor");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "nan");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	return 0;
}

__u32 __do_listen_events(struct nl80211_state *state,
			 const int n_waits, const __u32 *waits,
			 const int n_prints, const __u32 *prints,
			 struct print_event_args *args)
{
	struct nl_cb *cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	struct wait_event wait_ev;

	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		return -ENOMEM;
	}

	/* no sequence checking for multicast messages */
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	if (n_waits && waits) {
		wait_ev.cmds = waits;
		wait_ev.n_cmds = n_waits;
		wait_ev.prints = prints;
		wait_ev.n_prints = n_prints;
		wait_ev.pargs = args;
		register_handler(wait_event, &wait_ev);
	} else
		register_handler(print_event, args);

	wait_ev.cmd = 0;

	while (!wait_ev.cmd)
		nl_recvmsgs(state->nl_sock, cb);

	nl_cb_put(cb);

	return wait_ev.cmd;
}

__u32 listen_events(struct nl80211_state *state,
		    const int n_waits, const __u32 *waits)
{
	int ret;

	ret = __prepare_listen_events(state);
	if (ret)
		return ret;

	return __do_listen_events(state, n_waits, waits, 0, NULL, NULL);
}

static int print_events(struct nl80211_state *state,
			struct nl_msg *msg,
			int argc, char **argv,
			enum id_input id)
{
	struct print_event_args args;
	int num_time_formats = 0;
	int ret;

	memset(&args, 0, sizeof(args));

	argc--;
	argv++;

	while (argc > 0) {
		if (strcmp(argv[0], "-f") == 0)
			args.frame = true;
		else if (strcmp(argv[0], "-t") == 0) {
			num_time_formats++;
			args.time = true;
		} else if (strcmp(argv[0], "-T") == 0) {
			num_time_formats++;
			args.ctime = true;
		} else if (strcmp(argv[0], "-r") == 0) {
			num_time_formats++;
			args.reltime = true;
		} else
			return 1;
		argc--;
		argv++;
	}

	if (num_time_formats > 1)
		return 1;

	if (argc)
		return 1;

	ret = __prepare_listen_events(state);
	if (ret)
		return ret;

	return __do_listen_events(state, 0, NULL, 0, NULL, &args);
}
TOPLEVEL(event, "[-t|-T|-r] [-f]", 0, 0, CIB_NONE, print_events,
	"Monitor events from the kernel.\n"
	"-t - print timestamp\n"
	"-T - print absolute, human-readable timestamp\n"
	"-r - print relative timestamp\n"
	"-f - print full frame for auth/assoc etc.");
