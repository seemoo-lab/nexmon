/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STTYPE_SLICE_H
#define STTYPE_SLICE_H

#include "syntax-tree.h"
#include "drange.h"


/**
 * @brief Get the entity associated with a slice node.
 *
 * @param node The slice node to query.
 * @return The entity of the slice.
 */
stnode_t *
sttype_slice_entity(stnode_t *node);

/**
 * @brief Get the drange associated with a slice node.
 *
 * @param node The slice node to query.
 * @return The drange of the slice.
 */
drange_t *
sttype_slice_drange(stnode_t *node);

/**
 * @brief Steal the drange from a slice node, transferring ownership to the caller.
 *
 * @param node The slice node to steal the drange from.
 * @return The stolen drange.
 */
drange_t *
sttype_slice_drange_steal(stnode_t *node);

/**
 * @brief Set the slice entity and range for a given node.
 *
 * @param node The node to set the slice entity and range for.
 * @param field The field to be used as the slice entity.
 * @param drange_list The list of ranges to be associated with the slice.
 */
/* Set a range */
void
sttype_slice_set(stnode_t *node, stnode_t *field, GSList* drange_list);

/**
 * @brief Set a slice node with a single range.
 *
 * @param node The slice node to set.
 * @param field The entity field for the slice.
 * @param rn The range node to be added.
 */
void
sttype_slice_set1(stnode_t *node, stnode_t *field, drange_node *rn);

/**
 * @brief Set the drange for a slice node.
 *
 * @param node The slice node to set the drange for.
 * @param field The field associated with the slice.
 * @param dr The drange to assign to the slice.
 */
void
sttype_slice_set_drange(stnode_t *node, stnode_t *field, drange_t *dr);

/**
 * @brief Remove the drange from a slice node, clearing ownership.
 *
 * @param node The slice node to remove the drange from.
 */
void
sttype_slice_remove_drange(stnode_t *node);

#endif
