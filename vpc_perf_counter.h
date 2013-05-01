#ifndef _VPC_PERF_COUNTER_H_
#define _VPC_PERF_COUNTER_H_
#include "perf_counter.h"

extern perf_counter_t perf_counter_all;
extern perf_counter_t perf_counter_pagefault;
extern perf_counter_t perf_counter_bin_tran;
extern perf_counter_t perf_counter_get_native_code_address;
extern perf_counter_t perf_counter_pmem_sync;

void init_vpc_perf_counter();
void log_vpc_perf_counter();

#endif // _VPC_PERF_COUNTER_H_
