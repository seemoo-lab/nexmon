/*
 * src/nl-cls-add.c     Add classifier
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation version 2 of the License.
 *
 * Copyright (c) 2003-2009 Thomas Graf <tgraf@suug.ch>
 */

#include "cls/utils.h"

static int quiet = 0;

static void print_usage(void)
{
	printf(
"Usage: nl-cls-add [OPTION]... [CLASSIFIER] TYPE [TYPE OPTIONS]...\n"
"\n"
"Options\n"
" -q, --quiet               Do not print informal notifications.\n"
" -h, --help                Show this help.\n"
" -v, --version             Show versioning information.\n"
"\n"
"Classifier Options\n"
" -d, --dev=DEV             Device the classifier should be assigned to.\n"
" -p, --parent=HANDLE       Parent QDisc\n"
"     --proto=PROTO         Protocol (default=IPv4)\n"
"     --prio=NUM            Priority (0..256)\n"
"     --id=HANDLE           Unique identifier\n"
	);
	exit(0);
}

int main(int argc, char *argv[])
{
	struct nl_sock *sock;
	struct rtnl_cls *cls;
	struct nl_cache *link_cache;
	struct rtnl_cls_ops *ops;
	struct cls_module *mod;
	struct nl_dump_params dp = {
		.dp_type = NL_DUMP_DETAILS,
		.dp_fd = stdout,
	};
	char *kind;
	int err, nlflags = NLM_F_CREATE;
 
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
	
		c = getopt_long(argc, argv, "+qhva:d:", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case '?': exit(NLE_INVAL);
		case 'q': quiet = 1; break;
		case 'h': print_usage(); break;
		case 'v': nlt_print_version(); break;
		case 'd': parse_dev(cls, link_cache, optarg); break;
		case 'p': parse_parent(cls, optarg); break;
		case ARG_PRIO: parse_prio(cls, optarg); break;
		case ARG_ID: parse_handle(cls, optarg); break;
		case ARG_PROTO: parse_proto(cls, optarg); break;
		}
 	}

	if (optind >= argc) {
		print_usage();
		fatal(EINVAL, "Missing classifier type");
	}

	kind = argv[optind++];
	if ((err = rtnl_cls_set_kind(cls, kind)) < 0)
		fatal(ENOENT, "Unknown classifier type \"%s\".", kind);
	
	ops = rtnl_cls_get_ops(cls);
	if (!(mod = lookup_cls_mod(ops)))
		fatal(ENOTSUP, "Classifier type \"%s\" not supported.", kind);

	mod->parse_argv(cls, argc, argv);

	printf("Adding ");
	nl_object_dump(OBJ_CAST(cls), &dp);

	if ((err = rtnl_cls_add(sock, cls, nlflags)) < 0)
		fatal(err, "Unable to add classifier: %s", nl_geterror(err));

	if (!quiet) {
		printf("Added ");
		nl_object_dump(OBJ_CAST(cls), &dp);
 	}

	return 0;
}
