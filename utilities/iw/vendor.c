#include <errno.h>
#include <string.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "iw.h"

SECTION(vendor);

static int print_vendor_response(struct nl_msg *msg, void *arg)
{
	struct nlattr *attr;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	bool print_ascii = (bool) arg;
	uint8_t *data;
	int len;

	attr = nla_find(genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0),
			NL80211_ATTR_VENDOR_DATA);
	if (!attr) {
		fprintf(stderr, "vendor data attribute missing!\n");
		return NL_SKIP;
	}

	data = (uint8_t *) nla_data(attr);
	len = nla_len(attr);

	if (print_ascii)
		iw_hexdump("vendor response", data, len);
	else
		fwrite(data, 1, len, stdout);

	return NL_OK;
}

static int read_file(FILE *file, char *buf, size_t size)
{
	size_t count = 0;
	int data;

	while ((data = fgetc(file)) != EOF) {
		if (count >= size)
			return -EINVAL;
		buf[count] = data;
		count++;
	}

	return count;
}

static int read_hex(unsigned int argc, char **argv, char *buf, size_t size)
{
	unsigned int i, data;
	int res;

	if (argc > size)
		return -EINVAL;

	for (i = 0; i < argc; i++) {
		res = sscanf(argv[i], "0x%x", &data);
		if (res != 1 || data > 0xff)
			return -EINVAL;
		buf[i] = data;
	}

	return argc;
}

static int handle_vendor(struct nl80211_state *state,
			 struct nl_msg *msg, int argc, char **argv,
			 enum id_input id)
{
	unsigned int oui;
	unsigned int subcmd;
	char buf[2048] = {};
	int res, count = 0;
	FILE *file = NULL;

	if (argc < 3)
		return 1;

	res = sscanf(argv[0], "0x%x", &oui);
	if (res != 1) {
		printf("Vendor command must start with 0x\n");
		return 2;
	}

	res = sscanf(argv[1], "0x%x", &subcmd);
	if (res != 1) {
		printf("Sub command must start with 0x\n");
		return 2;
	}

	if (!strcmp(argv[2], "-"))
		file = stdin;
	else
		file = fopen(argv[2], "r");

	NLA_PUT_U32(msg, NL80211_ATTR_VENDOR_ID, oui);
	NLA_PUT_U32(msg, NL80211_ATTR_VENDOR_SUBCMD, subcmd);

	if (file) {
		count = read_file(file, buf, sizeof(buf));
		if (file != stdin)
			fclose(file);
	} else
		count = read_hex(argc - 2, &argv[2], buf, sizeof(buf));

	if (count < 0)
		return -EINVAL;

	if (count > 0)
		NLA_PUT(msg, NL80211_ATTR_VENDOR_DATA, count, buf);

	return 0;

nla_put_failure:
	if (file && file != stdin)
		fclose(file);
	return -ENOBUFS;
}

static int handle_vendor_recv(struct nl80211_state *state,
			      struct nl_msg *msg, int argc,
			      char **argv, enum id_input id)
{
	register_handler(print_vendor_response, (void *) true);
	return handle_vendor(state, msg, argc, argv, id);
}

static int handle_vendor_recv_bin(struct nl80211_state *state,
				  struct nl_msg *msg, int argc,
				  char **argv, enum id_input id)
{
	register_handler(print_vendor_response, (void *) false);
	return handle_vendor(state, msg, argc, argv, id);
}

COMMAND(vendor, send, "<oui> <subcmd> <filename|-|hex data>", NL80211_CMD_VENDOR, 0, CIB_NETDEV, handle_vendor, "");
COMMAND(vendor, recv, "<oui> <subcmd> <filename|-|hex data>", NL80211_CMD_VENDOR, 0, CIB_NETDEV, handle_vendor_recv, "");
COMMAND(vendor, recvbin, "<oui> <subcmd> <filename|-|hex data>", NL80211_CMD_VENDOR, 0, CIB_NETDEV, handle_vendor_recv_bin, "");
