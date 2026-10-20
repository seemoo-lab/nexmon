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

#define WLAN_CAPABILITY_ESS		(1<<0)
#define WLAN_CAPABILITY_IBSS		(1<<1)
#define WLAN_CAPABILITY_CF_POLLABLE	(1<<2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST	(1<<3)
#define WLAN_CAPABILITY_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE	(1<<5)
#define WLAN_CAPABILITY_PBCC		(1<<6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY	(1<<7)
#define WLAN_CAPABILITY_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_QOS		(1<<9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME	(1<<10)
#define WLAN_CAPABILITY_APSD		(1<<11)
#define WLAN_CAPABILITY_RADIO_MEASURE	(1<<12)
#define WLAN_CAPABILITY_DSSS_OFDM	(1<<13)
#define WLAN_CAPABILITY_DEL_BACK	(1<<14)
#define WLAN_CAPABILITY_IMM_BACK	(1<<15)
/* DMG (60gHz) 802.11ad */
/* type - bits 0..1 */
#define WLAN_CAPABILITY_DMG_TYPE_MASK		(3<<0)

#define WLAN_CAPABILITY_DMG_TYPE_IBSS		(1<<0) /* Tx by: STA */
#define WLAN_CAPABILITY_DMG_TYPE_PBSS		(2<<0) /* Tx by: PCP */
#define WLAN_CAPABILITY_DMG_TYPE_AP		(3<<0) /* Tx by: AP */

#define WLAN_CAPABILITY_DMG_CBAP_ONLY		(1<<2)
#define WLAN_CAPABILITY_DMG_CBAP_SOURCE		(1<<3)
#define WLAN_CAPABILITY_DMG_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_DMG_ECPAC		(1<<5)

#define WLAN_CAPABILITY_DMG_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_DMG_RADIO_MEASURE	(1<<12)

static unsigned char ms_oui[3]		= { 0x00, 0x50, 0xf2 };
static unsigned char ieee80211_oui[3]	= { 0x00, 0x0f, 0xac };
static unsigned char wfa_oui[3]		= { 0x50, 0x6f, 0x9a };

struct scan_params {
	bool unknown;
	enum print_ie_type type;
	bool show_both_ie_sets;
};

#define IEEE80211_COUNTRY_EXTENSION_ID 201

union ieee80211_country_ie_triplet {
	struct {
		__u8 first_channel;
		__u8 num_channels;
		__s8 max_power;
	} __attribute__ ((packed)) chans;
	struct {
		__u8 reg_extension_id;
		__u8 reg_class;
		__u8 coverage_class;
	} __attribute__ ((packed)) ext;
} __attribute__ ((packed));

