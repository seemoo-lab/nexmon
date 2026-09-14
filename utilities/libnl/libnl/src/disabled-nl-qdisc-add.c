/*
 * src/nl-qdisc-dump.c     Dump qdisc attributes
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#include "utils.h"
#include <netlink/route/sch/fifo.h>
#include <netlink/route/sch/prio.h>

static void print_usage(void)
{
	printf(
"Usage: nl-qdisc-add <ifindex> <handle> <parent> <kind>\n");
	exit(1);
}

static int parse_blackhole_opts(struct rtnl_qdisc *qdisc, char *argv[],
				int argc)
{
	return 0;
}

static int parse_pfifo_opts(struct rtnl_qdisc *qdisc, char *argv[], int argc)
{
	int err, limit;

	if (argc > 0) {
		if (argc != 2 || strcasecmp(argv[0], "limit")) {
			fprintf(stderr, "Usage: ... pfifo limit <limit>\n");
			return -1;
		}

		limit = strtoul(argv[1], NULL, 0);
		err = rtnl_qdisc_fifo_set_limit(qdisc, limit);
		if (err < 0) {
			fprintf(stderr, "%s\n", nl_geterror());
			return -1;
		}
	}

	return 0;
}

static int parse_bfifo_opts(struct rtnl_qdisc *qdisc, char *argv[], int argc)
{
	int err, limit;

	if (argc > 0) {
		if (argc != 2 || strcasecmp(argv[0], "limit")) {
			fprintf(stderr, "Usage: ... bfifo limit <limit>\n");
			return -1;
		}

		limit = nl_size2int(argv[1]);
		if (limit < 0) {
			fprintf(stderr, "Invalid value for limit.\n");
			return -1;
		}

		err = rtnl_qdisc_fifo_set_limit(qdisc, limit);
		if (err < 0) {
			fprintf(stderr, "%s\n", nl_geterror());
			return -1;
		}
	}

	return 0;
}

static int parse_prio_opts(struct rtnl_qdisc *qdisc, char *argv[], int argc)
{
	int i, err, bands;
	uint8_t map[] = QDISC_PRIO_DEFAULT_PRIOMAP;

	if (argc > 0) {
		if (argc < 2 || strcasecmp(argv[0], "bands"))
			goto usage;

		bands = strtoul(argv[1], NULL, 0);
		err = rtnl_qdisc_prio_set_bands(qdisc, bands);
		if (err < 0) {
			fprintf(stderr, "%s\n", nl_geterror());
			return -1;
		}
	}

	if (argc > 2) {
		if (argc < 5 || strcasecmp(argv[2], "map"))
			goto usage;

		for (i = 3; i < (argc & ~1U); i += 2) {
			int prio, band;

			prio = rtnl_str2prio(argv[i]);
			if (prio < 0 || prio > sizeof(map)/sizeof(map[0])) {
				fprintf(stderr, "Invalid priority \"%s\"\n",
					argv[i]);
				return -1;
			}

			band = strtoul(argv[i+1], NULL, 0);

			map[prio] = band;
		}
	}

	err = rtnl_qdisc_prio_set_priomap(qdisc, map, sizeof(map));
	if (err < 0) {
		fprintf(stderr, "%s\n", nl_geterror());
		return -1;
	}

	return 0;
usage:
	fprintf(stderr, "Usage: ... prio bands <nbands> map MAP\n"
			"MAP := <prio> <band>\n");
	return -1;
}

int main(int argc, char *argv[])
{
	struct nl_sock *nlh;
	struct rtnl_qdisc *qdisc;
	uint32_t handle, parent;
	int err = 1;

	if (nltool_init(argc, argv) < 0)
		return -1;

	if (argc < 5 || !strcmp(argv[1], "-h"))
		print_usage();

	nlh = nltool_alloc_handle();
	if (!nlh)
		goto errout;

	qdisc = rtnl_qdisc_alloc();
	if (!qdisc)
		goto errout_free_handle;

	rtnl_qdisc_set_ifindex(qdisc, strtoul(argv[1], NULL, 0));

	if (rtnl_tc_str2handle(argv[2], &handle) < 0) {
		fprintf(stderr, "%s\n", nl_geterror());
		goto errout_free_qdisc;
	}

	if (rtnl_tc_str2handle(argv[3], &parent) < 0) {
		fprintf(stderr, "%s\n", nl_geterror());
		goto errout_free_qdisc;
	}

	rtnl_qdisc_set_handle(qdisc, handle);
	rtnl_qdisc_set_parent(qdisc, parent);
	rtnl_qdisc_set_kind(qdisc, argv[4]);

	if (!strcasecmp(argv[4], "blackhole"))
		err = parse_blackhole_opts(qdisc, &argv[5], argc-5);
	else if (!strcasecmp(argv[4], "pfifo"))
		err = parse_pfifo_opts(qdisc, &argv[5], argc-5);
	else if (!strcasecmp(argv[4], "bfifo"))
		err = parse_bfifo_opts(qdisc, &argv[5], argc-5);
	else if (!strcasecmp(argv[4], "prio"))
		err = parse_prio_opts(qdisc, &argv[5], argc-5);
	else {
		fprintf(stderr, "Unknown qdisc \"%s\"\n", argv[4]);
		goto errout_free_qdisc;
	}

	if (err < 0)
		goto errout_free_qdisc;

	if (nltool_connect(nlh, NETLINK_ROUTE) < 0)
		goto errout_free_qdisc;

	if (rtnl_qdisc_add(nlh, qdisc, NLM_F_REPLACE) < 0) {
		fprintf(stderr, "Unable to add Qdisc: %s\n", nl_geterror());
		goto errout_close;
	}

	err = 0;
errout_close:
	nl_close(nlh);
errout_free_qdisc:
	rtnl_qdisc_put(qdisc);
errout_free_handle:
	nl_handle_destroy(nlh);
errout:
	return err;
}
