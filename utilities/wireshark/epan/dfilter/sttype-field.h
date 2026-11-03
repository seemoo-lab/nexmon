/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STTYPE_FIELD_H
#define STTYPE_FIELD_H

#include "dfilter-int.h"
#include "drange.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Retrieves the hfinfo for a given stnode.
 *
 * @param node The stnode to retrieve the hfinfo from.
 * @return field_info_t* A pointer to the hfinfo.
 */
WS_DLL_PUBLIC
header_field_info *
sttype_field_hfinfo(stnode_t *node);

/**
 * @brief Retrieves the ftenum_t for a given stnode.
 *
 * @param node The stnode to retrieve the ftenum_t from.
 * @return ftenum_t The ftenum_t of the field.
 */
ftenum_t
sttype_field_ftenum(stnode_t *node);

/**
 * @brief Retrieves the drange_t for a given stnode.
 *
 * @param node The stnode to retrieve the drange_t from.
 * @return drange_t* A pointer to the drange_t of the field.
 */
drange_t *
sttype_field_drange(stnode_t *node);

/**
 * @brief Steal the drange from a stnode_t.
 *
 * @param node The stnode_t from which to steal the drange.
 * @return drange_t* A pointer to the stolen drange
 */
drange_t *
sttype_field_drange_steal(stnode_t *node);

/**
 * @brief Check if a field is in raw format.
 *
 * @param node The stnode_t containing the field data.
 * @return true if the field is in raw format, false otherwise.
 */
bool
sttype_field_raw(stnode_t *node);

/**
 * @brief Get the value string of a field node.
 *
 * @param node The field node to get the value string from.
 * @return const char* The value string of the field.
 */
bool
sttype_field_value_string(stnode_t *node);

/* Set a range */

/**
 * @brief Set a range for a field node.
 *
 * @param node The field node to set the range for.
 * @param drange_list A list of range nodes representing the new range.
 */
void
sttype_field_set_range(stnode_t *node, GSList* drange_list);

/**
 * @brief Set a range for a field node.
 *
 * @param node The field node to set the range for.
 * @param rn The range node containing the range values.
 */
void
sttype_field_set_range1(stnode_t *node, drange_node *rn);

/**
 * @brief Set the range for a field node.
 *
 * @param node The field node to set the range for.
 * @param dr The range to set.
 */
void
sttype_field_set_drange(stnode_t *node, drange_t *dr);

/**
 * @brief Set whether a field is in raw mode.
 *
 * @param node The node containing the field to modify.
 * @param raw Whether the field should be in raw mode.
 */
void
sttype_field_set_raw(stnode_t *node, bool raw);

/**
 * @brief Set the value string for a field node.
 *
 * @param node The field node to set the value string for.
 * @param is_vs The boolean value indicating whether the value string should be set.
 */
void
sttype_field_set_value_string(stnode_t *node, bool is_vs);

/**
 * @brief Set the number value for a field node.
 *
 * @param node The field node to set the number value for.
 * @param number_str The string representation of the number to set.
 */
char *
sttype_field_set_number(stnode_t *node, const char *number_str);

/* Clear the 'drange' variable to remove responsibility for
 * freeing it. */
/**
 * @brief Remove a range from a field node.
 *
 * @param node The field node to remove the range from.
 */
void
sttype_field_remove_drange(stnode_t *node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STTYPE_FIELD_H */
