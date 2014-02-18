#ifndef ZLOG_H_
# define ZLOG_H_

#include <stdio.h>



// #define ZLOG_DISABLE_LOG 1
#define ZLOG_BUFFER_STR_MAX_LEN 256
#define ZLOG_BUFFER_SIZE (0x1 << 22)
// #define ZLOG_REAL_WORLD_TIME 1

// only for debug, enabling this will slow down the log
// #define ZLOG_FORCE_FLUSH_BUFFER


#define ZLOG_LOC __FILE__, __LINE__

// deprecated
#define LOC __FILE__, __LINE__

extern FILE* zlog_fout;
void zlog_init(char const* log_file);
void zlog_init_stdout(void);
void zlog_init_flush_thread();
void zlog_finish();
void zlog_flush_buffer();
void zlogf(char const * fmt, ...);
void zlogf_time(char const * fmt, ...);
void zlog(char* filename, int line, char const * fmt, ...);
void zlog_time(char* filename, int line, char const * fmt, ...);

#endif
