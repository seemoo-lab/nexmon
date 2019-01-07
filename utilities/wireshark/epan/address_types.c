/* address_types.c
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

#include "config.h"

#include <string.h>     /* for memcmp */
#include "packet.h"
#include "address_types.h"
#include "to_str.h"
#include "to_str-int.h"
#include "addr_resolv.h"
#include "wsutil/pint.h"
#include "wsutil/str_util.h"
#include "wsutil/inet_addr.h"

struct _address_type_t {
    int                     addr_type; /* From address_type enumeration or registered value */
    const char             *name;
    const char             *pretty_name;
    AddrValueToString       addr_to_str;
    AddrValueToStringLen    addr_str_len;
    AddrValueToByte         addr_to_byte;
    AddrColFilterString     addr_col_filter;
    AddrFixedLen            addr_fixed_len;
    AddrNameResolutionToString addr_name_res_str;
    AddrNameResolutionLen   addr_name_res_len;

    /* XXX - Some sort of compare functions (like ftype)? ***/
};

#define MAX_DISSECTOR_ADDR_TYPE     30
#define MAX_ADDR_TYPE_VALUE (AT_END_OF_LIST+MAX_DISSECTOR_ADDR_TYPE)

static int num_dissector_addr_type;
static address_type_t dissector_type_addresses[MAX_DISSECTOR_ADDR_TYPE];

/* Keep track of address_type_t's via their id number */
static address_type_t* type_list[MAX_ADDR_TYPE_VALUE];

/*
 * If a user _does_ pass in a too-small buffer, this is probably
 * going to be too long to fit.  However, even a partial string
 * starting with "[Buf" should provide enough of a clue to be
 * useful.
 */
#define BUF_TOO_SMALL_ERR "[Buffer too small]"

static void address_type_register(int addr_type, address_type_t *at)
{
    /* Check input */
    g_assert(addr_type < MAX_ADDR_TYPE_VALUE);
    g_assert(addr_type == at->addr_type);

    /* Don't re-register. */
    g_assert(type_list[addr_type] == NULL);

    /* Sanity check */
    DISSECTOR_ASSERT(at->name);
    DISSECTOR_ASSERT(at->pretty_name);
    DISSECTOR_ASSERT(at->addr_to_str);
    DISSECTOR_ASSERT(at->addr_str_len);
    DISSECTOR_ASSERT(((at->addr_name_res_str != NULL) && (at->addr_name_res_len != NULL)) ||
                     ((at->addr_name_res_str == NULL) && (at->addr_name_res_len == NULL)));

    type_list[addr_type] = at;
}

int address_type_dissector_register(const char* name, const char* pretty_name,
                                    AddrValueToString to_str_func, AddrValueToStringLen str_len_func,
                                    AddrValueToByte to_bytes_func, AddrColFilterString col_filter_str_func, AddrFixedLen fixed_len_func,
                                    AddrNameResolutionToString name_res_str_func, AddrNameResolutionLen name_res_len_func)
{
    int addr_type;

    /* Ensure valid data/functions for required fields */
    DISSECTOR_ASSERT(name);
    DISSECTOR_ASSERT(pretty_name);
    DISSECTOR_ASSERT(to_str_func);
    DISSECTOR_ASSERT(str_len_func);
    /* Either have both or neither */
    DISSECTOR_ASSERT(((name_res_str_func != NULL) && (name_res_len_func != NULL)) ||
                     ((name_res_str_func == NULL) && (name_res_len_func == NULL)));

    /* This shouldn't happen, so flag it for fixing */
    DISSECTOR_ASSERT(num_dissector_addr_type < MAX_DISSECTOR_ADDR_TYPE);

    addr_type = AT_END_OF_LIST+num_dissector_addr_type;
    dissector_type_addresses[num_dissector_addr_type].addr_type = addr_type;
    dissector_type_addresses[num_dissector_addr_type].name = name;
    dissector_type_addresses[num_dissector_addr_type].pretty_name = pretty_name;
    dissector_type_addresses[num_dissector_addr_type].addr_to_str = to_str_func;
    dissector_type_addresses[num_dissector_addr_type].addr_str_len = str_len_func;
    dissector_type_addresses[num_dissector_addr_type].addr_to_byte = to_bytes_func;
    dissector_type_addresses[num_dissector_addr_type].addr_col_filter = col_filter_str_func;
    dissector_type_addresses[num_dissector_addr_type].addr_fixed_len = fixed_len_func;
    dissector_type_addresses[num_dissector_addr_type].addr_name_res_str = name_res_str_func;
    dissector_type_addresses[num_dissector_addr_type].addr_name_res_len = name_res_len_func;

    type_list[addr_type] = &dissector_type_addresses[num_dissector_addr_type];

    num_dissector_addr_type++;

    return addr_type;
}

