#include <stdio.h>

extern unsigned long int geteflags(void);

int main(void)
{
	unsigned long int eflags = geteflags();
	printf("eflags == %lx\n",eflags);
	return 0;
}
