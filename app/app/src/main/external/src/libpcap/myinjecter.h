#ifndef MYINJECTER_H_
#define MYINJECTER_H_

#include "myinjecter_return.h"

extern int inject_it(const u_char *packet, int size, char *device, struct ret_msg *my_message);

#endif