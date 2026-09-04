/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/classifier.h>
#include <netlink/route/cls/police.h>

#include "nl-priv-dynamic-core/nl-core.h"

/**
 * @name Policer Type
 * @{
 */

static const struct trans_tbl police_types[] = {
	__ADD(TC_POLICE_UNSPEC,unspec),
	__ADD(TC_POLICE_OK,ok),
	__ADD(TC_POLICE_RECLASSIFY,reclassify),
	__ADD(TC_POLICE_SHOT,shot),
#ifdef TC_POLICE_PIPE
	__ADD(TC_POLICE_PIPE,pipe),
#endif
};

/**
 * Transform a policer type number into a character string (Reentrant).
 * @arg type		policer type
 * @arg buf		destination buffer
 * @arg len		buffer length
 *
 * Transforms a policer type number into a character string and stores
 * it in the provided buffer.
 *
 * @return The destination buffer or the type encoded in hex if no match was found.
 */
char * nl_police2str(int type, char *buf, size_t len)
{
	return __type2str(type, buf, len, police_types,
			  ARRAY_SIZE(police_types));
}

/**
 * Transform a character string into a policer type number
 * @arg name		policer type name
 *
 * Transform the provided character string specifying a policer
 * type into the corresponding numeric value
 *
 * @return Policer type number or a negative value.
 */
int nl_str2police(const char *name)
{
	return __str2type(name, police_types, ARRAY_SIZE(police_types));
}

/** @} */
