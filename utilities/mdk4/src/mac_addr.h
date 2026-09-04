#ifndef HAVE_MACADDR_H
#define HAVE_MACADDR_H

#include <string.h>
#include <net/ethernet.h>

#define NULLMAC "\x00\x00\x00\x00\x00\x00"
#define BROADCAST "\xFF\xFF\xFF\xFF\xFF\xFF"

#define MAC_IS_NULL(x) (! memcmp((x).ether_addr_octet, NULLMAC, ETHER_ADDR_LEN))
#define MAC_SET_NULL(x) (memset((x).ether_addr_octet, 0x00, ETHER_ADDR_LEN))
#define MAC_IS_BCAST(x) (! memcmp((x).ether_addr_octet, BROADCAST, ETHER_ADDR_LEN))
#define MAC_SET_BCAST(x) (memset((x).ether_addr_octet, 0xFF, ETHER_ADDR_LEN))
#define MAC_MATCHES(x,y) (! memcmp((x).ether_addr_octet, (y).ether_addr_octet, ETHER_ADDR_LEN))
#define MAC_COPY(x,y) (memcpy((x).ether_addr_octet, (y).ether_addr_octet, ETHER_ADDR_LEN))

#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif

static VARIABLE_IS_NOT_USED struct ether_addr SE_NULLMAC = { .ether_addr_octet = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

enum mac_kind {
  MAC_KIND_RANDOM,
  MAC_KIND_CLIENT,
  MAC_KIND_AP
};

struct ether_addr parse_mac(char *input);

struct ether_addr parse_half_mac(char *input);

// Generate a random MAC adress
// kind : Which kind of MAC should be generated?
//    0 : random MAC
//    1 : valid client MAC
//    2 : valid accesspoint MAC
struct ether_addr generate_mac(enum mac_kind kind);

// mac_base: User-Select first three bytes
//           If mac_base is NULLMAC, it uses base from known manufactor list
// mac_lower: Pointer to the lower 3 bytes of the user-selected start address, that will be zeroed
//            mac_lower will be BROADCAST if we ran out of adresses!
struct ether_addr get_next_mac(struct ether_addr mac_base, struct ether_addr *mac_lower);

void print_mac(struct ether_addr pmac);

#endif
