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
#include <argp-extern.h>
#include <string.h>
#include <byteswap.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/param.h> // for MIN macro

#include <sys/ioctl.h>
#include <arpa/inet.h>
#ifdef BUILD_ON_RPI
#include <types.h> //not sure why it was removed, but it is needed for typedefs like `uint`
#include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <stdbool.h>
#define TYPEDEF_BOOL
#include <errno.h>

#include <wlcnt.h>

#include <nexioctls.h>

#include <typedefs.h>
#include <bcmwifi_channels.h>
#include <b64.h>

#if ANDROID
#include <sys/system_properties.h>
#endif

#define HEXDUMP_COLS 16

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
extern struct nexio *nex_init_netlink(void);

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
unsigned char   get_chanspec = 0;
unsigned char   set_chanspec = 0;
char            *set_chanspec_value = NULL;
unsigned char   custom_cmd_value_int = false;
unsigned char   custom_cmd_value_base64 = false;
unsigned char   raw_output = false;
unsigned char   base64_output = false;
unsigned int    dump_objmem_addr = 0;
unsigned char   dump_objmem = false;
unsigned char   disassociate = false;
unsigned char   dump_wl_cnt = false;
unsigned char   revinfo = false;

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
    {"custom-cmd-value-base64", 'b', 0, 0, "Define that custom-cmd-value should be interpreted as base64 string"},
    {"base64-output", 'R', 0, 0, "Write base64 encoded strings to stdout instead of hex dumping"},
    {"raw-output", 'r', 0, 0, "Write raw output to stdout instead of hex dumping"},
    {"dump-wl_cnt", 'w', 0, 0, "Dump WL counters"},
    {"dump-objmem", 'o', "INT", 0, "Dumps objmem at addr INT"},
    {"chanspec", 'k', "CHAR/INT", OPTION_ARG_OPTIONAL, "Set chanspec either as integer (e.g., 0x1001, set -i) or as string (e.g., 64/80)."},
    {"security-cookie", 'x', "INT", OPTION_ARG_OPTIONAL, "Set/Get security cookie"},
    {"use-udp-tunneling", 'X', "INT", 0, "Use UDP tunneling with security cookie INT"},
    {"broadcast-ip", 'B', "CHAR", 0, "Broadcast IP to use for UDP tunneling (default: 192.168.222.255)"},
    {"revinfo", 'V', 0, 0, "Dump revision information of the Wi-Fi chip"},
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

        case 'k':
            if (arg) {
                set_chanspec = true;
                set_chanspec_value = arg;
            } else {
                get_chanspec = true;
            }
            break;

        case 'v':
            custom_cmd_value = arg;
            break;

        case 'i':
            if (!custom_cmd_value_base64)
                custom_cmd_value_int = true;
            else
                printf("ERR: you can only either use base64 or integer encoding.");
            break;

        case 'b':
            if (!custom_cmd_value_int)
                custom_cmd_value_base64 = true;
            else
                printf("ERR: you can only either use base64 or integer encoding.");
            break;

        case 'r':
            if (!base64_output)
                raw_output = true;
            else
                printf("ERR: you can only either use base64 or raw output.");
            break;

        case 'R':
            if (!raw_output)
                base64_output = true;
            else
                printf("ERR: you can only either use base64 or raw output.");
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

        case 'V':
            revinfo = true;
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

