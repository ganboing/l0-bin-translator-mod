#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

void InitTestTransOutput(void) __attribute__ ((constructor));
void FinishTestTransOutput(void) __attribute__((destructor));
static int outputfile = -1;

static const unsigned long suggest_map_to_addr = 0x500000000000ULL;

void* real_map_addr = 0;

void FlushTransOutput(void)
{
	if(msync(real_map_addr, (1024ULL*1024ULL), MS_SYNC) == -1)
	{
		exit(-300);
	}
}

void InitTestTransOutput(void)
{
	outputfile = open("/tmp/trans.out",O_RDWR);
	if(outputfile == -1)
	{
		exit(-100);
	}
	real_map_addr = mmap((void*)suggest_map_to_addr, (1024ULL*1024ULL),PROT_READ|PROT_WRITE,
			MAP_SHARED, outputfile, 0);
	if(((unsigned long)real_map_addr) < (0x400000000000ULL))
	{
		exit(-200);
	}
	memset(real_map_addr, 0,  (1024ULL*1024ULL));
}
void FinishTestTransOutput(void)
{
	munmap(real_map_addr, (1024ULL)*(1024ULL));
}

uint8_t* GetDisasmOutAddr(void)
{
	return real_map_addr;
}
