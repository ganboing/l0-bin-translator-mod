/*
 * testjit.c
 *
 *  Created on: Jul 9, 2013
 *      Author: PROGMAN
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUNC_SIZE 0x10

typedef void (*FuncType)(void);

void functobecopied(void)
{
	printf("this is a function\n");
	return;
}

int main()
{
	void* func_runtime = malloc(FUNC_SIZE);
	memcpy(func_runtime, functobecopied,FUNC_SIZE );
	(*((FuncType)func_runtime))();
	return 0;
}
