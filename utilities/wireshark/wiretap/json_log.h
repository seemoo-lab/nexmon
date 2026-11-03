/* @file
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "wtap.h"

/**
 * @brief Opens a JSON log file for reading.
 *
 * This function initializes the wtap structure to read from a JSON log file.
 *
 * @param wth Pointer to the wtap structure that will be initialized.
 * @param err Pointer to an integer where error codes can be stored.
 * @param err_info Pointer to a string where error information can be stored.
 * @return A value indicating the success or failure of the operation.
 */
wtap_open_return_val json_log_open(wtap *wth, int *err, char **err_info);
