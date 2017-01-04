#include <string.h>

#include "attacks_authflood.h"

int attack_count = 1;

struct attacks *load_attacks(int *count) {
  struct attacks *attacks = malloc(sizeof(struct attacks) * attack_count);
  
  attacks[0] = load_auth_dos();


  *count = attack_count;
  return attacks;
}