int parse_sched_scan(struct nl_msg *msg, int *argc, char ***argv)
{
	struct nl_msg *matchset = NULL, *freqs = NULL, *ssids = NULL;
	struct nl_msg *scan_plans = NULL;
	struct nlattr *match = NULL, *plan = NULL;
	enum {
		ND_TOPLEVEL,
		ND_MATCH,
		ND_FREQS,
		ND_ACTIVE,
		ND_PLANS,
	} parse_state = ND_TOPLEVEL;
	int c  = *argc;
	char *end, **v = *argv;
	int err = 0, i = 0;
	unsigned int freq, interval = 0, delay = 0, iterations = 0;
	bool have_matchset = false, have_freqs = false, have_ssids = false;
	bool have_active = false, have_passive = false, have_plans = false;
	uint32_t flags = 0;

	matchset = nlmsg_alloc();
	if (!matchset) {
		err = -ENOBUFS;
		goto out;
	}

	freqs = nlmsg_alloc();
	if (!freqs) {
		err = -ENOBUFS;
		goto out;
	}

	ssids = nlmsg_alloc();
	if (!ssids) {
		err = -ENOMEM;
		goto out;
	}

	scan_plans = nlmsg_alloc();
	if (!scan_plans) {
		err = -ENOBUFS;
		goto out;
	}

	while (c) {
		switch (parse_state) {
		case ND_TOPLEVEL:
			if (!strcmp(v[0], "interval")) {
				c--; v++;
				if (c == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				if (interval || have_plans) {
					err = -EINVAL;
					goto nla_put_failure;
				}
				interval = strtoul(v[0], &end, 10);
				if (*end || !interval) {
					err = -EINVAL;
					goto nla_put_failure;
				}
				NLA_PUT_U32(msg,
					    NL80211_ATTR_SCHED_SCAN_INTERVAL,
					    interval);
			} else if (!strcmp(v[0], "scan_plans")) {
				parse_state = ND_PLANS;
				if (have_plans || interval) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				have_plans = true;
				i = 0;
			} else if (!strcmp(v[0], "delay")) {
				c--; v++;
				if (c == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				if (delay) {
					err = -EINVAL;
					goto nla_put_failure;
				}
				delay = strtoul(v[0], &end, 10);
				if (*end) {
					err = -EINVAL;
					goto nla_put_failure;
				}
				NLA_PUT_U32(msg,
					    NL80211_ATTR_SCHED_SCAN_DELAY,
					    delay);
			} else if (!strcmp(v[0], "matches")) {
				parse_state = ND_MATCH;
				if (have_matchset) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				i = 0;
			} else if (!strcmp(v[0], "freqs")) {
				parse_state = ND_FREQS;
				if (have_freqs) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				have_freqs = true;
				i = 0;
			} else if (!strcmp(v[0], "active")) {
				parse_state = ND_ACTIVE;
				if (have_active || have_passive) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				have_active = true;
				i = 0;
			} else if (!strcmp(v[0], "passive")) {
				if (have_active || have_passive) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				have_passive = true;
			} else if (!strncmp(v[0], "randomise", 9) ||
				   !strncmp(v[0], "randomize", 9)) {
				flags |= NL80211_SCAN_FLAG_RANDOM_ADDR;
				err = parse_random_mac_addr(msg, v[0] + 9);
				if (err)
					goto nla_put_failure;
			} else if (!strncmp(v[0], "coloc", 5)) {
				flags |= NL80211_SCAN_FLAG_COLOCATED_6GHZ;
			} else if (!strncmp(v[0], "flush", 5)) {
				flags |= NL80211_SCAN_FLAG_FLUSH;
			} else {
				/* this element is not for us, so
				 * return to continue parsing.
				 */
				goto nla_put_failure;
			}
			c--; v++;

			break;
		case ND_MATCH:
			if (!strcmp(v[0], "ssid")) {
				c--; v++;
				if (c == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				/* TODO: for now we can only have an
				 * SSID in the match, so we can start
				 * the match nest here.
				 */
				match = nla_nest_start(matchset, i);
				if (!match) {
					err = -ENOBUFS;
					goto nla_put_failure;
				}

				NLA_PUT(matchset,
					NL80211_SCHED_SCAN_MATCH_ATTR_SSID,
					strlen(v[0]), v[0]);
				nla_nest_end(matchset, match);
				match = NULL;

				have_matchset = true;
				i++;
				c--; v++;
			} else {
				/* other element that cannot be part
				 * of a match indicates the end of the
				 * match. */
				/* need at least one match in the matchset */
				if (i == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				parse_state = ND_TOPLEVEL;
			}

			break;
		case ND_FREQS:
			freq = strtoul(v[0], &end, 10);
			if (*end) {
				if (i == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				parse_state = ND_TOPLEVEL;
			} else {
				NLA_PUT_U32(freqs, i, freq);
				i++;
				c--; v++;
			}
			break;
		case ND_ACTIVE:
			if (!strcmp(v[0], "ssid")) {
				c--; v++;
				if (c == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				NLA_PUT(ssids,
					NL80211_SCHED_SCAN_MATCH_ATTR_SSID,
					strlen(v[0]), v[0]);

				have_ssids = true;
				i++;
				c--; v++;
			} else {
				/* other element that cannot be part
				 * of a match indicates the end of the
				 * active set. */
				/* need at least one item in the set */
				if (i == 0) {
					err = -EINVAL;
					goto nla_put_failure;
				}

				parse_state = ND_TOPLEVEL;
			}
			break;
		case ND_PLANS:
			iterations = 0;
			interval = strtoul(v[0], &end, 10);
			if (*end) {
				char *iter;

				if (*end != ':') {
					err = -EINVAL;
					goto nla_put_failure;
				}

				iter = ++end;
				iterations = strtoul(iter, &end, 10);
				if (*end || !iterations) {
					err = -EINVAL;
					goto nla_put_failure;
				}
			}

			plan = nla_nest_start(scan_plans, i + 1);
			if (!plan) {
				err = -ENOBUFS;
				goto nla_put_failure;
			}

			NLA_PUT_U32(scan_plans,
				    NL80211_SCHED_SCAN_PLAN_INTERVAL,
				    interval);

			if (iterations)
				NLA_PUT_U32(scan_plans,
					    NL80211_SCHED_SCAN_PLAN_ITERATIONS,
					    iterations);
			else
				parse_state = ND_TOPLEVEL;

			nla_nest_end(scan_plans, plan);
			plan = NULL;
			i++;
			c--; v++;
			break;
		}
	}

	if (!have_ssids)
		NLA_PUT(ssids, 1, 0, "");
	if (!have_passive)
		nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids);
	if (have_freqs)
		nla_put_nested(msg, NL80211_ATTR_SCAN_FREQUENCIES, freqs);
	else
		flags |= NL80211_SCAN_FLAG_COLOCATED_6GHZ;
	if (have_matchset)
		nla_put_nested(msg, NL80211_ATTR_SCHED_SCAN_MATCH, matchset);
	if (have_plans)
		nla_put_nested(msg, NL80211_ATTR_SCHED_SCAN_PLANS, scan_plans);
	if (flags)
		NLA_PUT_U32(msg, NL80211_ATTR_SCAN_FLAGS, flags);

nla_put_failure:
	if (match)
		nla_nest_end(msg, match);
out:
	nlmsg_free(freqs);
	nlmsg_free(matchset);
	nlmsg_free(scan_plans);
	nlmsg_free(ssids);

	*argc = c;
	*argv = v;
	return err;
}

static int handle_scan(struct nl80211_state *state,
		       struct nl_msg *msg,
		       int argc, char **argv,
		       enum id_input id)
{
	struct nl_msg *ssids = NULL, *freqs = NULL;
	char *eptr;
	int err = -ENOBUFS;
	int i;
	enum {
		NONE,
		FREQ,
		IES,
		SSID,
		MESHID,
		DURATION,
		DONE,
	} parse = NONE;
	int freq;
	unsigned int duration = 0;
	bool passive = false, have_ssids = false, have_freqs = false;
	bool duration_mandatory = false;
	size_t ies_len = 0, meshid_len = 0;
	unsigned char *ies = NULL, *meshid = NULL, *tmpies = NULL;
	unsigned int flags = 0;

	ssids = nlmsg_alloc();
	if (!ssids)
		return -ENOMEM;

	freqs = nlmsg_alloc();
	if (!freqs) {
		nlmsg_free(ssids);
		return -ENOMEM;
	}

	for (i = 0; i < argc; i++) {
		switch (parse) {
		case NONE:
			if (strcmp(argv[i], "freq") == 0) {
				parse = FREQ;
				have_freqs = true;
				break;
			} else if (strcmp(argv[i], "ies") == 0) {
				parse = IES;
				break;
			} else if (strcmp(argv[i], "lowpri") == 0) {
				flags |= NL80211_SCAN_FLAG_LOW_PRIORITY;
				break;
			} else if (strcmp(argv[i], "flush") == 0) {
				flags |= NL80211_SCAN_FLAG_FLUSH;
				break;
			} else if (strcmp(argv[i], "ap-force") == 0) {
				flags |= NL80211_SCAN_FLAG_AP;
				break;
			} else if (strcmp(argv[i], "coloc") == 0) {
				flags |= NL80211_SCAN_FLAG_COLOCATED_6GHZ;
				break;
			} else if (strcmp(argv[i], "duration-mandatory") == 0) {
				duration_mandatory = true;
				break;
			} else if (strncmp(argv[i], "randomise", 9) == 0 ||
				   strncmp(argv[i], "randomize", 9) == 0) {
				flags |= NL80211_SCAN_FLAG_RANDOM_ADDR;
				err = parse_random_mac_addr(msg, argv[i] + 9);
				if (err)
					goto nla_put_failure;
				break;
			} else if (strcmp(argv[i], "ssid") == 0) {
				parse = SSID;
				have_ssids = true;
				break;
			} else if (strcmp(argv[i], "passive") == 0) {
				parse = DONE;
				passive = true;
				break;
			} else if (strcmp(argv[i], "meshid") == 0) {
				parse = MESHID;
				break;
			} else if (strcmp(argv[i], "duration") == 0) {
				parse = DURATION;
				break;
			}
			/* fall through - this is an error */
		case DONE:
			err = 1;
			goto nla_put_failure;
		case FREQ:
			freq = strtoul(argv[i], &eptr, 10);
			if (eptr != argv[i] + strlen(argv[i])) {
				/* failed to parse as number -- maybe a tag? */
				i--;
				parse = NONE;
				continue;
			}
			NLA_PUT_U32(freqs, i, freq);
			break;
		case IES:
			if (ies)
				free(ies);
			ies = parse_hex(argv[i], &ies_len);
			if (!ies)
				goto nla_put_failure;
			parse = NONE;
			break;
		case SSID:
			NLA_PUT(ssids, i, strlen(argv[i]), argv[i]);
			break;
		case MESHID:
			meshid_len = strlen(argv[i]);
			meshid = (unsigned char *) malloc(meshid_len + 2);
			if (!meshid)
				goto nla_put_failure;
			meshid[0] = 114; /* mesh element id */
			meshid[1] = meshid_len;
			memcpy(&meshid[2], argv[i], meshid_len);
			meshid_len += 2;
			parse = NONE;
			break;
		case DURATION:
			duration = strtoul(argv[i], &eptr, 10);
			parse = NONE;
			break;
		}
	}

	if (ies || meshid) {
		tmpies = (unsigned char *) malloc(ies_len + meshid_len);
		if (!tmpies)
			goto nla_put_failure;
		if (ies)
			memcpy(tmpies, ies, ies_len);
		if (meshid)
			memcpy(&tmpies[ies_len], meshid, meshid_len);
		if (nla_put(msg, NL80211_ATTR_IE, ies_len + meshid_len, tmpies) < 0)
			goto nla_put_failure;
	}

	if (!have_ssids)
		NLA_PUT(ssids, 1, 0, "");
	if (!passive)
		nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids);

	if (have_freqs)
		nla_put_nested(msg, NL80211_ATTR_SCAN_FREQUENCIES, freqs);
	else
		flags |=  NL80211_SCAN_FLAG_COLOCATED_6GHZ;
	if (flags)
		NLA_PUT_U32(msg, NL80211_ATTR_SCAN_FLAGS, flags);
	if (duration)
		NLA_PUT_U16(msg, NL80211_ATTR_MEASUREMENT_DURATION, duration);
	if (duration_mandatory) {
		if (duration) {
			NLA_PUT_FLAG(msg,
				     NL80211_ATTR_MEASUREMENT_DURATION_MANDATORY);
		} else {
			err = -EINVAL;
			goto nla_put_failure;
		}
	}

	err = 0;
 nla_put_failure:
	nlmsg_free(ssids);
	nlmsg_free(freqs);
	if (meshid)
		free(meshid);
	if (ies)
		free(ies);
	if (tmpies)
		free(tmpies);
	return err;
}

static void tab_on_first(bool *first)
{
	if (!*first)
		printf("\t");
	else
		*first = false;
}

struct ie_context {
	bool from_ap;
	bool is_vht_cap;
	const uint8_t *he_cap;
};

static void print_ssid(const uint8_t type, uint8_t len, const uint8_t *data,
		       const struct ie_context *ctx)
{
	printf(" ");
	print_ssid_escaped(len, data);
	printf("\n");
}

#define BSS_MEMBERSHIP_SELECTOR_VHT_PHY 126
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY 127

static void print_supprates(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	int i;

	printf(" ");

	for (i = 0; i < len; i++) {
		int r = data[i] & 0x7f;

		if (r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY && data[i] & 0x80)
			printf("VHT");
		else if (r == BSS_MEMBERSHIP_SELECTOR_HT_PHY && data[i] & 0x80)
			printf("HT");
		else
			printf("%d.%d", r/2, 5*(r&1));

		printf("%s ", data[i] & 0x80 ? "*" : "");
	}
	printf("\n");
}

static void print_rm_enabled_capabilities(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	__u64 capa = ((__u64) data[0]) |
		     ((__u64) data[1]) << 8 |
		     ((__u64) data[2]) << 16 |
		     ((__u64) data[3]) << 24 |
		     ((__u64) data[4]) << 32;

	printf("\n");
	printf("\t\tCapabilities: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
							     data[0], data[1],
							     data[2], data[3],
							     data[4]);

#define PRINT_RM_CAPA(_bit, _str) \
	do { \
		if (capa & BIT(_bit)) \
			printf("\t\t\t" _str "\n"); \
	} while (0)

	PRINT_RM_CAPA(0, "Link Measurement");
	PRINT_RM_CAPA(1, "Neighbor Report");
	PRINT_RM_CAPA(2, "Parallel Measurements");
	PRINT_RM_CAPA(3, "Repeated Measurements");
	PRINT_RM_CAPA(4, "Beacon Passive Measurement");
	PRINT_RM_CAPA(5, "Beacon Active Measurement");
	PRINT_RM_CAPA(6, "Beacon Table Measurement");
	PRINT_RM_CAPA(7, "Beacon Measurement Reporting Conditions");
	PRINT_RM_CAPA(8, "Frame Measurement");
	PRINT_RM_CAPA(9, "Channel Load");
	PRINT_RM_CAPA(10, "Noise Histogram Measurement");
	PRINT_RM_CAPA(11, "Statistics Measurement");
	PRINT_RM_CAPA(12, "LCI Measurement");
	PRINT_RM_CAPA(13, "LCI Azimuth");
	PRINT_RM_CAPA(14, "Transmit Stream/Category Measurement");
	PRINT_RM_CAPA(15, "Triggered Transmit Stream/Category");
	PRINT_RM_CAPA(16, "AP Channel Report");
	PRINT_RM_CAPA(17, "RM MIB Capability");

	PRINT_RM_CAPA(27, "Measurement Pilot Transmission Information");
	PRINT_RM_CAPA(28, "Neighbor Report TSF Offset");
	PRINT_RM_CAPA(29, "RCPI Measurement");
	PRINT_RM_CAPA(30, "RSNI Measurement");
	PRINT_RM_CAPA(31, "BSS Average Access Delay");
	PRINT_RM_CAPA(32, "BSS Available Admission");
	PRINT_RM_CAPA(33, "Antenna");
	PRINT_RM_CAPA(34, "FTM Range Report");
	PRINT_RM_CAPA(35, "Civic Location Measurement");

	printf("\t\tNonoperating Channel Max Measurement Duration: %i\n", data[3] >> 5);
	printf("\t\tMeasurement Pilot Capability: %i\n", data[4] & 7);
}

static void print_ds(const uint8_t type, uint8_t len, const uint8_t *data,
		     const struct ie_context *ctx)
{
	printf(" channel %d\n", data[0]);
}

static const char *country_env_str(char environment)
{
	switch (environment) {
	case 'I':
		return "Indoor only";
	case 'O':
		return "Outdoor only";
	case ' ':
		return "Indoor/Outdoor";
	default:
		return "bogus";
	}
}

static void print_country(const uint8_t type, uint8_t len, const uint8_t *data,
			  const struct ie_context *ctx)
{
	printf(" %.*s", 2, data);

	printf("\tEnvironment: %s\n", country_env_str(data[2]));

	data += 3;
	len -= 3;

	if (len < 3) {
		printf("\t\tNo country IE triplets present\n");
		return;
	}

	while (len >= 3) {
		int end_channel;
		union ieee80211_country_ie_triplet *triplet = (void *) data;

		if (triplet->ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
			printf("\t\tExtension ID: %d Regulatory Class: %d Coverage class: %d (up to %dm)\n",
			       triplet->ext.reg_extension_id,
			       triplet->ext.reg_class,
			       triplet->ext.coverage_class,
			       triplet->ext.coverage_class * 450);

			data += 3;
			len -= 3;
			continue;
		}

		/* 2 GHz */
		if (triplet->chans.first_channel <= 14)
			end_channel = triplet->chans.first_channel + (triplet->chans.num_channels - 1);
		else
			end_channel =  triplet->chans.first_channel + (4 * (triplet->chans.num_channels - 1));

		printf("\t\tChannels [%d - %d] @ %d dBm\n", triplet->chans.first_channel, end_channel, triplet->chans.max_power);

		data += 3;
		len -= 3;
	}

	return;
}

static void print_powerconstraint(const uint8_t type, uint8_t len,
				  const uint8_t *data,
				  const struct ie_context *ctx)
{
	printf(" %d dB\n", data[0]);
}

static void print_tpcreport(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	printf(" TX power: %d dBm\n", data[0]);
	/* printf(" Link Margin (%d dB) is reserved in Beacons\n", data[1]); */
}

static void print_erp(const uint8_t type, uint8_t len, const uint8_t *data,
		      const struct ie_context *ctx)
{
	if (data[0] == 0x00)
		printf(" <no flags>");
	if (data[0] & 0x01)
		printf(" NonERP_Present");
	if (data[0] & 0x02)
		printf(" Use_Protection");
	if (data[0] & 0x04)
		printf(" Barker_Preamble_Mode");
	printf("\n");
}

static void print_ap_channel_report(const uint8_t type, uint8_t len, const uint8_t *data,
				    const struct ie_context *ctx)
{
	uint8_t oper_class = data[0];
	int i;

	printf("\n");
	printf("\t\t * operating class: %d\n", oper_class);
	printf("\t\t * channel(s):");
	for (i = 1; i < len; ++i) {
		printf(" %d", data[i]);
	}
	printf("\n");
}

static void print_cipher(const uint8_t *data)
{
	if (memcmp(data, ms_oui, 3) == 0) {
		switch (data[3]) {
		case 0:
			printf("Use group cipher suite");
			break;
		case 1:
			printf("WEP-40");
			break;
		case 2:
			printf("TKIP");
			break;
		case 4:
			printf("CCMP");
			break;
		case 5:
			printf("WEP-104");
			break;
		default:
			printf("%.02x-%.02x-%.02x:%d",
				data[0], data[1] ,data[2], data[3]);
			break;
		}
	} else if (memcmp(data, ieee80211_oui, 3) == 0) {
		switch (data[3]) {
		case 0:
			printf("Use group cipher suite");
			break;
		case 1:
			printf("WEP-40");
			break;
		case 2:
			printf("TKIP");
			break;
		case 4:
			printf("CCMP");
			break;
		case 5:
			printf("WEP-104");
			break;
		case 6:
			printf("AES-128-CMAC");
			break;
		case 7:
			printf("NO-GROUP");
			break;
		case 8:
			printf("GCMP");
			break;
		case 9:
			printf("GCMP-256");
			break;
		case 11:
			printf("BIP-GMAC-128");
			break;
		case 12:
			printf("BIP-GMAC-256");
			break;
		case 13:
			printf("BIP-CMAC-256");
			break;
		default:
			printf("%.02x-%.02x-%.02x:%d",
				data[0], data[1] ,data[2], data[3]);
			break;
		}
	} else
		printf("%.02x-%.02x-%.02x:%d",
			data[0], data[1] ,data[2], data[3]);
}

static void print_auth(const uint8_t *data)
{
	if (memcmp(data, ms_oui, 3) == 0) {
		switch (data[3]) {
		case 1:
			printf("IEEE 802.1X");
			break;
		case 2:
			printf("PSK");
			break;
		default:
			printf("%.02x-%.02x-%.02x:%d",
				data[0], data[1] ,data[2], data[3]);
			break;
		}
	} else if (memcmp(data, ieee80211_oui, 3) == 0) {
		switch (data[3]) {
		case 1:
			printf("IEEE 802.1X");
			break;
		case 2:
			printf("PSK");
			break;
		case 3:
			printf("FT/IEEE 802.1X");
			break;
		case 4:
			printf("FT/PSK");
			break;
		case 5:
			printf("IEEE 802.1X/SHA-256");
			break;
		case 6:
			printf("PSK/SHA-256");
			break;
		case 7:
			printf("TDLS/TPK");
			break;
		case 8:
			printf("SAE");
			break;
		case 9:
			printf("FT/SAE");
			break;
		case 11:
			printf("IEEE 802.1X/SUITE-B");
			break;
		case 12:
			printf("IEEE 802.1X/SUITE-B-192");
			break;
		case 13:
			printf("FT/IEEE 802.1X/SHA-384");
			break;
		case 14:
			printf("FILS/SHA-256");
			break;
		case 15:
			printf("FILS/SHA-384");
			break;
		case 16:
			printf("FT/FILS/SHA-256");
			break;
		case 17:
			printf("FT/FILS/SHA-384");
			break;
		case 18:
			printf("OWE");
			break;
		case 19:
			printf("FT/PSK/SHA-384");
			break;
		case 20:
			printf("PSK/SHA-384");
			break;
		case 24:
			printf("SAE-EXT-KEY");
			break;
		case 25:
			printf("FT/SAE-EXT-KEY");
			break;
		default:
			printf("%.02x-%.02x-%.02x:%d",
				data[0], data[1] ,data[2], data[3]);
			break;
		}
	} else if (memcmp(data, wfa_oui, 3) == 0) {
		switch (data[3]) {
		case 1:
			printf("OSEN");
			break;
		case 2:
			printf("DPP");
			break;
		default:
			printf("%.02x-%.02x-%.02x:%d",
				data[0], data[1] ,data[2], data[3]);
			break;
		}
	} else
		printf("%.02x-%.02x-%.02x:%d",
			data[0], data[1] ,data[2], data[3]);
}

static void _print_rsn_ie(const char *defcipher, const char *defauth,
			  uint8_t len, const uint8_t *data, int is_osen)
{
	bool first = true;
	__u16 count, capa;
	int i;

	if (!is_osen) {
		__u16 version;
		version = data[0] + (data[1] << 8);
		tab_on_first(&first);
		printf("\t * Version: %d\n", version);

		data += 2;
		len -= 2;
	}

	if (len < 4) {
		tab_on_first(&first);
		printf("\t * Group cipher: %s\n", defcipher);
		printf("\t * Pairwise ciphers: %s\n", defcipher);
		return;
	}

	tab_on_first(&first);
	printf("\t * Group cipher: ");
	print_cipher(data);
	printf("\n");

	data += 4;
	len -= 4;

	if (len < 2) {
		tab_on_first(&first);
		printf("\t * Pairwise ciphers: %s\n", defcipher);
		return;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		goto invalid;

	tab_on_first(&first);
	printf("\t * Pairwise ciphers:");
	for (i = 0; i < count; i++) {
		printf(" ");
		print_cipher(data + 2 + (i * 4));
	}
	printf("\n");

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len < 2) {
		tab_on_first(&first);
		printf("\t * Authentication suites: %s\n", defauth);
		return;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		goto invalid;

	tab_on_first(&first);
	printf("\t * Authentication suites:");
	for (i = 0; i < count; i++) {
		printf(" ");
		print_auth(data + 2 + (i * 4));
	}
	printf("\n");

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len >= 2) {
		capa = data[0] | (data[1] << 8);
		tab_on_first(&first);
		printf("\t * Capabilities:");
		if (capa & 0x0001)
			printf(" PreAuth");
		if (capa & 0x0002)
			printf(" NoPairwise");
		switch ((capa & 0x000c) >> 2) {
		case 0:
			printf(" 1-PTKSA-RC");
			break;
		case 1:
			printf(" 2-PTKSA-RC");
			break;
		case 2:
			printf(" 4-PTKSA-RC");
			break;
		case 3:
			printf(" 16-PTKSA-RC");
			break;
		}
		switch ((capa & 0x0030) >> 4) {
		case 0:
			printf(" 1-GTKSA-RC");
			break;
		case 1:
			printf(" 2-GTKSA-RC");
			break;
		case 2:
			printf(" 4-GTKSA-RC");
			break;
		case 3:
			printf(" 16-GTKSA-RC");
			break;
		}
		if (capa & 0x0040)
			printf(" MFP-required");
		if (capa & 0x0080)
			printf(" MFP-capable");
		if (capa & 0x0200)
			printf(" Peerkey-enabled");
		if (capa & 0x0400)
			printf(" SPP-AMSDU-capable");
		if (capa & 0x0800)
			printf(" SPP-AMSDU-required");
		if (capa & 0x2000)
			printf(" Extended-Key-ID");
		printf(" (0x%.4x)\n", capa);
		data += 2;
		len -= 2;
	}

	if (len >= 2) {
		int pmkid_count = data[0] | (data[1] << 8);

		if (len >= 2 + 16 * pmkid_count) {
			tab_on_first(&first);
			printf("\t * %d PMKIDs\n", pmkid_count);
			/* not printing PMKID values */
			data += 2 + 16 * pmkid_count;
			len -= 2 + 16 * pmkid_count;
		} else
			goto invalid;
	}

	if (len >= 4) {
		tab_on_first(&first);
		printf("\t * Group mgmt cipher suite: ");
		print_cipher(data);
		printf("\n");
		data += 4;
		len -= 4;
	}

 invalid:
	if (len != 0) {
		printf("\t\t * bogus tail data (%d):", len);
		while (len) {
			printf(" %.2x", *data);
			data++;
			len--;
		}
		printf("\n");
	}
}

static void print_rsn_ie(const char *defcipher, const char *defauth,
			 uint8_t len, const uint8_t *data)
{
	_print_rsn_ie(defcipher, defauth, len, data, 0);
}

static void print_osen_ie(const char *defcipher, const char *defauth,
			  uint8_t len, const uint8_t *data)
{
	printf("\n\t");
	_print_rsn_ie(defcipher, defauth, len, data, 1);
}

static void print_rsn(const uint8_t type, uint8_t len, const uint8_t *data,
		      const struct ie_context *ctx)
{
	print_rsn_ie("CCMP", "IEEE 802.1X", len, data);
}

static void print_ht_capa(const uint8_t type, uint8_t len, const uint8_t *data,
			  const struct ie_context *ctx)
{
	printf("\n");
	print_ht_capability(data[0] | (data[1] << 8));
	print_ampdu_length(data[2] & 3);
	print_ampdu_spacing((data[2] >> 2) & 7);
	print_ht_mcs(data + 3);
}

static const char* ntype_11u(uint8_t t)
{
	switch (t) {
	case 0: return "Private";
	case 1: return "Private with Guest";
	case 2: return "Chargeable Public";
	case 3: return "Free Public";
	case 4: return "Personal Device";
	case 5: return "Emergency Services Only";
	case 14: return "Test or Experimental";
	case 15: return "Wildcard";
	default: return "Reserved";
	}
}

static const char* vgroup_11u(uint8_t t)
{
	switch (t) {
	case 0: return "Unspecified";
	case 1: return "Assembly";
	case 2: return "Business";
	case 3: return "Educational";
	case 4: return "Factory and Industrial";
	case 5: return "Institutional";
	case 6: return "Mercantile";
	case 7: return "Residential";
	case 8: return "Storage";
	case 9: return "Utility and Miscellaneous";
	case 10: return "Vehicular";
	case 11: return "Outdoor";
	default: return "Reserved";
	}
}

static void print_interworking(const uint8_t type, uint8_t len,
			       const uint8_t *data,
			       const struct ie_context *ctx)
{
	/* See Section 7.3.2.92 in the 802.11u spec. */
	printf("\n");
	if (len >= 1) {
		uint8_t ano = data[0];
		printf("\t\tNetwork Options: 0x%hx\n", (unsigned short)(ano));
		printf("\t\t\tNetwork Type: %i (%s)\n",
		       (int)(ano & 0xf), ntype_11u(ano & 0xf));
		if (ano & (1<<4))
			printf("\t\t\tInternet\n");
		if (ano & (1<<5))
			printf("\t\t\tASRA\n");
		if (ano & (1<<6))
			printf("\t\t\tESR\n");
		if (ano & (1<<7))
			printf("\t\t\tUESA\n");
	}
	if ((len == 3) || (len == 9)) {
		printf("\t\tVenue Group: %i (%s)\n",
		       (int)(data[1]), vgroup_11u(data[1]));
		printf("\t\tVenue Type: %i\n", (int)(data[2]));
	}
	if (len == 9)
		printf("\t\tHESSID: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		       data[3], data[4], data[5], data[6], data[7], data[8]);
	else if (len == 7)
		printf("\t\tHESSID: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
		       data[1], data[2], data[3], data[4], data[5], data[6]);
}

static void print_11u_advert(const uint8_t type, uint8_t len,
			     const uint8_t *data,
			     const struct ie_context *ctx)
{
	/* See Section 7.3.2.93 in the 802.11u spec. */
	/* TODO: This code below does not decode private protocol IDs */
	int idx = 0;
	printf("\n");
	while (idx < (len - 1)) {
		uint8_t qri = data[idx];
		uint8_t proto_id = data[idx + 1];
		printf("\t\tQuery Response Info: 0x%hx\n", (unsigned short)(qri));
		printf("\t\t\tQuery Response Length Limit: %i\n",
		       (qri & 0x7f));
		if (qri & (1<<7))
			printf("\t\t\tPAME-BI\n");
		switch(proto_id) {
		case 0:
			printf("\t\t\tANQP\n"); break;
		case 1:
			printf("\t\t\tMIH Information Service\n"); break;
		case 2:
			printf("\t\t\tMIH Command and Event Services Capability Discovery\n"); break;
		case 3:
			printf("\t\t\tEmergency Alert System (EAS)\n"); break;
		case 221:
			printf("\t\t\tVendor Specific\n"); break;
		default:
			printf("\t\t\tReserved: %i\n", proto_id); break;
		}
		idx += 2;
	}
}

static void print_11u_rcon(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	/* See Section 7.3.2.96 in the 802.11u spec. */
	int idx = 0;
	int ln0 = data[1] & 0xf;
	int ln1 = ((data[1] & 0xf0) >> 4);
	int ln2 = 0;
	printf("\n");

	if (ln1)
		ln2 = len - 2 - ln0 - ln1;

	printf("\t\tANQP OIs: %i\n", data[0]);

	if (ln0 > 0) {
		printf("\t\tOI 1: ");
		if (2 + ln0 > len) {
			printf("Invalid IE length.\n");
		} else {
			for (idx = 0; idx < ln0; idx++) {
				printf("%02hhx", data[2 + idx]);
			}
			printf("\n");
		}
	}

	if (ln1 > 0) {
		printf("\t\tOI 2: ");
		if (2 + ln0 + ln1 > len) {
			printf("Invalid IE length.\n");
		} else {
			for (idx = 0; idx < ln1; idx++) {
				printf("%02hhx", data[2 + ln0 + idx]);
			}
			printf("\n");
		}
	}

	if (ln2 > 0) {
		printf("\t\tOI 3: ");
		if (2 + ln0 + ln1 + ln2 > len) {
			printf("Invalid IE length.\n");
		} else {
			for (idx = 0; idx < ln2; idx++) {
				printf("%02hhx", data[2 + ln0 + ln1 + idx]);
			}
			printf("\n");
		}
	}
}

static void print_tx_power_envelope(const uint8_t type, uint8_t len,
				    const uint8_t *data,
				    const struct ie_context *ctx)
{
	const uint8_t local_max_tx_power_count = data[0] & 7;
	const uint8_t local_max_tx_power_unit_interp = (data[0] >> 3) & 7;
	int i;
	static const char *power_names[] = {
		"Local Maximum Transmit Power For 20 MHz",
		"Local Maximum Transmit Power For 40 MHz",
		"Local Maximum Transmit Power For 80 MHz",
		"Local Maximum Transmit Power For 160/80+80 MHz",
	};

	printf("\n");

	if (local_max_tx_power_count + 2 != len)
		return;
	if (local_max_tx_power_unit_interp != 0)
		return;
	for (i = 0; i < local_max_tx_power_count + 1; ++i) {
		int8_t power_val = ((int8_t)data[1 + i]) >> 1;
		int8_t point5 = data[1 + i] & 1;
		if (point5)
			printf("\t\t * %s: %i.5 dBm\n", power_names[i], power_val);
		else
			printf("\t\t * %s: %i dBm\n", power_names[i], power_val);
	}
}

static const char *ht_secondary_offset[4] = {
	"no secondary",
	"above",
	"[reserved!]",
	"below",
};

static void print_ht_op(const uint8_t type, uint8_t len, const uint8_t *data,
			const struct ie_context *ctx)
{
	static const char *protection[4] = {
		"no",
		"nonmember",
		"20 MHz",
		"non-HT mixed",
	};
	static const char *sta_chan_width[2] = {
		"20 MHz",
		"any",
	};

	printf("\n");
	printf("\t\t * primary channel: %d\n", data[0]);
	printf("\t\t * secondary channel offset: %s\n",
		ht_secondary_offset[data[1] & 0x3]);
	printf("\t\t * STA channel width: %s\n", sta_chan_width[(data[1] & 0x4)>>2]);
	printf("\t\t * RIFS: %d\n", (data[1] & 0x8)>>3);
	printf("\t\t * HT protection: %s\n", protection[data[2] & 0x3]);
	printf("\t\t * non-GF present: %d\n", (data[2] & 0x4) >> 2);
	printf("\t\t * OBSS non-GF present: %d\n", (data[2] & 0x10) >> 4);
	printf("\t\t * dual beacon: %d\n", (data[4] & 0x40) >> 6);
	printf("\t\t * dual CTS protection: %d\n", (data[4] & 0x80) >> 7);
	printf("\t\t * STBC beacon: %d\n", data[5] & 0x1);
	printf("\t\t * L-SIG TXOP Prot: %d\n", (data[5] & 0x2) >> 1);
	printf("\t\t * PCO active: %d\n", (data[5] & 0x4) >> 2);
	printf("\t\t * PCO phase: %d\n", (data[5] & 0x8) >> 3);
}

static void print_capabilities(const uint8_t type, uint8_t len,
			       const uint8_t *data,
			       const struct ie_context *ctx)
{
	int i, base, bit, si_duration = 0, max_amsdu = 0;
	bool s_psmp_support = false;

	for (i = 0; i < len; i++) {
		base = i * 8;

		for (bit = 0; bit < 8; bit++) {
			if (!(data[i] & (1 << bit)))
				continue;

			printf("\n\t\t *");

#define CAPA(bit, name)		case bit: printf(" " name); break

/* if the capability 'cap' exists add 'val' to 'sum'
 * otherwise print 'Reserved' */
#define ADD_BIT_VAL(bit, cap, sum, val)	case (bit): do {	\
	if (!(cap)) {						\
		printf(" Reserved");				\
		break;						\
	}							\
	sum += val;						\
	break;							\
} while (0)

			switch (bit + base) {
			CAPA(0, "HT Information Exchange Supported");
			CAPA(1, "reserved (On-demand Beacon)");
			CAPA(2, "Extended Channel Switching");
			CAPA(3, "reserved (Wave Indication)");
			CAPA(4, "PSMP Capability");
			CAPA(5, "reserved (Service Interval Granularity)");

			case 6:
				s_psmp_support = true;
				printf(" S-PSMP Capability");
				break;

			CAPA(7, "Event");
			CAPA(8, "Diagnostics");
			CAPA(9, "Multicast Diagnostics");
			CAPA(10, "Location Tracking");
			CAPA(11, "FMS");
			CAPA(12, "Proxy ARP Service");
			CAPA(13, "Collocated Interference Reporting");
			CAPA(14, "Civic Location");
			CAPA(15, "Geospatial Location");
			CAPA(16, "TFS");
			CAPA(17, "WNM-Sleep Mode");
			CAPA(18, "TIM Broadcast");
			CAPA(19, "BSS Transition");
			CAPA(20, "QoS Traffic Capability");
			CAPA(21, "AC Station Count");
			CAPA(22, "Multiple BSSID");
			CAPA(23, "Timing Measurement");
			CAPA(24, "Channel Usage");
			CAPA(25, "SSID List");
			CAPA(26, "DMS");
			CAPA(27, "UTC TSF Offset");
			CAPA(28, "TDLS Peer U-APSD Buffer STA Support");
			CAPA(29, "TDLS Peer PSM Support");
			CAPA(30, "TDLS channel switching");
			CAPA(31, "Interworking");
			CAPA(32, "QoS Map");
			CAPA(33, "EBR");
			CAPA(34, "SSPN Interface");
			CAPA(35, "Reserved");
			CAPA(36, "MSGCF Capability");
			CAPA(37, "TDLS Support");
			CAPA(38, "TDLS Prohibited");
			CAPA(39, "TDLS Channel Switching Prohibited");
			CAPA(40, "Reject Unadmitted Frame");

			ADD_BIT_VAL(41, s_psmp_support, si_duration, 1);
			ADD_BIT_VAL(42, s_psmp_support, si_duration, 2);
			ADD_BIT_VAL(43, s_psmp_support, si_duration, 4);

			CAPA(44, "Identifier Location");
			CAPA(45, "U-APSD Coexistence");
			CAPA(46, "WNM-Notification");
			CAPA(47, "Reserved");
			CAPA(48, "UTF-8 SSID");
			CAPA(49, "QMFActivated");
			CAPA(50, "QMFReconfigurationActivated");
			CAPA(51, "Robust AV Streaming");
			CAPA(52, "Advanced GCR");
			CAPA(53, "Mesh GCR");
			CAPA(54, "SCS");
			CAPA(55, "QLoad Report");
			CAPA(56, "Alternate EDCA");
			CAPA(57, "Unprotected TXOP Negotiation");
			CAPA(58, "Protected TXOP egotiation");
			CAPA(59, "Reserved");
			CAPA(60, "Protected QLoad Report");
			CAPA(61, "TDLS Wider Bandwidth");
			CAPA(62, "Operating Mode Notification");

			ADD_BIT_VAL(63, ctx->is_vht_cap, max_amsdu, 1);
			ADD_BIT_VAL(64, ctx->is_vht_cap, max_amsdu, 2);

			CAPA(65, "Channel Schedule Management");
			CAPA(66, "Geodatabase Inband Enabling Signal");
			CAPA(67, "Network Channel Control");
			CAPA(68, "White Space Map");
			CAPA(69, "Channel Availability Query");
			CAPA(70, "FTM Responder");
			CAPA(71, "FTM Initiator");
			CAPA(72, "Reserved");
			CAPA(73, "Extended Spectrum Management Capable");
			CAPA(74, "Reserved");
			CAPA(77, "TWT Requester Support");
			CAPA(78, "TWT Responder Support");
			CAPA(79, "OBSS Narrow Bandwith RU in UL OFDMA Tolerance Support");

			default:
				printf(" %d", bit + base);
				break;
			}
#undef ADD_BIT_VAL
#undef CAPA
		}
	}

	if (s_psmp_support)
		printf("\n\t\t * Service Interval Granularity is %d ms",
		       (si_duration + 1) * 5);

	if (ctx->is_vht_cap) {
		printf("\n\t\t * Max Number Of MSDUs In A-MSDU is ");
		switch (max_amsdu) {
		case 0:
			printf("unlimited");
			break;
		case 1:
			printf("32");
			break;
		case 2:
			printf("16");
			break;
		case 3:
			printf("8");
			break;
		default:
			break;
		}
	}

	printf("\n");
}

static void print_tim(const uint8_t type, uint8_t len, const uint8_t *data,
		      const struct ie_context *ctx)
{
	printf(" DTIM Count %u DTIM Period %u Bitmap Control 0x%x "
	       "Bitmap[0] 0x%x",
	       data[0], data[1], data[2], data[3]);
	if (len - 4)
		printf(" (+ %u octet%s)", len - 4, len - 4 == 1 ? "" : "s");
	printf("\n");
}

static void print_ibssatim(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	printf(" %d TUs\n", (data[1] << 8) + data[0]);
}

static void print_vht_capa(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	printf("\n");
	print_vht_info((__u32) data[0] | ((__u32)data[1] << 8) |
		       ((__u32)data[2] << 16) | ((__u32)data[3] << 24),
		       data + 4);
}

static void print_vht_oper(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	const char *chandwidths[] = {
		[0] = "20 or 40 MHz",
		[1] = "80 MHz",
		[3] = "80+80 MHz",
		[2] = "160 MHz",
	};

	printf("\n");
	printf("\t\t * channel width: %d (%s)\n", data[0],
		data[0] < ARRAY_SIZE(chandwidths) ? chandwidths[data[0]] : "unknown");
	printf("\t\t * center freq segment 1: %d\n", data[1]);
	printf("\t\t * center freq segment 2: %d\n", data[2]);
	printf("\t\t * VHT basic MCS set: 0x%.2x%.2x\n", data[4], data[3]);
}

static void print_supp_op_classes(const uint8_t type, uint8_t len,
				  const uint8_t *data,
				  const struct ie_context *ctx)
{
	uint8_t *p = (uint8_t*) data;
	const uint8_t *next_data = p + len;
	int zero_delimiter = 0;
	int one_hundred_thirty_delimiter = 0;

	printf("\n");
	printf("\t\t * current operating class: %d\n", *p);
	while (++p < next_data) {
		if (*p == 130) {
			one_hundred_thirty_delimiter = 1;
			break;
		}
		if (*p == 0) {
			zero_delimiter = 0;
			break;
		}
		printf("\t\t * operating class: %d\n", *p);
	}
	if (one_hundred_thirty_delimiter)
		while (++p < next_data) {
			printf("\t\t * current operating class extension: %d\n", *p);
		}
	if (zero_delimiter)
		while (++p < next_data - 1) {
			printf("\t\t * operating class tuple: %d %d\n", p[0], p[1]);
			if (*p == 0)
				break;
		}
}

static void print_measurement_pilot_tx(const uint8_t type, uint8_t len,
				       const uint8_t *data,
				       const struct ie_context *ctx)
{
	uint8_t *p, len_remaining;

	printf("\n");
	printf("\t\t * interval: %d TUs\n", data[0]);

	if (len <= 1)
		return;

	p = (uint8_t *) data + 1;
	len_remaining = len - 1;

	while (len_remaining >=5) {
		uint8_t subelement_id = *p, len, *end;

		p++;
		len = *p;
		p++;
		end = p + len;

		len_remaining -= 2;

		/* 802.11-2016 only allows vendor specific elements */
		if (subelement_id != 221) {
			printf("\t\t * <Invalid subelement ID %d>\n", subelement_id);
			return;
		}

		if (len < 3 || len > len_remaining) {
			printf(" <Parse error, element too short>\n");
			return;
		}

		printf("\t\t * vendor specific: OUI %.2x:%.2x:%.2x, data:",
			p[0], p[1], p[2]);
		/* add only two here and use ++p in while loop */
		p += 2;

		while (++p < end)
			printf(" %.2x", *p);
		printf("\n");

		len_remaining -= len;
	}
}

static void print_obss_scan_params(const uint8_t type, uint8_t len,
				   const uint8_t *data,
				   const struct ie_context *ctx)
{
	printf("\n");
	printf("\t\t * passive dwell: %d TUs\n", (data[1] << 8) | data[0]);
	printf("\t\t * active dwell: %d TUs\n", (data[3] << 8) | data[2]);
	printf("\t\t * channel width trigger scan interval: %d s\n", (data[5] << 8) | data[4]);
	printf("\t\t * scan passive total per channel: %d TUs\n", (data[7] << 8) | data[6]);
	printf("\t\t * scan active total per channel: %d TUs\n", (data[9] << 8) | data[8]);
	printf("\t\t * BSS width channel transition delay factor: %d\n", (data[11] << 8) | data[10]);
	printf("\t\t * OBSS Scan Activity Threshold: %d.%02d %%\n",
		((data[13] << 8) | data[12]) / 100, ((data[13] << 8) | data[12]) % 100);
}

static void print_secchan_offs(const uint8_t type, uint8_t len,
			       const uint8_t *data,
			       const struct ie_context *ctx)
{
	if (data[0] < ARRAY_SIZE(ht_secondary_offset))
		printf(" %s (%d)\n", ht_secondary_offset[data[0]], data[0]);
	else
		printf(" %d\n", data[0]);
}

static void print_bss_load(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	printf("\n");
	printf("\t\t * station count: %d\n", (data[1] << 8) | data[0]);
	printf("\t\t * channel utilisation: %d/255\n", data[2]);
	printf("\t\t * available admission capacity: %d [*32us]\n", (data[4] << 8) | data[3]);
}

static void print_mesh_conf(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	printf("\n");
	printf("\t\t * Active Path Selection Protocol ID: %d\n", data[0]);
	printf("\t\t * Active Path Selection Metric ID: %d\n", data[1]);
	printf("\t\t * Congestion Control Mode ID: %d\n", data[2]);
	printf("\t\t * Synchronization Method ID: %d\n", data[3]);
	printf("\t\t * Authentication Protocol ID: %d\n", data[4]);
	printf("\t\t * Mesh Formation Info:\n");
	printf("\t\t\t Number of Peerings: %d\n", (data[5] & 0x7E) >> 1);
	if (data[5] & 0x01)
		printf("\t\t\t Connected to Mesh Gate\n");
	if (data[5] & 0x80)
		printf("\t\t\t Connected to AS\n");
	printf("\t\t * Mesh Capability\n");
	if (data[6] & 0x01)
		printf("\t\t\t Accepting Additional Mesh Peerings\n");
	if (data[6] & 0x02)
		printf("\t\t\t MCCA Supported\n");
	if (data[6] & 0x04)
		printf("\t\t\t MCCA Enabled\n");
	if (data[6] & 0x08)
		printf("\t\t\t Forwarding\n");
	if (data[6] & 0x10)
		printf("\t\t\t MBCA Supported\n");
	if (data[6] & 0x20)
		printf("\t\t\t TBTT Adjusting\n");
	if (data[6] & 0x40)
		printf("\t\t\t Mesh Power Save Level\n");
}

static void print_s1g_capa(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	printf("\n");
	print_s1g_capability(data);
}

static void print_short_beacon_int(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	printf(" %d\n", (data[1] << 8) | data[0]);
}

static void print_s1g_oper(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	int oper_ch_width, prim_ch_width;
	int prim_ch_width_subfield = data[0] & 0x1;

	prim_ch_width = 2;

	/* B1-B4 BSS channel width subfield */
	switch ((data[0] >> 1) & 0xf) {
	case 0:
		oper_ch_width = 1;
		prim_ch_width = 1;
		if (!prim_ch_width_subfield) {
			oper_ch_width = -1;
			prim_ch_width = -1;
		}
	break;
	case 1:
		oper_ch_width = 2;
		if (prim_ch_width_subfield)
			prim_ch_width = 1;
		break;
	case 3:
		oper_ch_width = 4;
		if (prim_ch_width_subfield)
			prim_ch_width = 1;
		break;
	case 7:
		oper_ch_width = 8;
		if (prim_ch_width_subfield)
			prim_ch_width = 1;
		break;
	case 15:
		oper_ch_width = 16;
		if (prim_ch_width_subfield)
			prim_ch_width = 1;
		break;
	default:
		oper_ch_width = -1;
		prim_ch_width = -1;
		break;
	}

	printf("\n");
	printf("\t\tChannel width:\n");
	if (oper_ch_width == -1 || prim_ch_width == -1) {
		printf("\t\t\tBSS primary channel width: invalid\n");
		printf("\t\t\tBSS operating channel width: invalid\n");
	} else {
		printf("\t\t\tBSS primary channel width: %d MHz\n", prim_ch_width);
		printf("\t\t\tBSS operating channel width: %d MHz\n", oper_ch_width);
	}
	if (data[0] & BIT(5))
		printf("\t\t\t1 MHz primary channel located at the lower side of 2 MHz\n");
	else
		printf("\t\t\t1 MHz primary channel located at the upper side of 2 MHz\n");

	if (data[0] & BIT(7))
		printf("\t\t\tMCS 10 not recommended\n");

	printf("\t\t* operating class: %d\n", data[1]);
	printf("\t\t* primary channel number: %d\n", data[2]);

	printf("\t\t* channel index: %d\n", data[3]);

	printf("\t\tMax S1G MCS Map:\n");
	printf("\t\t\tFor 1 SS: %s\n", s1g_ss_max_support((data[4] >> 2) & 0x3));
	printf("\t\t\tFor 2 SS: %s\n", s1g_ss_max_support((data[4] >> 6) & 0x3));
	printf("\t\t\tFor 3 SS: %s\n", s1g_ss_max_support((data[5] >> 2) & 0x3));
	printf("\t\t\tFor 4 SS: %s\n", s1g_ss_max_support((data[5] >> 6) & 0x3));

	printf("\t\tMin S1G MCS Map:\n");
	printf("\t\t\tFor 1 SS: %s\n", s1g_ss_min_support(data[4] & 0x3));
	printf("\t\t\tFor 2 SS: %s\n", s1g_ss_min_support((data[4] >> 4) & 0x3));
	printf("\t\t\tFor 3 SS: %s\n", s1g_ss_min_support(data[5] & 0x3));
	printf("\t\t\tFor 4 SS: %s\n", s1g_ss_min_support((data[5] >> 4) & 0x3));
}

struct ie_print {
	const char *name;
	void (*print)(const uint8_t type, uint8_t len, const uint8_t *data,
		      const struct ie_context *ctx);
	uint8_t minlen, maxlen;
	uint8_t flags;
};

static void print_ie(const struct ie_print *p, const uint8_t type, uint8_t len,
		     const uint8_t *data,
		     const struct ie_context *ctx)
{
	int i;

	if (!p->print)
		return;

	printf("\t%s:", p->name);
	if (len < p->minlen || len > p->maxlen) {
		if (len > 1) {
			printf(" <invalid: %d bytes:", len);
			for (i = 0; i < len; i++)
				printf(" %.02x", data[i]);
			printf(">\n");
		} else if (len)
			printf(" <invalid: 1 byte: %.02x>\n", data[0]);
		else
			printf(" <invalid: no data>\n");
		return;
	}

	p->print(type, len, data, ctx);
}

#define PRINT_IGN {		\
	.name = "IGNORE",	\
	.print = NULL,		\
	.minlen = 0,		\
	.maxlen = 255,		\
}

static const struct ie_print ieprinters[] = {
	[EID_SSID] = { "SSID", print_ssid, 0, 32,
		       BIT(PRINT_SCAN) | BIT(PRINT_LINK) | BIT(PRINT_LINK_MLO_MLD), },
	[EID_SUPP_RATES] = { "Supported rates", print_supprates, 0, 255, BIT(PRINT_SCAN), },
	[EID_DS_PARAMS] = { "DS Parameter set", print_ds, 1, 1, BIT(PRINT_SCAN), },
	[EID_TIM] = { "TIM", print_tim, 4, 255, BIT(PRINT_SCAN), },
	[EID_IBSS_TIM_PARAMS] = { "IBSS ATIM window", print_ibssatim, 2, 2, BIT(PRINT_SCAN), },
	[EID_COUNTRY] = { "Country", print_country, 3, 255, BIT(PRINT_SCAN), },
	[EID_BSS_LOAD] = { "BSS Load", print_bss_load, 5, 5, BIT(PRINT_SCAN), },
	[EID_POWER_CONSTRAINT] = { "Power constraint", print_powerconstraint,
				   1, 1, BIT(PRINT_SCAN), },
	[EID_TPC_REPORT] = { "TPC report", print_tpcreport, 2, 2, BIT(PRINT_SCAN), },
	[EID_ERP_INFO] = { "ERP", print_erp, 1, 255, BIT(PRINT_SCAN), },
	[EID_HT_CAPABILITY] = { "HT capabilities", print_ht_capa, 26, 26, BIT(PRINT_SCAN), },
	[EID_ERP_D4_0] = { "ERP D4.0", print_erp, 1, 255, BIT(PRINT_SCAN), },
	[EID_AP_CHAN_REPORT] = { "AP Channel Report", print_ap_channel_report,
				 1, 255, BIT(PRINT_SCAN), },
	[EID_SUPP_OP_CLASSES] = { "Supported operating classes",
				  print_supp_op_classes, 1, 255, BIT(PRINT_SCAN), },
	[EID_MEASUREMENT_PILOT_TX] = { "Measurement Pilot Transmission",
				       print_measurement_pilot_tx, 1, 255, BIT(PRINT_SCAN), },
	[EID_OVERLAP_BSS_SCAN_PARAM] = { "Overlapping BSS scan params",
					 print_obss_scan_params, 14, 255, BIT(PRINT_SCAN), },
	[EID_HT_OPERATION] = { "HT operation", print_ht_op, 22, 22, BIT(PRINT_SCAN), },
	[EID_SECONDARY_CH_OFFSET] = { "Secondary Channel Offset",
				      print_secchan_offs, 1, 1, BIT(PRINT_SCAN), },
	[EID_VHT_CAPABILITY] = { "VHT capabilities", print_vht_capa, 12, 255, BIT(PRINT_SCAN), },
	[EID_VHT_OPERATION] = { "VHT operation", print_vht_oper, 5, 255, BIT(PRINT_SCAN), },
	[EID_RSN] = { "RSN", print_rsn, 2, 255, BIT(PRINT_SCAN), },
	[EID_EXT_SUPP_RATES] = { "Extended supported rates", print_supprates,
				 0, 255, BIT(PRINT_SCAN), },
	[EID_RM_ENABLED_CAPABILITIES] = { "RM enabled capabilities",
					  print_rm_enabled_capabilities, 5, 5, BIT(PRINT_SCAN), },
	[EID_MESH_CONFIG] = { "MESH Configuration", print_mesh_conf, 7, 7, BIT(PRINT_SCAN), },
	[EID_MESH_ID] = { "MESH ID", print_ssid, 0, 32, BIT(PRINT_SCAN) | BIT(PRINT_LINK), },
	[EID_EXT_CAPABILITY] = { "Extended capabilities", print_capabilities,
				 0, 255, BIT(PRINT_SCAN), },
	[EID_INTERWORKING] = { "802.11u Interworking", print_interworking,
			       0, 255, BIT(PRINT_SCAN), },
	[EID_ADVERTISEMENT] = { "802.11u Advertisement", print_11u_advert,
				0, 255, BIT(PRINT_SCAN), },
	[EID_ROAMING_CONSORTIUM] = { "802.11u Roaming Consortium",
				     print_11u_rcon, 2, 255, BIT(PRINT_SCAN), },
	[EID_TRANSMIT_POWER_ENVELOPE] = { "Transmit Power Envelope",
					  print_tx_power_envelope, 2, 5, BIT(PRINT_SCAN), },
	[EID_SHORT_BEACON_INTERVAL] = { "Short beacon interval",
					print_short_beacon_int, 2, 2, BIT(PRINT_SCAN), },
	[EID_S1G_CAPABILITY] = { "S1G capabilities", print_s1g_capa, 15, 15, BIT(PRINT_SCAN), },
	[EID_S1G_OPERATION] = { "S1G operation", print_s1g_oper, 6, 6, BIT(PRINT_SCAN), },
};

static void print_wifi_wpa(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	print_rsn_ie("TKIP", "IEEE 802.1X", len, data);
}

static void print_wifi_osen(const uint8_t type, uint8_t len,
			    const uint8_t *data,
			    const struct ie_context *ctx)
{
	print_osen_ie("OSEN", "OSEN", len, data);
}

static bool print_wifi_wmm_param(const uint8_t *data, uint8_t len)
{
	int i;
	static const char *aci_tbl[] = { "BE", "BK", "VI", "VO" };

	if (len < 19)
		goto invalid;

	if (data[0] != 1) {
		printf("Parameter: not version 1: ");
		return false;
	}

	printf("\t * Parameter version 1");

	data++;

	if (data[0] & 0x80)
		printf("\n\t\t * u-APSD");

	data += 2;

	for (i = 0; i < 4; i++) {
		printf("\n\t\t * %s:", aci_tbl[(data[0] >> 5) & 3]);
		if (data[0] & 0x10)
			printf(" acm");
		printf(" CW %d-%d", (1 << (data[1] & 0xf)) - 1,
				    (1 << (data[1] >> 4)) - 1);
		printf(", AIFSN %d", data[0] & 0xf);
		if (data[2] | data[3])
			printf(", TXOP %d usec", (data[2] + (data[3] << 8)) * 32);
		data += 4;
	}

	printf("\n");
	return true;

 invalid:
 	printf("invalid: ");
 	return false;
}

static void print_wifi_wmm(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	int i;

	switch (data[0]) {
	case 0x00:
		printf(" information:");
		break;
	case 0x01:
		if (print_wifi_wmm_param(data + 1, len - 1))
			return;
		break;
	default:
		printf(" type %d:", data[0]);
		break;
	}

	for(i = 1; i < len; i++)
		printf(" %.02x", data[i]);
	printf("\n");
}

static const char * wifi_wps_dev_passwd_id(uint16_t id)
{
	switch (id) {
	case 0:
		return "Default (PIN)";
	case 1:
		return "User-specified";
	case 2:
		return "Machine-specified";
	case 3:
		return "Rekey";
	case 4:
		return "PushButton";
	case 5:
		return "Registrar-specified";
	default:
		return "??";
	}
}

static void print_wifi_wps(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	bool first = true;
	__u16 subtype, sublen;

	while (len >= 4) {
		subtype = (data[0] << 8) + data[1];
		sublen = (data[2] << 8) + data[3];
		if (sublen > len - 4)
			break;

		switch (subtype) {
		case 0x104a:
			tab_on_first(&first);
			if (sublen < 1) {
				printf("\t * Version: (invalid "
				       "length %d)\n", sublen);
				break;
			}
			printf("\t * Version: %d.%d\n", data[4] >> 4, data[4] & 0xF);
			break;
		case 0x1011:
			tab_on_first(&first);
			printf("\t * Device name: %.*s\n", sublen, data + 4);
			break;
		case 0x1012: {
			uint16_t id;
			tab_on_first(&first);
			if (sublen != 2) {
				printf("\t * Device Password ID: (invalid length %d)\n",
				       sublen);
				break;
			}
			id = data[4] << 8 | data[5];
			printf("\t * Device Password ID: %u (%s)\n",
			       id, wifi_wps_dev_passwd_id(id));
			break;
		}
		case 0x1021:
			tab_on_first(&first);
			printf("\t * Manufacturer: %.*s\n", sublen, data + 4);
			break;
		case 0x1023:
			tab_on_first(&first);
			printf("\t * Model: %.*s\n", sublen, data + 4);
			break;
		case 0x1024:
			tab_on_first(&first);
			printf("\t * Model Number: %.*s\n", sublen, data + 4);
			break;
		case 0x103b: {
			__u8 val;

			if (sublen < 1) {
				printf("\t * Response Type: (invalid length %d)\n",
				       sublen);
				break;
			}
			val = data[4];
			tab_on_first(&first);
			printf("\t * Response Type: %d%s\n",
			       val, val == 3 ? " (AP)" : "");
			break;
		}
		case 0x103c: {
			__u8 val;

			if (sublen < 1) {
				printf("\t * RF Bands: (invalid length %d)\n",
				       sublen);
				break;
			}
			val = data[4];
			tab_on_first(&first);
			printf("\t * RF Bands: 0x%x\n", val);
			break;
		}
		case 0x1041: {
			__u8 val;

			if (sublen < 1) {
				printf("\t * Selected Registrar: (invalid length %d)\n",
				       sublen);
				break;
			}
			val = data[4];
			tab_on_first(&first);
			printf("\t * Selected Registrar: 0x%x\n", val);
			break;
		}
		case 0x1042:
			tab_on_first(&first);
			printf("\t * Serial Number: %.*s\n", sublen, data + 4);
			break;
		case 0x1044: {
			__u8 val;

			if (sublen < 1) {
				printf("\t * Wi-Fi Protected Setup State: (invalid length %d)\n",
				       sublen);
				break;
			}
			val = data[4];
			tab_on_first(&first);
			printf("\t * Wi-Fi Protected Setup State: %d%s%s\n",
			       val,
			       val == 1 ? " (Unconfigured)" : "",
			       val == 2 ? " (Configured)" : "");
			break;
		}
		case 0x1047:
			tab_on_first(&first);
			printf("\t * UUID: ");
			if (sublen != 16) {
				printf("(invalid, length=%d)\n", sublen);
				break;
			}
			printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-"
				"%02x%02x-%02x%02x%02x%02x%02x%02x\n",
				data[4], data[5], data[6], data[7],
				data[8], data[9], data[10], data[11],
				data[12], data[13], data[14], data[15],
				data[16], data[17], data[18], data[19]);
			break;
		case 0x1049:
			tab_on_first(&first);
			if (sublen == 6 &&
			    data[4] == 0x00 &&
			    data[5] == 0x37 &&
			    data[6] == 0x2a &&
			    data[7] == 0x00 &&
			    data[8] == 0x01) {
				uint8_t v2 = data[9];
				printf("\t * Version2: %d.%d\n", v2 >> 4, v2 & 0xf);
			} else {
				printf("\t * Unknown vendor extension. len=%u\n",
				       sublen);
			}
			break;
		case 0x1054: {
			tab_on_first(&first);
			if (sublen != 8) {
				printf("\t * Primary Device Type: (invalid length %d)\n",
				       sublen);
				break;
			}
			printf("\t * Primary Device Type: "
			       "%u-%02x%02x%02x%02x-%u\n",
			       data[4] << 8 | data[5],
			       data[6], data[7], data[8], data[9],
			       data[10] << 8 | data[11]);
			break;
		}
		case 0x1057: {
			__u8 val;
			tab_on_first(&first);
			if (sublen < 1) {
				printf("\t * AP setup locked: (invalid length %d)\n",
				       sublen);
				break;
			}
			val = data[4];
			printf("\t * AP setup locked: 0x%.2x\n", val);
			break;
		}
		case 0x1008:
		case 0x1053: {
			__u16 meth;
			bool comma;

			if (sublen < 2) {
				printf("\t * Config methods: (invalid length %d)\n",
				       sublen);
				break;
			}
			meth = (data[4] << 8) + data[5];
			comma = false;
			tab_on_first(&first);
			printf("\t * %sConfig methods:",
			       subtype == 0x1053 ? "Selected Registrar ": "");
#define T(bit, name) do {		\
	if (meth & (1<<bit)) {		\
		if (comma)		\
			printf(",");	\
		comma = true;		\
		printf(" " name);	\
	} } while (0)
			T(0, "USB");
			T(1, "Ethernet");
			T(2, "Label");
			T(3, "Display");
			T(4, "Ext. NFC");
			T(5, "Int. NFC");
			T(6, "NFC Intf.");
			T(7, "PBC");
			T(8, "Keypad");
			printf("\n");
			break;
#undef T
		}
		default: {
			const __u8 *subdata = data + 4;
			__u16 tmplen = sublen;

			tab_on_first(&first);
			printf("\t * Unknown TLV (%#.4x, %d bytes):",
			       subtype, tmplen);
			while (tmplen) {
				printf(" %.2x", *subdata);
				subdata++;
				tmplen--;
			}
			printf("\n");
			break;
		}
		}

		data += sublen + 4;
		len -= sublen + 4;
	}

	if (len != 0) {
		printf("\t\t * bogus tail data (%d):", len);
		while (len) {
			printf(" %.2x", *data);
			data++;
			len--;
		}
		printf("\n");
	}
}

static const struct ie_print wifiprinters[] = {
	[1] = { "WPA", print_wifi_wpa, 2, 255, BIT(PRINT_SCAN), },
	[2] = { "WMM", print_wifi_wmm, 1, 255, BIT(PRINT_SCAN), },
	[4] = { "WPS", print_wifi_wps, 0, 255, BIT(PRINT_SCAN), },
};

static inline void print_p2p(const uint8_t type, uint8_t len,
			     const uint8_t *data,
			     const struct ie_context *ctx)
{
	bool first = true;
	__u8 subtype;
	__u16 sublen;

	while (len >= 3) {
		subtype = data[0];
		sublen = (data[2] << 8) + data[1];

		if (sublen > len - 3)
			break;

		switch (subtype) {
		case 0x02: /* capability */
			tab_on_first(&first);
			if (sublen < 2) {
				printf("\t * malformed capability\n");
				break;
			}
			printf("\t * Group capa: 0x%.2x, Device capa: 0x%.2x\n",
				data[3], data[4]);
			break;
		case 0x0d: /* device info */
			if (sublen < 6 + 2 + 8 + 1) {
				printf("\t * malformed device info\n");
				break;
			}
			/* fall through */
		case 0x00: /* status */
		case 0x01: /* minor reason */
		case 0x03: /* device ID */
		case 0x04: /* GO intent */
		case 0x05: /* configuration timeout */
		case 0x06: /* listen channel */
		case 0x07: /* group BSSID */
		case 0x08: /* ext listen timing */
		case 0x09: /* intended interface address */
		case 0x0a: /* manageability */
		case 0x0b: /* channel list */
		case 0x0c: /* NoA */
		case 0x0e: /* group info */
		case 0x0f: /* group ID */
		case 0x10: /* interface */
		case 0x11: /* operating channel */
		case 0x12: /* invitation flags */
		case 0xdd: /* vendor specific */
		default: {
			const __u8 *subdata = data + 3;
			__u16 tmplen = sublen;

			tab_on_first(&first);
			printf("\t * Unknown TLV (%#.2x, %d bytes):",
			       subtype, tmplen);
			while (tmplen) {
				printf(" %.2x", *subdata);
				subdata++;
				tmplen--;
			}
			printf("\n");
			break;
		}
		}

		data += sublen + 3;
		len -= sublen + 3;
	}

	if (len != 0) {
		tab_on_first(&first);
		printf("\t * bogus tail data (%d):", len);
		while (len) {
			printf(" %.2x", *data);
			data++;
			len--;
		}
		printf("\n");
	}
}

static inline void print_hs20_ind(const uint8_t type, uint8_t len,
				  const uint8_t *data,
				  const struct ie_context *ctx)
{
	/* I can't find the spec for this...just going off what wireshark uses. */
	printf("\n");
	if (len > 0)
		printf("\t\tDGAF: %i\n", (int)(data[0] & 0x1));
	else
		printf("\t\tUnexpected length: %i\n", len);
}

static void print_wifi_owe_tarns(const uint8_t type, uint8_t len,
				 const uint8_t *data,
				 const struct ie_context *ctx)
{
	char mac_addr[20];
	int ssid_len;

	printf("\n");
	if (len < 7)
		return;

	mac_addr_n2a(mac_addr, data);
	printf("\t\tBSSID: %s\n", mac_addr);

	ssid_len = data[6];
	if (ssid_len > len - 7)
		return;
	printf("\t\tSSID: ");
	print_ssid_escaped(ssid_len, data + 7);
	printf("\n");

	/* optional elements */
	if (len >= ssid_len + 9) {
		printf("\t\tBand Info: %u\n", data[ssid_len + 7]);
		printf("\t\tChannel Info: %u\n", data[ssid_len + 8]);
	}
}

static const struct ie_print wfa_printers[] = {
	[9] = { "P2P", print_p2p, 2, 255, BIT(PRINT_SCAN), },
	[16] = { "HotSpot 2.0 Indication", print_hs20_ind, 1, 255, BIT(PRINT_SCAN), },
	[18] = { "HotSpot 2.0 OSEN", print_wifi_osen, 1, 255, BIT(PRINT_SCAN), },
	[28] = { "OWE Transition Mode", print_wifi_owe_tarns, 7, 255, BIT(PRINT_SCAN), },
};

static void print_vendor(unsigned char len, unsigned char *data,
			 bool unknown, enum print_ie_type ptype)
{
	int i;

	if (len < 3) {
		printf("\tVendor specific: <too short> data:");
		for(i = 0; i < len; i++)
			printf(" %.02x", data[i]);
		printf("\n");
		return;
	}

	if (len >= 4 && memcmp(data, ms_oui, 3) == 0) {
		if (data[3] < ARRAY_SIZE(wifiprinters) &&
		    wifiprinters[data[3]].name &&
		    wifiprinters[data[3]].flags & BIT(ptype)) {
			print_ie(&wifiprinters[data[3]],
				 data[3], len - 4, data + 4,
				 NULL);
			return;
		}
		if (!unknown)
			return;
		printf("\tMS/WiFi %#.2x, data:", data[3]);
		for(i = 0; i < len - 4; i++)
			printf(" %.02x", data[i + 4]);
		printf("\n");
		return;
	}

	if (len >= 4 && memcmp(data, wfa_oui, 3) == 0) {
		if (data[3] < ARRAY_SIZE(wfa_printers) &&
		    wfa_printers[data[3]].name &&
		    wfa_printers[data[3]].flags & BIT(ptype)) {
			print_ie(&wfa_printers[data[3]],
				 data[3], len - 4, data + 4,
				 NULL);
			return;
		}
		if (!unknown)
			return;
		printf("\tWFA %#.2x, data:", data[3]);
		for(i = 0; i < len - 4; i++)
			printf(" %.02x", data[i + 4]);
		printf("\n");
		return;
	}

	if (!unknown)
		return;

	printf("\tVendor specific: OUI %.2x:%.2x:%.2x, data:",
		data[0], data[1], data[2]);
	for (i = 3; i < len; i++)
		printf(" %.2x", data[i]);
	printf("\n");
}

static void print_he_capa(const uint8_t type, uint8_t len, const uint8_t *data,
			  const struct ie_context *ctx)
{
	printf("\n");
	print_he_capability(data, len);
}

static void print_he_oper(const uint8_t type, uint8_t len, const uint8_t *data,
			  const struct ie_context *ctx)
{
	printf("\n");
	print_he_operation(data, len);
}

static void print_eht_capa(const uint8_t type, uint8_t len,
			   const uint8_t *data, const struct ie_context *ctx)
{
	printf("\n");
	print_eht_capability(data, len, ctx->he_cap, ctx->from_ap);
}

static void _print_multi_link(const uint8_t type, uint8_t len, const uint8_t *data,
			      const struct ie_context *ctx)
{
	printf("\n");
	print_multi_link(data, len);
}

static void print_eht_oper(const uint8_t type, uint8_t len, const uint8_t *data,
			   const struct ie_context *ctx)
{
	printf("\n");
	print_eht_operation(data, len);
}

static const struct ie_print ext_printers[] = {
	[EID_EXT_HE_CAPABILITY] = { "HE capabilities", print_he_capa, 21, 54, BIT(PRINT_SCAN), },
	[EID_EXT_HE_OPERATION] = { "HE Operation", print_he_oper, 6, 15, BIT(PRINT_SCAN), },
	[EID_EXT_EHT_CAPABILITY] = { "EHT capabilities", print_eht_capa, 13, 30, BIT(PRINT_SCAN), },
	[EID_EXT_MULTI_LINK] = { "Multi-Link", _print_multi_link, 5, 23, BIT(PRINT_SCAN), },
	[EID_EXT_EHT_OPERATION] = { "EHT Operation", print_eht_oper, 5, 10, BIT(PRINT_SCAN), },
};

static void print_extension(unsigned char len, unsigned char *ie,
			    const struct ie_context *ctx,
			    bool unknown, enum print_ie_type ptype)
{
	unsigned char tag;

	if (len < 1) {
		printf("\tExtension IE: <empty>\n");
		return;
	}

	tag = ie[0];
	if (tag < ARRAY_SIZE(ext_printers) && ext_printers[tag].name &&
	    ext_printers[tag].flags & BIT(ptype)) {
		print_ie(&ext_printers[tag], tag, len - 1, ie + 1, ctx);
		return;
	}

	if (unknown) {
		int i;

		printf("\tUnknown Extension ID (%d):", ie[0]);
		for (i = 1; i < len; i++)
			printf(" %.2x", ie[i]);
		printf("\n");
	}
}

static void init_context(struct ie_context *ctx,
			 unsigned char *ie, int ielen, bool from_ap)
{
	unsigned char *pos = ie;
	int remaining = ielen;

	memset(ctx, 0, sizeof(*ctx));

	if (!ie || !ielen)
		return;

	ctx->from_ap = from_ap;

	while (remaining >= 2 && remaining - 2 >= pos[1]) {
		switch (pos[0]) {
		case EID_VHT_CAPABILITY:
			ctx->is_vht_cap = true;
			break;
		case EID_EXTENSION:
			switch (pos[2]) {
			case EID_EXT_HE_CAPABILITY:
				ctx->he_cap = pos + 3;
				break;
			}
			break;
		}

		remaining -= pos[1] + 2;
		pos += pos[1] + 2;
	}
}

void print_ies(unsigned char *ie, int ielen, bool unknown,
	       enum print_ie_type ptype, bool from_ap)
{
	struct ie_context ctx;

	if (!ie)
		return;

	init_context(&ctx, ie, ielen, from_ap);

	while (ielen >= 2 && ielen - 2 >= ie[1]) {
		if (ie[0] < ARRAY_SIZE(ieprinters) &&
		    ieprinters[ie[0]].name &&
		    ieprinters[ie[0]].flags & BIT(ptype) &&
			    ie[1] > 0) {
			print_ie(&ieprinters[ie[0]],
				 ie[0], ie[1], ie + 2, &ctx);
		} else if (ie[0] == 221 /* vendor */) {
			print_vendor(ie[1], ie + 2, unknown, ptype);
		} else if (ie[0] == 255 /* extension */) {
			print_extension(ie[1], ie + 2, &ctx, unknown, ptype);
		} else if (unknown) {
			int i;

			printf("\tUnknown IE (%d):", ie[0]);
			for (i=0; i<ie[1]; i++)
				printf(" %.2x", ie[2+i]);
			printf("\n");
		}
		ielen -= ie[1] + 2;
		ie += ie[1] + 2;
	}
}

static void print_capa_dmg(__u16 capa)
{
	switch (capa & WLAN_CAPABILITY_DMG_TYPE_MASK) {
	case WLAN_CAPABILITY_DMG_TYPE_AP:
		printf(" DMG_ESS");
		break;
	case WLAN_CAPABILITY_DMG_TYPE_PBSS:
		printf(" DMG_PCP");
		break;
	case WLAN_CAPABILITY_DMG_TYPE_IBSS:
		printf(" DMG_IBSS");
		break;
	}

	if (capa & WLAN_CAPABILITY_DMG_CBAP_ONLY)
		printf(" CBAP_Only");
	if (capa & WLAN_CAPABILITY_DMG_CBAP_SOURCE)
		printf(" CBAP_Src");
	if (capa & WLAN_CAPABILITY_DMG_PRIVACY)
		printf(" Privacy");
	if (capa & WLAN_CAPABILITY_DMG_ECPAC)
		printf(" ECPAC");
	if (capa & WLAN_CAPABILITY_DMG_SPECTRUM_MGMT)
		printf(" SpectrumMgmt");
	if (capa & WLAN_CAPABILITY_DMG_RADIO_MEASURE)
		printf(" RadioMeasure");
}

static void print_capa_non_dmg(__u16 capa)
{
	if (capa & WLAN_CAPABILITY_ESS)
		printf(" ESS");
	if (capa & WLAN_CAPABILITY_IBSS)
		printf(" IBSS");
	if (capa & WLAN_CAPABILITY_CF_POLLABLE)
		printf(" CfPollable");
	if (capa & WLAN_CAPABILITY_CF_POLL_REQUEST)
		printf(" CfPollReq");
	if (capa & WLAN_CAPABILITY_PRIVACY)
		printf(" Privacy");
	if (capa & WLAN_CAPABILITY_SHORT_PREAMBLE)
		printf(" ShortPreamble");
	if (capa & WLAN_CAPABILITY_PBCC)
		printf(" PBCC");
	if (capa & WLAN_CAPABILITY_CHANNEL_AGILITY)
		printf(" ChannelAgility");
	if (capa & WLAN_CAPABILITY_SPECTRUM_MGMT)
		printf(" SpectrumMgmt");
	if (capa & WLAN_CAPABILITY_QOS)
		printf(" QoS");
	if (capa & WLAN_CAPABILITY_SHORT_SLOT_TIME)
		printf(" ShortSlotTime");
	if (capa & WLAN_CAPABILITY_APSD)
		printf(" APSD");
	if (capa & WLAN_CAPABILITY_RADIO_MEASURE)
		printf(" RadioMeasure");
	if (capa & WLAN_CAPABILITY_DSSS_OFDM)
		printf(" DSSS-OFDM");
	if (capa & WLAN_CAPABILITY_DEL_BACK)
		printf(" DelayedBACK");
	if (capa & WLAN_CAPABILITY_IMM_BACK)
		printf(" ImmediateBACK");
}

static int print_bss_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	char mac_addr[20], dev[20];
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
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { },
	};
	struct scan_params *params = arg;
	int show = params->show_both_ie_sets ? 2 : 1;
	bool is_dmg = false;

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

	mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_BSSID]));
	printf("BSS %s", mac_addr);
	if (tb[NL80211_ATTR_IFINDEX]) {
		if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);
		printf("(on %s)", dev);
	}

	if (bss[NL80211_BSS_STATUS]) {
		switch (nla_get_u32(bss[NL80211_BSS_STATUS])) {
		case NL80211_BSS_STATUS_AUTHENTICATED:
			printf(" -- authenticated");
			break;
		case NL80211_BSS_STATUS_ASSOCIATED:
			printf(" -- associated");
			break;
		case NL80211_BSS_STATUS_IBSS_JOINED:
			printf(" -- joined");
			break;
		default:
			printf(" -- unknown status: %d",
				nla_get_u32(bss[NL80211_BSS_STATUS]));
			break;
		}
	}
	printf("\n");

	if (bss[NL80211_BSS_LAST_SEEN_BOOTTIME]) {
		unsigned long long bt;
		bt = (unsigned long long)nla_get_u64(bss[NL80211_BSS_LAST_SEEN_BOOTTIME]);
		printf("\tlast seen: %llu.%.3llus [boottime]\n", bt/1000000000, (bt%1000000000)/1000000);
	}

	if (bss[NL80211_BSS_TSF]) {
		unsigned long long tsf;
		tsf = (unsigned long long)nla_get_u64(bss[NL80211_BSS_TSF]);
		printf("\tTSF: %llu usec (%llud, %.2lld:%.2llu:%.2llu)\n",
			tsf, tsf/1000/1000/60/60/24, (tsf/1000/1000/60/60) % 24,
			(tsf/1000/1000/60) % 60, (tsf/1000/1000) % 60);
	}
	if (bss[NL80211_BSS_FREQUENCY]) {
		int freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
		if (bss[NL80211_BSS_FREQUENCY_OFFSET])
			printf("\tfreq: %d.%d\n", freq,
			    nla_get_u32(bss[NL80211_BSS_FREQUENCY_OFFSET]));
		else
			printf("\tfreq: %d\n", freq);

		if (freq > 45000)
			is_dmg = true;
	}
	if (bss[NL80211_BSS_BEACON_INTERVAL])
		printf("\tbeacon interval: %d TUs\n",
			nla_get_u16(bss[NL80211_BSS_BEACON_INTERVAL]));
	if (bss[NL80211_BSS_CAPABILITY]) {
		__u16 capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
		printf("\tcapability:");
		if (is_dmg)
			print_capa_dmg(capa);
		else
			print_capa_non_dmg(capa);
		printf(" (0x%.4x)\n", capa);
	}
	if (bss[NL80211_BSS_SIGNAL_MBM]) {
		int s = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
		printf("\tsignal: %d.%.2d dBm\n", s/100, s%100);
	}
	if (bss[NL80211_BSS_SIGNAL_UNSPEC]) {
		unsigned char s = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
		printf("\tsignal: %d/100\n", s);
	}
	if (bss[NL80211_BSS_SEEN_MS_AGO]) {
		int age = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);
		printf("\tlast seen: %d ms ago\n", age);
	}

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS] && show--) {
		struct nlattr *ies = bss[NL80211_BSS_INFORMATION_ELEMENTS];
		struct nlattr *bcnies = bss[NL80211_BSS_BEACON_IES];

		if (bss[NL80211_BSS_PRESP_DATA] ||
		    (bcnies && (nla_len(ies) != nla_len(bcnies) ||
				memcmp(nla_data(ies), nla_data(bcnies),
				       nla_len(ies)))))
			printf("\tInformation elements from Probe Response "
			       "frame:\n");
		print_ies(nla_data(ies), nla_len(ies),
			  params->unknown, params->type, true);
	}
	if (bss[NL80211_BSS_BEACON_IES] && show--) {
		printf("\tInformation elements from Beacon frame:\n");
		print_ies(nla_data(bss[NL80211_BSS_BEACON_IES]),
			  nla_len(bss[NL80211_BSS_BEACON_IES]),
			  params->unknown, params->type, true);
	}

	return NL_SKIP;
}

