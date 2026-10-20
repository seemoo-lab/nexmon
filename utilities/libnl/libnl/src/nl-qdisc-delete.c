/*
 * src/nl-qdisc-delete.c     Delete Queuing Disciplines
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

static int quiet = 0, default_yes = 0, deleted = 0, interactive = 0;
struct nl_sock *sock;

static void print_usage(void)
{
	printf(
	"Usage: nl-qdisc-delete [OPTION]... [QDISC]\n"
	"\n"
	"Options\n"
	" -i, --interactive     Run interactively\n"
	"     --yes             Set default answer to yes\n"
	" -q, --quiet           Do not print informal notifications\n"
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

static void delete_cb(struct nl_object *obj, void *arg)
{
	struct rtnl_qdisc *qdisc = nl_object_priv(obj);
	struct nl_dump_params params = {
		.dp_type = NL_DUMP_LINE,
		.dp_fd = stdout,
	};
	int err;

	if (interactive && !nl_cli_confirm(obj, &params, default_yes))
		return;

	if ((err = rtnl_qdisc_delete(sock, qdisc)) < 0)
		nl_cli_fatal(err, "Unable to delete qdisc: %s\n", nl_geterror(err));

	if (!quiet) {
		printf("Deleted ");
		nl_object_dump(obj, &params);
	}

	deleted++;
}

int main(int argc, char *argv[])
{
	struct rtnl_qdisc *qdisc;
	struct nl_cache *link_cache, *qdisc_cache;
 
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
			{ "interactive", 0, 0, 'i' },
			{ "yes", 0, 0, ARG_YES },
			{ "quiet", 0, 0, 'q' },
			{ "help", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ "dev", 1, 0, 'd' },
			{ "parent", 1, 0, 'p' },
			{ "handle", 1, 0, 'H' },
			{ "kind", 1, 0, 'k' },
			{ 0, 0, 0, 0 }
		};
	
		c = getopt_long(argc, argv, "iqhvd:p:H:k:", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'i': interactive = 1; break;
		case ARG_YES: default_yes = 1; break;
		case 'q': quiet = 1; break;
		case 'h': print_usage(); break;
		case 'v': nl_cli_print_version(); break;
		case 'd': nl_cli_qdisc_parse_dev(qdisc, link_cache, optarg); break;
		case 'p': nl_cli_qdisc_parse_parent(qdisc, optarg); break;
		case 'H': nl_cli_qdisc_parse_handle(qdisc, optarg); break;
		case 'k': nl_cli_qdisc_parse_kind(qdisc, optarg); break;
		}
 	}

	nl_cache_foreach_filter(qdisc_cache, OBJ_CAST(qdisc), delete_cb, NULL);

	if (!quiet)
		printf("Deleted %d qdiscs\n", deleted);

	return 0;
}
