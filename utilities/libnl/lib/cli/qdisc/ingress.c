/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Cong Wang <xiyou.wangcong@gmail.com>
 */

#include "nl-default.h"

#include <netlink/cli/utils.h>
#include <netlink/cli/tc.h>

static void print_usage(void)
{
	printf(
"Usage: nl-qdisc-add [...] ingress\n"
"\n"
"OPTIONS\n"
"     --help                Show this help text.\n"
"\n"
"EXAMPLE"
"    # Attach ingress to eth1\n"
"    nl-qdisc-add --dev=eth1 --parent=root ingress\n");
}

static void ingress_parse_argv(struct rtnl_tc *tc, int argc, char **argv)
{
	for (;;) {
		int c, optidx = 0;
		static struct option long_opts[] = {
			{ "help", 0, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "h", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_usage();
			return;
		}
	}
}

static struct nl_cli_tc_module ingress_module =
{
	.tm_name		= "ingress",
	.tm_type		= RTNL_TC_TYPE_QDISC,
	.tm_parse_argv		= ingress_parse_argv,
};

static void _nl_init ingress_init(void)
{
	nl_cli_tc_register(&ingress_module);
}

static void _nl_exit ingress_exit(void)
{
	nl_cli_tc_unregister(&ingress_module);
}
