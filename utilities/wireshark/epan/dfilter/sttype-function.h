/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef STTYPE_FUNCTION_H
#define STTYPE_FUNCTION_H

#include "dfilter-int.h"
#include "dfunctions.h"

/**
 * @brief Set the parameters for a function stnode_t.
 *
 * @param node The function node to set parameters for.
 * @param params The list of parameters to set.
 */
void
sttype_function_set_params(stnode_t *node, GSList *params);

/**
 * @brief Get the function-definition record for a function stnode_t.
 *
 * @param node The function node to get the definition for.
 * @return df_func_def_t* A pointer to the function-definition record.
 */
df_func_def_t* sttype_function_funcdef(stnode_t *node);

/**
 * @brief Get the name of a function from an stnode_t.
 *
 * @param node The stnode_t containing the function information.
 * @return const char* The name of the function.
 */
const char *sttype_function_name(stnode_t *node);

/**
 * @brief Get the parameters for a function stnode_t.
 *
 * @param node The function node to get parameters from.
 * @return GSList* A list of parameters for the function.
 */
GSList* sttype_function_params(stnode_t *node);

/**
 * @brief Free the memory of a parameter list.
 *
 * @param params The GSList containing the parameters to be freed.
 */
void st_funcparams_free(GSList *params);

#endif
