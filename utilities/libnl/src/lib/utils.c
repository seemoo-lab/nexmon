/*
 * src/utils.c		Utilities
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2009 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @defgroup cli Command Line Interface API
 *
 * @{
 */

#include <netlink/cli/utils.h>

uint32_t nl_cli_parse_u32(const char *arg)
{
	unsigned long lval;
	char *endptr;

	lval = strtoul(arg, &endptr, 0);
	if (endptr == arg || lval == ULONG_MAX)
		nl_cli_fatal(EINVAL, "Unable to parse \"%s\", not a number.",
			     arg);

	return (uint32_t) lval;
}

void nl_cli_print_version(void)
{
	printf("libnl tools version %s\n", LIBNL_VERSION);
	printf(
	"Copyright (C) 2003-2009 Thomas Graf <tgraf@redhat.com>\n"
	"\n"
	"This program comes with ABSOLUTELY NO WARRANTY. This is free \n"
	"software, and you are welcome to redistribute it under certain\n"
	"conditions. See the GNU General Public License for details.\n"
	);

	exit(0);
}

void nl_cli_fatal(int err, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "Error: ");

	if (fmt) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	} else
		fprintf(stderr, "%s\n", strerror(err));

	exit(abs(err));
}

int nl_cli_connect(struct nl_sock *sk, int protocol)
{
	int err;

	if ((err = nl_connect(sk, protocol)) < 0)
		nl_cli_fatal(err, "Unable to connect netlink socket: %s",
			     nl_geterror(err));

	return err;
}

struct nl_sock *nl_cli_alloc_socket(void)
{
	struct nl_sock *sock;

	if (!(sock = nl_socket_alloc()))
		nl_cli_fatal(ENOBUFS, "Unable to allocate netlink socket");

	return sock;
}

struct nl_addr *nl_cli_addr_parse(const char *str, int family)
{
	struct nl_addr *addr;
	int err;

	if ((err = nl_addr_parse(str, family, &addr)) < 0)
		nl_cli_fatal(err, "Unable to parse address \"%s\": %s",
			     str, nl_geterror(err));

	return addr;
}

int nl_cli_parse_dumptype(const char *str)
{
	if (!strcasecmp(str, "brief"))
		return NL_DUMP_LINE;
	else if (!strcasecmp(str, "details") || !strcasecmp(str, "detailed"))
		return NL_DUMP_DETAILS;
	else if (!strcasecmp(str, "stats"))
		return NL_DUMP_STATS;
	else if (!strcasecmp(str, "env"))
		return NL_DUMP_ENV;
	else
		nl_cli_fatal(EINVAL, "Invalid dump type \"%s\".\n", str);

	return 0;
}

int nl_cli_confirm(struct nl_object *obj, struct nl_dump_params *params,
		   int default_yes)
{
	int answer;

	nl_object_dump(obj, params);
	printf("Delete? (%c/%c) ",
		default_yes ? 'Y' : 'y',
		default_yes ? 'n' : 'N');

	do {
		answer = tolower(getchar());
		if (answer == '\n')
			answer = default_yes ? 'y' : 'n';
	} while (answer != 'y' && answer != 'n');

	return answer == 'y';
}

struct nl_cache *nl_cli_alloc_cache(struct nl_sock *sock, const char *name,
			    int (*ac)(struct nl_sock *, struct nl_cache **))
{
	struct nl_cache *cache;
	int err;

	if ((err = ac(sock, &cache)) < 0)
		nl_cli_fatal(err, "Unable to allocate %s cache: %s",
			     name, nl_geterror(err));

	nl_cache_mngt_provide(cache);

	return cache;
}

/** @} */
