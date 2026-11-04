/** @file
** Lua BitOp -- a bit operations library for Lua 5.1/5.2.
** http://bitop.luajit.org/
**
** Copyright (C) 2008-2012 Mike Pall. All rights reserved.
**
** SPDX-License-Identifier: MIT
**
*/

#ifndef _LUA_BITOP_H
#define _LUA_BITOP_H

/**
 * @brief Open the bit library in Lua.
 *
 * This function is called to load and initialize the bit library into a Lua state.
 *
 * @param L The Lua state to which the bit library will be loaded.
 * @return The number of values pushed onto the stack (1).
 */
extern int luaopen_bit(lua_State *L);

#endif /* _LUA_BITOP_H */
