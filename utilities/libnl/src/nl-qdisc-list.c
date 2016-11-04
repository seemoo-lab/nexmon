/*
 * src/nl-qdisc-list.c     List Qdiscs
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2009 Thomas Graf <tgraf@suug.ch>
 */

#include <netlink/cli/utils.h>
#include <netlink/cli/qdisc.h>
#include <netlink/cli/link.h>

static int quiet = 0;

static void print_usage(void)
{
	printf(
	"Usage: nl-qdisc-list [OPTION]... [QDISC]\n"
	"\n"
	"Options\n"
	" -f, --format=TYPE     Output format { brief | details | stats }\n"
	" -h, --help            Show this help\n"
	" -v, --version         Show versioning information\n"
	"\n"
	"QDisc Options\n"
	" -d, --dev=DEV         Device the qdisc is attached to\n"
	" -p, --parent=HANDLE   Identifier of parent qdisc\n"
	" -H, --handle=HANDLE   Identifier\n"
	" -k, --kind=NAME       Kind of qdisc (e.g. pfifo_fast)\n"
	);
	exit(0);
}

int main(int argc, char *argv[])
{
	struct nl_sock *sock;
	struct rtnl_qdisc *qdisc;
	struct nl_cache *link_cache, *qdisc_cache;
	struct nl_dump_params params = {
		.dp_type = NL_DUMP_LINE,
		.dp_fd = stdout,
	};
 
	sock = nl_cli_alloc_socket();
	nl_cli_connect(sock, NETLINK_ROUTE);
	link_cache = nl_cli_link_alloc_cache(sock);
	qdisc_cache = nl_cli_qdisc_alloc_cache(sock);
 	qdisc = nl_cli_qdisc_alloc();
 
	for (;;) {
		int c, optidx = 0;
		enum {
			ARG_YES = 257,
		};
		static struct option long_opts[] = {
			{ "format", 1, 0, 'f' },
			{ "quiet", 0, 0, 'q' },
			{ "help", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ "dev", 1, 0, 'd' },
			{ "parent", 1, 0, 'p' },
			{ "handle", 1, 0, 'H' },
			{ "kind", 1, 0, 'k' },
			{ 0, 0, 0, 0 }
		};
	
		c = getopt_long(argc, argv, "f:qhvd:p:H:k:",
				long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'f': params.dp_type = nl_cli_parse_dumptype(optarg); break;
		case 'q': quiet = 1; break;
		case 'h': print_usage(); break;
		case 'v': nl_cli_print_version(); break;
		case 'd': nl_cli_qdisc_parse_dev(qdisc, link_cache, optarg); break;
		case 'p': nl_cli_qdisc_parse_parent(qdisc, optarg); break;
		case 'H': nl_cli_qdisc_parse_handle(qdisc, optarg); break;
		case 'k': nl_cli_qdisc_parse_kind(qdisc, optarg); break;
		}
 	}

	nl_cache_dump_filter(qdisc_cache, &params, OBJ_CAST(qdisc));

	return 0;
}
