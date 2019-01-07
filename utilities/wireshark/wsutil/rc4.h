/*
   Unix SMB/CIFS implementation.

   a partial implementation of RC4 designed for use in the
   SMB authentication protocol

   Copyright (C) Andrew Tridgell 1998

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __RC4_H__
#define __RC4_H__

#include "ws_symbol_export.h"

typedef struct _rc4_state_struct {
  unsigned char s_box[256];
  unsigned char index_i;
  unsigned char index_j;
} rc4_state_struct;

WS_DLL_PUBLIC
void crypt_rc4_init(rc4_state_struct *rc4_state,
		    const unsigned char *key, int key_len);

WS_DLL_PUBLIC
void crypt_rc4(rc4_state_struct *rc4_state, unsigned char *data, int data_len);

#endif
