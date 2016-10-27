#include <stdio.h>

float f=1.11;
double d=3.14;

char s[] = "2.22,1.234567";

int main()
{
	sscanf(s, "%f,%lf", &f, &d);
    printf("f=%f, d=%lf\n", f, d);
	return 0;
}

#ifndef __NO_SYSTEM_INIT
void SystemInit()
{}
#endif
