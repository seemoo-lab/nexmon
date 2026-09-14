#include <errno.h>

#include "nl80211.h"
#include "iw.h"


static int parse_rate_chunk(const char *arg, __u8 *nss, __u16 *mcs, unsigned int mode)
{
	unsigned int count, i;
	unsigned int inss, mcs_start, mcs_end, tab[12];
	unsigned int max_mcs = 0, max_nss = 0;

	*nss = 0; *mcs = 0;

	if (mode == NL80211_TXRATE_HE) {
		max_mcs = 11;
		max_nss = NL80211_HE_NSS_MAX;
	} else {
		max_mcs = 9;
		max_nss = NL80211_VHT_NSS_MAX;
	}

	if (strchr(arg, '-')) {
		/* Format: NSS:MCS_START-MCS_END */
		count = sscanf(arg, "%u:%u-%u", &inss, &mcs_start, &mcs_end);

		if (count != 3)
			return 0;

		if (inss < 1 || inss > max_nss)
			return 0;

		if (mcs_start > mcs_end)
			return 0;

		if (mcs_start > max_mcs || mcs_end > max_mcs)
			return 0;

		*nss = inss;
		for (i = mcs_start; i <= mcs_end; i++)
			*mcs |= 1 << i;

	} else {
		/* Format: NSS:MCSx,MCSy,... */
		if (mode == NL80211_TXRATE_HE) {
			count = sscanf(arg, "%u:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
				       &inss, &tab[0], &tab[1], &tab[2], &tab[3],
				       &tab[4], &tab[5], &tab[6], &tab[7], &tab[8],
				       &tab[9], &tab[10], &tab[11]);
		} else {
			count = sscanf(arg, "%u:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u", &inss,
				       &tab[0], &tab[1], &tab[2], &tab[3], &tab[4],
				       &tab[5], &tab[6], &tab[7], &tab[8], &tab[9]);
		}

		if (count < 2)
			return 0;

		if (inss < 1 || inss > max_nss)
			return 0;

		*nss = inss;
		for (i = 0; i < count - 1; i++) {
			if (tab[i] > max_mcs)
				return 0;
			*mcs |= 1 << tab[i];
		}
	}

	return 1;
}

static int parse_vht_chunk(const char *arg, __u8 *nss, __u16 *mcs)
{
	return parse_rate_chunk(arg, nss, mcs, NL80211_TXRATE_VHT);
}

static int parse_he_chunk(const char *arg, __u8 *nss, __u16 *mcs)
{
	return parse_rate_chunk(arg, nss, mcs, NL80211_TXRATE_HE);
}

static int setup_vht(struct nl80211_txrate_vht *txrate_vht,
		     int argc, char **argv)
{
	__u8 nss;
	__u16 mcs;
	int i;

	memset(txrate_vht, 0, sizeof(*txrate_vht));

	for (i = 0; i < argc; i++) {
		if (!parse_vht_chunk(argv[i], &nss, &mcs))
			return 0;

		nss--;
		txrate_vht->mcs[nss] |= mcs;
	}

	return 1;
}

static int setup_he(struct nl80211_txrate_he *txrate_he,
		    int argc, char **argv)
{
	__u8 nss;
	__u16 mcs;
	int i;

	memset(txrate_he, 0, sizeof(*txrate_he));

	for (i = 0; i < argc; i++) {
		if (!parse_he_chunk(argv[i], &nss, &mcs))
			return 0;

		nss--;
		txrate_he->mcs[nss] |= mcs;
	}

	return 1;
}

#define HE_GI_STR_MAX	16
#define HE_GI_08_STR "0.8"
#define HE_GI_16_STR "1.6"
#define HE_GI_32_STR "3.2"
static int parse_he_gi(char *he_gi)
{
	if (he_gi == NULL)
		return 0;

	if (!strncmp(he_gi, HE_GI_08_STR, sizeof(HE_GI_08_STR)))
		return NL80211_RATE_INFO_HE_GI_0_8;
	if (!strncmp(he_gi, HE_GI_16_STR, sizeof(HE_GI_16_STR)))
		return NL80211_RATE_INFO_HE_GI_1_6;
	if (!strncmp(he_gi, HE_GI_32_STR, sizeof(HE_GI_32_STR)))
		return NL80211_RATE_INFO_HE_GI_3_2;

	return -1;
}

#define VHT_ARGC_MAX	100

