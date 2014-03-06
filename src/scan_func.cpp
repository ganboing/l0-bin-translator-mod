#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, char** argv)
{
	int i0_prog_fd = open(argv[1], O_RDONLY);
	if(i0_prog_fd<0)
	{
		exit(-1);
	}
	struct stat file_info;
	fstat(i0_prog_fd, &file_info);
	__off_t i0_prog = file_info.st_size;
	void* mapped_text_seg = mmap(NULL, i0_prog,PROT_READ, MAP_PRIVATE,i0_prog_fd,0);
	while(1)
	{

	}
	return 0;
}
