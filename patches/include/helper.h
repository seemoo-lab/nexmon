#ifndef HELPER_H
#define HELPER_H

#include "types.h"

void *
skb_push(sk_buff *p, unsigned int len);

void *
skb_pull(sk_buff *p, unsigned int len);

// somehow the strings are not removed during optimization, so that they end up in the binary, hence, move the functions somewhere else, where they are only included if they are needed.
inline uint16
htons(uint16 a)
{
	return (a & 0xff00) >> 8 | (a & 0xff) << 8;
}
/*

inline uint32
htonl(uint32 a)
{
	return (a & 0xff000000) >> 24 | (a & 0xff0000) >> 8 | (a & 0xff00) << 8 | (a & 0xff) << 24;
}

inline uint16
ntohs(uint16 a)
{
	return htons(a);
}

inline uint32
ntohl(uint32 a)
{
	return htonl(a);
}*/

inline void *
get_stack_ptr() {
    void *stack = 0x0;
    __asm("mov %0, sp" : "=r" (stack));
    return stack;
}

/*
inline int
get_register(int reg_nr) {
    int reg_content = 0;
    switch(reg_nr) {
        case 0:
            __asm("mov %0, r0" : "=r" (reg_content));
            break;
        case 1:
            __asm("mov %0, r1" : "=r" (reg_content));
            break;
        case 2:
            __asm("mov %0, r2" : "=r" (reg_content));
            break;
        case 3:
            __asm("mov %0, r3" : "=r" (reg_content));
            break;
        case 10:
            __asm("mov %0, r10" : "=r" (reg_content));
            break;
        default:
            // not impl. do nothing
            break;
    }
    return reg_content;
}

inline void
copy_stack(void *dest, int copy_size) {
    printf("copy_stack: %d\n", copy_size);
    if(copy_size > 0) {
        memcpy( (void *) (dest), get_stack_ptr(), copy_size);
    }
    return;
}

inline void
hexdump(char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != 0)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
*/

#endif /* HELPER_H */