int set_bitrates(struct nl_msg *msg,
		 int argc, char **argv,
		 enum nl80211_attrs attr)
{
	struct nlattr *nl_rates, *nl_band;
	int i, ret = 0;
	bool have_legacy_24 = false, have_legacy_5 = false;
	uint8_t legacy_24[32], legacy_5[32];
	int n_legacy_24 = 0, n_legacy_5 = 0;
	uint8_t *legacy = NULL;
	int *n_legacy = NULL;
	bool have_ht_mcs_24 = false, have_ht_mcs_5 = false;
	bool have_vht_mcs_24 = false, have_vht_mcs_5 = false;
	bool have_he_mcs_24 = false, have_he_mcs_5 = false;
	bool have_he_mcs_6 = false;
	uint8_t ht_mcs_24[77], ht_mcs_5[77];
	int n_ht_mcs_24 = 0, n_ht_mcs_5 = 0;
	struct nl80211_txrate_vht txrate_vht_24 = {};
	struct nl80211_txrate_vht txrate_vht_5 = {};
	struct nl80211_txrate_he txrate_he_24 = {};
	struct nl80211_txrate_he txrate_he_5 = {};
	struct nl80211_txrate_he txrate_he_6 = {};
	uint8_t *mcs = NULL;
	int *n_mcs = NULL;
	char *vht_argv_5[VHT_ARGC_MAX] = {}; char *vht_argv_24[VHT_ARGC_MAX] = {};
	char *he_argv_5[VHT_ARGC_MAX] = {}; char *he_argv_24[VHT_ARGC_MAX] = {};
	char *he_argv_6[VHT_ARGC_MAX] = {};
	char **vht_argv = NULL, **he_argv = NULL;
	int vht_argc_5 = 0; int vht_argc_24 = 0;
	int he_argc_5 = 0; int he_argc_24 = 0;
	int he_argc_6 = 0;
	int *vht_argc = NULL, *he_argc = NULL;
	int sgi_24 = 0, sgi_5 = 0, lgi_24 = 0, lgi_5 = 0;
	int has_he_gi_24 = 0, has_he_gi_5 = 0, has_he_ltf_24 = 0, has_he_ltf_5 = 0;
	int has_he_gi_6 = 0, has_he_ltf_6 = 0;
	int he_gi = 0, he_ltf = 0;
	char *he_gi_argv = NULL;

	enum {
		S_NONE,
		S_LEGACY,
		S_HT,
		S_VHT,
		S_HE,
		S_GI,
		S_HE_GI,
		S_HE_LTF,
	} parser_state = S_NONE;

