#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "manufactor.h"
#include "mac_addr.h"


struct ether_addr parse_mac(char *input)
{
// Parsing input MAC adresses like 00:00:11:22:aa:BB or 00001122aAbB
    struct ether_addr mac_p;
    uint8_t *bytes = mac_p.ether_addr_octet;

    if (input[2] == ':') {
      sscanf(input, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", bytes, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);
    } else {
      sscanf(input, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx", bytes, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);
    }

    return mac_p;
}


struct ether_addr parse_half_mac(char *input)
{
// Parsing input half MAC adresses like 00:00:11 or 000011
// Octets 3 to 5 will be 0x00
    struct ether_addr mac_p;
    uint8_t *bytes = mac_p.ether_addr_octet;

    if (input[2] == ':') {
      sscanf(input, "%hhx:%hhx:%hhx", bytes, bytes+1, bytes+2);
    } else {
      sscanf(input, "%2hhx%2hhx%2hhx", bytes, bytes+1, bytes+2);
    }

    bytes[3] = bytes[4] = bytes[5] = 0x00;
    
    return mac_p;
}


struct ether_addr generate_valid_mac(int type, int list_len)
{
    int t, pos;
    struct ether_addr mac_v;

    pos = random();
    pos = pos % list_len;

    // SAMPLE LINE
    // 000123000000/FFFFFF000000

    if (type == 0) {
	for (t=0; t<ETHER_ADDR_LEN; t++) {
	    if (!memcmp(clients[pos]+(t*2+13), "FF", 2) || !memcmp(clients[pos]+(t*2+13), "ff", 2)) {
	        sscanf((char *) (clients[pos] + (2 * t)), "%2hhx", &mac_v.ether_addr_octet[t]);
	    } else mac_v.ether_addr_octet[t] = random();
	}
    } else {
	for (t=0; t<ETHER_ADDR_LEN; t++) {
	    if (!memcmp(accesspoints[pos]+(t*2+13), "FF", 2) || !memcmp(accesspoints[pos]+(t*2+13), "ff", 2)) {
	        sscanf((char *) (accesspoints[pos] + (2 * t)), "%2hhx", &mac_v.ether_addr_octet[t]);
	    } else mac_v.ether_addr_octet[t] = random();
	}
    }

    return mac_v;
}


struct ether_addr generate_mac(enum mac_kind kind)
{
// Generate a random MAC adress
// kind : Which kind of MAC should be generated?
//    0 : random MAC
//    1 : valid client MAC
//    2 : valid accesspoint MAC

    struct ether_addr gmac;
    int t;

    for (t=0; t<ETHER_ADDR_LEN; t++)
        gmac.ether_addr_octet[t] = random();

    if (kind == MAC_KIND_CLIENT)
	gmac = generate_valid_mac(0, clients_count);
    if (kind == MAC_KIND_AP)
	gmac = generate_valid_mac(1, accesspoints_count);
    
    return gmac;
}


void increase_mac_adress(struct ether_addr *macaddr)
{
    macaddr->ether_addr_octet[2]++;
    if (macaddr->ether_addr_octet[2] == 0) {
	macaddr->ether_addr_octet[1]++;
	if (macaddr->ether_addr_octet[1] == 0) {
	    macaddr->ether_addr_octet[0]++;
	}
    }
}


struct ether_addr get_next_mac(struct ether_addr mac_base, struct ether_addr *mac_lower)
{
    static int pos = -2;
    static struct ether_addr lowb;
    static struct ether_addr upb;
    struct ether_addr mac_v;
    
    if (pos == -2) {
	MAC_SET_BCAST(lowb);
	MAC_SET_BCAST(upb);
	pos = -1;
    }

    if (MAC_IS_NULL(mac_base)) {	//Use internal database
	//Increase lower bytes
	increase_mac_adress(&lowb);
	//Get new upper bytes?
	if (! memcmp(lowb.ether_addr_octet, "\x00\x00\x00", 3)) {
	    //New pos in client list
	    pos++;
	    if (pos == clients_count) {
		MAC_SET_BCAST((*mac_lower));
		MAC_SET_NULL(mac_v);
		return mac_v;
	    }
	    //Filling the first three bytes
	    sscanf((char *) clients[pos], "%2hhx%2hhx%2hhx", upb.ether_addr_octet, upb.ether_addr_octet+1, upb.ether_addr_octet+2);
	}
	memcpy(mac_v.ether_addr_octet, upb.ether_addr_octet, 3);
	memcpy(mac_v.ether_addr_octet+3, lowb.ether_addr_octet, 3);
    } else {				//Use MAC given by user
	increase_mac_adress(&lowb);

	if (! MAC_IS_NULL(*mac_lower)) {	//Use start MAC given by user
	    memcpy(lowb.ether_addr_octet, mac_lower->ether_addr_octet, 3);
	    MAC_SET_NULL(*mac_lower);
	}

	if (! memcmp(lowb.ether_addr_octet, "\xFF\xFF\xFF", 3)) {
	    MAC_SET_BCAST((*mac_lower));
	    MAC_SET_NULL(mac_v);
	    return mac_v;
	}
	memcpy(mac_v.ether_addr_octet, mac_base.ether_addr_octet, 3);
	memcpy(mac_v.ether_addr_octet+3, lowb.ether_addr_octet, 3);
    }

    return mac_v;
}


void print_mac(struct ether_addr pmac)
{
    uint8_t *p = pmac.ether_addr_octet;
    
    printf("%02X:%02X:%02X:%02X:%02X:%02X", p[0], p[1], p[2], p[3], p[4], p[5]);
}