int address_type_get_by_name(const char* name)
{
    address_type_t** addr;

    for (addr = type_list; addr != NULL; addr++)
    {
        if (!strcmp((*addr)->name, name))
        {
            return (*addr)->addr_type;
        }
    }

    return -1;
}

/******************************************************************************
 * AT_NONE
 ******************************************************************************/
int none_addr_to_str(const address* addr _U_, gchar *buf, int buf_len _U_)
{
    buf[0] = '\0';
    return none_addr_str_len(addr);
}

int none_addr_str_len(const address* addr _U_)
{
    return 1; /* NULL character for empty string */
}

int none_addr_len(void)
{
    return 0;
}

static int none_name_res_len(void)
{
    return 5;
}

static const gchar* none_name_res_str(const address* addr _U_)
{
    return "NONE";
}

/******************************************************************************
 * AT_ETHER
 ******************************************************************************/
int ether_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    bytes_to_hexstr_punct(buf, (const guint8*)addr->data, 6, ':');
    buf[17] = '\0';
    return ether_str_len(addr);
}

int ether_str_len(const address* addr _U_)
{
    return 18;
}

static const char* ether_col_filter_str(const address* addr _U_, gboolean is_src)
{
    if (is_src)
        return "eth.src";

    return "eth.dst";
}

int ether_len(void)
{
    return 6;
}

const gchar* ether_name_resolution_str(const address* addr)
{
    return get_ether_name((const guint8 *)addr->data);
}

int ether_name_resolution_len(void)
{
    return MAX_ADDR_STR_LEN; /* XXX - This can be lower */
}

/******************************************************************************
 * AT_IPv4
 ******************************************************************************/
static int ipv4_to_str(const address* addr, gchar *buf, int buf_len)
{
    ip_to_str_buf((const guint8*)addr->data, buf, buf_len);
    return (int)(strlen(buf)+1);
}

static int ipv4_str_len(const address* addr _U_)
{
    return MAX_IP_STR_LEN;
}

static const char* ipv4_col_filter_str(const address* addr _U_, gboolean is_src)
{
    if (is_src)
        return "ip.src";

    return "ip.dst";
}

static int ipv4_len(void)
{
    return 4;
}

static const gchar* ipv4_name_res_str(const address* addr)
{
    guint32 ip4_addr;
    memcpy(&ip4_addr, addr->data, sizeof ip4_addr);
    return get_hostname(ip4_addr);
}

static int ipv4_name_res_len(void)
{
    return MAX_ADDR_STR_LEN; /* XXX - This can be lower */
}

/******************************************************************************
 * AT_IPv6
 ******************************************************************************/
static int ipv6_to_str(const address* addr, gchar *buf, int buf_len)
{
    ip6_to_str_buf((const struct e_in6_addr *)addr->data, buf, buf_len);
    return (int)(strlen(buf)+1);
}

static int ipv6_str_len(const address* addr _U_)
{
    return MAX_IP6_STR_LEN;
}

static const char* ipv6_col_filter_str(const address* addr _U_, gboolean is_src)
{
    if (is_src)
        return "ipv6.src";

    return "ipv6.dst";
}

static int ipv6_len(void)
{
    return 16;
}

static const gchar* ipv6_name_res_str(const address* addr)
{
    struct e_in6_addr ip6_addr;
    memcpy(&ip6_addr.bytes, addr->data, sizeof ip6_addr.bytes);
    return get_hostname6(&ip6_addr);
}

static int ipv6_name_res_len(void)
{
    return MAX_ADDR_STR_LEN; /* XXX - This can be lower */
}