	for (i = 0; i < argc; i++) {
		char *end;
		double tmpd;
		long tmpl;

		if (strcmp(argv[i], "legacy-2.4") == 0) {
			if (have_legacy_24)
				return 1;
			parser_state = S_LEGACY;
			legacy = legacy_24;
			n_legacy = &n_legacy_24;
			have_legacy_24 = true;
		} else if (strcmp(argv[i], "legacy-5") == 0) {
			if (have_legacy_5)
				return 1;
			parser_state = S_LEGACY;
			legacy = legacy_5;
			n_legacy = &n_legacy_5;
			have_legacy_5 = true;
		}
		else if (strcmp(argv[i], "ht-mcs-2.4") == 0) {
			if (have_ht_mcs_24)
				return 1;
			parser_state = S_HT;
			mcs = ht_mcs_24;
			n_mcs = &n_ht_mcs_24;
			have_ht_mcs_24 = true;
		} else if (strcmp(argv[i], "ht-mcs-5") == 0) {
			if (have_ht_mcs_5)
				return 1;
			parser_state = S_HT;
			mcs = ht_mcs_5;
			n_mcs = &n_ht_mcs_5;
			have_ht_mcs_5 = true;
		} else if (strcmp(argv[i], "vht-mcs-2.4") == 0) {
			if (have_vht_mcs_24)
				return 1;
			parser_state = S_VHT;
			vht_argv = vht_argv_24;
			vht_argc = &vht_argc_24;
			have_vht_mcs_24 = true;
		} else if (strcmp(argv[i], "vht-mcs-5") == 0) {
			if (have_vht_mcs_5)
				return 1;
			parser_state = S_VHT;
			vht_argv = vht_argv_5;
			vht_argc = &vht_argc_5;
			have_vht_mcs_5 = true;
		} else if (strcmp(argv[i], "he-mcs-2.4") == 0) {
			if (have_he_mcs_24)
				return 1;
			parser_state = S_HE;
			he_argv = he_argv_24;
			he_argc = &he_argc_24;
			have_he_mcs_24 = true;
		} else if (strcmp(argv[i], "he-mcs-5") == 0) {
			if (have_he_mcs_5)
				return 1;
			parser_state = S_HE;
			he_argv = he_argv_5;
			he_argc = &he_argc_5;
			have_he_mcs_5 = true;
		} else if (strcmp(argv[i], "he-mcs-6") == 0) {
			if (have_he_mcs_6)
				return 1;
			parser_state = S_HE;
			he_argv = he_argv_6;
			he_argc = &he_argc_6;
			have_he_mcs_6 = true;
		} else if (strcmp(argv[i], "sgi-2.4") == 0) {
			sgi_24 = 1;
			parser_state = S_GI;
		} else if (strcmp(argv[i], "sgi-5") == 0) {
			sgi_5 = 1;
			parser_state = S_GI;
		} else if (strcmp(argv[i], "lgi-2.4") == 0) {
			lgi_24 = 1;
			parser_state = S_GI;
		} else if (strcmp(argv[i], "lgi-5") == 0) {
			lgi_5 = 1;
			parser_state = S_GI;
		} else if (strcmp(argv[i], "he-gi-2.4") == 0) {
			has_he_gi_24 = 1;
			parser_state = S_HE_GI;
		} else if (strcmp(argv[i], "he-gi-5") == 0) {
			has_he_gi_5 = 1;
			parser_state = S_HE_GI;
		} else if (strcmp(argv[i], "he-gi-6") == 0) {
			has_he_gi_6 = 1;
			parser_state = S_HE_GI;
		} else if (strcmp(argv[i], "he-ltf-2.4") == 0) {
			has_he_ltf_24 = 1;
			parser_state = S_HE_LTF;
		} else if (strcmp(argv[i], "he-ltf-5") == 0) {
			has_he_ltf_5 = 1;
			parser_state = S_HE_LTF;
		} else if (strcmp(argv[i], "he-ltf-6") == 0) {
			has_he_ltf_6 = 1;
			parser_state = S_HE_LTF;
		} else switch (parser_state) {
		case S_LEGACY:
			tmpd = strtod(argv[i], &end);
			if (*end != '\0')
				return 1;
			if (tmpd < 1 || tmpd > 255 * 2)
				return 1;
			legacy[(*n_legacy)++] = tmpd * 2;
			break;
		case S_HT:
			tmpl = strtol(argv[i], &end, 0);
			if (*end != '\0')
				return 1;
			if (tmpl < 0 || tmpl > 255)
				return 1;
			mcs[(*n_mcs)++] = tmpl;
			break;
		case S_VHT:
			if (*vht_argc >= VHT_ARGC_MAX)
				return 1;
			vht_argv[(*vht_argc)++] = argv[i];
			break;
		case S_HE:
			if (*he_argc >= VHT_ARGC_MAX)
				return 1;
			he_argv[(*he_argc)++] = argv[i];
			break;
		case S_GI:
			break;
		case S_HE_GI:
			he_gi_argv = argv[i];
			break;
		case S_HE_LTF:
			he_ltf = strtol(argv[i], &end, 0);
			if (*end != '\0')
				return 1;
			if (he_ltf < 0 || he_ltf > 4)
				return 1;
			he_ltf = he_ltf >> 1;
			break;
		default:
			if (attr != NL80211_ATTR_TX_RATES)
				goto next;
			return 1;
		}
	}

next:
	if (attr != NL80211_ATTR_TX_RATES)
		ret = i;

	if (have_vht_mcs_24)
		if (!setup_vht(&txrate_vht_24, vht_argc_24, vht_argv_24))
			return -EINVAL;

	if (have_vht_mcs_5)
		if (!setup_vht(&txrate_vht_5, vht_argc_5, vht_argv_5))
			return -EINVAL;

	if (have_he_mcs_24)
		if (!setup_he(&txrate_he_24, he_argc_24, he_argv_24))
			return -EINVAL;

	if (have_he_mcs_5)
		if (!setup_he(&txrate_he_5, he_argc_5, he_argv_5))
			return -EINVAL;

	if (have_he_mcs_6)
		if (!setup_he(&txrate_he_6, he_argc_6, he_argv_6))
			return -EINVAL;

	if (sgi_5 && lgi_5)
		return 1;

	if (sgi_24 && lgi_24)
		return 1;

	if (he_gi_argv) {
		he_gi = parse_he_gi(he_gi_argv);
		if (he_gi < 0)
			return 1;
	}

	nl_rates = nla_nest_start(msg, attr);
	if (!nl_rates)
		goto nla_put_failure;

