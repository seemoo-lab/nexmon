/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2017 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <stdarg.h>
#include <wrapper.h>

static int argprintf_written = 0;
static char *argprintf_arg = 0;
static int argprintf_len = 0;
static bool argprintf_first_call = 1;

void _hexdump(char *desc, void *addr, int len, int (*_printf)(const char *, ...));

int
argprintf(const char *format, ...)
{
    va_list args;
    int rc;

    if (argprintf_first_call) {
        memset(argprintf_arg, 0, argprintf_len);
        argprintf_first_call = 0;
    }

    va_start(args, format);
    rc = vsnprintf(argprintf_arg + argprintf_written, argprintf_len - argprintf_written, format, args);
    argprintf_written += rc;
    va_end(args);
    return (rc);
}

void
arghexdump(char *desc, void *addr, int len)
{
    _hexdump(desc, addr, len, argprintf);
}

void
argprintf_init(char *arg, int len)
{
	argprintf_written = 0;
    argprintf_first_call = 1;
    argprintf_arg = arg;
    argprintf_len = len;
}
