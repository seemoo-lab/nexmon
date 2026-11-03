/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GENCODE_H
#define GENCODE_H

#include "dfilter-int.h"

/**
 * @brief Generates code for a given data flow work structure.
 *
 * This function initializes various data structures and generates code based on the data flow work.
 *
 * @param dfw Pointer to the data flow work structure.
 */
void
dfw_gencode(dfwork_t *dfw);

int*

/**
 * @brief Retrieves an array of interesting fields from a dfwork_t structure.
 *
 * This function returns an array containing the keys of all fields marked as interesting in the given dfwork_t structure.
 *
 * @param dfw Pointer to the dfwork_t structure containing the interesting fields.
 * @param caller_num_fields Pointer to an integer that will be set to the number of interesting fields returned.
 * @return An array of integers representing the keys of the interesting fields, or NULL if there are no interesting fields.
 */
dfw_interesting_fields(dfwork_t *dfw, int *caller_num_fields);

#endif
