#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void InitTestTransOutput(void) __attribute__ ((constructor));

static int outputfile = -1;

void InitTestTransOutput(void)
{
	outputfile = open("/tmp/trans.out",O_RDWR);
	if(outputfile == -1)
	{
		exit(-100);
	}
}
