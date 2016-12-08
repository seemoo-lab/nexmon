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

#define HEXDUMP_COLS 8

#define WLC_IOCTL_MAGIC          0x14e46c77
#define DHD_IOCTL_MAGIC          0x00444944
#define WLC_GET_MAGIC                     0
#define DHD_GET_MAGIC                     0

#define WLC_GET_PROMISC                   9
#define WLC_SET_PROMISC                  10

#define WLC_GET_MONITOR                 107
#define WLC_SET_MONITOR                 108

#define WLC_GET_SCANSUPPRESS            115
#define WLC_SET_SCANSUPPRESS            116

/* Linux network driver ioctl encoding */
typedef struct nex_ioctl {
    unsigned int cmd;   /* common ioctl definition */
    void *buf;  /* pointer to user buffer */
    unsigned int len;   /* length of user buffer */
    bool set;   /* get or set request (optional) */
    unsigned int used;  /* bytes read or written (optional) */
    unsigned int needed;    /* bytes needed (optional) */
    unsigned int driver;    /* to identify target driver */
} nex_ioctl_t;

unsigned char   set_monitor = 0;
unsigned char   set_monitor_value = 0;
unsigned char   get_monitor = 0;
unsigned char   set_promisc = 0;
unsigned char   set_promisc_value = 0;
unsigned char   get_promisc = 0;
unsigned char   set_scansuppress = 0;
unsigned char   set_scansuppress_value = 0;
unsigned char   get_scansuppress = 0;
unsigned int    custom_cmd = 0;
signed char     custom_cmd_set = -1;
unsigned int    custom_cmd_buf_len = 4;
void            *custom_cmd_buf = NULL;
char            *custom_cmd_value = NULL;
unsigned char   custom_cmd_value_int = false;
unsigned char   raw_output = false;
unsigned int    dump_objmem_addr = 0;
unsigned char   dump_objmem = false;

const char *argp_program_version = "nexutil";
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "nexutil -- a program to control a nexmon firmware for broadcom chips.";

