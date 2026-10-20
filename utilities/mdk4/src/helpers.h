#ifndef HAVE_HELPERS_H
#define HAVE_HELPERS_H

char generate_channel();

char *generate_ssid(unsigned char malformed);

// Call this again to read line after line
// At end of file, it returns NULL
// Start from the beginning by setting reset true
char *read_next_line(char *filename, char reset);

//Sleeps till the next packet should be sent base on pps packets per second
void sleep_till_next_packet(unsigned int pps);

//Convert hex input to binary memory, user needs to free result
unsigned char *hex2bin(char *in, int *len);

#endif