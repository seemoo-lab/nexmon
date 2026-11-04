/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STTYPE_SET_H
#define STTYPE_SET_H

#include <wireshark.h>

#include "syntax-tree.h"

/**
 * @brief Convert a set of values to a range.
 *
 * @param node_left Pointer to the left node of the range.
 * @param node_right Pointer to the right node of the range.
 * @return True if conversion is successful, False otherwise.
 */
bool
sttype_set_convert_to_range(stnode_t **node_left, stnode_t **node_right);

/**
 * @brief Free the memory allocated for a list of set nodes.
 *
 * @param params Pointer to the list of set nodes to free.
 */
void
set_nodelist_free(GSList *params);

#endif
