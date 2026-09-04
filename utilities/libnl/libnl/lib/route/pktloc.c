/*
 * lib/route/pktloc.c     Packet Location Aliasing
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation version 2 of the License.
 *
 * Copyright (c) 2008-2010 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup tc
 * @defgroup pktloc Packet Location Aliasing
 * Packet Location Aliasing
 *
 * The packet location aliasing interface eases the use of offset definitions
 * inside packets by allowing them to be referenced by name. Known positions
 * of protocol fields are stored in a configuration file and associated with
 * a name for later reference. The configuration file is distributed with the
 * library and provides a well defined set of definitions for most common
 * protocol fields.
 *
 * @subsection pktloc_examples Examples
 * @par Example 1.1 Looking up a packet location
 * @code
 * struct rtnl_pktloc *loc;
 *
 * rtnl_pktloc_lookup("ip.src", &loc);
 * @endcode
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/pktloc.h>

#include "pktloc_syntax.h"
#include "pktloc_grammar.h"

/** @cond */
#define PKTLOC_NAME_HT_SIZ 256

static struct nl_list_head pktloc_name_ht[PKTLOC_NAME_HT_SIZ];

/* djb2 */
unsigned int pktloc_hash(const char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash % PKTLOC_NAME_HT_SIZ;
}


void rtnl_pktloc_add(struct rtnl_pktloc *loc)
{
	nl_list_add_tail(&loc->list, &pktloc_name_ht[pktloc_hash(loc->name)]);
}

extern int pktloc_parse(void *scanner);

/** @endcond */

static void rtnl_pktloc_free(struct rtnl_pktloc *loc)
{
	if (!loc)
		return;

	free(loc->name);
	free(loc);
}

static int read_pktlocs(void)
{
	YY_BUFFER_STATE buf;
	yyscan_t scanner = NULL;
	static time_t last_read;
	struct stat st = {0};
	char *path;
	int i, err;
	FILE *fd;

	asprintf(&path, "%s/pktloc", SYSCONFDIR);

	/* if stat fails, just try to read the file */
	if (stat(path, &st) == 0) {
		/* Don't re-read file if file is unchanged */
		if (last_read == st.st_mtime)
			return 0;
	}

	if (!(fd = fopen(path, "r")))
		return -NLE_PKTLOC_FILE;

	for (i = 0; i < PKTLOC_NAME_HT_SIZ; i++) {
		struct rtnl_pktloc *loc, *n;

		nl_list_for_each_entry_safe(loc, n, &pktloc_name_ht[i], list)
			rtnl_pktloc_free(loc);

		nl_init_list_head(&pktloc_name_ht[i]);
	}

	if ((err = pktloc_lex_init(&scanner)) < 0)
		return -NLE_FAILURE;

	buf = pktloc__create_buffer(fd, YY_BUF_SIZE, scanner);
	pktloc__switch_to_buffer(buf, scanner);

	if ((err = pktloc_parse(scanner)) < 0)
		return -NLE_FAILURE;

	if (scanner)
		pktloc_lex_destroy(scanner);

	free(path);
	last_read = st.st_mtime;

	return 0;
}

/**
 * Lookup packet location alias
 * @arg name		Name of packet location.
 *
 * Tries to find a matching packet location alias for the supplied
 * packet location name.
 *
 * The file containing the packet location definitions is automatically
 * re-read if its modification time has changed since the last call.
 *
 * @return 0 on success or a negative error code.
 * @retval NLE_PKTLOC_FILE Unable to open packet location file.
 * @retval NLE_OBJ_NOTFOUND No matching packet location alias found.
 */
int rtnl_pktloc_lookup(const char *name, struct rtnl_pktloc **result)
{
	struct rtnl_pktloc *loc;
	int hash, err;

	if ((err = read_pktlocs()) < 0)
		return err;

	hash = pktloc_hash(name);
	nl_list_for_each_entry(loc, &pktloc_name_ht[hash], list) {
		if (!strcasecmp(loc->name, name)) {
			*result = loc;
			return 0;
		}
	}

	return -NLE_OBJ_NOTFOUND;
}

static int __init pktloc_init(void)
{
	int i;

	for (i = 0; i < PKTLOC_NAME_HT_SIZ; i++)
		nl_init_list_head(&pktloc_name_ht[i]);
	
	return 0;
}
