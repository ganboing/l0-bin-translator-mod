#include <stdio.h>
#include <stdarg.h>

void InitModLog(void) __attribute__((constructor));
void FinalModLog(void) __attribute__((destructor));

static FILE* debug_log = NULL;

void ModLog(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(debug_log,fmt,va);
	fflush(debug_log);
	va_end(va);
}

void InitModLog(void)
{
	debug_log=fopen("//lhome//bgan//l0log//bintran_debug.log","a+");
}

void FinalModLog(void)
{
	fclose(debug_log);
}

void LogSyncTest(void)
{
	int x;
	__sync_lock_test_and_set(&x,(int)(10));
}
