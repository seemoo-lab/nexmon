/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#ifndef __LIB_IDIAG_NL_IDIAG_H__
#define __LIB_IDIAG_NL_IDIAG_H__

#include "nl-priv-dynamic-core/object-api.h"

struct idiagnl_meminfo {
	NLHDR_COMMON

	uint32_t idiag_rmem;
	uint32_t idiag_wmem;
	uint32_t idiag_fmem;
	uint32_t idiag_tmem;
};

struct idiagnl_vegasinfo {
	NLHDR_COMMON

	uint32_t tcpv_enabled;
	uint32_t tcpv_rttcnt;
	uint32_t tcpv_rtt;
	uint32_t tcpv_minrtt;
};

#endif /* __LIB_IDIAG_NL_IDIAG_H__ */
