#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "PrintDisasm.h"
#include "zlog_mod.h"

int main()
{
	struct stat file_detail;
	static const uint64_t suggest_map_addr = 0x400000000;
	static void* real_map_addr;
	int i0_prog_file = open("/tmp/i0_prog.out", O_RDONLY);
	if(i0_prog_file == -1)
	{
		exit(-100);
	}
	unsigned long filesize;
	if(fstat(i0_prog_file, &file_detail)!=0)
	{
		exit(-200);
	}
	filesize = ((unsigned long)file_detail.st_size);
	real_map_addr = mmap((void*)suggest_map_addr, filesize, PROT_READ, MAP_SHARED, i0_prog_file, 0);
	if(real_map_addr != ((void*)-1))
	{
		printf("map success @ %lx\n", real_map_addr);
	}
	else
	{
		exit(-300);
	}
}
