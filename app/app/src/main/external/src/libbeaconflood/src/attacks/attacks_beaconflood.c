#include <string.h>

#include "attacks_beaconflood.h"

int attack_count = 1;

struct attacks *load_attacks(int *count) {
  struct attacks *attacks = malloc(sizeof(struct attacks) * attack_count);
  
  attacks[0] = load_beacon_flood();


  *count = attack_count;
  return attacks;
}
