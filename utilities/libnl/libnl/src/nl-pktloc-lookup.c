/*
 * src/nl-pktloc-lookup.c     Lookup packet location alias
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2010 Thomas Graf <tgraf@suug.ch>
 */

#include <netlink/cli/utils.h>
#include <netlink/route/pktloc.h>

static void print_usage(void)
{
	printf("Usage: nl-pktloc-lookup <name>\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	struct rtnl_pktloc *loc;
	int err;

	if (argc < 2)
		print_usage();

	if ((err = rtnl_pktloc_lookup(argv[1], &loc)) < 0)
		nl_cli_fatal(err, "Unable to lookup packet location: %s",
			nl_geterror(err));

	printf("%s: %u %u+%u 0x%x %u\n", loc->name, loc->align,
		loc->layer, loc->offset, loc->mask, loc->flags);

	return 0;
}
