/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_ID_PRIVATE_H
#define ISL_ID_PRIVATE_H

#include <isl/id.h>

struct isl_id {
	int ref;
	isl_ctx *ctx;

	const char *name;
	void *user;
	uint32_t hash;
};

uint32_t isl_hash_id(uint32_t hash, __isl_keep isl_id *id);

extern isl_id isl_id_none;

#endif