	if (have_legacy_24 || have_ht_mcs_24 || have_vht_mcs_24 || have_he_mcs_24 ||
	    sgi_24 || lgi_24 || has_he_gi_24 || has_he_ltf_24) {
		nl_band = nla_nest_start(msg, NL80211_BAND_2GHZ);
		if (!nl_band)
			goto nla_put_failure;
		if (have_legacy_24)
			nla_put(msg, NL80211_TXRATE_LEGACY, n_legacy_24, legacy_24);
		if (have_ht_mcs_24)
			nla_put(msg, NL80211_TXRATE_HT, n_ht_mcs_24, ht_mcs_24);
		if (have_vht_mcs_24)
			nla_put(msg, NL80211_TXRATE_VHT, sizeof(txrate_vht_24), &txrate_vht_24);
		if (have_he_mcs_24)
			nla_put(msg, NL80211_TXRATE_HE, sizeof(txrate_he_24),
				&txrate_he_24);
		if (sgi_24)
			nla_put_u8(msg, NL80211_TXRATE_GI, NL80211_TXRATE_FORCE_SGI);
		if (lgi_24)
			nla_put_u8(msg, NL80211_TXRATE_GI, NL80211_TXRATE_FORCE_LGI);
		if (has_he_gi_24)
			nla_put_u8(msg, NL80211_TXRATE_HE_GI, he_gi);
		if (has_he_ltf_24)
			nla_put_u8(msg, NL80211_TXRATE_HE_LTF, he_ltf);
		nla_nest_end(msg, nl_band);
	}

	if (have_legacy_5 || have_ht_mcs_5 || have_vht_mcs_5 || have_he_mcs_5 ||
	    sgi_5 || lgi_5 || has_he_gi_5 || has_he_ltf_5) {
		nl_band = nla_nest_start(msg, NL80211_BAND_5GHZ);
		if (!nl_band)
			goto nla_put_failure;
		if (have_legacy_5)
			nla_put(msg, NL80211_TXRATE_LEGACY, n_legacy_5, legacy_5);
		if (have_ht_mcs_5)
			nla_put(msg, NL80211_TXRATE_HT, n_ht_mcs_5, ht_mcs_5);
		if (have_vht_mcs_5)
			nla_put(msg, NL80211_TXRATE_VHT, sizeof(txrate_vht_5), &txrate_vht_5);
		if (have_he_mcs_5)
			nla_put(msg, NL80211_TXRATE_HE, sizeof(txrate_he_5),
				&txrate_he_5);
		if (sgi_5)
			nla_put_u8(msg, NL80211_TXRATE_GI, NL80211_TXRATE_FORCE_SGI);
		if (lgi_5)
			nla_put_u8(msg, NL80211_TXRATE_GI, NL80211_TXRATE_FORCE_LGI);
		if (has_he_gi_5)
			nla_put_u8(msg, NL80211_TXRATE_HE_GI, he_gi);
		if (has_he_ltf_5)
			nla_put_u8(msg, NL80211_TXRATE_HE_LTF, he_ltf);
		nla_nest_end(msg, nl_band);
	}

	if (have_he_mcs_6 || has_he_gi_6 || has_he_ltf_6) {
		nl_band = nla_nest_start(msg, NL80211_BAND_6GHZ);
		if (!nl_band)
			goto nla_put_failure;
		if (have_he_mcs_6)
			nla_put(msg, NL80211_TXRATE_HE, sizeof(txrate_he_6),
				&txrate_he_6);
		if (has_he_gi_6)
			nla_put_u8(msg, NL80211_TXRATE_HE_GI, he_gi);
		if (has_he_ltf_6)
			nla_put_u8(msg, NL80211_TXRATE_HE_LTF, he_ltf);
		nla_nest_end(msg, nl_band);
	}

	nla_nest_end(msg, nl_rates);

	return ret;
 nla_put_failure:
	return -ENOBUFS;
}

static int handle_bitrates(struct nl80211_state *state,
			   struct nl_msg *msg,
			   int argc, char **argv,
			   enum id_input id)
{
	return set_bitrates(msg, argc, argv, NL80211_ATTR_TX_RATES);
}

#define DESCR_LEGACY "[legacy-<2.4|5> <legacy rate in Mbps>*]"
#define DESCR DESCR_LEGACY " [ht-mcs-<2.4|5> <MCS index>*] [vht-mcs-<2.4|5>  [he-mcs-<2.4|5|6> <NSS:MCSx,MCSy... | NSS:MCSx-MCSy>*] [sgi-2.4|lgi-2.4] [sgi-5|lgi-5]"

COMMAND(set, bitrates, "[legacy-<2.4|5> <legacy rate in Mbps>*] [ht-mcs-<2.4|5> <MCS index>*] [vht-mcs-<2.4|5> [he-mcs-<2.4|5|6> <NSS:MCSx,MCSy... | NSS:MCSx-MCSy>*] [sgi-2.4|lgi-2.4] [sgi-5|lgi-5] [he-gi-<2.4|5|6> <0.8|1.6|3.2>] [he-ltf-<2.4|5|6> <1|2|4>]",
	NL80211_CMD_SET_TX_BITRATE_MASK, 0, CIB_NETDEV, handle_bitrates,
	"Sets up the specified rate masks.\n"
	"Not passing any arguments would clear the existing mask (if any).");