/******************************************************************************
 * AT_IPX
 ******************************************************************************/
static int ipx_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    const guint8 *addrdata = (const guint8 *)addr->data;
    gchar *bufp = buf;

    bufp = bytes_to_hexstr(bufp, &addrdata[0], 4); /* 8 bytes */
    *bufp++ = '.'; /*1 byte */
    bufp = bytes_to_hexstr(bufp, &addrdata[4], 6); /* 12 bytes */
    *bufp++ = '\0'; /* NULL terminate */
    return (int)(bufp - buf);
}

static int ipx_str_len(const address* addr _U_)
{
    return 22;
}

static int ipx_len(void)
{
    return 10;
}

/******************************************************************************
 * AT_FC
 ******************************************************************************/
static int fc_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    gchar *bufp = buf;

    bufp = bytes_to_hexstr_punct(bufp, (const guint8 *)addr->data, 3, '.');
    *bufp++ = '\0'; /* NULL terminate */

    return (int)(bufp - buf);
}

static int fc_str_len(const address* addr _U_)
{
    return 9;
}

static int fc_len(void)
{
    return 3;
}

/******************************************************************************
 * AT_FCWWN
 * XXX - Doubles as a "field type", should it be defined here?
 ******************************************************************************/
/* FC Network Header Network Address Authority Identifiers */
#define FC_NH_NAA_IEEE          1   /* IEEE 802.1a */
#define FC_NH_NAA_IEEE_E        2   /* IEEE Exteneded */
#define FC_NH_NAA_LOCAL         3
#define FC_NH_NAA_IP            4   /* 32-bit IP address */
#define FC_NH_NAA_IEEE_R        5   /* IEEE Registered */
#define FC_NH_NAA_IEEE_R_E      6   /* IEEE Registered Exteneded */
/* according to FC-PH 3 draft these are now reclaimed and reserved */
#define FC_NH_NAA_CCITT_INDV    12  /* CCITT 60 bit individual address */
#define FC_NH_NAA_CCITT_GRP     14  /* CCITT 60 bit group address */

static int fcwwn_str_len(const address* addr _U_)
{
    return 24;
}

static int fcwwn_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    const guint8 *addrp = (const guint8*)addr->data;

    buf = bytes_to_hexstr_punct(buf, addrp, 8, ':'); /* 23 bytes */
    *buf = '\0';

    return fcwwn_str_len(addr);
}

static int fcwwn_len(void)
{
    return FCWWN_ADDR_LEN;
}

static const gchar* fcwwn_name_res_str(const address* addr)
{
    const guint8 *addrp = (const guint8*)addr->data;
    int fmt;
    guint8 oui[6];

    fmt = (addrp[0] & 0xF0) >> 4;
    switch (fmt) {

    case FC_NH_NAA_IEEE:
    case FC_NH_NAA_IEEE_E:

        memcpy (oui, &addrp[2], 6);
        return get_manuf_name(oui);

    case FC_NH_NAA_IEEE_R:
        oui[0] = ((addrp[0] & 0x0F) << 4) | ((addrp[1] & 0xF0) >> 4);
        oui[1] = ((addrp[1] & 0x0F) << 4) | ((addrp[2] & 0xF0) >> 4);
        oui[2] = ((addrp[2] & 0x0F) << 4) | ((addrp[3] & 0xF0) >> 4);
        oui[3] = ((addrp[3] & 0x0F) << 4) | ((addrp[4] & 0xF0) >> 4);
        oui[4] = ((addrp[4] & 0x0F) << 4) | ((addrp[5] & 0xF0) >> 4);
        oui[5] = ((addrp[5] & 0x0F) << 4) | ((addrp[6] & 0xF0) >> 4);

        return get_manuf_name(oui);
    }

    return "";
}

static int fcwwn_name_res_len(void)
{
    return MAX_ADDR_STR_LEN; /* XXX - This can be lower */
}

/******************************************************************************
 * AT_STRINGZ
 ******************************************************************************/
static int stringz_addr_to_str(const address* addr, gchar *buf, int buf_len)
{
    g_strlcpy(buf, (const gchar *)addr->data, buf_len);
    return (int)(strlen(buf)+1);
}

