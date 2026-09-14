#ifndef HAVE_CHANNELHOPPER_H
#define HAVE_CHANNELHOPPER_H

//Takes a list of channels as string in format "1,2,3,4,5,6,7"
//Starts hopping channels, switches channel round-robin style every useconds µs
//If called again, it does nothing, only one hopper is possible (of course)
void init_channel_hopper(char *chanlist, int useconds);

#endif
