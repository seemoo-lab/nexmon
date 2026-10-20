#include <errno.h>
#include <string.h>

#include "nl80211.h"
#include "iw.h"

SECTION(ocb);

static int join_ocb(struct nl80211_state *state,
		    struct nl_msg *msg, int argc, char **argv,
		    enum id_input id)
{
	struct chandef chandef;
	int err, parsed;

	if (argc < 2)
		return 1;

	err = parse_freqchan(&chandef, false, argc, argv, &parsed, false);

	if (err)
		return err;

	err = put_chandef(msg, &chandef);
	if (err)
		return err;

	return 0;
}
COMMAND(ocb, join, "<freq in MHz> <5MHz|10MHz>",
	NL80211_CMD_JOIN_OCB, 0, CIB_NETDEV, join_ocb,
	"Join the OCB mode network.");

static int leave_ocb(struct nl80211_state *state,
		     struct nl_msg *msg, int argc, char **argv,
		     enum id_input id)
{
	if (argc)
		return 1;

	return 0;
}
COMMAND(ocb, leave, NULL, NL80211_CMD_LEAVE_OCB, 0, CIB_NETDEV, leave_ocb,
	"Leave the OCB mode network.");
