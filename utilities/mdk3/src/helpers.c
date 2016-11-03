#define _GNU_SOURCE //For getline() support

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "helpers.h"

char generate_channel()
{
// Generate a random channel

    char c = 0;
    c = (random() % 14) + 1;
    return c;
}


char generate_printable_char(unsigned char malformed)
{
// Generate random printable ascii char, or just a random byte

    char rnd = 0;

    if (malformed) {
      rnd = random();
    } else {
      rnd = (random() % 94) + ' ';
    }

    return rnd;
}


char *generate_ssid(unsigned char malformed)
{
    char *ssid = (char*) malloc(256);
    int len=0;
    int t;

    if (malformed) {
      len = (random() % 256);
    } else {
      len = (random() % 32);
    }

    for (t=0; t<len; t++) ssid[t] = generate_printable_char(malformed);
    ssid[len]='\x00';

    return ssid;
}


int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void sleep_till_next_packet(unsigned int pps) {
  static struct timeval *lastvisit = NULL;
  struct timeval now, next, wait;
  unsigned int tbp;
  
  if (!pps) return;

  if (! lastvisit) {
    lastvisit = malloc(sizeof(struct timeval));
    gettimeofday(lastvisit, NULL);
    return;
  }
  
  next.tv_sec = lastvisit->tv_sec; next.tv_usec = lastvisit->tv_usec;
  tbp = 1000000 / pps;
  next.tv_usec += tbp;
  if (next.tv_usec > 999999) { next.tv_usec -= 1000000; next.tv_sec++; }
  
  gettimeofday(&now, NULL);
  if (! timeval_subtract(&wait, &next, &now))
    usleep(wait.tv_usec);
  
  gettimeofday(lastvisit, NULL);
}

char *read_next_line(char *filename, char reset)
{
    static int last_pos = 0;
    int bytesread;
    char *line = NULL;
    char **pline = &line;
    FILE *file_fp;
    size_t initsize = 1;
    
    if (reset) last_pos = 0;
    
    if ((file_fp = fopen(filename, "r")) == NULL) {
      printf("Cannot open file: %s\n", filename);
      exit(2);
    }
    
    fseek(file_fp, last_pos, SEEK_SET);
    bytesread = getline(pline, &initsize, file_fp);
    line = *pline;
    
    if (bytesread == -1) {
      last_pos = 0;
      free(line);	//Thanks valgrind for getting this BITCH. even if nothing is read from file, memory is still allocated by getline!
      fclose(file_fp);
      return NULL;
    }

    last_pos = ftell(file_fp);
    fclose(file_fp);
    
    //Remove newline if any
    if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = 0x00;
    
    return line;
}

unsigned char *hex2bin(char *in, int *len) {
  *len = strlen(in) / 2;
  unsigned char *out = malloc(*len);
  int i;
  
  for (i=0; i<*len; i++) {
    sscanf(in + (i*2), "%2hhx", out+i);
  }
  
  return out;
}
