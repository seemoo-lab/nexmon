#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "osdep.h"

#define MAX_CHAN_COUNT 128

int chans [MAX_CHAN_COUNT] = { 1, 7, 13, 2, 8, 3, 14, 9, 4, 10, 5, 11, 6, 12, 0 };

pthread_t *hopper = NULL;

int hopper_useconds = 0;

void channel_hopper()
{
    // A simple thread to hop channels
    int cclp = 0;
    
    while (1) {
	osdep_set_channel(chans[cclp]);
	cclp++;
	if (chans[cclp] == 0) cclp = 0;
	usleep(hopper_useconds);
    }
}

void init_channel_hopper(char *chanlist, int useconds)
{
    // Channel list chans[MAX_CHAN_COUNT] has been initialized with declaration for all b/g channels
    char *token = NULL;
    int chan_cur = EOF;
    int lpos = 0;

    if (hopper) {
      printf("There is already a channel hopper running, skipping this one!\n");
    }
    
    if (chanlist == NULL) {    // No channel list given - using defaults
	printf("\nUsing default channels for hopping every %d milliseconds.\n", useconds/1000);
    } else {

	while((token = strsep(&chanlist, ",")) != NULL) {
	    if(sscanf(token, "%d", &chan_cur) != EOF) {
		chans[lpos] = chan_cur;
		lpos++;
		if (lpos == MAX_CHAN_COUNT) {
		    fprintf(stderr, "Exceeded max channel list entries, list truncated.\n");
		    break;
		}
	    }
	}

	chans[lpos] = 0;
    }

    hopper_useconds = useconds;
    hopper = malloc(sizeof(pthread_t));
    pthread_create(hopper, NULL, (void *) channel_hopper, NULL);
}
