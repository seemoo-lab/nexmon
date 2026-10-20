/* Table of territories.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

#ifndef _COUNTRY_TABLE_H
#define _COUNTRY_TABLE_H

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


struct country_table_entry
{
  const char *code;
  const char *english;
};

extern struct country_table_entry country_table[];
extern const size_t country_table_size;


#ifdef __cplusplus
}
#endif


#endif /* _COUNTRY_TABLE_H */
