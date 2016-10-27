#include <stdio.h>

float f=1.11;
double d=3.14;

int main()
{
    printf("f=%f, d=%f\n", f, d);
	return 0;
}

#ifndef __NO_SYSTEM_INIT
void SystemInit()
{}
#endif
