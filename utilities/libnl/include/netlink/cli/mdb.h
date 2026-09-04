/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef __NETLINK_CLI_MDB_H_
#define __NETLINK_CLI_MDB_H_

#include <netlink/route/mdb.h>

#define nl_cli_mdb_alloc_cache(sk) \
		nl_cli_alloc_cache_flags((sk), "mdb", NL_CACHE_AF_ITER, rtnl_mdb_alloc_cache)

#endif
