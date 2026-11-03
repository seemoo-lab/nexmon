/** @file
 *
 * Wireshark's interface to the Lua Programming Language
 *
 * (c) 2006, Luis E. Garcia Ontanon <luis@ontanon.org>
 * (c) 2008, Balint Reczey <balint.reczey@ericsson.com>
 * (c) 2011, Stig Bjorlykke <stig@bjorlykke.org>
 * (c) 2014, Hadriel Kaplan <hadrielk@yahoo.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#include "wslua.h"

/**
 * @brief Pushes a Columns object onto the Lua stack.
 *
 * @param L The Lua state.
 * @param c The Columns object to push.
 */
void Push_Columns(lua_State *L, Columns c);

/**
 * @brief Retrieves the index of the Columns object in the Lua stack.
 *
 * @param L The Lua state.
 * @return The index of the Columns object in the Lua stack.
 */
int get_Columns_index(lua_State *L);
