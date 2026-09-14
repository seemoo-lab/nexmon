/*
 * src/nl-cls-delete.c     Delete Classifier
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008 Thomas Graf <tgraf@suug.ch>
 */

#include "cls/utils.h"

static int interactive = 0, default_yes = 0, quiet = 0;
static int deleted = 0;
static struct nl_sock *sock;

static void print_usage(void)
{
	printf(
	"Usage: nl-cls-list [OPTION]... [CLASSIFIER]\n"
	"\n"
	"Options\n"
	" -i, --interactive     Run interactively\n"
	"     --yes             Set default answer to yes\n"
	" -q, --quiet		Do not print informal notifications\n"
	" -h, --help            Show this help\n"
	" -v, --version		Show versioning information\n"
	"\n"
	"Classifier Options\n"
	" -d, --dev=DEV         Device the classifier should be assigned to.\n"
	" -p, --parent=HANDLE   Parent qdisc/class\n"
	"     --proto=PROTO     Protocol\n"
	"     --prio=NUM        Priority (0..256)\n"
	"     --id=HANDLE       Unique identifier\n"
	);
	exit(0);
}

static void delete_cb(struct nl_object *obj, void *arg)
{
	struct rtnl_cls *cls = (struct rtnl_cls *) obj;
	struct nl_dump_params params = {
		.dp_type = NL_DUMP_LINE,
		.dp_fd = stdout,
	};
	int err;

	if (interactive && !nlt_confirm(obj, &params, default_yes))
		return;

	if ((err = rtnl_cls_delete(sock, cls, 0)) < 0)
		fatal(err, "Unable to delete classifier: %s",
		      nl_geterror(err));

	if (!quiet) {
		printf("Deleted ");
		nl_object_dump(obj, &params);
	}

	deleted++;
}

int main(int argc, char *argv[])
{
	struct nl_cache *link_cache, *cls_cache;
	struct rtnl_cls *cls;
	int nf = 0, err;

	sock = nlt_alloc_socket();
	nlt_connect(sock, NETLINK_ROUTE);
	link_cache = nlt_alloc_link_cache(sock);
	cls = nlt_alloc_cls();

	for (;;) {
		int c, optidx = 0;
		enum {
			ARG_PRIO = 257,
			ARG_PROTO = 258,
			ARG_ID,
			ARG_YES,
		};
		static struct option long_opts[] = {
			{ "interactive", 0, 0, 'i' },
			{ "yes", 0, 0, ARG_YES },
			{ "quiet", 0, 0, 'q' },
			{ "help", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ "dev", 1, 0, 'd' },
			{ "parent", 1, 0, 'p' },
			{ "proto", 1, 0, ARG_PROTO },
			{ "prio", 1, 0, ARG_PRIO },
			{ "id", 1, 0, ARG_ID },
			{ 0, 0, 0, 0 }
		};
	
		c = getopt_long(argc, argv, "iqhvd:p:", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'i': interactive = 1; break;
		case ARG_YES: default_yes = 1; break;
		case 'q': quiet = 1; break;
		case 'h': print_usage(); break;
		case 'v': nlt_print_version(); break;
		case 'd': nf++; parse_dev(cls, link_cache, optarg); break;
		case 'p': nf++; parse_parent(cls, optarg); break;
		case ARG_PRIO: nf++; parse_prio(cls, optarg); break;
		case ARG_ID: nf++; parse_handle(cls, optarg); break;
		case ARG_PROTO: nf++; parse_proto(cls, optarg); break;
		}
	}

	if (nf == 0 && !interactive && !default_yes) {
		fprintf(stderr, "You attempted to delete all classifiers in "
			"non-interactive mode, aborting.\n");
		exit(0);
	}

	err = rtnl_cls_alloc_cache(sock, rtnl_cls_get_ifindex(cls),
				   rtnl_cls_get_parent(cls), &cls_cache);
	if (err < 0)
		fatal(err, "Unable to allocate classifier cache: %s",
		      nl_geterror(err));

	nl_cache_foreach_filter(cls_cache, OBJ_CAST(cls), delete_cb, NULL);

	if (!quiet)
		printf("Deleted %d classifiers\n", deleted);

	return 0;
}
