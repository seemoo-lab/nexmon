/* Formatted output with a POSIX compatible format string.
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

/* This file implements the bulk of the POSIX:2024 specification for the 'printf'
   command:
   <https://pubs.opengroup.org/onlinepubs/9799919799/utilities/printf.html>
   <https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap05.html#tag_05>
   including the floating-point conversion specifiers 'a', 'A', 'e', 'E',
   'f', 'F', 'g', 'G', but without the obsolescent 'b' conversion specifier.  */

#ifndef _PRINTF_COMMAND_H
#define _PRINTF_COMMAND_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the number of arguments that a format string consumes.  */
extern size_t printf_consumed_arguments (const char *format);

/* Applies a format string to a sequence of string arguments.  */
extern void printf_command (const char *format, size_t args_each_round,
                            size_t argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* _PRINTF_COMMAND_H */
