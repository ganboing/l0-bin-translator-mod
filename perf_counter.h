#ifndef _PERF_COUNTER_H_
#define _PERF_COUNTER_H_
#include <sys/time.h>

typedef struct {
    struct timeval accum_time;
    struct timeval cur_begin_time;
    struct timeval cur_end_time;
} perf_counter_t;

int init_perf_counter(perf_counter_t* pcounter);
void begin_cur_perf_counter(perf_counter_t* pcounter);
void end_cur_perf_counter(perf_counter_t* pcounter);
struct timeval get_perf_counter(perf_counter_t* pcounter);

#endif // _PERF_COUNTER_H_
