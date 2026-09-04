#include <stdlib.h>
#include <string.h>

#include "attacks.h"
#include "beacon_flood.h"
#include "auth_dos.h"
#include "probing.h"
#include "deauth.h"
#include "countermeasures.h"
#include "eapol.h"
#include "ieee80211s.h"
#include "wids.h"
#include "fuzzer.h"
#include "poc.h"


int attack_count = 10;

struct attacks *load_attacks(int *count) {
  struct attacks *attacks = malloc(sizeof(struct attacks) * attack_count);

  attacks[0] = load_beacon_flood();
  attacks[1] = load_auth_dos();
  attacks[2] = load_probing();
  attacks[3] = load_deauth();
  attacks[4] = load_countermeasures();
  attacks[5] = load_eapol();
  attacks[6] = load_ieee80211s();
  attacks[7] = load_wids();
  attacks[8] = load_fuzz();
  attacks[9] = load_poc();

  *count = attack_count;
  return attacks;
}
