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
static volatile int argprintf_in_progress = 0;
static char *argprintf_arg = 0;
static int argprintf_len = 0;
static bool argprintf_first_call = 1;

void _hexdump(char *desc, void *addr, int len, int (*_printf)(const char *, ...));

int
argprintf(const char *format, ...)
{
    va_list args;
    int rc;

    if (!argprintf_arg || argprintf_len <= 0)
        return 0;

    if (argprintf_in_progress)
        return 0;
    argprintf_in_progress = 1;

    if (argprintf_first_call) {
        memset(argprintf_arg, 0, argprintf_len);
        argprintf_first_call = 0;
    }

    if (argprintf_written >= argprintf_len) {
        argprintf_in_progress = 0;
        return 0;
    }

    va_start(args, format);
    rc = vsnprintf(argprintf_arg + argprintf_written, argprintf_len - argprintf_written, format, args);
    va_end(args);

    if (rc < 0) {
        argprintf_in_progress = 0;
        return rc;
    }

    if (argprintf_written + rc >= argprintf_len)
        argprintf_written = argprintf_len;
    else
        argprintf_written += rc;

    argprintf_in_progress = 0;
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
    argprintf_in_progress = 1;
    argprintf_written = 0;
    argprintf_first_call = 1;
    argprintf_arg = arg;
    argprintf_len = (len > 0) ? len : 0;
    argprintf_in_progress = 0;
}