static struct scan_params scan_params;

static int handle_scan_dump(struct nl80211_state *state,
			    struct nl_msg *msg,
			    int argc, char **argv,
			    enum id_input id)
{
	if (argc > 1)
		return 1;

	memset(&scan_params, 0, sizeof(scan_params));

	if (argc == 1 && !strcmp(argv[0], "-u"))
		scan_params.unknown = true;
	else if (argc == 1 && !strcmp(argv[0], "-b"))
		scan_params.show_both_ie_sets = true;

	scan_params.type = PRINT_SCAN;

	register_handler(print_bss_handler, &scan_params);
	return 0;
}

static int handle_scan_combined(struct nl80211_state *state,
				struct nl_msg *msg,
				int argc, char **argv,
				enum id_input id)
{
	char **trig_argv;
	static char *dump_argv[] = {
		NULL,
		"scan",
		"dump",
		NULL,
	};
	static const __u32 cmds[] = {
		NL80211_CMD_NEW_SCAN_RESULTS,
		NL80211_CMD_SCAN_ABORTED,
	};
	int trig_argc, dump_argc, err;
	int i;

	if (argc >= 3 && !strcmp(argv[2], "-u")) {
		dump_argc = 4;
		dump_argv[3] = "-u";
	} else if (argc >= 3 && !strcmp(argv[2], "-b")) {
		dump_argc = 4;
		dump_argv[3] = "-b";
	} else
		dump_argc = 3;

