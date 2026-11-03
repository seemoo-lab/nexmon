/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SEMCHECK_H
#define SEMCHECK_H

#include "dfilter-int.h"

/**
 * @brief Perform semantic checking on a filter expression.
 *
 * This function initiates the semantic checking process for a filter expression
 * represented by the given dfwork_t structure. It checks the arithmetic and function
 * calls within the expression to ensure they are semantically valid.
 *
 * @param dfw Pointer to the dfwork_t structure containing the filter expression.
 * @return ftenum_t The result of the semantic check, indicating whether the filter is valid or not.
 */
bool
dfw_semcheck(dfwork_t *dfw);

/**
 * @brief Checks arithmetic operations.
 *
 * @param dfw Pointer to the current dissection work context.
 * @param st_node Pointer to the syntax tree node representing the operation.
 * @param logical_ftype The logical type of the operation.
 */
ftenum_t
check_arithmetic(dfwork_t *dfw, stnode_t *st_node, ftenum_t logical_ftype);

/**
 * @brief Checks if a function call is valid based on its parameters.
 *
 * Validates that the number of arguments in a function call matches the expected range and calls the appropriate semantic check function for the function.
 *
 * @param dfw The current state of the dissection work.
 * @param st_node The syntax tree node representing the function call.
 * @param logical_ftype The logical type of the function.
 * @return ftenum_t The result of the semantic check.
 */
ftenum_t
check_function(dfwork_t *dfw, stnode_t *st_node, ftenum_t logical_ftype);

/**
 * @brief Checks if a slice can be resolved and returns its logical field type.
 *
 * This function resolves an unparsed node and determines the logical field type of the sliced entity.
 *
 * @param dfw The current dissection work context.
 * @param st The syntax tree node to check.
 * @param logical_ftype The logical field type of the sliced entity.
 * @return ftenum_t The resolved logical field type or FT_NONE if not applicable.
 */
ftenum_t
check_slice(dfwork_t *dfw, stnode_t *st, ftenum_t logical_ftype);

/**
 * @brief Resolve an unparsed node in a display filter expression.
 *
 * This function attempts to resolve an unparsed node by converting it into a field node if possible.
 * If resolution fails and strict mode is enabled, it raises an error; otherwise, it mutates the node to a literal.
 *
 * @param dfw The current display filter work context.
 * @param st The node to be resolved.
 * @param strict Whether to raise an error if resolution fails.
 */
void
resolve_unparsed(dfwork_t *dfw, stnode_t *st, bool strict);

/**
 * @brief Retrieves the logical field type for a given node.
 *
 * Determines the field type based on the type of the node and its children.
 *
 * @param dfw The current working context.
 * @param st_node The node to analyze.
 * @return The logical field type.
 */
ftenum_t
get_logical_ftype(dfwork_t *dfw, stnode_t *st_node);

/**
 * @brief Checks if two field types are compatible.
 *
 * This function determines whether two field types can be compared or used together in a dissector filter expression.
 *
 * @param a The first field type to compare.
 * @param b The second field type to compare.
 * @return true if the field types are compatible, false otherwise.
 */
bool
compatible_ftypes(ftenum_t a, ftenum_t b);

#endif