/* Produce a human-readable string for boardrev */
char *
bcm_brev_str(uint32 brev, char *buf)
{
    if (brev < 0x100)
        snprintf(buf, 8, "%d.%d", (brev & 0xf0) >> 4, brev & 0xf);
    else
        snprintf(buf, 8, "%c%03x", ((brev & 0xf000) == 0x1000) ? 'P' : 'A', brev & 0xfff);

    return (buf);
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
#ifdef USE_NETLINK
        nexio = nex_init_netlink();
#else
        nexio = nex_init_ioctl(ifname);
#endif

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

    if (get_chanspec) {
        char charbuf[9] = "chanspec";
        uint16 chanspec = 0;
        ret = nex_ioctl(nexio, WLC_GET_VAR, charbuf, 9, false);
        chanspec = *(uint16 *) charbuf;
        printf("chanspec: 0x%04x, %s\n", chanspec, wf_chspec_ntoa(chanspec, charbuf));
    }

    if (set_chanspec) {
        char charbuf[13] = "chanspec";
        uint32 *chanspec = (uint32 *) &charbuf[9];

        if (custom_cmd_value_int)
            *chanspec = strtoul(set_chanspec_value, NULL, 0);
        else
            *chanspec = wf_chspec_aton(set_chanspec_value);

        if (*chanspec == 0)
            printf("invalid chanspec\n");
        else
            ret = nex_ioctl(nexio, WLC_SET_VAR, charbuf, 13, true);
    }

    if (custom_cmd_set != -1) {
        custom_cmd_buf = malloc(custom_cmd_buf_len);
        if (!custom_cmd_buf)
            return -1;

        memset(custom_cmd_buf, 0, custom_cmd_buf_len);

        if (custom_cmd_set == 1 && raw_output) { // set command using raw input
            //freopen(NULL, "rb", stdin);
            fread(custom_cmd_buf, 1, custom_cmd_buf_len, stdin);
        } else {
            if (custom_cmd_value) {
                if (custom_cmd_value_int) {
                    *(uint32 *) custom_cmd_buf = strtoul(custom_cmd_value, NULL, 0);
                } else if (custom_cmd_value_base64) {
                    size_t decoded_len = 0;
                    unsigned char *decoded = b64_decode_ex(custom_cmd_value, strlen(custom_cmd_value), &decoded_len);
                    memcpy(custom_cmd_buf, decoded, MIN(decoded_len, custom_cmd_buf_len));
                } else {
                    strncpy(custom_cmd_buf, custom_cmd_value, custom_cmd_buf_len);
                }
            } else {
                if (custom_cmd_value_int) {
                    *(uint32 *) custom_cmd_buf = strtoul(custom_cmd_value, NULL, 0);
                }
            }
        }

        /* NOTICE: Using SDIO to communicate to the firmware, the maximum CDC message length 
         * is limited to CDC_MAX_MSG_SIZE = ETHER_MAX_LEN = 1518, however only 1502 bytes 
         * arrive in the ioctl function, the rest might be used for the ioctl header.
         */
        if (custom_cmd_buf_len > 1502 && custom_cmd_set)
            fprintf(stderr, "WARN: Using SDIO, the ioctl payload length is limited to 1502 bytes.\n");
        ret = nex_ioctl(nexio, custom_cmd, custom_cmd_buf, custom_cmd_buf_len, custom_cmd_set);

        if (custom_cmd_set == false) {
            if (raw_output) {
                fwrite(custom_cmd_buf, sizeof(char), custom_cmd_buf_len, stdout);
                fflush(stdout);
            } else if (base64_output) {
                char *encoded = b64_encode(custom_cmd_buf, custom_cmd_buf_len);
                fwrite(encoded, sizeof(char), strlen(encoded), stdout);
                fflush(stdout);
            } else {
                hexdump(custom_cmd_buf, custom_cmd_buf_len);
            }
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
            printf("%8p %08x\n", custom_cmd_buf_pos, *custom_cmd_buf_pos);
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

    if (revinfo) {
        typedef struct wlc_rev_info {
            uint        vendorid;   /* PCI vendor id */
            uint        deviceid;   /* device id of chip */
            uint        radiorev;   /* radio revision */
            uint        chiprev;    /* chip revision */
            uint        corerev;    /* core revision */
            uint        boardid;    /* board identifier (usu. PCI sub-device id) */
            uint        boardvendor;    /* board vendor (usu. PCI sub-vendor id) */
            uint        boardrev;   /* board revision */
            uint        driverrev;  /* driver version */
            uint        ucoderev;   /* microcode version */
            uint        bus;        /* bus type */
            uint        chipnum;    /* chip number */
            uint        phytype;    /* phy type */
            uint        phyrev;     /* phy revision */
            uint        anarev;     /* anacore rev */
            uint        chippkg;    /* chip package info */
            uint        nvramrev;   /* nvram revision number */
        } wlc_rev_info_t;

        char b[8];
        char str[17][32] = { 0 };
        wlc_rev_info_t revinfo;
        
        memset(&revinfo, 0, sizeof(revinfo));

        ret = nex_ioctl(nexio, WLC_GET_REVINFO, &revinfo, sizeof(revinfo), false);

#ifdef ANDROID
        char model_string[PROP_VALUE_MAX + 1];
        __system_property_get("ro.product.model", model_string);
#else
        char model_string[] = "unknown";
#endif
        char fw_ver[256] = "ver\0";
        nex_ioctl(nexio, WLC_GET_VAR, fw_ver, sizeof(fw_ver), false);
        char *fw_ver2 = strstr(fw_ver, "version") + 8;
        fw_ver2[strlen(fw_ver2) - 1] = 0;

        snprintf(str[0], sizeof(str[0]), "0x%x", revinfo.vendorid);
        snprintf(str[1], sizeof(str[0]), "0x%x", revinfo.deviceid);
        snprintf(str[2], sizeof(str[0]), "0x%x", revinfo.radiorev);
        snprintf(str[3], sizeof(str[0]), "0x%x", revinfo.chipnum);
        snprintf(str[4], sizeof(str[0]), "0x%x", revinfo.chiprev);
        snprintf(str[5], sizeof(str[0]), "0x%x", revinfo.chippkg);
        snprintf(str[6], sizeof(str[0]), "0x%x", revinfo.corerev);
        snprintf(str[7], sizeof(str[0]), "0x%x", revinfo.boardid);
        snprintf(str[8], sizeof(str[0]), "0x%x", revinfo.boardvendor);
        snprintf(str[9], sizeof(str[0]), "%s", bcm_brev_str(revinfo.boardrev, b));
        snprintf(str[10], sizeof(str[0]), "0x%x", revinfo.driverrev);
        snprintf(str[11], sizeof(str[0]), "0x%x", revinfo.ucoderev);
        snprintf(str[12], sizeof(str[0]), "0x%x", revinfo.bus);
        snprintf(str[13], sizeof(str[0]), "0x%x", revinfo.phytype);
        snprintf(str[14], sizeof(str[0]), "0x%x", revinfo.phyrev);
        snprintf(str[15], sizeof(str[0]), "0x%x", revinfo.anarev);
        snprintf(str[16], sizeof(str[0]), "0x%x", revinfo.nvramrev);

#ifdef ANDROID
        printf("platform %s\n", model_string);
#endif
        printf("firmware %s\n", fw_ver2);
        printf("vendorid %s\n", str[0]);
        printf("deviceid %s\n", str[1]);
        printf("radiorev %s\n", str[2]);
        printf("chipnum %s\n", str[3]);
        printf("chiprev %s\n", str[4]);
        printf("chippackage %s\n", str[5]);
        printf("corerev %s\n", str[6]);
        printf("boardid %s\n", str[7]);
        printf("boardvendor %s\n", str[8]);
        printf("boardrev %s\n", str[9]);
        printf("driverrev %s\n", str[10]);
        printf("ucoderev %s\n", str[11]);
        printf("bus %s\n", str[12]);
        printf("phytype %s\n", str[13]);
        printf("phyrev %s\n", str[14]);
        printf("anarev %s\n", str[15]);
        printf("nvramrev %s\n", str[16]);

        printf("\n");
        printf("platform             | firmware                         | vendorid | deviceid | radiorev   | chipnum | chiprev | chippackage | corerev | boardid | boardvendor | boardrev | driverrev | ucoderev  | bus | phytype | phyrev | anarev | nvramrev\n");
        printf("-------------------- | -------------------------------- | -------- | -------- | ---------- | ------- | ------- | ----------- | ------- | ------- | ----------- | -------- | --------- | --------- | --- | ------- | ------ | ------ | --------\n");
        printf("%-20s | %-32s | %8s | %8s | %10s | %7s | %7s | %11s | %7s | %7s | %11s | %8s | %9s | %9s | %3s | %7s | %6s | %6s | %8s\n", 
            model_string, fw_ver2, str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9], str[10], str[11], str[12], str[13], str[14], str[15], str[16]);
    }

    return 0;
}
