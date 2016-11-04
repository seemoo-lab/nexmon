#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "brute.h"

struct row {
  char start;
  char end;
};

struct charclass {
  char ident;
  unsigned char count;
  struct row rows[4];
};

#define CLASSES_COUNT 4
struct charclass classes[CLASSES_COUNT] =
{ { .ident = 'n', .count = 1, .rows = { { .start = '0', .end = '9' } } },
  { .ident = 'l', .count = 1, .rows = { { .start = 'a', .end = 'z' } } },
  { .ident = 'u', .count = 1, .rows = { { .start = 'A', .end = 'Z' } } },
  { .ident = 's', .count = 4, .rows = { { .start = '!', .end = '/' },
					{ .start = ':', .end = '@' },
					{ .start = '[', .end = '`' },
					{ .start = '{', .end = '~' } } } };


struct charclass *get_charclass_for_ident(char ident) {
  int i;
  
  for (i=0; i<CLASSES_COUNT; i++) {
    if (classes[i].ident == ident) return &(classes[i]);
  }
  
  return NULL;
}

struct charclass *get_class_for_char(char ch) {
  int i, j;
  
  for (i=0; i<CLASSES_COUNT; i++) {
    for (j=0; j<classes[i].count; j++) {
      if ((ch >= classes[i].rows[j].start) && (ch <= classes[i].rows[j].end)) return &(classes[i]);
    }
  }
  
  return 0;
}

//Return 1 if current charclass has been exhausted
int increment_char(char *here, struct charclass *curcls) {
  int i;
  
  for (i=0; i<curcls->count; i++) {
    if (*here == curcls->rows[i].end) { //We're at the end of some row!
      if (i == curcls->count - 1) { //And it was the last row
	return 1;
      } else {
	*here = curcls->rows[i + 1].start;
	return 0;
      }
    }
  }
  
  (*here)++;
  return 0;
}

//if last is NULL a new word with length len will be allocated and returned
//Use this word for the next calls to get_brute_word
//if last is set, len is ignored and the next valid word is returned (overwrites last, do not free()!)
char *get_brute_word(char *cls, char *last, unsigned int len) {
  unsigned int i;
  int ii;
  struct charclass *class[len];
  char *nextclass;
  
  for(i=0; i<strlen(cls); i++) {
    if (! get_charclass_for_ident(cls[i])) {
      printf("Unknown character class %c\n", cls[i]);
      return NULL;
    }
  }
  
  if (! last) {
    last = malloc(len + 1);
    for (i=0; i<len; i++) last[i] = get_charclass_for_ident(cls[0])->rows[0].start;
    last[len] = 0x00;
    return last;
  }
  
  for (i=0; i<strlen(last); i++) {
    class[i] = get_class_for_char(last[i]);
    if (! strchr(cls, class[i]->ident)) {
      printf("Character %c is not in any of the selected character classes!\n", last[i]);
      return NULL;
    }
  }
  
  for (ii=strlen(last)-1; ii>=0; ii--) {
    if (increment_char(&(last[ii]), class[ii])) {
      nextclass = strchr(cls, class[ii]->ident) + 1;
      if (nextclass == (cls + strlen(cls))) {
	class[ii] = get_charclass_for_ident(cls[0]);
	last[ii] = class[ii]->rows[0].start;
	continue;
      }
      class[ii] = get_charclass_for_ident(*nextclass);
      last[ii] = class[ii]->rows[0].start;
      return last;
    }
    return last;
  }
  
  return NULL;
}
