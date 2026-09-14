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
struct greylist *blist = NULL;
struct greylist *wlist = NULL;

char black = 0;
char white = 0;


struct greylist *add_to_greylist(struct ether_addr new, struct greylist *gl) {
  struct greylist *gnew = malloc(sizeof(struct greylist));

  gnew->mac = new;

  if (gl) {
    gnew->next = gl->next;
    gl->next = gnew;
  } else {
    gl = gnew;
    gnew->next = gnew;
  }

  return gl;
}

struct greylist *search_in_greylist(struct ether_addr mac, struct greylist *gl) {
  struct greylist *first;

  if (! gl) return NULL;

  first = gl;
  
  do {
    if (MAC_MATCHES(mac, gl->mac)) {
      return gl;
    }
    gl = gl->next;
  } while (gl != first);

  return NULL;
}

void load_greylist(greylist_type type, char *filename) {
  char *entry;

  if (filename) {

    entry = read_next_line(filename, 1);
    while(entry) 
    {
      if (!search_in_greylist(parse_mac(entry), glist)) //Only add new entries
      {
	      glist = add_to_greylist(parse_mac(entry), glist);
      }
      free(entry);
      entry = read_next_line(filename, 0);
    }

    if(type == BLACK_LIST)
    {
      blist = glist;
      black = 1;
    }
    else if(type == WHITE_LIST)
    {
      wlist = glist;
      white = 1;
    }
  }
}

void load_blacklist(char *filename)
{
  load_greylist(BLACK_LIST, filename);
}

void load_whitelist(char *filename)
{
  load_greylist(WHITE_LIST, filename);
}

char is_blacklisted(struct ether_addr mac) 
{
  struct greylist *entry; 

  if (black) 
  {
     entry = search_in_greylist(mac, blist);
    if (entry) 
      return 1;
  } 

  return 0;
}

char is_whitelisted(struct ether_addr mac) 
{
  struct greylist *entry; 

  if (white) 
  {
    entry = search_in_greylist(mac, wlist);

    if (entry) 
      return 1;
  } 

  return 0;
}