	trig_argc = 3 + (argc - 2) + (3 - dump_argc);
	trig_argv = calloc(trig_argc, sizeof(*trig_argv));
	if (!trig_argv)
		return -ENOMEM;
	trig_argv[0] = argv[0];
	trig_argv[1] = "scan";
	trig_argv[2] = "trigger";

	for (i = 0; i < argc - 2 - (dump_argc - 3); i++)
		trig_argv[i + 3] = argv[i + 2 + (dump_argc - 3)];
	err = handle_cmd(state, id, trig_argc, trig_argv);
	free(trig_argv);
	if (err)
		return err;

	/*
	 * WARNING: DO NOT COPY THIS CODE INTO YOUR APPLICATION
	 *
	 * This code has a bug, which requires creating a separate
	 * nl80211 socket to fix:
	 * It is possible for a NL80211_CMD_NEW_SCAN_RESULTS or
	 * NL80211_CMD_SCAN_ABORTED message to be sent by the kernel
	 * before (!) we listen to it, because we only start listening
	 * after we send our scan request.
	 *
	 * Doing it the other way around has a race condition as well,
	 * if you first open the events socket you may get a notification
	 * for a previous scan.
	 *
	 * The only proper way to fix this would be to listen to events
	 * before sending the command, and for the kernel to send the
	 * scan request along with the event, so that you can match up
	 * whether the scan you requested was finished or aborted (this
	 * may result in processing a scan that another application
	 * requested, but that doesn't seem to be a problem).
	 *
	 * Alas, the kernel doesn't do that (yet).
	 */

