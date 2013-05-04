#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

FILE* debug_log = NULL;

void ModLog(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(debug_log,fmt,va);
	va_end(va);
}

void InitModLog(void) __attribute__((constructor))
{
	debug_log=fopen("//lhome//bgan//l0log//bintran_debug.log","a+");
}

void FinalModLog(void) __attribute__((destructor))
{
	fclose(debug_log);
}
