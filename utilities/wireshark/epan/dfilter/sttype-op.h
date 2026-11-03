/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STTYPE_TEST_H
#define STTYPE_TEST_H

#include "syntax-tree.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Set a unary operation on a node.
 *
 * @param node The node to set the operation on.
 * @param op The operation to set.
 * @param val1 The first value for the operation.
 */
void
sttype_oper_set1(stnode_t *node, stnode_op_t op, stnode_t *val1);

/**
 * @brief Set a binary operation on a node.
 *
 * @param node The node to set the operation on.
 * @param op The operation to set.
 * @param val1 The first value for the operation.
 * @param val2 The second value for the operation.
 */
void
sttype_oper_set2(stnode_t *node, stnode_op_t op, stnode_t *val1, stnode_t *val2);

/**
 * @brief Set the first argument for an operator node.
 *
 * @param node The operator node to modify.
 * @param val1 The new value for the first argument.
 */
void
sttype_oper_set1_args(stnode_t *node, stnode_t *val1);

/**
 * @brief Set two arguments for an operator node.
 *
 * @param node The syntax tree node to set.
 * @param val1 The first operand.
 * @param val2 The second operand.
 */
void
sttype_oper_set2_args(stnode_t *node, stnode_t *val1, stnode_t *val2);

/**
 * @brief Set the operator for a given node.
 *
 * @param node The operator node to modify.
 * @param op The new operator value.
 */
void
sttype_oper_set_op(stnode_t *node, stnode_op_t op);

/**
 * @brief Get the operation type from a stnode_t.
 *
 * @param node Pointer to the stnode_t structure.
 * @return The operation type of the node.
 */
stnode_op_t
sttype_oper_get_op(stnode_t *node);

/**
 * @brief Retrieves the operation and values from a stnode_t.
 *
 * @param node The stnode_t to retrieve data from.
 * @param p_op Pointer to store the operation type, or NULL if not needed.
 * @param p_val1 Pointer to store the first value, or NULL if not needed.
 * @param p_val2 Pointer to store the second value, or NULL if not needed.
 */
WS_DLL_PUBLIC
void
sttype_oper_get(stnode_t *node, stnode_op_t *p_op, stnode_t **p_val1, stnode_t **p_val2);

/**
 * @brief Set the matching behavior for a node.
 *
 * @param node Pointer to the node whose matching behavior is to be set.
 * @param how The new matching behavior to set.
 */
void
sttype_test_set_match(stnode_t *node, stmatch_t how);

/**
 * @brief Retrieves the match type for a given operation node.
 *
 * @param node Pointer to the operation node.
 * @return The match type of the operation.
 */
stmatch_t
sttype_test_get_match(stnode_t *node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STTYPE_TEST_H */
