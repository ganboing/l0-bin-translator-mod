/*
 * testjit.c
 *
 *  Created on: Jul 9, 2013
 *      Author: PROGMAN
 */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define FUNC_SIZE 0x40

typedef void (*FuncType)(void);

void test(void)
{
	printf("this is a function\n");
}

void functobecopied(void)
{
	uint64_t funcaddr = ((uint64_t)test);
	(*((FuncType)funcaddr))();
	return;
}

int main()
{
	unsigned long pagesize = sysconf(_SC_PAGE_SIZE);
	void* func_runtime = malloc(FUNC_SIZE);
	memcpy(func_runtime, functobecopied,FUNC_SIZE );
	mprotect((((uint64_t)func_runtime)&(~(pagesize-1))), (FUNC_SIZE + pagesize - 1), PROT_READ | PROT_WRITE | PROT_EXEC);
	(*((FuncType)func_runtime))();
	return 0;
}