	if (listen_events(state, ARRAY_SIZE(cmds), cmds) ==
					NL80211_CMD_SCAN_ABORTED) {
		printf("scan aborted!\n");
		return 0;
	}

	dump_argv[0] = argv[0];
	return handle_cmd(state, id, dump_argc, dump_argv);
}
TOPLEVEL(scan, "[-u] [freq <freq>*] [duration <dur>] [ies <hex as 00:11:..>] [meshid <meshid>] [lowpri,flush,ap-force,duration-mandatory] [randomise[=<addr>/<mask>]] [ssid <ssid>*|passive]", 0, 0,
	 CIB_NETDEV, handle_scan_combined,
	 "Scan on the given frequencies and probe for the given SSIDs\n"
	 "(or wildcard if not given) unless passive scanning is requested.\n"
	 "If -u is specified print unknown data in the scan results.\n"
	 "Specified (vendor) IEs must be well-formed.");
COMMAND(scan, dump, "[-u]",
	NL80211_CMD_GET_SCAN, NLM_F_DUMP, CIB_NETDEV, handle_scan_dump,
	"Dump the current scan results. If -u is specified, print unknown\n"
	"data in scan results.");
COMMAND(scan, trigger, "[freq <freq>*] [duration <dur>] [ies <hex as 00:11:..>] [meshid <meshid>] [lowpri,flush,ap-force,duration-mandatory,coloc] [randomise[=<addr>/<mask>]] [ssid <ssid>*|passive]",
	NL80211_CMD_TRIGGER_SCAN, 0, CIB_NETDEV, handle_scan,
	 "Trigger a scan on the given frequencies with probing for the given\n"
	 "SSIDs (or wildcard if not given) unless passive scanning is requested.\n"
	 "Duration(in TUs), if specified, will be used to set dwell times.\n");


