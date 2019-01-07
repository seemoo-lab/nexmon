/*
 * g711.h
 *
 * Definitions for routines for u-law, A-law and linear PCM conversions
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __G711_H__
#define __G711_H__

#include "ws_symbol_export.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WS_DLL_PUBLIC unsigned char linear2alaw( int );
WS_DLL_PUBLIC int alaw2linear( unsigned char );
WS_DLL_PUBLIC unsigned char linear2ulaw( int );
WS_DLL_PUBLIC int ulaw2linear( unsigned char );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __G711_H__ */
