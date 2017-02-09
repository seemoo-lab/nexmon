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
 * Copyright (c) 2016 NexMon Team                                          *
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

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
//#include <argp-extern.h>
#include <argp.h>
#include <string.h>
#include <byteswap.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/ioctl.h>
#ifdef BUILD_ON_RPI
#include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <stdbool.h>
#include <errno.h>

#include "wlcnt.h"

#include <nexioctls.h>

#define HEXDUMP_COLS 8

#define IPADDR(a,b,c,d) ((d) << 24 | (c) << 16 | (b) << 8 | (a))

struct nexio {
    struct ifreq *ifr;
    int sock_rx_ioctl;
    int sock_rx_frame;
    int sock_tx;
};

extern int nex_ioctl(struct nexio *nexio, int cmd, void *buf, int len, bool set);
extern struct nexio *nex_init_ioctl(const char *ifname);
extern struct nexio *nex_init_udp(unsigned int securitycookie, unsigned int txip);

char            *ifname = "wlan0";
unsigned char   set_monitor = 0;
unsigned char   set_monitor_value = 0;
unsigned char   get_monitor = 0;
unsigned char   set_promisc = 0;
unsigned char   set_promisc_value = 0;
unsigned char   get_promisc = 0;
unsigned char   set_scansuppress = 0;
unsigned char   set_scansuppress_value = 0;
unsigned char   get_scansuppress = 0;
unsigned char   set_securitycookie = 0;
unsigned int    set_securitycookie_value = 0;
unsigned char   get_securitycookie = 0;
unsigned int    use_udp_tunneling = 0; // contains the securtiy cookie
unsigned int    txip = IPADDR(192,168,222,255);
unsigned int    custom_cmd = 0;
signed char     custom_cmd_set = -1;
unsigned int    custom_cmd_buf_len = 4;
void            *custom_cmd_buf = NULL;
char            *custom_cmd_value = NULL;
unsigned char   custom_cmd_value_int = false;
unsigned char   raw_output = false;
unsigned int    dump_objmem_addr = 0;
unsigned char   dump_objmem = false;
unsigned char   disassociate = false;
unsigned char   dump_wl_cnt = false;

const char *argp_program_version = VERSION;
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "nexutil -- a program to control a nexmon firmware for broadcom chips.";

