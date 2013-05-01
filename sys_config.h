#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

#include "mem.h"

// *************************
// VPC ID

// if defined, the prefix is 13, otherwise 
// the prefix is 8
// #define VPCID_IP_PREFIX_13 1
#define VPCID_IP_PREFIX_8 1

// *************************
// Scheduler

// only one watcher instance in runner Q
#define ACTIVATE_ONLY_ONE_WATCHER_INSTANCE

// scheduling algorithm number: configuration in the config file
// Must choose one from the followings
#define DETERMISNISTIC_SCHEDULING 0
#define DELAY_SCHEDULING 1

#define DETERMISNISTIC_SCHEDULING_STR "deterministic_scheduling"
#define DELAY_SCHEDULING_STR "delay_scheduling"

// before used
// #define CONFIG_MIN_LEN_FOR_LOCALITY (0x50000)
// very large
#define CONFIG_MIN_LEN_FOR_LOCALITY (0x50000000000)

// very large -- disabled it
// #define CONFIG_MIN_LEN_FOR_LOCALITY (0x7000000000000000)

#define SCHEDULER_RCMB_MAX_SIZE 2048
#define SCHEDULER_RUNNER_Q_MAX_SIZE 4096
// #define SCHEDULER_LOCALITY_INFO_SIZE 256
#define SCHEDULER_LOCALITY_INFO_SIZE 2048

// persistent memory configuration
#ifdef PPM_12TB

#define SPAWN_ALLOCATED_STACK_BASE    (0x30000000000)
#define SPAWN_ALLOCATED_STACK_LEN     (0x1000)
#define SPAWN_ALLOCATED_STACK_END     (0x3efffffffff)

// shalloc area allocated  range
#define SHALLOC_AREA_ALLOC_BASE       (0x20000000000)
#define SHALLOC_AREA_ALLOC_MAX_LEN    (0x10000000000)

// idle handler heap
#define IDLE_HANDLE_RUNNER_HEAP_BEGIN (0x3f800000000)
#define IDLE_HANDLE_RUNNER_HEAP_LEN   (0x1000)

// I0 Code meta
#define I0_CODE_META_BEGIN            (0x3f000000000)
#define I0_CODE_META_LEN              (0x1000)

#else // PPM_12TB

#define SPAWN_ALLOCATED_STACK_BASE   (0x70000000000)
#define SPAWN_ALLOCATED_STACK_LEN    (0x1000)
#define SPAWN_ALLOCATED_STACK_END    (0x7efffffffff)

//shalloc area allocated  range
#define SHALLOC_AREA_ALLOC_BASE (0x50000000000)
#define SHALLOC_AREA_ALLOC_MAX_LEN (0x20000000000)

// idle handler heap
#define IDLE_HANDLE_RUNNER_HEAP_BEGIN 0x7f800000000
#define IDLE_HANDLE_RUNNER_HEAP_LEN 0x1000

// I0 Code meta
#define I0_CODE_META_BEGIN   (0x7f000000000)
#define I0_CODE_META_LEN   (0x1000)

#endif // PPM_12TB


#define RUNNER_ID_ALLOCATE_BASE   (0x10000000000)
#define RUNNER_ID_ALLOCATE_INC    (0x1000)

// reserved # of IDs for special runners
#define RUNNER_ID_RESERVE_N       (100)
#define RUNNER_ID_LOADER          (0x10000000000)
#define RUNNER_ID_IDLE_HDLR       (0x10000001000)

// interface to MEM
#define __TEST_IF_PERF__

#define IF_BETWEEN_RETRY_USLEEP 100000
#define IF_MAX_RETRY_NUM 100
// creating ss check whether ~cur copy are ready
// not always true. Need retry.
#define IF_MAX_RETRY_NUM_CS 100000
#define IF_MAX_RETRY_NUM_COMMIT 1
#define IF_MAX_RETRY_NUM_COMMIT_DELETE 1

// *************************
// VPC and scheduler

#define PR_RUNNER_SB 0x100000100;
#define PR_RUNNER_SS 0x100000108;
#define PR_RUNNER_ID 0x100000120;

// auto init area in PR
#define PR_AUTO_INIT_AREA_BASE (0x200100000)
#define PR_AUTO_INIT_AREA_MAX_LEN (0x1000)