static int stringz_addr_str_len(const address* addr)
{
    return addr->len+1;
}

/******************************************************************************
 * AT_EUI64
 ******************************************************************************/
static int eui64_addr_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    buf = bytes_to_hexstr_punct(buf, (const guint8 *)addr->data, 8, ':');
    *buf = '\0'; /* NULL terminate */
    return sizeof(buf) + 1;
}

static int eui64_str_len(const address* addr _U_)
{
    return EUI64_STR_LEN;
}

static int eui64_len(void)
{
    return 8;
}

/******************************************************************************
 * AT_IB
 ******************************************************************************/
static int
ib_addr_to_str( const address *addr, gchar *buf, int buf_len){
    if (addr->len >= 16) { /* GID is 128bits */
        #define PREAMBLE_STR_LEN ((int)(sizeof("GID: ") - 1))
        g_strlcpy(buf, "GID: ", buf_len);
        if (buf_len < PREAMBLE_STR_LEN ||
                ws_inet_ntop6(addr->data, buf + PREAMBLE_STR_LEN,
                          buf_len - PREAMBLE_STR_LEN) == NULL ) /* Returns NULL if no space and does not touch buf */
            g_strlcpy(buf, BUF_TOO_SMALL_ERR, buf_len); /* Let the unexpected value alert user */
    } else {    /* this is a LID (16 bits) */
        guint16 lid_number;

        memcpy((void *)&lid_number, addr->data, sizeof lid_number);
        g_snprintf(buf,buf_len,"LID: %u",lid_number);
    }

    return sizeof(buf) + 1;
}

static int ib_str_len(const address* addr _U_)
{
    return MAX_ADDR_STR_LEN; /* XXX - This is overkill */
}

/******************************************************************************
 * AT_AX25
 ******************************************************************************/
static int ax25_addr_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    const guint8 *addrdata = (const guint8 *)addr->data;
    gchar *bufp = buf;

    *bufp++ = printable_char_or_period(addrdata[0] >> 1);
    *bufp++ = printable_char_or_period(addrdata[1] >> 1);
    *bufp++ = printable_char_or_period(addrdata[2] >> 1);
    *bufp++ = printable_char_or_period(addrdata[3] >> 1);
    *bufp++ = printable_char_or_period(addrdata[4] >> 1);
    *bufp++ = printable_char_or_period(addrdata[5] >> 1);
    *bufp++ = '-';
    bufp = uint_to_str_back(bufp, (addrdata[6] >> 1) & 0x0f);
    *bufp++ = '\0'; /* NULL terminate */

    return (int)(bufp - buf);
}

static int ax25_addr_str_len(const address* addr _U_)
{
    return 21; /* Leaves extra space (10 bytes) just for uint_to_str_back() */
}

static const char* ax25_col_filter_str(const address* addr _U_, gboolean is_src)
{
    if (is_src)
        return "ax25.src";

    return "ax25.dst";
}

static int ax25_len(void)
{
    return AX25_ADDR_LEN;
}

/******************************************************************************
 * END OF PROVIDED ADDRESS TYPES
 ******************************************************************************/