static struct argp_option options[] = {
    {"set-monitor", 'm', "BOOL", 0, "Set monitor mode"},
    {"set-promisc", 'p', "BOOL", 0, "Set promiscuous mode"},
    {"set-scansuppress", 'c', "BOOL", 0, "Set scan suppress setting to avoid scanning"},
    {"get-monitor", 'n', 0, 0, "Get monitor mode"},
    {"get-promisc", 'q', 0, 0, "Get promiscuous mode"},
    {"get-scansuppress", 'd', 0, 0, "Get scan suppress setting"},
    {"get-custom-cmd", 'g', "INT", 0, "Get custom command, e.g. 107 for WLC_GET_VAR"},
    {"set-custom-cmd", 's', "INT", 0, "Set custom command, e.g. 108 for WLC_SET_VAR"},
    {"custom-cmd-buf-len", 'l', "INT", 0, "Custom command buffer length (default: 4)"},
    {"custom-cmd-value", 'v', "CHAR/INT", 0, "Initialization value for the buffer used by custom command"},
    {"custom-cmd-value-int", 'i', 0, 0, "Define that custom-cmd-value should be interpreted as integer"},
    {"raw-output", 'r', 0, 0, "Write raw output to stdout instead of hex dumping"},
    {"dump-objmem", 'o', "INT", 0, "Dumps objmem at addr INT"},
    { 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    switch (key) {
        case 'm':
            set_monitor = true;
            set_monitor_value = strncmp(arg, "true", 4) ? false : true;
            break;

        case 'p':
            set_promisc = true;
            set_promisc_value = strncmp(arg, "true", 4) ? false : true;
            break;

        case 'c':
            set_scansuppress = true;
            set_scansuppress_value = strncmp(arg, "true", 4) ? false : true;
            break;

        case 'n':
            get_monitor = true;
            break;

        case 'q':
            get_promisc = true;
            break;

        case 'd':
            get_scansuppress = true;
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
        
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

static int __nex_driver_io(struct ifreq *ifr, nex_ioctl_t *ioc)
{
    int s;
    int ret = 0;

    /* pass ioctl data */
    ifr->ifr_data = (void *) ioc;

    /* open socket to kernel */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("error: socket\n");

    ret = ioctl(s, SIOCDEVPRIVATE, ifr);
    if (ret < 0 && errno != EAGAIN)
        printf("%s: error\n", __FUNCTION__);

    /* cleanup */
    close(s);
    return ret;
}

/* This function is called by ioctl_setinformation_fe or ioctl_queryinformation_fe
 * for executing  remote commands or local commands
 */
static int
nex_ioctl(struct ifreq *ifr, int cmd, void *buf, int len, bool set)
{
    nex_ioctl_t ioc;
    int ret = 0;

    /* By default try to execute wl commands */
    int driver_magic = WLC_IOCTL_MAGIC;
    int get_magic = WLC_GET_MAGIC;

    // For local dhd commands execute dhd
    //driver_magic = DHD_IOCTL_MAGIC;
    //get_magic = DHD_GET_MAGIC;

    /* do it */
    ioc.cmd = cmd;
    ioc.buf = buf;
    ioc.len = len;
    ioc.set = set;
    ioc.driver = driver_magic;

    ret = __nex_driver_io(ifr, &ioc);
    if (ret < 0 && cmd != get_magic)
        ret = -1;
    return ret;
}

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
    struct ifreq ifr;
    int ret;
    int buf = 0;

    argp_parse(&argp, argc, argv, 0, 0, 0);

    memset(&ifr, 0, sizeof(struct ifreq));
    memcpy(ifr.ifr_name, "wlan0", 5);

    if (set_monitor) {
        buf = set_monitor_value;
        ret = nex_ioctl(&ifr, WLC_SET_MONITOR, &buf, 4, true);
    }

    if (set_promisc) {
        buf = set_promisc_value;
        ret = nex_ioctl(&ifr, WLC_SET_PROMISC, &buf, 4, true);
    }

    if (set_scansuppress) {
        buf = set_scansuppress_value;
        if (set_scansuppress_value)
            ret = nex_ioctl(&ifr, WLC_SET_SCANSUPPRESS, &buf, 4, true);
        else
            ret = nex_ioctl(&ifr, WLC_SET_SCANSUPPRESS, &buf, 1, true);
    }

    if (get_monitor) {
        ret = nex_ioctl(&ifr, WLC_GET_MONITOR, &buf, 4, false);
        printf("monitor: %d\n", buf);
    }

    if (get_promisc) {
        ret = nex_ioctl(&ifr, WLC_GET_PROMISC, &buf, 4, false);
        printf("promisc: %d\n", buf);
    }

    if (get_scansuppress) {
        ret = nex_ioctl(&ifr, WLC_GET_SCANSUPPRESS, &buf, 4, false);
        printf("scansuppress: %d\n", buf);
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

        ret = nex_ioctl(&ifr, custom_cmd, custom_cmd_buf, custom_cmd_buf_len, custom_cmd_set);

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
            ret = nex_ioctl(&ifr, 406, custom_cmd_buf_pos, 0x2000, false);    
            custom_cmd_buf_pos += 0x2000 / 4;
        }
        if (custom_cmd_buf_len % 0x2000 != 0) {
            *(unsigned int *) custom_cmd_buf_pos = dump_objmem_addr + i * 0x2000 / 4;
            ret = nex_ioctl(&ifr, 406, custom_cmd_buf_pos, custom_cmd_buf_len % 0x2000, false);
        }

        if (raw_output) {
            fwrite(custom_cmd_buf, sizeof(char), custom_cmd_buf_len, stdout);
            fflush(stdout);
        } else {
            hexdump(custom_cmd_buf, custom_cmd_buf_len);
        }
    }

    return 0;
}
