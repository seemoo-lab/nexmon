/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STTYPE_NUMBER_H
#define STTYPE_NUMBER_H

#include "dfilter-int.h"

/**
 * @brief Get the type of a number node.
 *
 * @param st The number node to query.
 * @return The type of the number node.
 */
stnumber_t
sttype_number_get_type(stnode_t*st);

/**
 * @brief Set the integer value of a number node.
 *
 * @param st The number node to set.
 * @param value The integer value to assign.
 */
void
sttype_number_set_integer(stnode_t *st, int64_t value);

/**
 * @brief Get the integer value from a number node.
 *
 * @param st The number node to retrieve the integer value from.
 * @return The integer value stored in the number node.
 */
int64_t
sttype_number_get_integer(stnode_t *st);

/**
 * @brief Set the integer value of a number node.
 *
 * @param st The number node to set.
 * @param value The integer value to assign.
 */
void
sttype_number_set_unsigned(stnode_t *st, uint64_t value);

/**
 * @brief Get the integer value from a number node.
 *
 * @param st The number node to retrieve the integer value from.
 * @return The integer value stored in the number node.
 */
uint64_t
sttype_number_get_unsigned(stnode_t *st);

/**
 * @brief Set the float value of a number node.
 *
 * @param st The number node to set.
 * @param value The float value to assign.
 */
void
sttype_number_set_float(stnode_t *st, double value);

/**
 * @brief Retrieves the float value from a number node.
 *
 * @param st The number node to retrieve the float value from.
 * @return The float value stored in the number node.
 */
double
sttype_number_get_float(stnode_t *st);

#endif