void address_types_initialize(void)
{
    static address_type_t none_address = {
        AT_NONE,            /* addr_type */
        "AT_NONE",          /* name */
        "No address",       /* pretty_name */
        none_addr_to_str,   /* addr_to_str */
        none_addr_str_len,  /* addr_str_len */
        NULL,               /* addr_to_byte */
        NULL,               /* addr_col_filter */
        none_addr_len,      /* addr_fixed_len */
        none_name_res_str, /* addr_name_res_str */
        none_name_res_len, /* addr_name_res_len */
    };

    static address_type_t ether_address = {
        AT_ETHER,           /* addr_type */
        "AT_ETHER",         /* name */
        "Ethernet address", /* pretty_name */
        ether_to_str,       /* addr_to_str */
        ether_str_len,      /* addr_str_len */
        NULL,               /* addr_to_byte */
        ether_col_filter_str, /* addr_col_filter */
        ether_len,          /* addr_fixed_len */
        ether_name_resolution_str, /* addr_name_res_str */
        ether_name_resolution_len, /* addr_name_res_len */
    };

    static address_type_t ipv4_address = {
        AT_IPv4,            /* addr_type */
        "AT_IPv4",          /* name */
        "IPv4 address",     /* pretty_name */
        ipv4_to_str,        /* addr_to_str */
        ipv4_str_len,       /* addr_str_len */
        NULL,               /* addr_to_byte */
        ipv4_col_filter_str, /* addr_col_filter */
        ipv4_len,           /* addr_fixed_len */
        ipv4_name_res_str, /* addr_name_res_str */
        ipv4_name_res_len, /* addr_name_res_len */
    };

    static address_type_t ipv6_address = {
        AT_IPv6,            /* addr_type */
        "AT_IPv6",          /* name */
        "IPv6 address",     /* pretty_name */
        ipv6_to_str,        /* addr_to_str */
        ipv6_str_len,       /* addr_str_len */
        NULL,               /* addr_to_byte */
        ipv6_col_filter_str, /* addr_col_filter */
        ipv6_len,            /* addr_fixed_len */
        ipv6_name_res_str, /* addr_name_res_str */
        ipv6_name_res_len, /* addr_name_res_len */
   };

    static address_type_t ipx_address = {
        AT_IPX,             /* addr_type */
        "AT_IPX",           /* name */
        "IPX address",      /* pretty_name */
        ipx_to_str,         /* addr_to_str */
        ipx_str_len,        /* addr_str_len */
        NULL,               /* addr_to_byte */
        NULL,               /* addr_col_filter */
        ipx_len,            /* addr_fixed_len */
        NULL,               /* addr_name_res_str */
        NULL,               /* addr_name_res_len */
    };

    static address_type_t fc_address = {
        AT_FC,          /* addr_type */
        "AT_FC",        /* name */
        "FC address",   /* pretty_name */
        fc_to_str,      /* addr_to_str */
        fc_str_len,     /* addr_str_len */
        NULL,           /* addr_to_byte */
        NULL,           /* addr_col_filter */
        fc_len,         /* addr_fixed_len */
        NULL,           /* addr_name_res_str */
        NULL,           /* addr_name_res_len */
    };

    static address_type_t fcwwn_address = {
        AT_FCWWN,       /* addr_type */
        "AT_FCWWN",     /* name */
        "Fibre Channel WWN",    /* pretty_name */
        fcwwn_to_str,   /* addr_to_str */
        fcwwn_str_len,  /* addr_str_len */
        NULL,           /* addr_to_byte */
        NULL,           /* addr_col_filter */
        fcwwn_len,         /* addr_fixed_len */
        fcwwn_name_res_str, /* addr_name_res_str */
        fcwwn_name_res_len, /* addr_name_res_len */
    };

    static address_type_t stringz_address = {
        AT_STRINGZ,          /* addr_type */
        "AT_STRINGZ",        /* name */
        "String address",   /* pretty_name */
        stringz_addr_to_str, /* addr_to_str */
        stringz_addr_str_len, /* addr_str_len */
        NULL,              /* addr_to_byte */
        NULL,              /* addr_col_filter */
        NULL,              /* addr_fixed_len */
        NULL,              /* addr_name_res_str */
        NULL,              /* addr_name_res_len */
    };

    static address_type_t eui64_address = {
        AT_EUI64,          /* addr_type */
        "AT_EUI64",        /* name */
        "IEEE EUI-64",     /* pretty_name */
        eui64_addr_to_str, /* addr_to_str */
        eui64_str_len,     /* addr_str_len */
        NULL,              /* addr_to_byte */
        NULL,              /* addr_col_filter */
        eui64_len,         /* addr_fixed_len */
        NULL,              /* addr_name_res_str */
        NULL,              /* addr_name_res_len */
    };

    static address_type_t ib_address = {
        AT_IB,           /* addr_type */
        "AT_IB",         /* name */
        "Infiniband GID/LID",   /* pretty_name */
        ib_addr_to_str,  /* addr_to_str */
        ib_str_len,      /* addr_str_len */
        NULL,              /* addr_to_byte */
        NULL,              /* addr_col_filter */
        NULL,              /* addr_fixed_len */
        NULL,              /* addr_name_res_str */
        NULL,              /* addr_name_res_len */
    };

    static address_type_t ax25_address = {
        AT_AX25,          /* addr_type */
        "AT_AX25",        /* name */
        "AX.25 Address",  /* pretty_name */
        ax25_addr_to_str, /* addr_to_str */
        ax25_addr_str_len,/* addr_str_len */
        NULL,             /* addr_to_byte */
        ax25_col_filter_str, /* addr_col_filter */
        ax25_len,          /* addr_fixed_len */
        NULL,              /* addr_name_res_str */
        NULL,              /* addr_name_res_len */
    };

    num_dissector_addr_type = 0;

    /* Initialize the type array.  This is mostly for handling
       "dissector registered" address type range (for NULL checking) */
    memset(type_list, 0, MAX_ADDR_TYPE_VALUE*sizeof(address_type_t*));

    address_type_register(AT_NONE, &none_address );
    address_type_register(AT_ETHER, &ether_address );
    address_type_register(AT_IPv4, &ipv4_address );
    address_type_register(AT_IPv6, &ipv6_address );
    address_type_register(AT_IPX, &ipx_address );
    address_type_register(AT_FC, &fc_address );
    address_type_register(AT_FCWWN, &fcwwn_address );
    address_type_register(AT_STRINGZ, &stringz_address );
    address_type_register(AT_EUI64, &eui64_address );
    address_type_register(AT_IB, &ib_address );
    address_type_register(AT_AX25, &ax25_address );
}

