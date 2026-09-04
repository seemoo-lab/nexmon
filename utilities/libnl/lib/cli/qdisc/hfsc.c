/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Cong Wang <xiyou.wangcong@gmail.com>
 */

#include "nl-default.h"

#include <linux/pkt_sched.h>

#include <netlink/cli/utils.h>
#include <netlink/cli/tc.h>
#include <netlink/route/qdisc/hfsc.h>

static void print_qdisc_usage(void)
{
	printf(
"Usage: nl-qdisc-add [...] hfsc [OPTIONS]...\n"
"\n"
"OPTIONS\n"
"     --help                Show this help text.\n"
"     --default=ID          Default class for unclassified traffic.\n"
"\n"
"EXAMPLE"
"    # Create hfsc root qdisc 1: and direct unclassified traffic to class 1:10\n"
"    nl-qdisc-add --dev=eth1 --parent=root --handle=1: hfsc --default=10\n");
}

static void hfsc_parse_qdisc_argv(struct rtnl_tc *tc, int argc, char **argv)
{
	struct rtnl_qdisc *qdisc = (struct rtnl_qdisc *) tc;

	for (;;) {
		int c, optidx = 0;
		enum {
			ARG_DEFAULT = 257,
		};
		static struct option long_opts[] = {
			{ "help", 0, 0, 'h' },
			{ "default", 1, 0, ARG_DEFAULT },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "hv", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_qdisc_usage();
			return;

		case ARG_DEFAULT:
			rtnl_qdisc_hfsc_set_defcls(qdisc, nl_cli_parse_u32(optarg));
			break;
		}
	}
}

static void print_class_usage(void)
{
	printf(
"Usage: nl-class-add [...] hfsc [OPTIONS]...\n"
"\n"
"OPTIONS\n"
"     --help                Show this help text.\n"
"     --ls=SC               Link-sharing service curve\n"
"     --rt=SC               Real-time service curve\n"
"     --sc=SC               Specifiy both of the above\n"
"     --ul=SC               Upper limit\n"
"     where SC := [ [ m1 bits ] d usec ] m2 bits\n"
"\n"
"EXAMPLE"
"    # Attach class 1:1 to hfsc qdisc 1: and use rt and ls curve\n"
"    nl-class-add --dev=eth1 --parent=1: --classid=1:1 hfsc --sc=m1:250,d:8,m2:100\n");
}

static int
hfsc_get_sc(char *optarg, struct tc_service_curve *sc)
{
	unsigned int m1 = 0, d = 0, m2 = 0;
	char *tmp = strdup(optarg);
	char *p, *endptr;
	char *pp = tmp;

	if (!tmp)
		return -ENOMEM;

	p = strstr(pp, "m1:");
	if (p) {
		char *q;
		p += 3;
		if (*p == 0)
			goto err;
		q = strchr(p, ',');
		if (!q)
			goto err;
		*q = 0;
		m1 = strtoul(p, &endptr, 10);
		if (endptr == p)
			goto err;
		pp = q + 1;
	}

	p = strstr(pp, "d:");
	if (p) {
		char *q;
		p += 2;
		if (*p == 0)
			goto err;
		q = strchr(p, ',');
		if (!q)
			goto err;
		*q = 0;
		d = strtoul(p, &endptr, 10);
		if (endptr == p)
			goto err;
		pp = q + 1;
	}

	p = strstr(pp, "m2:");
	if (p) {
		p += 3;
		if (*p == 0)
			goto err;
		m2 = strtoul(p, &endptr, 10);
		if (endptr == p)
			goto err;
	} else
		goto err;

	free(tmp);
	sc->m1 = m1;
	sc->d  = d;
	sc->m2 = m2;
	return 0;

err:
	free(tmp);
	return -EINVAL;
}

static void hfsc_parse_class_argv(struct rtnl_tc *tc, int argc, char **argv)
{
	struct rtnl_class *class = (struct rtnl_class *) tc;
	int arg_ok = 0, ret = -EINVAL;

	for (;;) {
		int c, optidx = 0;
		enum {
			ARG_RT = 257,
			ARG_LS = 258,
			ARG_SC,
			ARG_UL,
		};
		static struct option long_opts[] = {
			{ "help", 0, 0, 'h' },
			{ "rt", 1, 0, ARG_RT },
			{ "ls", 1, 0, ARG_LS },
			{ "sc", 1, 0, ARG_SC },
			{ "ul", 1, 0, ARG_UL },
			{ 0, 0, 0, 0 }
		};
		struct tc_service_curve tsc;

		c = getopt_long(argc, argv, "h", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_class_usage();
			return;

		case ARG_RT:
			ret = hfsc_get_sc(optarg, &tsc);
			if (ret < 0) {
				nl_cli_fatal(ret, "Unable to parse sc "
					"\"%s\": Invalid format.", optarg);
			}

			rtnl_class_hfsc_set_rsc(class, &tsc);
			arg_ok++;
			break;

		case ARG_LS:
			ret = hfsc_get_sc(optarg, &tsc);
			if (ret < 0) {
				nl_cli_fatal(ret, "Unable to parse sc "
					"\"%s\": Invalid format.", optarg);
			}

			rtnl_class_hfsc_set_fsc(class, &tsc);
			arg_ok++;
			break;

		case ARG_SC:
			ret = hfsc_get_sc(optarg, &tsc);
			if (ret < 0) {
				nl_cli_fatal(ret, "Unable to parse sc "
					"\"%s\": Invalid format.", optarg);
			}

			rtnl_class_hfsc_set_rsc(class, &tsc);
			rtnl_class_hfsc_set_fsc(class, &tsc);
			arg_ok++;
			break;

		case ARG_UL:
			ret = hfsc_get_sc(optarg, &tsc);
			if (ret < 0) {
				nl_cli_fatal(ret, "Unable to parse sc "
					"\"%s\": Invalid format.", optarg);
			}

			rtnl_class_hfsc_set_usc(class, &tsc);
			arg_ok++;
			break;
		}
	}

	if (!arg_ok)
		nl_cli_fatal(ret, "Invalid arguments");
}

static struct nl_cli_tc_module hfsc_qdisc_module =
{
	.tm_name		= "hfsc",
	.tm_type		= RTNL_TC_TYPE_QDISC,
	.tm_parse_argv		= hfsc_parse_qdisc_argv,
};

static struct nl_cli_tc_module hfsc_class_module =
{
	.tm_name		= "hfsc",
	.tm_type		= RTNL_TC_TYPE_CLASS,
	.tm_parse_argv		= hfsc_parse_class_argv,
};

static void _nl_init hfsc_init(void)
{
	nl_cli_tc_register(&hfsc_qdisc_module);
	nl_cli_tc_register(&hfsc_class_module);
}

static void _nl_exit hfsc_exit(void)
{
	nl_cli_tc_unregister(&hfsc_class_module);
	nl_cli_tc_unregister(&hfsc_qdisc_module);
}