static struct argp_option options[] = {
    {"interface-name", 'I', "CHAR", 0, "Set interface name (default: wlan0)"},
    {"monitor", 'm', "INT", OPTION_ARG_OPTIONAL, "Set/Get monitor mode"},
    {"promisc", 'p', "INT", OPTION_ARG_OPTIONAL, "Set/Get promiscuous mode"},
    {"scansuppress", 'c', "INT", OPTION_ARG_OPTIONAL, "Set/Get scan suppress setting to avoid scanning"},
    {"disassociate", 'd', 0, 0, "Disassociate from access point"},
    {"get-custom-cmd", 'g', "INT", 0, "Get custom command, e.g. 107 for WLC_GET_VAR"},
    {"set-custom-cmd", 's', "INT", 0, "Set custom command, e.g. 108 for WLC_SET_VAR"},
    {"custom-cmd-buf-len", 'l', "INT", 0, "Custom command buffer length (default: 4)"},
    {"custom-cmd-value", 'v', "CHAR/INT", 0, "Initialization value for the buffer used by custom command"},
    {"custom-cmd-value-int", 'i', 0, 0, "Define that custom-cmd-value should be interpreted as integer"},
    {"raw-output", 'r', 0, 0, "Write raw output to stdout instead of hex dumping"},
    {"dump-wl_cnt", 'w', 0, 0, "Dump WL counters"},
    {"dump-objmem", 'o', "INT", 0, "Dumps objmem at addr INT"},
    {"security-cookie", 'x', "INT", OPTION_ARG_OPTIONAL, "Set/Get security cookie"},
    {"use-udp-tunneling", 'X', "INT", 0, "Use UDP tunneling with security cookie INT"},
    {"broadcast-ip", 'B', "CHAR", 0, "Broadcast IP to use for UDP tunneling (default: 192.168.222.255)"},
    { 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    switch (key) {
        case 'I':
            ifname = arg;
            break;

        case 'm':
            if (arg) {
                set_monitor = true;
                set_monitor_value = strtol(arg, NULL, 0);
            } else {
                get_monitor = true;
            }
            break;

        case 'p':
            if (arg) {
                set_promisc = true;
                set_promisc_value = strtol(arg, NULL, 0);
            } else {
                get_promisc = true;
            }
            break;

        case 'c':
            if (arg) {
                set_scansuppress = true;
                set_scansuppress_value = strtol(arg, NULL, 0);
            } else {
                get_scansuppress = true;
            }
            break;

        case 'd':
            disassociate = true;
            break;

        case 'g':
            custom_cmd_set = false;
            custom_cmd = strtol(arg, NULL, 0);
            break;

        case 's':
            custom_cmd_set = true;
            custom_cmd = strtol(arg, NULL, 0);
            break;
        
        case 'l':
            custom_cmd_buf_len = strtol(arg, NULL, 0);
            break;

        case 'v':
            custom_cmd_value = arg;
            break;

        case 'i':
            custom_cmd_value_int = true;
            break;

        case 'r':
            raw_output = true;
            break;

        case 'o':
            dump_objmem_addr = strtol(arg, NULL, 0);
            dump_objmem = true;
            break;

        case 'w':
            dump_wl_cnt = true;
            break;

        case 'x':
            if (arg) {
                set_securitycookie = true;
                set_securitycookie_value = strtol(arg, NULL, 0);
            } else {
                get_securitycookie = true;
            }
            break;

        case 'X':
            if (set_securitycookie == false) {
                use_udp_tunneling = strtol(arg, NULL, 0);
                if (use_udp_tunneling == 0)
                    printf("ERR: You need to use a security cookie different from 0.");
            } else {
                printf("ERR: You cannot use -x in combination with -X.");
            }
            break;

        case 'B':
            if (arg) {
                txip = inet_addr(arg);
            }
            break;
        
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

/* source: http://grapsus.net/blog/post/Hexadecimal-dump-in-C */
void
hexdump(void *mem, unsigned int len)
{
    unsigned int i, j;
    
    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        if (i % HEXDUMP_COLS == 0) {
                printf("0x%06x: ", i);
        }

        if(i < len) {
            printf("%02x ", 0xFF & ((char*)mem)[i]);
        } else {
            printf("   ");
        }

        if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for(j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if(j >= len) {
                    putchar(' ');
                } else if(isprint(((char*)mem)[j])) {
                    putchar(0xFF & ((char*)mem)[j]);        
                } else {
                        putchar('.');
                }
            }
            putchar('\n');
        }
    }
}

int
main(int argc, char **argv)
{
    struct nexio *nexio;
    int ret;
    int buf = 0;

    argp_parse(&argp, argc, argv, 0, 0, 0);

    if (use_udp_tunneling != 0)
        nexio = nex_init_udp(use_udp_tunneling, txip);
    else
        nexio = nex_init_ioctl(ifname);

    if (set_monitor) {
        buf = set_monitor_value;
        ret = nex_ioctl(nexio, WLC_SET_MONITOR, &buf, 4, true);
    }

    if (set_promisc) {
        buf = set_promisc_value;
        ret = nex_ioctl(nexio, WLC_SET_PROMISC, &buf, 4, true);
    }

    if (set_scansuppress) {
        buf = set_scansuppress_value;
        if (set_scansuppress_value)
            ret = nex_ioctl(nexio, WLC_SET_SCANSUPPRESS, &buf, 4, true);
        else
            ret = nex_ioctl(nexio, WLC_SET_SCANSUPPRESS, &buf, 1, true);
    }

    if (set_securitycookie) {
        buf = set_securitycookie_value;
        ret = nex_ioctl(nexio, NEX_SET_SECURITYCOOKIE, &buf, 4, true);
    }

    if (get_monitor) {
        ret = nex_ioctl(nexio, WLC_GET_MONITOR, &buf, 4, false);
        printf("monitor: %d\n", buf);
    }

    if (get_promisc) {
        ret = nex_ioctl(nexio, WLC_GET_PROMISC, &buf, 4, false);
        printf("promisc: %d\n", buf);
    }

    if (get_scansuppress) {
        ret = nex_ioctl(nexio, WLC_GET_SCANSUPPRESS, &buf, 4, false);
        printf("scansuppress: %d\n", buf);
    }

    if (get_securitycookie) {
        ret = nex_ioctl(nexio, NEX_GET_SECURITYCOOKIE, &buf, 4, false);
        printf("securitycookie: %d\n", buf);
    }

    if (custom_cmd_set != -1) {
        custom_cmd_buf = malloc(custom_cmd_buf_len);
        if (!custom_cmd_buf)
            return -1;

        memset(custom_cmd_buf, 0, custom_cmd_buf_len);

        if (custom_cmd_value)
            if (custom_cmd_value_int)
                *(int *) custom_cmd_buf = strtol(custom_cmd_value, NULL, 0);
            else
                strncpy(custom_cmd_buf, custom_cmd_value, custom_cmd_buf_len);

        ret = nex_ioctl(nexio, custom_cmd, custom_cmd_buf, custom_cmd_buf_len, custom_cmd_set);

        if (custom_cmd_set == false)
            if (raw_output) {
                fwrite(custom_cmd_buf, sizeof(char), custom_cmd_buf_len, stdout);
                fflush(stdout);
            } else {
                hexdump(custom_cmd_buf, custom_cmd_buf_len);
            }
    }

    if (dump_objmem) {
        custom_cmd_buf = malloc(custom_cmd_buf_len);
        if (!custom_cmd_buf)
            return -1;

        memset(custom_cmd_buf, 0, custom_cmd_buf_len);

        unsigned int *custom_cmd_buf_pos = (unsigned int *) custom_cmd_buf;

        int i = 0;
        for (i = 0; i < custom_cmd_buf_len / 0x2000; i++) {
            *custom_cmd_buf_pos = dump_objmem_addr + i * 0x2000 / 4;
            printf("%08x %08x\n", (int) custom_cmd_buf_pos, *custom_cmd_buf_pos);
            ret = nex_ioctl(nexio, 406, custom_cmd_buf_pos, 0x2000, false);    
            custom_cmd_buf_pos += 0x2000 / 4;
        }
        if (custom_cmd_buf_len % 0x2000 != 0) {
            *(unsigned int *) custom_cmd_buf_pos = dump_objmem_addr + i * 0x2000 / 4;
            ret = nex_ioctl(nexio, 406, custom_cmd_buf_pos, custom_cmd_buf_len % 0x2000, false);
        }

        if (raw_output) {
            fwrite(custom_cmd_buf, sizeof(char), custom_cmd_buf_len, stdout);
            fflush(stdout);
        } else {
            hexdump(custom_cmd_buf, custom_cmd_buf_len);
        }
    }

    if (disassociate) {
        buf = 1;
        ret = nex_ioctl(nexio, WLC_DISASSOC, &buf, 4, true);
    }

    if (dump_wl_cnt) {
        wl_cnt_t cnt;
        wl_cnt_t *_cnt = &cnt;
        memset(_cnt, 0, sizeof(cnt));
        ret = nex_ioctl(nexio, NEX_GET_WL_CNT, _cnt, sizeof(cnt), false);
        unsigned int i;
        printf("version: %d\n", _cnt->version);
        printf("length: %d\n", _cnt->length);
        for (i = 1; i < sizeof(cnt)/4; i++) {
            printf("%s: %d (%s)\n", wl_cnt_varname[i], ((uint32 *) _cnt)[i], wl_cnt_description[i]);
        }
        //hexdump(_cnt, sizeof(_cnt)); 
    }

    return 0;
}
