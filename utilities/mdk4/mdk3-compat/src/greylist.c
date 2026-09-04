#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "greylist.h"
#include "helpers.h"

struct greylist {
  struct ether_addr mac;
  struct greylist *next;
};

struct greylist *glist = NULL;
char black = 0;

struct greylist *add_to_greylist(struct ether_addr new, struct greylist *glist) {
  struct greylist *gnew = malloc(sizeof(struct greylist));
  
  gnew->mac = new;
  
  if (glist) {
    gnew->next = glist->next;
    glist->next = gnew;
  } else {
    glist = gnew;
    gnew->next = gnew;
  }
  
  return glist;
}

struct greylist *search_in_greylist(struct ether_addr mac, struct greylist *glist) {
  struct greylist *first;
  
  if (! glist) return NULL;
  
  first = glist;
  do {
    if (MAC_MATCHES(mac, glist->mac)) {
      return glist;
    }
    glist = glist->next;
  } while (glist != first);
  
  return NULL;
}

void load_greylist(char isblacklist, char *filename) {
  char *entry;
  
  if (filename) {
    entry = read_next_line(filename, 1);
    while(entry) {
      if (! search_in_greylist(parse_mac(entry), glist)) {	//Only add new entries
	glist = add_to_greylist(parse_mac(entry), glist);
      }
      free(entry);
      entry = read_next_line(filename, 0);
    }
  }
  
  black = isblacklist;
}

char is_blacklisted(struct ether_addr mac) {
  struct greylist *entry = search_in_greylist(mac, glist);
  
  if (black) {
    if (entry) return 1;
    else return 0;
  } else {
    if (entry) return 0;
    else return 1;
  }
}

char is_whitelisted(struct ether_addr mac) {
  if (is_blacklisted(mac)) {
    return 0;
  } else {
    return 1;
  }
}