// #define DEFAULT_STACK_START  (1UL << 40) // + (0xfff);
// #define DEFAULT_STACK_SIZE (1UL << 12) // (1UL << 22);

// *************************
// Interface

#define NEW_RUNNER_BACK_FILL   (0x100000400)
#define NEW_RUNNER_ID_BACK_FILL   (0x100000410)

// system call
#define SYSCALL_ID_ADDR (0x100000420)
#define SYSCALL_COMM_AREA_ADDR (0x100001000)
#define SYSCALL_COMM_AREA_LEN (0x1000)

#define SYSCALL_ID_MALLOC  (1)
#define SYSCALL_ID_USLEEP  (2)

// streaming I/O
#define SYSCALL_ID_SREAD   (3)
#define SYSCALL_ID_SWRITE  (4)
#define SYSCALL_ID_SLISTEN (5)

#define REG_FILE_BEGIN         (0x200000000)
#define REG_FILE_END           (0x200000040)

#define IO_STDIN_Q              (0x100000200)
#define IO_STDOUT_Q             (0x100000208)


// *************************
// binary translator

// #define _CONV_V0_

// Note: highest 64-bit possible memory address (byte): 0x7fffffffefff
// Note: 64-bit : 0x7fffffffeff8

#define I0_CODE_BEGIN   (0x400000000)
// #define I0_CODE_MAX_LEN   (0x20000)
#define I0_CODE_MAX_LEN   (0x30000)

// maximum translating block size for each time
#define I0_TRANSLATE_BLOCK_LEN   (0x10000000) 

// special FIs
#define LOADER_FI (0)
#define IDLE_HANDLER_FI (8)

/*
#define I0_TRANSLATED_BLOCK_TABLE (0xffe00010000)
#define I0_TRANSLATED_BLOCK_TABLE_MAX_LEN   (0x10000) 

#define NATIVE_CODE_BEGIN (0xe1000000000)
#define NATIVE_CODE_BEGIN_MAX_LEN   (0x10000) 

#define NATIVE_TRAMPOLINE (0xffe10000000)
#define NATIVE_TRAMPOLINE_MAX_LEN   (0x10000) 
*/

#define I0_TRANSLATED_BLOCK_TABLE (0x110000000)
#define I0_TRANSLATED_BLOCK_TABLE_MAX_LEN   (0x10000) 


#define NATIVE_CODE_BEGIN (0x110010000)
#define NATIVE_CODE_BEGIN_MAX_LEN   (0x100000) 

#define NATIVE_TRAMPOLINE (0x120000000)
#define NATIVE_TRAMPOLINE_MAX_LEN   (0x100000) 

// the INDIR_JUMP configs determines the hash function
// in sys_indirect_jump
#define NATIVE_INDIR_JUMP (0x130000000)
#define NATIVE_INDIR_JUMP_MAX_LEN   (0x100000) 
#define NATIVE_INDIR_JUMP_ENTRY_SIZE (0x80)

// in PR:
#define SYS_CALL_TABLE (0x100000200)
#define SYS_CALL_TABLE_MAX_LEN   (0x200) 

#define SYS_TRAMPOLINE_HANDLER  (0x100000200)
#define SYS_RUNNER_WRAPPER      (0x100000208)
#define SYS_BACK_RUNNER_WRAPPER (0x100000210)
#define SYS_NEW_RUNNER          (0x100000218)
#define SYS_INDIRECT_JUMP       (0x100000220)
#define SYS_STDIN_Q             (0x100000228)
#define SYS_STDOUT_Q            (0x100000230)
#define SYS_INDIR_JUMP_HANDLER  (0x100000238)

// Profiling counter: last
#define PROFILLING_LAST       (0x140000000)

/*
#define SYS_CALL_TABLE (0xffe00000000)
#define SYS_CALL_TABLE_MAX_LEN   (0x10000) 

#define SYS_TRAMPOLINE_HANDLER  (0xffe00000000)
#define SYS_RUNNER_WRAPPER      (0xffe00000008)
#define SYS_BACK_RUNNER_WRAPPER (0xffe00000010)
#define SYS_NEW_RUNNER          (0xffe00000018)
*/

// Interrupt descriptor table
#define IDT_BASE       (0x100000800)

#endif // _SYS_CONFIG_H_