/* Given an address type id, return an address_type_t* */
#define ADDR_TYPE_LOOKUP(addr_type, result)    \
    /* Check input */                          \
    g_assert(addr_type < MAX_ADDR_TYPE_VALUE); \
    result = type_list[addr_type];

static int address_type_get_length(const address* addr)
{
    address_type_t *at;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if (at == NULL)
        return 0;

    return at->addr_str_len(addr);
}

gchar*
address_to_str(wmem_allocator_t *scope, const address *addr)
{
    gchar *str;
    int len = address_type_get_length(addr);

    if (len <= 0)
        len = MAX_ADDR_STR_LEN;

    str=(gchar *)wmem_alloc(scope, len);
    address_to_str_buf(addr, str, len);
    return str;
}

void address_to_str_buf(const address* addr, gchar *buf, int buf_len)
{
    address_type_t *at;

    if (!buf || !buf_len)
        return;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if ((at == NULL) || (at->addr_to_str == NULL))
    {
        buf[0] = '\0';
        return;
    }

    at->addr_to_str(addr, buf, buf_len);
}


guint address_to_bytes(const address *addr, guint8 *buf, guint buf_len)
{
    address_type_t *at;
    guint copy_len = 0;

    if (!buf || !buf_len)
        return 0;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if (at == NULL)
        return 0;

    if (at->addr_to_byte == NULL)
    {
        /* If a specific function isn't provided, just do a memcpy */
        copy_len = MIN(((guint)addr->len), buf_len);
        memcpy(buf, addr->data, copy_len);
    }
    else
    {
        copy_len = at->addr_to_byte(addr, buf, buf_len);
    }

    return copy_len;
}

const gchar *
address_to_name(const address *addr)
{
    address_type_t *at;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if (at == NULL)
    {
        return NULL;
    }

    /*
     * XXX - addr_name_res_str is expected to return a string from
     * a persistent database, so that it lives a long time, past
     * the lifetime of addr itself.
     *
     * We'd like to avoid copying, so this is what we do here.
     */
    switch (addr->type) {

    case AT_STRINGZ:
        return (const gchar *)addr->data;

    default:
        if (at->addr_name_res_str != NULL)
            return at->addr_name_res_str(addr);
        else
            return NULL;
    }
}

gchar *
address_to_display(wmem_allocator_t *allocator, const address *addr)
{
    gchar *str = NULL;
    const gchar *result = address_to_name(addr);

    if (result != NULL) {
        str = wmem_strdup(allocator, result);
    }
    else if (addr->type == AT_NONE) {
        str = wmem_strdup(allocator, "NONE");
    }
    else {
        str = (gchar *) wmem_alloc(allocator, MAX_ADDR_STR_LEN);
        address_to_str_buf(addr, str, MAX_ADDR_STR_LEN);
    }

    return str;
}

