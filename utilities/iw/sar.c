#include <errno.h>
#include <string.h>

#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "iw.h"

static int set_sar_specs(struct nl80211_state *state,
			 struct nl_msg *msg,
			 int argc, char **argv,
			 enum id_input id)
{
	struct nlattr *nl_sar, *nl_specs, *nl_sub;
	enum nl80211_sar_type type;
	__u32 idx;
	__s32 pwr;
	char *tmp;
	int count, i;

	if (argc <= 1)
		return -EINVAL;

	type = atoi(argv[0]);

	nl_sar = nla_nest_start(msg, NL80211_ATTR_SAR_SPEC);
	if (!nl_sar)
		goto nla_put_failure;

	NLA_PUT_U32(msg, NL80211_SAR_ATTR_TYPE, type);

	nl_specs = nla_nest_start(msg, NL80211_SAR_ATTR_SPECS);
	if (!nl_specs)
		goto nla_put_failure;

	for (i = 1; i < argc; i++) {
		tmp = strchr(argv[i], ':');
		if (!tmp)
			return -EINVAL;

		if (tmp != strrchr(argv[i], ':'))
			return -EINVAL;

		count = sscanf(argv[i], "%u:%d", &idx, &pwr);
		if (count != 2)
			return -EINVAL;

		nl_sub = nla_nest_start(msg, i - 1);
		if (!nl_sub)
			goto nla_put_failure;

		NLA_PUT_U32(msg, NL80211_SAR_ATTR_SPECS_RANGE_INDEX, idx);
		NLA_PUT_S32(msg, NL80211_SAR_ATTR_SPECS_POWER, pwr);

		nla_nest_end(msg, nl_sub);
	}

	nla_nest_end(msg, nl_specs);
	nla_nest_end(msg, nl_sar);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}

COMMAND(set, sar_specs, "<sar type> <range index:sar power>*",
	NL80211_CMD_SET_SAR_SPECS, 0, CIB_PHY, set_sar_specs,
	"Set SAR specs corresponding to SAR capa of wiphy.");