static int handle_scan_abort(struct nl80211_state *state,
			     struct nl_msg *msg,
			     int argc, char **argv,
			     enum id_input id)
{
	return 0;
}
COMMAND(scan, abort, "",
	NL80211_CMD_ABORT_SCAN, 0, CIB_NETDEV, handle_scan_abort,
	"Abort ongoing scan");

static int handle_start_sched_scan(struct nl80211_state *state,
				   struct nl_msg *msg,
				   int argc, char **argv, enum id_input id)
{
	return parse_sched_scan(msg, &argc, &argv);
}

static int handle_stop_sched_scan(struct nl80211_state *state,
				  struct nl_msg *msg, int argc, char **argv,
				  enum id_input id)
{
	if (argc != 0)
		return 1;

	return 0;
}

COMMAND(scan, sched_start,
	SCHED_SCAN_OPTIONS,
	NL80211_CMD_START_SCHED_SCAN, 0, CIB_NETDEV, handle_start_sched_scan,
	"Start a scheduled scan at the specified interval on the given frequencies\n"
	"with probing for the given SSIDs (or wildcard if not given) unless passive\n"
	"scanning is requested.  If matches are specified, only matching results\n"
	"will be returned.");
COMMAND(scan, sched_stop, "",
	NL80211_CMD_STOP_SCHED_SCAN, 0, CIB_NETDEV, handle_stop_sched_scan,
	"Stop an ongoing scheduled scan.");
