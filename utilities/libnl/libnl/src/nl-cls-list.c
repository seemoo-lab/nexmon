/*
 * src/nl-cls-list.c     	List classifiers
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008 Thomas Graf <tgraf@suug.ch>
 */

#include "cls/utils.h"

static struct nl_sock *sock;
static struct rtnl_cls *cls;
static struct nl_dump_params params = {
	.dp_type = NL_DUMP_LINE,
};

static void print_usage(void)
{
	printf(
	"Usage: nl-cls-list [OPTION]... [CLASSIFIER]\n"
	"\n"
	"Options\n"
	" -f, --format=TYPE     Output format { brief | details | stats }\n"
	" -h, --help            Show this help text.\n"
	" -v, --version         Show versioning information.\n"
	"\n"
	"Classifier Options\n"
	" -d, --dev=DEV         Device the classifier should be assigned to.\n"
	" -p, --parent=HANDLE   Parent qdisc/class\n"
	"     --proto=PROTO     Protocol\n"
	"     --prio=NUM        Priority\n"
	"     --id=NUM          Identifier\n"
	);
	exit(0);
}

static void print_cls(struct nl_object *obj, void *arg)
{
	struct nl_cache *cls_cache;
	int err, ifindex;

	if (obj)
		ifindex = rtnl_link_get_ifindex((struct rtnl_link *) obj);
	else
		ifindex = rtnl_cls_get_ifindex(cls);

	err = rtnl_cls_alloc_cache(sock, ifindex, rtnl_cls_get_parent(cls),
				   &cls_cache);
	if (err < 0)
		fatal(err, "Unable to allocate classifier cache: %s",
		      nl_geterror(err));

	nl_cache_dump_filter(cls_cache, &params, OBJ_CAST(cls));
	nl_cache_free(cls_cache);
}

int main(int argc, char *argv[])
{
	struct nl_cache *link_cache;
	int dev = 0;

	params.dp_fd = stdout;
	sock = nlt_alloc_socket();
	nlt_connect(sock, NETLINK_ROUTE);
	link_cache = nlt_alloc_link_cache(sock);
	cls = nlt_alloc_cls();

	for (;;) {
		int c, optidx = 0;
		enum {
			ARG_PROTO = 257,
			ARG_PRIO = 258,
			ARG_ID,
		};
		static struct option long_opts[] = {
			{ "format", 1, 0, 'f' },
			{ "help", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ "dev", 1, 0, 'd' },
			{ "parent", 1, 0, 'p' },
			{ "proto", 1, 0, ARG_PROTO },
			{ "prio", 1, 0, ARG_PRIO },
			{ "id", 1, 0, ARG_ID },
			{ 0, 0, 0, 0 }
		};
	
		c = getopt_long(argc, argv, "+f:qhva:d:", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case '?': exit(NLE_INVAL);
		case 'f': params.dp_type = nlt_parse_dumptype(optarg); break;
		case 'h': print_usage(); break;
		case 'v': nlt_print_version(); break;
		case 'd': dev = 1; parse_dev(cls, link_cache, optarg); break;
		case 'p': parse_parent(cls, optarg); break;
		case ARG_PRIO: parse_prio(cls, optarg); break;
		case ARG_ID: parse_handle(cls, optarg); break;
		case ARG_PROTO: parse_proto(cls, optarg); break;
		}
 	}

	if (!dev)
		nl_cache_foreach(link_cache, print_cls, NULL);
	else
		print_cls(NULL, NULL);

	return 0;
}