static void address_with_resolution_to_str_buf(const address* addr, gchar *buf, int buf_len)
{
    address_type_t *at;
    int addr_len;
    gsize pos;
    gboolean empty;

    if (!buf || !buf_len)
        return;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if (at == NULL)
    {
        buf[0] = '\0';
        return;
    }

#if 0 /* XXX - If this remains a static function, we've already made this check in the only
         function that can call it.  If this function becomes "public", need to put this
         check back in */
    /* No name resolution support, just return address string */
    if (at->addr_name_res_str == NULL)
        return address_to_str_buf(addr, buf, buf_len);
#endif

    /* Copy the resolved name */
    pos = g_strlcpy(buf, at->addr_name_res_str(addr), buf_len);

    /* Don't wrap "emptyness" in parentheses */
    if (addr->type == AT_NONE)
        return;

    /* Make sure there is enough room for the address string wrapped in parentheses */
    if ((int)(pos + 4 + at->addr_str_len(addr)) >= buf_len)
        return;

    empty = (pos <= 1) ? TRUE : FALSE;

    if (!empty)
    {
        buf[pos++] = ' ';
        buf[pos++] = '(';
    }

    addr_len = at->addr_to_str(addr, &buf[pos], (int)(buf_len-pos));
    pos += addr_len - 1; /* addr_len includes the trailing '\0' */

    if (!empty)
    {
        buf[pos++] = ')';
        buf[pos++] = '\0';
    }
}

gchar* address_with_resolution_to_str(wmem_allocator_t *scope, const address *addr)
{
    address_type_t *at;
    int len;
    gchar *str;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if (at == NULL)
        return wmem_strdup(scope, "");

    /* No name resolution support, just return address string */
    if ((at->addr_name_res_str == NULL) ||
            (ADDR_RESOLV_MACADDR(addr) && !gbl_resolv_flags.mac_name) ||
            (ADDR_RESOLV_NETADDR(addr) && !gbl_resolv_flags.network_name)) {
        return address_to_str(scope, addr);
    }

    len = at->addr_name_res_len() + at->addr_str_len(addr) + 4; /* For format of %s (%s) */

    str=(gchar *)wmem_alloc(scope, len);
    address_with_resolution_to_str_buf(addr, str, len);
    return str;
}


const char* address_type_column_filter_string(const address* addr, gboolean src)
{
    address_type_t *at;

    ADDR_TYPE_LOOKUP(addr->type, at);

    if ((at == NULL) || (at->addr_col_filter == NULL))
    {
        return "";
    }

    return at->addr_col_filter(addr, src);
}

gchar*
tvb_address_to_str(wmem_allocator_t *scope, tvbuff_t *tvb, int type, const gint offset)
{
    address addr;
    address_type_t *at;

    ADDR_TYPE_LOOKUP(type, at);

    if (at == NULL)
    {
        return NULL;
    }

    /* The address type must have a fixed length to use this function */
    /* For variable length fields, use tvb_address_var_to_str() */
    if (at->addr_fixed_len == NULL)
    {
        g_assert_not_reached();
        return NULL;
    }

    set_address_tvb(&addr, type, at->addr_fixed_len(), tvb, offset);

    return address_to_str(scope, &addr);
}

gchar* tvb_address_var_to_str(wmem_allocator_t *scope, tvbuff_t *tvb, address_type type, const gint offset, int length)
{
    address addr;

    set_address_tvb(&addr, type, length, tvb, offset);

    return address_to_str(scope, &addr);
}

gchar*
tvb_address_with_resolution_to_str(wmem_allocator_t *scope, tvbuff_t *tvb, int type, const gint offset)
{
    address addr;
    address_type_t *at;

    ADDR_TYPE_LOOKUP(type, at);

    if (at == NULL)
    {
        return NULL;
    }

    /* The address type must have a fixed length to use this function */
    /* For variable length fields, use tvb_address_var_with_resolution_to_str() */
    if (at->addr_fixed_len == NULL)
    {
        g_assert_not_reached();
        return NULL;
    }

    set_address_tvb(&addr, type, at->addr_fixed_len(), tvb, offset);

    return address_with_resolution_to_str(scope, &addr);
}


/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
