/*
 * Bug report: Zhiqiang Ma ( http://www.zhiqiangma.com )
 */

#include<stdlib.h>
#include<stdio.h>

#include <string.h>
#include <time.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <setjmp.h>
#include <stdint.h>

#include <execinfo.h>

#include "codec.h"
#include "vpc.h"
#include "sys_config.h"
#include "bin_translator.h"
#include "vpc_perf_counter.h"
#include "zlog.h"
#include "zlog_mod.h"

#include "profiling_entries.h"

static void error(char *msg)
{

    // start stack trace
    const int MAX_TRACE_DEPTH = 20;
    size_t bt_size;
    void *bt[MAX_TRACE_DEPTH];
    char **bt_strs;
    int i;


    zlogf("ERROR: %s\n", msg);
    zlog_flush_buffer();

    bt_size = backtrace(bt, MAX_TRACE_DEPTH);
    bt_strs = backtrace_symbols(bt, bt_size);

    for(i = 0; i < bt_size; ++i)
        zlogf("%d: %s\n", i, bt_strs[i]);

    zlog_flush_buffer();

    vpc_error(msg);

    exit(1);
}

static void warning(char *msg)
{
    zlogf("WARNING: %s\n", msg);
}

uint64_t g_profiling_counters[PROFILING_MAX_ENTRY_NUM] = {};

void init_profiling_last()
{
    *(uint64_t*)(PROFILLING_LAST) = 0;
}

void log_profiling_counter()
{
    uint32_t i = 0;

    zlogf("Profilling counters:\n");

    for (i = 0; i < g_profiling_entry_num; i++) {
        zlogf("%#lx -> %ld\n", g_profiling_entrys[i], g_profiling_counters[i]);
    }
}

// -1 if not exist
int32_t pos_profiling_entry(uint64_t e)
{
    int32_t ret = 0;
    int32_t i;

    // Note: can be faster. But let's use this till it turns to be a
    // bottleneck.
    for (i = 0; i < g_profiling_entry_num; i++) {
        if (g_profiling_entrys[i] == e) {
            return i;
        }
    }

    return -1;
}

int32_t is_profiling_entry(uint64_t e)
{
    return (pos_profiling_entry(e) != -1);
}

uint64_t* get_profiling_counter_addr(uint64_t e)
{
    // Note: can be improved for better performance
    int32_t pos = 0;
    pos = pos_profiling_entry(e);

    if (pos == -1) {
        error("get_profiling_counter_addr: e is not in profiling entries\n");
    }

    return (g_profiling_counters + pos);
}

void set_profiling_counter(uint64_t e, uint64_t v)
{
    uint64_t *addr = NULL;

    addr = get_profiling_counter_addr(e);

    *addr = v;

    return;
}

uint64_t get_profiling_counter(uint64_t e)
{
    uint64_t *addr = NULL;

    addr = get_profiling_counter_addr(e);

    return *addr;
}

void init_profiling_counters()
{
    uint32_t i = 0;

    for (i = 0; i < g_profiling_entry_num; i++) {
        g_profiling_counters[i] = 0;
    }
}

void gen_profiling_code(uint64_t e, int is_write_n, char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    instr_t instr_int8;

    // %rdi: cycle, %rsi: entry
    if (is_write_n) {
#ifdef _DEBUG_PROFILLING_
        zlogf("Gen profiling code.\n");
#endif
        // ** get cycle
        // rdtsc
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx rdtsc\n", n+(*nl));
#endif
        n[(*nl)++] = 0x0f;
        n[(*nl)++] = 0x31;
        //
        // ** set cycle
        // shl $32, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx shl $%#lx, %%rdx\n", n+(*nl), 32);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xc1;
        n[(*nl)++] = 0xe2;
        n[(*nl)++] = 32;

        // leaq (%rdx, %rax), %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx leaq (%%rdx, %%rax), %%rdi\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x8d;
        n[(*nl)++] = 0x3c;
        n[(*nl)++] = 0x02;

        // ** set entry
        // movq $e, %rsi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx movq %#lx, %%rsi\n", e, n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xbe;
        *((uint64_t*)(n + (*nl))) = (uint64_t)(e);
        *nl += SIZE_E;

    } else {
        *nl += 2 + 4 + 4 + 2 + SIZE_E;
    }

    // int 8
    instr_int8.opcode = OP_INT;
    instr_int8.opr1 = 8;

    translate2x86_64_int(&instr_int8, is_write_n, n, nl, t, tl);

    if (is_write_n) {
        // get cycle
        // rdtsc
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx rdtsc\n", n+(*nl));
#endif
        n[(*nl)++] = 0x0f;
        n[(*nl)++] = 0x31;

        // save cycle
        // shl $32, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx shl $%#lx, %%rdx\n", n+(*nl), 32);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xc1;
        n[(*nl)++] = 0xe2;
        n[(*nl)++] = 32;

        // orq %rdx, %rax 
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("profiling native instr: @%#lx orq %%rdx, %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x09;
        n[(*nl)++] = 0xd0;

        // movq %rax, $PROFILLING_LAST
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%rax, %#lx\n", n+(*nl), (uint64_t)PROFILLING_LAST);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xa3;
        *((uint64_t*)(n + (*nl))) = (uint64_t)PROFILLING_LAST;
        *nl += SIZE_E;

    } else {
        *nl += 2 + 4 + 3 + 2 + SIZE_E;
    }

    if (is_write_n) {
        // initial counter
        set_profiling_counter(e, 0);
    }

    return;
}

// Note: must set. as default mode.
// binatry translator optimization:
// use leaq for addq
#define BT_OPT_ADDQ_LEAQ

translated_blocks_t *_pblockmeta = NULL;
i0_code_meta_t *_pi0codemeta = NULL;

long _waiting_list_num = 0;
struct {
    char *i0;
    int8_t found;
} _waiting_list[WAITING_LIST_SIZE];



// jump table
// add record into pblocmeta
inline void add_blocks_entry(char* i0_code, char* native_code)
{
    _pblockmeta->blocks[_pblockmeta->num].i0 = i0_code;
    _pblockmeta->blocks[_pblockmeta->num].native = native_code;
    _pblockmeta->num += 1;
    return;
}

// waiting list operations
// Note: not multi-threading safe
//
inline void initial_waiting_list()
{
    _waiting_list_num = 0;
    return;
}

inline void add_waiting_list(char *i0)
{
    if (_waiting_list_num < WAITING_LIST_SIZE) {
        _waiting_list[_waiting_list_num].i0 = i0;
        _waiting_list[_waiting_list_num].found = 0;
        _waiting_list_num++;
    } else {
        error("Waiting list is too large. Maybe change the WAITING_LIST_SIZE config.\n");
    }
    return;
}

// return position on found. otherwise: -1
inline long in_waiting_list(char *i0)
{
    long i = 0;
    for (i = 0; i < _waiting_list_num; i++) {
        if (
                ! _waiting_list[i].found
                &&
                _waiting_list[i].i0 == i0
           ) {
            return i;
        }
    }

    return -1;
}

inline void waiting_list_found(long i)
{
    _waiting_list[i].found = 1;
    return;
}

int load_i0_code(char *file, char *i0_code, uint64_t *i0_code_len)
{
    FILE *fcode = NULL;
    int flen = 0;
    fcode = fopen(file, "rb");

    char *buffer = NULL;
    int buffer_c = 0;
    // int buffer_unit = 2048;
    int buffer_unit = 512;
    int buffer_read = 0;

    if (fcode == NULL) {
        error("Opening i0_code file fails.\n");
    }

    // Get file length
    // fseek(fcode, 0, SEEK_END);
    // flen=ftell(fcode);
    // fseek(fcode, 0, SEEK_SET);
    flen = file_length(file);

    *i0_code_len = flen;

#ifdef _DEBUG_MORE_
    int i = 0;
    char *m = i0_code;
    zlogf("Try to read i0_code: \n");
    zlogf("i0_code_len: %d\n", *i0_code_len);
    for (i = 0; i < *i0_code_len; i++) {
        zlogf("0x%02lx: 0x%02x\n", (uint64_t)(m+i), *(m+i) & 0xFF);
        if ( (i % 50) == 0 ) {
            getchar();
        }
    }
#endif

    // Read file contents into mem
    // fread(i0_code, flen, 1, fcode);
    // Note: flen >= 4096 has problem

    buffer = i0_code;
    buffer_c = 0;

    while (buffer_c < flen) {
        if (buffer_c + buffer_unit < flen) {
            buffer_read = buffer_unit;
        }
        else {
            buffer_read = flen - buffer_c;
        }
        buffer_c += buffer_read;

        // fread(buffer, buffer_read, 1, fcode);
        fread(buffer, 1, buffer_read, fcode);
        //    error("fread error!\n");

        buffer += buffer_read;
    }

#ifdef _DEBUG_MORE_
    zlogf("Finish fread.\n");
#endif
    fclose(fcode);

#ifdef _DEBUG_MORE_
    i = 0;
    m = i0_code;
    zlogf("Print i0_code: \n");
    zlogf("i0_code_len: %d\n", *i0_code_len);
    for (i = 0; i < *i0_code_len; i++) {
        zlogf("0x%02lx: 0x%02x\n", (uint64_t)(m+i), *(m+i) & 0xFF);
        if ( (i % 50) == 0 ) {
            getchar();
        }
    }
#endif

    return 0;
}

int initial_i0_code_meta_data()
{
    // i0 code meta
    _pi0codemeta = (i0_code_meta_t*)I0_CODE_META_BEGIN;
    return 0;
}

int initial_i0_code_meta_data_code_len(uint64_t len)
{
    // store the code length into i0 code meta
    _pi0codemeta->i0_code_file_len = len;
    return 0;
}

int initial_translator_data()
{
    _pblockmeta = (translated_blocks_t*)I0_TRANSLATED_BLOCK_TABLE;
   
    _pblockmeta->num = 0;
    _pblockmeta->next_native = (char*)NATIVE_CODE_BEGIN;
    _pblockmeta->next_trampoline = (char*)NATIVE_TRAMPOLINE;

    initial_i0_code_meta_data();
    return 0;
}

int initial_IDT()
{
    // set IDT addresses
    *(uint64_t**)((uint64_t)IDT_BASE + 0x03*8) = (uint64_t*)sys_idt_03;
    zlogf("IDT 03 in %#lx: %#lx\n", 
            (uint64_t**)((uint64_t)IDT_BASE + 0x03*8), *((uint64_t**)((uint64_t)IDT_BASE + 0x03*8)) );

    *(uint64_t**)((uint64_t)IDT_BASE + 0x08*8) = (uint64_t*)sys_idt_08;
    zlogf("IDT 08 in %#lx: %#lx\n", 
            (uint64_t**)((uint64_t)IDT_BASE + 0x08*8), *((uint64_t**)((uint64_t)IDT_BASE + 0x08*8)) );

    // sys call
    *(uint64_t**)((uint64_t)IDT_BASE + 0x80*8) = (uint64_t*)sys_idt_80;
    zlogf("IDT 80 in %#lx: %#lx\n", 
            (uint64_t**)((uint64_t)IDT_BASE + 0x80*8), *((uint64_t**)((uint64_t)IDT_BASE + 0x80*8)) );

    return 0;

}

int initial_indir_jump_table()
{
    char* t = (char*)NATIVE_INDIR_JUMP;
    long table_len = (int)NATIVE_INDIR_JUMP_MAX_LEN;
    long entry_size = (long)NATIVE_INDIR_JUMP_ENTRY_SIZE;
    long tl = 0;

    // set indirect jump table to 0
    memset((void*)NATIVE_INDIR_JUMP, 0, (size_t)NATIVE_INDIR_JUMP_MAX_LEN);

    // set all entryies to jump to indirect_jump_handler

    tl = 0;
    while (tl < table_len) {
        // movq indir_jump_handler, %rax
        t[tl+0] = 0x48;
        t[tl+1] = 0xa1;
        *((uint64_t*)(t + tl + 2)) = (uint64_t)(SYS_INDIR_JUMP_HANDLER);

        // jmpq *%rax
        t[tl+SIZE_E+2] = 0xff;
        t[tl+SIZE_E+3] = 0xe0;

        tl += entry_size;
    }

    return 0;
}

int initial_sys_call_table()
{
    // set trampoline handler address
    *((uint64_t**)SYS_TRAMPOLINE_HANDLER) = (uint64_t*)sys_trampoline_handler;
    zlogf("sys_trampoline_handler in %#lx: %#lx\n", 
            (uint64_t**)SYS_TRAMPOLINE_HANDLER, *((uint64_t**)SYS_TRAMPOLINE_HANDLER));

    // set back_runner_wrapper address
    *((uint64_t**)SYS_BACK_RUNNER_WRAPPER) = (uint64_t*)sys_back_runner_wrapper;
    zlogf("sys_back_runner_wrapper in %#lx: %#lx\n", 
            (uint64_t**)SYS_BACK_RUNNER_WRAPPER, *((uint64_t**)SYS_BACK_RUNNER_WRAPPER));

    // set sys_new_runner address
    *((uint64_t**)SYS_NEW_RUNNER) = (uint64_t*)sys_new_runner;
    zlogf("sys_new_runner in %#lx: %#lx\n", 
            (uint64_t**)SYS_NEW_RUNNER, *((uint64_t**)SYS_NEW_RUNNER));

    // set sys_indirect_jump address
    *((uint64_t**)SYS_INDIRECT_JUMP) = (uint64_t*)sys_indirect_jump;
    zlogf("sys_indirect_jump in %#lx: %#lx\n", 
            (uint64_t**)SYS_INDIRECT_JUMP, *((uint64_t**)SYS_INDIRECT_JUMP));

    // set sys_indirect_jump_handler address
    *((uint64_t**)SYS_INDIR_JUMP_HANDLER) = (uint64_t*)sys_indirect_jump_handler;
    zlogf("sys_indirect_jump_handler in %#lx: %#lx\n", 
            (uint64_t**)SYS_INDIR_JUMP_HANDLER, *((uint64_t**)SYS_INDIR_JUMP_HANDLER));

    // set sys_stdin_q address
    *((uint64_t**)SYS_STDIN_Q) = (uint64_t*)sys_stdin_q;
    zlogf("sys_stdin_q in %#lx: %#lx\n", 
            (uint64_t**)SYS_STDIN_Q, *((uint64_t**)SYS_STDIN_Q));

    // set sys_stdout_q address
    *((uint64_t**)SYS_STDOUT_Q) = (uint64_t*)sys_stdout_q;
    zlogf("sys_stdout_q in %#lx: %#lx\n", 
            (uint64_t**)SYS_STDOUT_Q, *((uint64_t**)SYS_STDOUT_Q));

    return 0;
}

/*
 * bit_len <= 31
 */
inline unsigned int _bit2uint(char *c, unsigned int bit_start, unsigned int bit_len)
{
    unsigned int result = 0;
    unsigned int tmp = 0;

    unsigned int tc1 = 0;
    unsigned int tc2 = 0;

    // go the nearest char
    c += (bit_start / 8);
    bit_start = bit_start % 8;

    // get out char
    tc1 = (unsigned int)(*c);
    tc1 = tc1 & 0xFF;

    if ((bit_start + bit_len) > 7) {
        tc2 = (unsigned int)(*(c+1));
        tc2 = tc2 & 0xFF;
        result = (tc1 << 8) + tc2;
#ifdef _DEBUG_
        // zlogf("_bit2uint result1: 0x%x\n", result);
#endif
        // aligned to int
        tmp = (16 - bit_len - bit_start);
        result = result >> tmp;
    } else {
        result = tc1;

        // aligned to int
        tmp = (8 - bit_len - bit_start);
        result = result >> tmp;
    }

    tmp = (0x1 << bit_len) - 0x1;
    result = result & tmp;

    return result;
}

/*
 * bit_len <= 8
 */
inline unsigned int bit2uint(char *c, unsigned int *bit_start, unsigned int bit_len)
{
    unsigned int result = 0;
#ifdef _DEBUG_MORE_BIT2UINT_
    zlogf("bit2uint(0x%02x%02x%02x, %u, %u) :", 
            *c & 0xFF, *(c+1) & 0xFF, *(c+2) & 0xFF,
            *bit_start, bit_len);
#endif
    result = _bit2uint(c, *bit_start, bit_len);
    *bit_start += bit_len;
#ifdef _DEBUG_MORE_BIT2UINT_
    zlogf("0x%02x\n", result & 0xFF);
#endif
    return result;
}

void print_instr_opr(unsigned int addrm, uint64_t opr1, uint32_t disp)
{
    switch (addrm) {
        case ADDRM_IMMEDIATE:
            zlogf("$%#lx", opr1);
            break;
        case ADDRM_ABSOLUTE:
            zlogf("%#lx", opr1);
            break;
        case ADDRM_INDIRECT:
            zlogf("(%#lx)", opr1);
            break;
        case ADDRM_DISPLACEMENT:
            zlogf("%d(%#lx)", disp, opr1);
            break;
    }
}

int print_instr(instr_t *i)
{
    int j = 0;
#ifndef _DEBUG_PRINT_INSTR_
    return 0;
#endif
    switch (i->opcode) {
        case OP_NOP:

            zlogf("Instr %#lx: opcode: 0x%x (nop)\n",
                    i->addr,
                    i->opcode);

            break;
        case OP_CONV:
// #ifdef _CONV_V0_
//             zlogf("Instr %#lx: opcode: 0x%x (mov V0), option: %u, l1: %u, l2: %u, addrm1: %u, addrm2: %u, opr1: %#lx, disp1: %#x, opr2: %#lx, disp2: %#x\n",
//                     i->addr,
//                     i->opcode,
//                     i->option,
//                     i->l1,
//                     i->l2,
//                     i->addrm1,
//                     i->addrm2,
//                     i->opr1,
//                     i->disp1,
//                     i->opr2,
//                     i->disp2
//                   );
// #else
            zlogf("Instr %#lx: opcode: 0x%x (conv), attr1: %u, attr2: %u, ",
                    i->addr,
                    i->opcode,
                    i->mattr1,
                    i->mattr2
                  );
            print_instr_opr(i->addrm1, i->opr1, i->disp1);
            zlogf(" , ");
            print_instr_opr(i->addrm2, i->opr2, i->disp2);
            zlogf("\n");

            /*
            zlogf("Instr %#lx: opcode: 0x%x (mov), mattr1: %u, mattr2: %u, addrm1: %u, addrm2: %u, opr1: %#lx, disp1: %#x, opr2: %#lx, disp2: %#x\n",
                    i->addr,
                    i->opcode,
                    i->mattr1,
                    i->mattr2,
                    i->addrm1,
                    i->addrm2,
                    i->opr1,
                    i->disp1,
                    i->opr2,
                    i->disp2
                  );
            */
// #endif
            break;
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_SHIFT:

            zlogf("Instr %#lx: opcode: ",
                    i->addr
                 );
            switch (i->opcode) {
                case OP_ADD:
                    zlogf("add");
                    break;
                case OP_SUB:
                    zlogf("sub");
                    break;
                case OP_MUL:
                    zlogf("mul");
                    break;
                case OP_DIV:
                    zlogf("div");
                    break;
                case OP_AND:
                    zlogf("and");
                    break;
                case OP_OR:
                    zlogf("or");
                    break;
                case OP_XOR:
                    zlogf("xor");
                    break;
                case OP_SHIFT:
                    zlogf("shift");
                    break;
                default:
                    error("not supported OP.\n");
            }
            zlogf(", attr: %u, ", i->attr);
            /*
            zlogf(", addrm1: %u, addrm2: %u, addrm3: %u, attr: %u, opr1: %#lx, disp1: %#x, opr2: %#lx, disp2: %#x, opr3: %#lx, disp3: %#x\n",
                    i->addrm1,
                    i->addrm2,
                    i->addrm3,
                    i->attr,
                    i->opr1,
                    i->disp1,
                    i->opr2,
                    i->disp2,
                    i->opr3,
                    i->disp3
                  );
                  */
            print_instr_opr(i->addrm1, i->opr1, i->disp1);
            zlogf(" , ");
            print_instr_opr(i->addrm2, i->opr2, i->disp2);
            zlogf(" , ");
            print_instr_opr(i->addrm3, i->opr3, i->disp3);
            zlogf("\n");
            break;

        case OP_SPAWN:

            zlogf("Instr %#lx: opcode: 0x%x (spawn), addrm1: %u, addrm2: %u, addrm3: %u, addrm4: %u, opr1: %#lx, disp1: %#x, opr2: %#lx, disp2: %#x, opr3: %#lx, disp3: %#x, opr4: %#lx, disp4: %#x\n",
                    i->addr,
                    i->opcode,
                    i->addrm1,
                    i->addrm2,
                    i->addrm3,
                    i->addrm4,
                    i->opr1,
                    i->disp1,
                    i->opr2,
                    i->disp2,
                    i->opr3,
                    i->disp3,
                    i->opr4,
                    i->disp4);

            break;

        case OP_SCMP:

            zlogf("Instr %#lx: opcode: 0x%x (scmp), ",
                    i->addr,
                    i->opcode);
            for (j = 0; j < 5; j++) {
                // zlogf("addrms[%d]: %#lx, oprs[%d]: %#lx, disps[%d]: %#lx, ", j, i->addrms[j], j, i->oprs[j], j, i->disps[j]);
                print_instr_opr(i->addrms[j], i->oprs[j], i->disps[j]);
                zlogf(" , ");
            }

            zlogf("\n");
            break;

        case OP_EXIT:

            zlogf("Instr %#lx: opcode: 0x%x (exit), option: %u\n",
                    i->addr,
                    i->opcode,
                    i->option);

            break;

        case OP_INT:

            zlogf("Instr %#lx: opcode: 0x%x (int), opr1: %#lx\n",
                    i->addr,
                    i->opr1);

            break;

        case OP_B:
            switch (i->option) {
                case OPT_B_J:
                case OPT_B_IJ:
                    zlogf("Instr %#lx: opcode: 0x%x (b), option: %u (j/ji), ra: %u, opr1: %#lx, disp1: %#x \n",
                            i->addr,
                            i->opcode,
                            i->option,
                            i->ra,
                            i->opr1,
                            i->disp1
                          );
                    break;
                default:
                    zlogf("Instr %#lx: opcode: 0x%x (b), option: %u, addrm1: %u, addrm2: %u, ra: %u, attr: %u, opr1: %#lx, disp1: %#x, opr2: %#lx, disp2: %#x, opr3: %#lx, disp3: %#x \n",
                            i->addr,
                            i->opcode,
                            i->option,
                            i->addrm1,
                            i->addrm2,
                            i->ra,
                            i->attr,
                            i->opr1,
                            i->disp1,
                            i->opr2,
                            i->disp2,
                            i->opr3,
                            i->disp3
                          );
                    break;
            }
            break;
        default:
            zlogf("Instruction: unknown\n");
            error("Unknow instruction.\n");
    }
    return 0;
}


int initial_instr(instr_t *i)
{
    i->addr_size_mode = EBM; // EBM is the default one
    i->opcode = OP_UNDEFINED;
    i->option = OPT_UNDEFINED;
    // i->l1 = LEN_UNDEFINED;
    // i->l2 = LEN_UNDEFINED;
    i->addrm1 = ADDRM_UNDEFINED;
    i->addrm2 = ADDRM_UNDEFINED;
    i->addrm3 = ADDRM_UNDEFINED;
    i->addrm4 = ADDRM_UNDEFINED;
    i->attr = ATTR_UNDEFINED;
    i->ra = JUMP_UNDEFINED;
    i->opr1 = 0;
    i->opr2 = 0;
    i->opr3 = 0;
    i->opr4 = 0;
    i->disp1 = 0;
    i->disp2 = 0;
    i->disp3 = 0;
    i->disp4 = 0;

    return 0;
}

inline int x86_64_in_jmp_range(uint64_t cur, uint64_t target)
{
    int64_t rel = 0;

    rel = (long)target - (long)cur;
    if (rel > X86_64_JUMP_RANG_LOWER &&
            rel < X86_64_JUMP_RANG_UPPER) {
        return 1;
    } else {
        return 0;
    }

    return 0;
}

int add_trampoline_new(uint64_t target, uint64_t entry_to_update, char *t, uint64_t *tl, uint64_t type)
{
    // uint64_t trampoline = (uint64_t)(t + (*tl));
    // movq trampoline_entry, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: add tramp: @%#lx movq $%#lx, %%rdi\n", t+(*tl), entry_to_update);
#endif
    t[(*tl)++] = 0x48;
    t[(*tl)++] = 0xbf;
    *((uint64_t*)(t + (*tl))) = entry_to_update;
    *tl += SIZE_E;

    // movq i0_target, %rsi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: add tramp: @%#lx movq $%#lx, %%rsi\n", t+(*tl), target);
#endif
    t[(*tl)++] = 0x48;
    t[(*tl)++] = 0xbe;
    *((uint64_t*)(t + (*tl))) = target;
    *tl += SIZE_E;

    // movq type, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: add tramp: @%#lx movq $%#lx, %%rdx\n", t+(*tl), type);
#endif
    t[(*tl)++] = 0x48;
    t[(*tl)++] = 0xba;
    *((uint64_t*)(t + (*tl))) = type;
    *tl += SIZE_E;

    // movq handler, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: add tramp: @%#lx movq sys_trampoline_handler, %%rax\n", t+(*tl));
#endif
    t[(*tl)++] = 0x48;
    t[(*tl)++] = 0xa1;
    *((uint64_t*)(t + (*tl))) = (uint64_t)(SYS_TRAMPOLINE_HANDLER);
    *tl += SIZE_E;

    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: add tramp: @%#lx jmpq *%%rax\n", t+(*tl));
#endif
    t[(*tl)++] = 0xff;
    t[(*tl)++] = 0xe0;

    return 0;
}




inline int is_in_reg_file(uint64_t opr)
{
    if (opr >= REG_FILE_BEGIN && opr < REG_FILE_END) {
        return 1;
    } else {
        return 0;
    }
}

inline int opr_reg(uint64_t opr)
{
    return (opr - REG_FILE_BEGIN)/8;
}

inline int check_is_stdout(uint64_t opr, unsigned int addrm)
{
    if (addrm == ADDRM_ABSOLUTE) {
        if (opr == IO_STDOUT_Q) {
            return 1;
        }
    }

    return 0;
}

inline int check_is_stdin(uint64_t opr, unsigned int addrm)
{
    if (addrm == ADDRM_ABSOLUTE) {
        if (opr == IO_STDIN_Q) {
            return 1;
        }
    }

    return 0;
}

// x86-64 encoding
inline uint32_t x86_64_reg_encoding(uint64_t reg)
{
    uint32_t encoding = 0;

    switch (reg) {
        case I0_REG_FILE0:
            encoding = 3;
            break;
        case I0_REG_FILE1:
            encoding = 1;
            break;
        case I0_REG_FILE2:
            encoding = 15;
            break;
        case I0_REG_FILE3:
            encoding = 14;
            break;
        case I0_REG_FILE4:
            encoding = 13;
            break;
        case I0_REG_FILE5:
            encoding = 10;
            break;
        case I0_REG_FILE6:
            encoding = 9;
            break;
        case I0_REG_FILE7:
            encoding = 8;
            break;
        case X86_64_R_RDI:
            encoding = 7;
            break;
        case X86_64_R_RSI:
            encoding = 6;
            break;
        case X86_64_R_RAX:
            encoding = 0;
            break;
        case X86_64_R_RDX:
            encoding = 2;
            break;
        default:
            zlogf("reg_encoding: Not supported x86-64 register: %#lx\n", reg);
            error("reg_encoding: Not supported x86-64 register\n");
    }
    return encoding;
}


// old ones

// // when mattr is not Q: reg is always eax/al/rax
//     inline void translate2x86_64_read_indirect_reg_file_0
// (uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
// {
//     int oprreg = -1;
//     int regcomb = 0;
//   
//     if (mattr == MATTR_SB || mattr == MATTR_UB) {
//         error("reg_file with SB/UB is not implemented yet.\n");
//     }
// 
//     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//         error("reg_file with SL/UL is not implemented yet.\n");
//     }
// 
//     // determine oprreg for the opr
//     // tricky here: just map the REG_FILEX
//     oprreg = (opr - REG_FILE_BEGIN)/8;
// 
//     if (is_write_n) {
//         // movq oprreg, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//         zlogf("native instr: @%#lx movq (%%oprreg %d), %%reg %d\n", n+(*nl), oprreg, reg);
// #endif
//         if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
//             // %rbx and %rcx are special
//             n[(*nl)++] = 0x48;
//         } else {
//             n[(*nl)++] = 0x49;
//         }
//         n[(*nl)++] = 0x8b;
// 
//         // reg combination
//         regcomb = (oprreg << 16) + reg;
//         switch (regcomb) {
//             case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
//             case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
//             case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
//             case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break; // FILE4 is different
//             case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
//             default:
//                 error("translate2x86_64_read_indirect_reg_file: unsupported regcomb.\n");
//         }
//     } else {
//         *nl += 3;
//         if (oprreg == I0_REG_FILE4) {
//             *nl += 1;
//         }
//     }
// }
// 
// // when mattr is not Q: reg is always eax/al/rax
//     inline void translate2x86_64_read_reg_file_base_disp_0
// (uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
// {
//     int oprreg = -1;
//     int regcomb = 0;
//   
//     // if (mattr == MATTR_SB || mattr == MATTR_UB) {
//     //     error("reg_file with SB/UB is not implemented yet. working.\n");
//     // }
// 
//     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//         error("reg_file with SL/UL is not implemented yet.\n");
//     }
// 
//     // determine oprreg for the opr
//     // tricky here: just map the REG_FILEX
//     oprreg = (opr - REG_FILE_BEGIN)/8;
// 
//     if (is_write_n) {
//         if (mattr == MATTR_SB || mattr == MATTR_UB) {
//             // movb disp(oprreg), al
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             zlogf("native instr: @%#lx movb %d(%%oprreg %d), %%al\n", n+(*nl), disp, oprreg);
// #endif
//             if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
//                 // %rbx and %rcx are special
//                 ; 
//             } else {
//                 n[(*nl)++] = 0x41;
//             }
//             n[(*nl)++] = 0x8a;
// 
//             // reg combination
//             switch (oprreg) {
//                 case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
//                 case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
//                 case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
//                 case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
//                 case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
//                 case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
//                 case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
//                 case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
//                 default:
//                      error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
//             }
//             // disp
//             *((int32_t*)(n + (*nl))) = (int32_t)disp;
//             *nl += SIZE_F;
//  
//             // error("reg_file with SB/UB is not implemented yet. working.\n");
//         } else {
//             // Q
//             // movq disp(oprreg), reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             zlogf("native instr: @%#lx movq %d(%%oprreg %d), %%reg %d\n", n+(*nl), disp, oprreg, reg);
// #endif
//             if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
//                 // %rbx and %rcx are special
//                 n[(*nl)++] = 0x48;
//             } else {
//                 n[(*nl)++] = 0x49;
//             }
//             n[(*nl)++] = 0x8b;
// 
//             // reg combination
//             regcomb = (oprreg << 16) + reg;
//             switch (regcomb) {
//                 case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
//                 case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
//                 case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
//                 case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
//                 case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
//                 case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
//                 case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
//                 case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
//                 case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
//                 case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
//                 case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
//                 case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
//                 case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
//                 case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
//                 case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
//                 case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
//                 case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
//                 case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
//                 case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
//                 case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
//                 case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
//                 case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
//                 case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
//                 case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
//                 case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
//                 case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
//                 case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
//                 case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
//                 case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
//                 case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
//                 case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
//                 case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
//                 default:
//                                                             error("translate2x86_64_read_reg_file_base_disp: unsupported regcomb.\n");
//             }
//             // disp
//             *((int32_t*)(n + (*nl))) = (int32_t)disp;
//             *nl += SIZE_F;
//         }
//     } else {
//         if (mattr == MATTR_SB || mattr == MATTR_UB) {
//             if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
//                 *nl += 0;
//             }
//             else {
//                 *nl += 1;
//             }
//             *nl += 1 + 1 + SIZE_F;
//         }
//         else {
//             *nl += 3 + SIZE_F;
//         }
//     }
// }
// 
// // when mattr is not Q: reg is always eax/al/rax
//     inline void translate2x86_64_read_reg_file_0
// (uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
// {
//     int oprreg = -1;
//     int regcomb = 0;
//   
//     if (mattr == MATTR_SB || mattr == MATTR_UB) {
//         error("reg_file with SB/UB is not implemented yet.\n");
//     }
// 
//     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//         error("reg_file with SL/UL is not implemented yet.\n");
//     }
// 
//     // determine oprreg for the opr
//     // tricky here: just map the REG_FILEX
//     oprreg = (opr - REG_FILE_BEGIN)/8;
// 
//     if (is_write_n) {
//         // movq oprreg, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//         zlogf("native instr: @%#lx movq %%oprreg %d, %%reg %d\n", n+(*nl), oprreg, reg);
// #endif
//         if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
//             // %rbx and %rcx are special
//             n[(*nl)++] = 0x48;
//         } else {
//             n[(*nl)++] = 0x4c;
//         }
//         n[(*nl)++] = 0x89;
// 
//         // reg combination
//         regcomb = (oprreg << 16) + reg;
//         switch (regcomb) {
//             case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xdf; break;
//             case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xde; break;
//             case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xda; break;
//             case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xd8; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xcf; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xce; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xca; break;
//             case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc8; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xfe; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xfa; break;
//             case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xf8; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf7; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xf2; break;
//             case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xf0; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xef; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xee; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xea; break;
//             case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xe8; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xd7; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xd6; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
//             case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xd0; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xcf; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xce; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xca; break;
//             case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc8; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xc7; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xc6; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xc2; break;
//             case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
//             default:
//                 error("translate2x86_64_read_reg_file: unsupported regcomb.\n");
//         }
//     } else {
//         *nl += 3;
//     }
// }
// 
// /* floating point only: implement D only currently
//  * when mattr is set
//  * reg is always eax/ax
//  */
//     inline void translate2x86_64_fp_read_0
// (uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
// {
//     // TODO implement it
//     int in_reg_file = 0;
//     unsigned int mattr = 0;
// 
// #ifdef _DEBUG_MORE_
//     zlogf("translate2x86_64_fp_read: addrm: %d\n", addrm);
// #endif
//     
//     if (mattr == MATTR_SB || mattr == MATTR_UB) {
//         error("_translate2x86_64_fp_read: B is not implemented yet.\n");
//     }
// 
//     switch (addrm) {
//         case ADDRM_IMMEDIATE:
//             if (is_write_n) {
//                 if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                     // movl $opr, %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movl $%#lx, %%rax\n", n+(*nl), opr);
// #endif
//                     n[(*nl)++] = 0xb8;
//                     // $opr
//                     *((uint32_t*)(n + (*nl))) = (uint32_t)opr;
//                     *nl += SIZE_F;
//                 }
//                 else {
//                     // movq $opr, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movq $%#lx, %%reg %d\n", n+(*nl), opr, reg);
// #endif
//                     n[(*nl)++] = 0x48;
//                     // reg
//                     switch (reg) {
//                         case X86_64_R_RDI:
//                             n[(*nl)++] = 0xbf;
//                             break;
//                         case X86_64_R_RSI:
//                             n[(*nl)++] = 0xbe;
//                             break;
//                         case X86_64_R_RAX:
//                             n[(*nl)++] = 0xb8;
//                             break;
//                         default:
//                             error("translate2x86_64_fp_read: unsupported reg #1.\n");
//                     }
//                     // $opr
//                     *((uint64_t*)(n + (*nl))) = opr;
//                     *nl += SIZE_E;
//                 }
//             } else {
//                 if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                     *nl += 1 + SIZE_F;
//                 }
//                 else {
//                     *nl += 2 + SIZE_E;
//                 }
//             }
//             break;
//         case ADDRM_ABSOLUTE:
//             in_reg_file = 0;
//             // check whether using reg file
//             in_reg_file = is_in_reg_file(opr);
//             if (in_reg_file) {
//                 translate2x86_64_read_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
//             } else {
//                 if (is_write_n) {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         // movl opr, %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movl %#lx, %%eax\n", n+(*nl), opr);
// #endif
//                         n[(*nl)++] = 0xa1;
//                         *((uint64_t*)(n + (*nl))) = opr;
//                         *nl += SIZE_E;
//                     }
//                     else {
//                         // movabsq opr, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0xa1;
//                         *((uint64_t*)(n + (*nl))) = opr;
//                         *nl += SIZE_E;
//                         // movq %rax, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movq %%rax, %%reg %d\n", n+(*nl), reg);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x89;
//                         // reg
//                         switch (reg) {
//                             case X86_64_R_RDI:
//                                 n[(*nl)++] = 0xc7;
//                                 break;
//                             case X86_64_R_RSI:
//                                 n[(*nl)++] = 0xc6;
//                                 break;
//                             case X86_64_R_RDX:
//                                 n[(*nl)++] = 0xc2;
//                                 break;
//                                 /*
//                                    case X86_64_R_RCX:
//                                    n[(*nl)++] = 0xc1;
//                                    break;
//                                    */
//                             case X86_64_R_RAX:
//                                 n[(*nl)++] = 0xc0;
//                                 break;
//                             default:
//                                 error("translate2x86_64_fp_read: unsupported reg #2.\n");
//                         }
//                     }
//                 } else {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         *nl += 1 + SIZE_E;
//                     }
//                     else {
//                         *nl += 2 + SIZE_E + 3;
//                     }
//                 }
//             }
//             break;
//         case ADDRM_INDIRECT:
// 
//             if (is_write_n) {
//                 // movabsq opr, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                 zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
// #endif
//                 n[(*nl)++] = 0x48;
//                 n[(*nl)++] = 0xa1;
//                 *((uint64_t*)(n + (*nl))) = opr;
//                 *nl += SIZE_E;
//                 if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                     // movl (%rax), %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movl (%%rax), %%eax %d\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0x8b;
//                     n[(*nl)++] = 0x00;
//                 }
//                 else {
//                     // mov (%rax), reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx mov (%%rax), %%reg %d\n", n+(*nl), reg);
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0x8b;
//                     // reg
//                     switch (reg) {
//                         case X86_64_R_RDI:
//                             n[(*nl)++] = 0x38;
//                             break;
//                         case X86_64_R_RSI:
//                             n[(*nl)++] = 0x30;
//                             break;
//                         default:
//                             error("translate_read: unsupported reg #3.\n");
//                     }
//                 }
//             } else {
//                 if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                     *nl += 2 + SIZE_E + 2;
//                 }
//                 else {
//                     *nl += 2 + SIZE_E + 3;
//                 }
//             }
//             break;
//         case ADDRM_DISPLACEMENT:
//             // check reg file
//             in_reg_file = 0;
//             // check whether using reg file
//             in_reg_file = is_in_reg_file(opr);
//             if (in_reg_file) {
//                 // TODO: implement float
//                 error("float: read base+disp mode use reg is not implemented yet. \n");
//                 // translate2x86_64_read_reg_file(opr, disp, reg, n, nl, is_write_n);
//             } else {
// 
//                 if (is_write_n) {
//                     // movq $opr, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0xa1;
//                     *((uint64_t*)(n + (*nl))) = opr;
//                     *nl += SIZE_E;
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         // mov disp(%rax), %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov %d(%%rax), %%eax %d\n", n+(*nl), disp);
// #endif
//                         n[(*nl)++] = 0x8b;
//                         n[(*nl)++] = 0x80;
// 
//                     }
//                     else {
//                         // mov disp(%rax), reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov %d(%%rax), %%reg %d\n", n+(*nl), disp, reg);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x8b;
//                         // reg
//                         switch (reg) {
//                             case X86_64_R_RDI:
//                                 n[(*nl)++] = 0xb8;
//                                 break;
//                             case X86_64_R_RSI:
//                                 n[(*nl)++] = 0xb0;
//                                 break;
//                             case X86_64_R_RAX:
//                                 n[(*nl)++] = 0x80;
//                                 break;
//                             default:
//                                 error("translate_read: unsupported reg #1.\n");
//                         }
//                     }
//                     // disp
//                     *((uint32_t*)(n + (*nl))) = disp;
//                     *nl += SIZE_F;
//                 } else {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         *nl += 2 + SIZE_E + 2 + SIZE_F;
//                     }
//                     else {
//                         *nl += 2 + SIZE_E + 3 + SIZE_F;
//                     }
//                 }
//             }
//             break;
//         default:
//             error("translate_read: undefined/unsupported ADDRM.\n");
//     }
// }
// 
// 
// /* integer only
//  * suppor t mattr
//  * when mattr is set
//  * reg is always eax/ax
//  */
//     inline void _translate2x86_64_read_0
// (uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
// {
//     int in_reg_file = 0;
//     int is_stdin = 0;
// 
// #ifdef _DEBUG_MORE_
//     zlogf("translate2x86_64_read: addrm: %d\n", addrm);
// #endif
//     /* 
//     // TODO debugging here
//     if (mattr == MATTR_SB || mattr == MATTR_UB) {
//         error("_translate2x86_64_read: B is not implemented yet.\n");
//     }
//     */
// 
//     switch (addrm) {
//         case ADDRM_IMMEDIATE:
//             if (is_write_n) {
//                 if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                     // movl $opr, %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movl $%#lx, %%rax\n", n+(*nl), opr);
// #endif
//                     n[(*nl)++] = 0xb8;
//                     // $opr
//                     *((uint32_t*)(n + (*nl))) = (uint32_t)opr;
//                     *nl += SIZE_F;
//                 } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                     // movb $opr, %al
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movb $%#lx, %%al\n", n+(*nl), (uint8_t)opr);
// #endif
//                     n[(*nl)++] = 0xb0;
//                     // $opr
//                     *((uint8_t*)(n + (*nl))) = (uint8_t)opr;
//                     *nl += SIZE_B;
//                 }
//                 else {
//                     // movq $opr, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movq $%#lx, %%reg %d\n", n+(*nl), opr, reg);
// #endif
//                     n[(*nl)++] = 0x48;
//                     // reg
//                     switch (reg) {
//                         case X86_64_R_RDI:
//                             n[(*nl)++] = 0xbf;
//                             break;
//                         case X86_64_R_RSI:
//                             n[(*nl)++] = 0xbe;
//                             break;
//                         case X86_64_R_RAX:
//                             n[(*nl)++] = 0xb8;
//                             break;
//                         default:
//                             error("_translate_read: unsupported reg #1.\n");
//                     }
//                     // $opr
//                     *((uint64_t*)(n + (*nl))) = opr;
//                     *nl += SIZE_E;
//                 }
//             } else {
//                 if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                     *nl += 1 + SIZE_F;
//                 } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                     *nl += 1 + SIZE_B;
//                 }
//                 else {
//                     *nl += 2 + SIZE_E;
//                 }
//             }
//             break;
//         case ADDRM_ABSOLUTE:
//             in_reg_file = 0;
//             is_stdin = 0;
//             // check whether using reg file
//             in_reg_file = is_in_reg_file(opr);
//             // check whether read from stdin
//             is_stdin = check_is_stdin(opr, addrm);
//             // reg file
//             if (in_reg_file) {
//                 translate2x86_64_read_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
//             }
//             // read from stdin
//             else if (is_stdin) {
//                 if (is_write_n) {
//                     // leaq 0(%rip), %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0x8d;
//                     n[(*nl)++] = 0x05;
//                     n[(*nl)++] = 0x00;
//                     n[(*nl)++] = 0x00;
//                     n[(*nl)++] = 0x00;
//                     n[(*nl)++] = 0x00;
// 
//                     // pushq %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0x50;
// 
//                     // movq sys_stdin_q, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movq sys_stdin_q, %%rax\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0xa1;
//                     *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDIN_Q);
//                     *nl += SIZE_E;
// 
//                     // jmpq *%rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0xff;
//                     n[(*nl)++] = 0xe0;
// 
//                     // Note: %rax is poped by sys_new_runner
//                     // and return value is in %rdi
//  
//                     // movq %rdi, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movq %%rdi, %%reg %d\n", n+(*nl), reg);
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0x89;
//                     // reg
//                     switch (reg) {
//                         case X86_64_R_RDI:
//                             n[(*nl)++] = 0xff;
//                             break;
//                         case X86_64_R_RSI:
//                             n[(*nl)++] = 0xfe;
//                             break;
//                         case X86_64_R_RDX:
//                             n[(*nl)++] = 0xfa;
//                             break;
//                         case X86_64_R_RAX:
//                             n[(*nl)++] = 0xf8;
//                             break;
//                         default:
//                             error("_translate_read: unsupported reg #2.\n");
//                     }
//                 } else {
//                     *nl += 7 + 1 + 2 + SIZE_E + 2 + 3;
//                 }
// 
//             }
//             // normal memory operation
//             else {
//                 if (is_write_n) {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         // movl opr, %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movl %#lx, %%eax\n", n+(*nl), opr);
// #endif
//                         n[(*nl)++] = 0xa1;
//                         *((uint64_t*)(n + (*nl))) = opr;
//                         *nl += SIZE_E;
//                     }
//                     else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                          // movl opr, %al
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movb %#lx, %%al\n", n+(*nl), opr);
// #endif
//                         n[(*nl)++] = 0xa0;
//                         *((uint64_t*)(n + (*nl))) = opr;
//                         *nl += SIZE_E;
//                     } else {
//                         // movabsq opr, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0xa1;
//                         *((uint64_t*)(n + (*nl))) = opr;
//                         *nl += SIZE_E;
//                         // movq %rax, reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movq %%rax, %%reg %d\n", n+(*nl), reg);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x89;
//                         // reg
//                         switch (reg) {
//                             case X86_64_R_RDI:
//                                 n[(*nl)++] = 0xc7;
//                                 break;
//                             case X86_64_R_RSI:
//                                 n[(*nl)++] = 0xc6;
//                                 break;
//                             case X86_64_R_RDX:
//                                 n[(*nl)++] = 0xc2;
//                                 break;
//                                 /*
//                                    case X86_64_R_RCX:
//                                    n[(*nl)++] = 0xc1;
//                                    break;
//                                    */
//                             case X86_64_R_RAX:
//                                 n[(*nl)++] = 0xc0;
//                                 break;
//                             default:
//                                 error("_translate_read: unsupported reg #2.\n");
//                         }
//                     }
//                 } else {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         *nl += 1 + SIZE_E;
//                     } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                         *nl += 1 + SIZE_E;
//                     }
//                     else {
//                         *nl += 2 + SIZE_E + 3;
//                     }
//                 }
//             }
//             break;
//         case ADDRM_INDIRECT:
//             in_reg_file = 0;
//             // check whether using reg file
//             in_reg_file = is_in_reg_file(opr);
//             if (in_reg_file) {
//                 translate2x86_64_read_indirect_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
//             } else {
//                 if (is_write_n) {
//                     // movabsq opr, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0xa1;
//                     *((uint64_t*)(n + (*nl))) = opr;
//                     *nl += SIZE_E;
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         // movl (%rax), %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movl (%%rax), %%eax %d\n", n+(*nl));
// #endif
//                         n[(*nl)++] = 0x8b;
//                         n[(*nl)++] = 0x00;
//                     } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                         // movl (%rax), %al
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movl (%%rax), %%al %d\n", n+(*nl));
// #endif
//                         n[(*nl)++] = 0x8a;
//                         n[(*nl)++] = 0x00;
//                     }
//                     else {
//                         // mov (%rax), reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov (%%rax), %%reg %d\n", n+(*nl), reg);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x8b;
//                         // reg
//                         switch (reg) {
//                             case X86_64_R_RDI:
//                                 n[(*nl)++] = 0x38;
//                                 break;
//                             case X86_64_R_RSI:
//                                 n[(*nl)++] = 0x30;
//                                 break;
//                             default:
//                                 error("_translate_read: unsupported reg #3.\n");
//                         }
//                     }
//                 } else {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         *nl += 2 + SIZE_E + 2;
//                     } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                         *nl += 2 + SIZE_E + 2;
//                     }
//                     else {
//                         *nl += 2 + SIZE_E + 3;
//                     }
//                 }
//             }
//             break;
//         case ADDRM_DISPLACEMENT:
//             // check reg file
//             in_reg_file = 0;
//             // check whether using reg file
//             in_reg_file = is_in_reg_file(opr);
//             if (in_reg_file) {
//                 // error("read base+disp mode use reg is not implemented yet. \n");
//                 translate2x86_64_read_reg_file_base_disp_0(opr, disp, reg, n, nl, is_write_n, mattr);
//             } else {
// 
//                 if (is_write_n) {
//                     // movq $opr, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
// #endif
//                     n[(*nl)++] = 0x48;
//                     n[(*nl)++] = 0xa1;
//                     *((uint64_t*)(n + (*nl))) = opr;
//                     *nl += SIZE_E;
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         // mov disp(%rax), %eax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov %d(%%rax), %%eax %d\n", n+(*nl), disp);
// #endif
//                         n[(*nl)++] = 0x8b;
//                         n[(*nl)++] = 0x80;
// 
//                     } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                         // mov disp(%rax), %al
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov %d(%%rax), %%al %d\n", n+(*nl), disp);
// #endif
//                         n[(*nl)++] = 0x8a;
//                         n[(*nl)++] = 0x80;
// 
//                     }
//                     else {
//                         // mov disp(%rax), reg
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov %d(%%rax), %%reg %d\n", n+(*nl), disp, reg);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x8b;
//                         // reg
//                         switch (reg) {
//                             case X86_64_R_RDI:
//                                 n[(*nl)++] = 0xb8;
//                                 break;
//                             case X86_64_R_RSI:
//                                 n[(*nl)++] = 0xb0;
//                                 break;
//                             case X86_64_R_RAX:
//                                 n[(*nl)++] = 0x80;
//                                 break;
//                             default:
//                                 error("_translate_read: unsupported reg #1.\n");
//                         }
//                     }
//                     // disp
//                     *((uint32_t*)(n + (*nl))) = disp;
//                     *nl += SIZE_F;
//                 } else {
//                     if (mattr == MATTR_SF || mattr == MATTR_UF) {
//                         *nl += 2 + SIZE_E + 2 + SIZE_F;
//                     } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
//                         *nl += 2 + SIZE_E + 2 + SIZE_F;
//                     }
//                     else {
//                         *nl += 2 + SIZE_E + 3 + SIZE_F;
//                     }
//                 }
//             }
//             break;
//         default:
//             error("_translate_read: undefined/unsupported ADDRM.\n");
//     }
// }
// 
//     inline void translate2x86_64_read_0
// (uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
// {
//     _translate2x86_64_read_0(opr, disp, addrm, reg, n, nl, is_write_n, MATTR_UNDEFINED);
// }
// 


// new ones

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_read_indirect_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;

    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        // error("reg_file with SB/UB is not implemented yet.\n");
        translate2x86_64_read_reg_file_base_disp(opr, 0, reg, n, nl, is_write_n, mattr);
        return;
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq oprreg, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq (%%oprreg %d), %%reg %d\n", n+(*nl), oprreg, reg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x49;
        }
        n[(*nl)++] = 0x8b;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break; // FILE4 is different
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
            default:
                error("translate2x86_64_read_indirect_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
        if (oprreg == I0_REG_FILE4) {
            *nl += 1;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_read_reg_file_base_disp
(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet. working.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb disp(oprreg), al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %d(%%oprreg %d), %%al\n", n+(*nl), disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x8a;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
                default:
                     error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
 
            // error("reg_file with SB/UB is not implemented yet. working.\n");
        } else {
            // Q
            // movq disp(oprreg), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %d(%%oprreg %d), %%reg %d\n", n+(*nl), disp, oprreg, reg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x8b;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
                default:
                                                            error("translate2x86_64_read_reg_file_base_disp: unsupported regcomb.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
        }
    } else {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                *nl += 0;
            }
            else {
                *nl += 1;
            }
            *nl += 1 + 1 + SIZE_F;
        }
        else {
            *nl += 3 + SIZE_F;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_read_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        error("reg_file with SB/UB is not implemented yet.\n");
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq oprreg, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%oprreg %d, %%reg %d\n", n+(*nl), oprreg, reg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x4c;
        }
        n[(*nl)++] = 0x89;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xdf; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xde; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xda; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xd8; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xcf; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xce; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xca; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc8; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xfe; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xfa; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xf8; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf7; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xf2; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xf0; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xef; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xee; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xea; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xe8; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xd7; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xd6; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xd0; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xcf; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xce; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xca; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc8; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xc7; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xc6; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xc2; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
            default:
                error("translate2x86_64_read_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
    }
}

/* floating point only: implement D only currently
 * when mattr is set
 * reg is always eax/ax
 */
    inline void translate2x86_64_fp_read
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
{
    // TODO implement it
    int in_reg_file = 0;
    unsigned int mattr = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_fp_read: addrm: %d\n", addrm);
#endif
    
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        error("_translate2x86_64_fp_read: B is not implemented yet.\n");
    }

    switch (addrm) {
        case ADDRM_IMMEDIATE:
            if (is_write_n) {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    // movl $opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movl $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0xb8;
                    // $opr
                    *((uint32_t*)(n + (*nl))) = (uint32_t)opr;
                    *nl += SIZE_F;
                }
                else {
                    // movq $opr, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq $%#lx, %%reg %d\n", n+(*nl), opr, reg);
#endif
                    n[(*nl)++] = 0x48;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xbf;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xbe;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xb8;
                            break;
                        default:
                            error("translate2x86_64_fp_read: unsupported reg #1.\n");
                    }
                    // $opr
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                }
            } else {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    *nl += 1 + SIZE_F;
                }
                else {
                    *nl += 2 + SIZE_E;
                }
            }
            break;
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_read_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %#lx, %%eax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else {
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // movq %rax, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %%reg %d\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xc7;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xc6;
                                break;
                            case X86_64_R_RDX:
                                n[(*nl)++] = 0xc2;
                                break;
                                /*
                                   case X86_64_R_RCX:
                                   n[(*nl)++] = 0xc1;
                                   break;
                                   */
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0xc0;
                                break;
                            default:
                                error("translate2x86_64_fp_read: unsupported reg #2.\n");
                        }
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:

            if (is_write_n) {
                // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0xa1;
                *((uint64_t*)(n + (*nl))) = opr;
                *nl += SIZE_E;
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    // movl (%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movl (%%rax), %%eax %d\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x8b;
                    n[(*nl)++] = 0x00;
                }
                else {
                    // mov (%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx mov (%%rax), %%reg %d\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8b;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0x38;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0x30;
                            break;
                        default:
                            error("translate_read: unsupported reg #3.\n");
                    }
                }
            } else {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    *nl += 2 + SIZE_E + 2;
                }
                else {
                    *nl += 2 + SIZE_E + 3;
                }
            }
            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // TODO: implement float
                error("float: read base+disp mode use reg is not implemented yet. \n");
                // translate2x86_64_read_reg_file(opr, disp, reg, n, nl, is_write_n);
            } else {

                if (is_write_n) {
                    // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // mov disp(%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%eax %d\n", n+(*nl), disp);
#endif
                        n[(*nl)++] = 0x8b;
                        n[(*nl)++] = 0x80;

                    }
                    else {
                        // mov disp(%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%reg %d\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x8b;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("translate_read: unsupported reg #1.\n");
                        }
                    }
                    // disp
                    *((uint32_t*)(n + (*nl))) = disp;
                    *nl += SIZE_F;
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        default:
            error("translate_read: undefined/unsupported ADDRM.\n");
    }
}


/* integer only
 * suppor t mattr
 * when mattr is set
 * reg is always eax/ax
 */
    inline void _translate2x86_64_read
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int in_reg_file = 0;
    int is_stdin = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_read: addrm: %d\n", addrm);
#endif

    // safety check first
    if (mattr != MATTR_UNDEFINED && reg != X86_64_R_RAX) {
        error("_translate2x86_64_read: reg must be rax/ax when mattr is set.\n");
    }

    switch (addrm) {
        case ADDRM_IMMEDIATE:
            if (is_write_n) {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    // movl $opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movl $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0xb8;
                    // $opr
                    *((uint32_t*)(n + (*nl))) = (uint32_t)opr;
                    *nl += SIZE_F;
                } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                    // movb $opr, %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movb $%#lx, %%al\n", n+(*nl), (uint8_t)opr);
#endif
                    n[(*nl)++] = 0xb0;
                    // $opr
                    *((uint8_t*)(n + (*nl))) = (uint8_t)opr;
                    *nl += SIZE_B;
                }
                else {
                    // movq $opr, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq $%#lx, %%reg %d\n", n+(*nl), opr, reg);
#endif
                    n[(*nl)++] = 0x48;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xbf;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xbe;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xb8;
                            break;
                        default:
                            error("_translate_read: unsupported reg #1.\n");
                    }
                    // $opr
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                }
            } else {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    *nl += 1 + SIZE_F;
                } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                    *nl += 1 + SIZE_B;
                }
                else {
                    *nl += 2 + SIZE_E;
                }
            }
            break;
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            is_stdin = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            // check whether read from stdin
            is_stdin = check_is_stdin(opr, addrm);
            // reg file
            if (in_reg_file) {
                translate2x86_64_read_reg_file(opr, reg, n, nl, is_write_n, mattr);
            }
            // read from stdin
            else if (is_stdin) {
                if (is_write_n) {
                    // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8d;
                    n[(*nl)++] = 0x05;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;

                    // pushq %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x50;

                    // movq sys_stdin_q, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq sys_stdin_q, %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDIN_Q);
                    *nl += SIZE_E;

                    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = 0xe0;

                    // Note: %rax is poped by sys_new_runner
                    // and return value is in %rdi
 
                    // movq %rdi, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq %%rdi, %%reg %d\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x89;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xff;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xfe;
                            break;
                        case X86_64_R_RDX:
                            n[(*nl)++] = 0xfa;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xf8;
                            break;
                        default:
                            error("_translate_read: unsupported reg #2.\n");
                    }
                } else {
                    *nl += 7 + 1 + 2 + SIZE_E + 2 + 3;
                }

            }
            // normal memory operation
            else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %#lx, %%eax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                         // movl opr, %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %#lx, %%al\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa0;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    } else {
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // movq %rax, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %%reg %d\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xc7;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xc6;
                                break;
                            case X86_64_R_RDX:
                                n[(*nl)++] = 0xc2;
                                break;
                                /*
                                   case X86_64_R_RCX:
                                   n[(*nl)++] = 0xc1;
                                   break;
                                   */
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0xc0;
                                break;
                            default:
                                error("_translate_read: unsupported reg #2.\n");
                        }
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_read_indirect_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {
                    // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl (%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl (%%rax), %%eax %d\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x8b;
                        n[(*nl)++] = 0x00;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl (%rax), %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl (%%rax), %%al %d\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x8a;
                        n[(*nl)++] = 0x00;
                    }
                    else {
                        // mov (%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov (%%rax), %%reg %d\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x8b;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0x38;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0x30;
                                break;
                            default:
                                error("_translate_read: unsupported reg #3.\n");
                        }
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + SIZE_E + 2;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + SIZE_E + 2;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }
            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // error("read base+disp mode use reg is not implemented yet. \n");
                translate2x86_64_read_reg_file_base_disp(opr, disp, reg, n, nl, is_write_n, mattr);
            } else {

                if (is_write_n) {
                    // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // mov disp(%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%eax %d\n", n+(*nl), disp);
#endif
                        n[(*nl)++] = 0x8b;
                        n[(*nl)++] = 0x80;

                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // mov disp(%rax), %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%al %d\n", n+(*nl), disp);
#endif
                        n[(*nl)++] = 0x8a;
                        n[(*nl)++] = 0x80;

                    }
                    else {
                        // mov disp(%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%reg %d\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x8b;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("_translate_read: unsupported reg #1.\n");
                        }
                    }
                    // disp
                    *((uint32_t*)(n + (*nl))) = disp;
                    *nl += SIZE_F;
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + SIZE_E + 2 + SIZE_F;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        default:
            error("_translate_read: undefined/unsupported ADDRM.\n");
    }
}


// return register whether the destination is in
    inline int translate2x86_64_read
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int suggested_reg, char *n, uint64_t *nl, int is_write_n)
{

    _translate2x86_64_read(opr, disp, addrm, suggested_reg, n, nl, is_write_n, MATTR_UNDEFINED);
    return suggested_reg;
}

// old ones

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file_base_disp_0
(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb al, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %d(%%oprreg %d), %%al %d\n", n+(*nl), disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
                default:
                                       error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            // movq reg, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, %d(%%oprreg %d)\n", n+(*nl), reg, disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
                default:
                                                            error("translate2x86_64_write_reg_file_base_disp: unsupported regcomb.\n");
            }

            // zlogf("Debug 1: %d\n", disp);
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
        }
    } else {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                *nl += 0;
            }
            else {
                *nl += 1;
            }
            *nl += 1 + 1 + SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            *nl += 3 + SIZE_F;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file_0
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        error("reg_file with SB/UB is not supported.\n");
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not supported.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq reg, oprreg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%reg %d, %%oprreg %d\n", n+(*nl), reg, oprreg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x49;
        }
        n[(*nl)++] = 0x89;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfb; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc3; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc7; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfe; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc6; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfd; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc5; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfa; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc2; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf8; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
            default:
                error("translate2x86_64_write_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_indirect_reg_file_0
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {

        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb %al, (oprreg)

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %%al %d, (%%oprreg %d)\n", n+(*nl), oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x06; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break; // %r13 is special
                case (I0_REG_FILE5): n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x00; break;
                default:
                     error("translate2x86_64_write_indirect_reg_file: SB/UB: unsupported oprreg.\n");
            }

        }
        else {
            // UQ / SQ
            // movq reg, (oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, (%%oprreg %d)\n", n+(*nl), reg, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
                                                            // %r13 FILE4 is
                                                            // special here
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
                default:
                                                            error("translate2x86_64_write_indirect_reg_file: unsupported regcomb.\n");
            }
        }
    } else {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                (*nl)++;
            }
            (*nl) += 2;

            if (oprreg == I0_REG_FILE4) {
                // %r13 is special
                (*nl)++;
            }
        }
        else {
            *nl += 3;
            if (oprreg == I0_REG_FILE4) {
                *nl += 1;
            }
        }
    }
}



/*
 * suppor t mattr
 * when mattr is set
 * reg is always eax/al
 */
    inline void _translate2x86_64_write_0
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int in_reg_file = 0;
    int is_stdout = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_write: addrm: %d\n", addrm);
#endif

    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("_translate2x86_64_write: B is not implemented yet.\n");
    // }

    if (mattr != MATTR_UNDEFINED && reg != X86_64_R_RAX) {
        error("_translate2x86_64_write: when mattr is used, reg should always be ax/rax\n");
    }

    switch (addrm) {
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            is_stdout = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            // check whether write to stdout
            is_stdout = check_is_stdout(opr, addrm);
            if (in_reg_file) {
                translate2x86_64_write_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
            } 
            // write to stdout
            else if (is_stdout) {
                if (is_write_n) {
                    // movq reg, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq %%reg, %%rdi\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x89;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xff;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xf7;
                            break;
                        case X86_64_R_RDX:
                            n[(*nl)++] = 0xd7;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xc7;
                            break;
                        default:
                            error("translate_write: unsupported reg for stdout.\n");
                    }

                    // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8d;
                    n[(*nl)++] = 0x05;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;

                    // pushq %rax
                    // Note: %rax is poped by caller
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x50;

                    // movq sys_stdout_q, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq sys_stdout_q, %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDOUT_Q);
                    *nl += SIZE_E;

                    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = 0xe0;
                } else {
                    *nl += 3 + 7 + 1 + 2 + SIZE_E + 2;
                }
            }
            // normal write op
            else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %%eax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al, %#lx\n", opr, n+(*nl));
#endif
                        n[(*nl)++] = 0xa2;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else {
                        // movabsq reg, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %%reg %d, %%rax\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xf8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xf0;
                                break;
                            default:
                                error("translate_write: unsupported reg #2.\n");
                        }
                        // movq %rax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 3 + 2 + SIZE_E;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_write_indirect_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {

                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %edx, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x10;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, %dl
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al %d, %%dl)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %dl, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%dl %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0x10;
                    }
                    else {
                        // MATTR_SE or MATTR_UE


                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, (%%rax)\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0x38;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0x30;
                                break;
                            default:
                                error("translate_write: unsupported reg #3.\n");
                        }
                    }
                }
                else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    } 
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }

            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // error("write base+disp mode use reg is not implemented yet. \n");
                translate2x86_64_write_reg_file_base_disp_0(opr, disp, reg, n, nl, is_write_n, mattr);
            } else {

                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov %edx, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x90;
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                    else if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("translate_write: unsupported reg #1.\n");
                        }
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        case ADDRM_IMMEDIATE:
            error("translate_write: should never write to immediate addmr.\n");
            break;
        default:
            error("translate_write: undefined/unsupported ADDRM.\n");
    }
}

    inline void translate2x86_64_write_0
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
{
    _translate2x86_64_write_0(opr, disp, addrm, reg, n, nl, is_write_n, MATTR_UNDEFINED);
}



// new one

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file_base_disp
(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb al, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %%al, %d(%%oprreg %d) %d\n", n+(*nl), disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
                default:
                                       error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            // movq reg, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, %d(%%oprreg %d)\n", n+(*nl), reg, disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
                default:
                                                            error("translate2x86_64_write_reg_file_base_disp: unsupported regcomb.\n");
            }

            // zlogf("Debug 1: %d\n", disp);
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
        }
    } else {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                *nl += 0;
            }
            else {
                *nl += 1;
            }
            *nl += 1 + 1 + SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            *nl += 3 + SIZE_F;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {

        error("reg_file with SB/UB is not supported.\n");
        // TODO: A temporary implementation. Should be more efficient.
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not supported.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq reg, oprreg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%reg %d, %%oprreg %d\n", n+(*nl), reg, oprreg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x49;
        }
        n[(*nl)++] = 0x89;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfb; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc3; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc7; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfe; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc6; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfd; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc5; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfa; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc2; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf8; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
            default:
                error("translate2x86_64_write_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_indirect_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {

        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb %al, (oprreg)

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %%al %d, (%%oprreg %d)\n", n+(*nl), oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x06; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break; // %r13 is special
                case (I0_REG_FILE5): n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x00; break;
                default:
                     error("translate2x86_64_write_indirect_reg_file: SB/UB: unsupported oprreg.\n");
            }

        }
        else {
            // UQ / SQ
            // movq reg, (oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, (%%oprreg %d)\n", n+(*nl), reg, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
                                                            // %r13 FILE4 is
                                                            // special here
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
                default:
                                                            error("translate2x86_64_write_indirect_reg_file: unsupported regcomb.\n");
            }
        }
    } else {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                (*nl)++;
            }
            (*nl) += 2;

            if (oprreg == I0_REG_FILE4) {
                // %r13 is special
                (*nl)++;
            }
        }
        else {
            *nl += 3;
            if (oprreg == I0_REG_FILE4) {
                *nl += 1;
            }
        }
    }
}



/*
 * suppor t mattr
 * when mattr is set
 * reg is always eax/al
 */
    inline void _translate2x86_64_write
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int in_reg_file = 0;
    int is_stdout = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_write: addrm: %d\n", addrm);
#endif

    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("_translate2x86_64_write: B is not implemented yet.\n");
    // }

    if (mattr != MATTR_UNDEFINED && reg != X86_64_R_RAX) {
        error("_translate2x86_64_write: when mattr is used, reg should always be ax/rax\n");
    }

    switch (addrm) {
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            is_stdout = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            // check whether write to stdout
            is_stdout = check_is_stdout(opr, addrm);
            if (in_reg_file) {
                translate2x86_64_write_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } 
            // write to stdout
            else if (is_stdout) {
                if (is_write_n) {
                    // movq reg, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq %%reg, %%rdi\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x89;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xff;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xf7;
                            break;
                        case X86_64_R_RDX:
                            n[(*nl)++] = 0xd7;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xc7;
                            break;
                        default:
                            error("translate_write: unsupported reg for stdout.\n");
                    }

                    // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8d;
                    n[(*nl)++] = 0x05;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;

                    // pushq %rax
                    // Note: %rax is poped by caller
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x50;

                    // movq sys_stdout_q, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq sys_stdout_q, %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDOUT_Q);
                    *nl += SIZE_E;

                    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = 0xe0;
                } else {
                    *nl += 3 + 7 + 1 + 2 + SIZE_E + 2;
                }
            }
            // normal write op
            else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %%eax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al, %#lx\n", opr, n+(*nl));
#endif
                        n[(*nl)++] = 0xa2;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else {
                        // movabsq reg, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %%reg %d, %%rax\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xf8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xf0;
                                break;
                            default:
                                error("translate_write: unsupported reg #2.\n");
                        }
                        // movq %rax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 3 + 2 + SIZE_E;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_write_indirect_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {

                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %edx, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x10;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, %dl
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al %d, %%dl)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %dl, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%dl %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0x10;
                    }
                    else {
                        // MATTR_SE or MATTR_UE


                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, (%%rax)\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0x38;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0x30;
                                break;
                            default:
                                error("translate_write: unsupported reg #3.\n");
                        }
                    }
                }
                else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    } 
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }

            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // error("write base+disp mode use reg is not implemented yet. \n");
                translate2x86_64_write_reg_file_base_disp(opr, disp, reg, n, nl, is_write_n, mattr);
            } else {

                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov %edx, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x90;
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("translate_write: unsupported reg #1.\n");
                        }
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        case ADDRM_IMMEDIATE:
            error("translate_write: should never write to immediate addmr.\n");
            break;
        default:
            error("translate_write: undefined/unsupported ADDRM.\n");
    }
}

    inline void translate2x86_64_write
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
{
    _translate2x86_64_write(opr, disp, addrm, reg, n, nl, is_write_n, MATTR_UNDEFINED);
}


int translate2x86_64_nop(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    if (is_write_n) {
        // nop
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx nop\n", n+(*nl));
#endif
        n[(*nl)++] = 0x90; 
    } else {
        *nl += 1;
    }

    return 0;
}

inline int x86_64_encode_cmpq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2) 
{
    int reg1_en = 0;
    int reg2_en = 0;
    char c;

    if (is_write_n) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("opt native instr: @%#lx cmpq %%oprreg %d, %%oprreg %d\n", n+(*nl), reg1, reg2);
#endif
        reg1_en = x86_64_reg_encoding(reg1);
        reg2_en = x86_64_reg_encoding(reg2);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg1_en & 0x8) >> 1; // R
        c += (reg2_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // Opcode
        n[(*nl)++] = 0x39;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // ModRM
        c = 0xc0; // mod 11 (binary)
        c += (reg1_en & 0x7) << 3; // rrr
        c += (reg2_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
    } else {
        *nl += 3;
    }
    return 0;
}

int opt_translate2x86_64_br_cmp(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    int oprreg1 = -1;
    int oprreg2 = -1;

    // reg1 compare reg2
    // compq reg2, reg1
    if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) && 
            (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
            (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) )
       ) {
        // check only supported options
        switch (i->option) {
            case OPT_B_SL:
                return 0;
                break;
            case OPT_B_LE:
            case OPT_B_L:
            case OPT_B_E:
            case OPT_B_NE:
                break;
            default:
                return 0;
                break;
        }

        // determine oprreg for the opr
        // tricky here: just map the REG_FILEX
        oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;
        oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
        
        // encode it
        x86_64_encode_cmpq_rr(is_write_n, n, nl, oprreg2, oprreg1);
        
        return 1;
    }

    return 0;
}

int translate2x86_64_br(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    uint64_t entry_to_update = 0;
    uint64_t trampoline = 0;
    int32_t relative_addr = 0;

    switch (i->option) {
        case OPT_B_J:
            if (i->ra == JUMP_A) {
                entry_to_update = (uint64_t)(n + (*nl));
                trampoline = (uint64_t)(t+(*tl));

                // 5 is the length of jmp instruction in x86_64
                if (x86_64_in_jmp_range(entry_to_update+5, trampoline)) {

                    if (is_write_n) {
                        entry_to_update = (uint64_t)(n + (*nl));
                        relative_addr = trampoline - entry_to_update - 5;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jmp %d\n", n+(*nl), relative_addr);
#endif
                        n[(*nl)++] = 0xe9;
                        // relative address
                        *((uint32_t*)(n + (*nl))) = relative_addr;

                        *nl += 4;

                        add_trampoline_new(i->opr1, entry_to_update, t, tl, TRAMP_TYPE_JMP);

                    } else {
                        *nl += 5;
                    }

                } else {
                    // default handling
                    if (is_write_n) {
                        // trampoline
                        entry_to_update = (uint64_t)(n + (*nl));
                        // movq $trampoline, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov $%#lx, %%rax\n", n+(*nl), (t+(*tl)));
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xb8;
                        // trampoline entry
                        *((uint64_t*)(n + (*nl))) = (uint64_t)(t + (*tl));
                        *nl += SIZE_E;
                        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                        n[(*nl)++] = 0xff;
                        n[(*nl)++] = 0xe0;

                        add_trampoline_new(i->opr1, entry_to_update, t, tl, TRAMP_TYPE_DEFAULT);
                    } else {
                        *nl += 2 + SIZE_E + 2;
                    }
                }
            } else {
                error("Translate: JUMP_R is not implemented.\n");
            }
            break;
        case OPT_B_LE:
        case OPT_B_L:
        case OPT_B_E:
        case OPT_B_NE:
        case OPT_B_SL:
            // phase1: compare
            if (opt_translate2x86_64_br_cmp(i, is_write_n, n, nl, t, tl)) {
                // translated using optimized version
            } else {
                // READ(D1, %rdi)
                translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
                // READ(D2, %rsi)
                translate2x86_64_read(i->opr2, i->disp2, i->addrm2, X86_64_R_RSI, n, nl, is_write_n);
                if (is_write_n) {
                    if (i->option == OPT_B_SL) {
                        // Save %rcx first.
                        // save %rcx as it is in register file
                        // pushq %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx push %%rcx\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x51;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov $8, %%rcx\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xc7;
                        n[(*nl)++] = 0xc1;

                        *((uint32_t*)(n + (*nl))) = 8;
                        *nl += 4;

                        // cmpsq
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx repz cmpsb\n", n+(*nl));
#endif
                        n[(*nl)++] = 0xf3;
                        n[(*nl)++] = 0xa6;

                        // pop %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx pop %%rcx\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x59;

                    }
                    else {
                        // TODO: compare D and F
                        // cmpq %rsi, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx cmpq %%rsi, %%rdi\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x39;
                        n[(*nl)++] = 0xf7;

                    }
                } else {
                    if (i->option == OPT_B_SL) {
                        *nl += 1 + 7 + 2 + 1;
                    }
                    else {
                        *nl += 3; 
                    }
                }
            }
            // phase2: jump phase
            entry_to_update = (uint64_t)(n + (*nl));
            trampoline = (uint64_t)(t+(*tl));

            // 6 is the length of jcc instruction in x86_64
            if (x86_64_in_jmp_range(entry_to_update+6, trampoline)) {
                if (is_write_n) {
                    relative_addr = trampoline - entry_to_update - 6;
                    // next:
                    // 6B jcc 5B
                    if (i->ra == JUMP_A) {
                        // jcc relative_addr
                        // note: the trampoline handler partially update
                        // the instruction
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jcc %d\n", n+(*nl), relative_addr);
#endif
                        n[(*nl)++] = 0x0f;
                        switch (i->option) {
                            case OPT_B_LE:
                                if (i->attr == ATTR_SE) {
                                    n[(*nl)++] = 0x8e;
                                }
                                else {
                                    n[(*nl)++] = 0x86;
                                }
                                break;
                            case OPT_B_L:
                                if (i->attr == ATTR_SE) {
                                    n[(*nl)++] = 0x8c;
                                }
                                else {
                                    n[(*nl)++] = 0x82;
                                }
                                break;
                            case OPT_B_E:
                                n[(*nl)++] = 0x84;
                                break;
                            case OPT_B_NE:
                                n[(*nl)++] = 0x85;
                                break;
                            case OPT_B_SL:
                                // Note: D2 > D1. Hence, it is jg
                                // jg +5
                                n[(*nl)++] = 0x8f;
                                break;
                            default:
                                ; // never goes here
                        }
                        // relative address
                        *((uint32_t*)(n + (*nl))) = relative_addr;
                        *nl += 4;

                        add_trampoline_new(i->opr3, entry_to_update, t, tl, TRAMP_TYPE_JCC);
                    } else {
                        // JUMP_R
                        error("Translate: JUMP_R is not implemented.\n");
                    }

                } else {
                    *nl += 6;
                }

            } else {
                // default handling

                if (is_write_n) {
                    // next:
                    // 6B jcc 5B
                    // 5B jmp 12B
                    // 10B mov $I, %rax
                    // 2B jmp *%rax
                    if (i->ra == JUMP_A) {
                        // jcc 5
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jcc +5\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x0f;
                        switch (i->option) {
                            case OPT_B_LE:
                                if (i->attr == ATTR_SE) {
                                    n[(*nl)++] = 0x8e;
                                }
                                else {
                                    n[(*nl)++] = 0x86;
                                }
                                break;
                            case OPT_B_L:
                                if (i->attr == ATTR_SE) {
                                    n[(*nl)++] = 0x8c;
                                }
                                else {
                                    n[(*nl)++] = 0x82;
                                }
                                break;
                            case OPT_B_E:
                                n[(*nl)++] = 0x84;
                                break;
                            case OPT_B_NE:
                                n[(*nl)++] = 0x85;
                                break;
                            case OPT_B_SL:
                                // Note: D2 > D1. Hence, it is jg
                                // jg +5
                                n[(*nl)++] = 0x8f;
                                break;
                            default:
                                ; // never goes here
                        }
                        // relative address
                        *((uint32_t*)(n + (*nl))) = 5;
                        *nl += 4;
                        // jmp 12
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jmp +12\n", n+(*nl));
#endif
                        n[(*nl)++] = 0xe9;
                        // relative address
                        *((uint32_t*)(n + (*nl))) = 12;
                        *nl += 4;

                        // trampoline
                        entry_to_update = (uint64_t)(n + (*nl));
                        // movq $trampoline, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov $%#lx, %%rax\n", n+(*nl), (t+(*tl)));
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xb8;
                        // trampoline entry
                        *((uint64_t*)(n + (*nl))) = (uint64_t)(t + (*tl));
                        *nl += SIZE_E;
                        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                        n[(*nl)++] = 0xff;
                        n[(*nl)++] = 0xe0;

                        // add_trampoline_new(i->opr3, entry_to_update, t, tl, TRAMP_TYPE_JCC);
                        add_trampoline_new(i->opr3, entry_to_update, t, tl, TRAMP_TYPE_DEFAULT);

                    } else {
                        // JUMP_R
                        error("Translate: JUMP_R is not implemented.\n");
                    }

                } else {
                    *nl += 6 + 5 + 2 + SIZE_E + 2;
                }
            }
            break;
        case OPT_B_Z:
        case OPT_B_NZ:
            // READ(D1, %rdi)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
            if (is_write_n) {
                // TODO: compare D and F
                // movq $0, %rsi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq $0, %%rsi\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0xc7;
                n[(*nl)++] = 0xc6;
                n[(*nl)++] = 0x00;
                n[(*nl)++] = 0x00;
                n[(*nl)++] = 0x00;
                n[(*nl)++] = 0x00;

                // cmpq %rsi, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cmpq %%rsi, %%rdi\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x39;
                n[(*nl)++] = 0xf7;

            } else {
                *nl += 7 + 3;
            }
            // phase2: jmp
            entry_to_update = (uint64_t)(n + (*nl));
            trampoline = (uint64_t)(t+(*tl));

            // 6 is the length of jcc instruction in x86_64
            if (x86_64_in_jmp_range(entry_to_update+6, trampoline)) {
                if (is_write_n) {
                    relative_addr = trampoline - entry_to_update - 6;
                    // next:
                    // 6B jcc 5B
                    if (i->ra == JUMP_A) {
                        // jcc relative_addr
                        // note: the trampoline handler partially update
                        // the instruction

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jcc %d\n", n+(*nl), relative_addr);
#endif
                        n[(*nl)++] = 0x0f;
                        switch (i->option) {
                            case OPT_B_Z:
                                n[(*nl)++] = 0x84;
                                break;
                            case OPT_B_NZ:
                                n[(*nl)++] = 0x85;
                                break;
                            default:
                                ; // never goes here
                        }

                        // relative address
                        *((uint32_t*)(n + (*nl))) = relative_addr;
                        *nl += 4;

                        add_trampoline_new(i->opr3, entry_to_update, t, tl, TRAMP_TYPE_JCC);
                    } else {
                        // JUMP_R
                        error("Translate: JUMP_R is not implemented.\n");
                    }

                } else {
                    *nl += 6;
                }

            } else {
                if (is_write_n) {
                    // next:
                    // 6B jcc 5B
                    // 5B jmp 12B
                    // 10B mov $I, %rax
                    // 2B jmp *%rax
                    if (i->ra == JUMP_A) {
                        // jcc 5
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jcc +5\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x0f;
                        switch (i->option) {
                            case OPT_B_Z:
                                n[(*nl)++] = 0x84;
                                break;
                            case OPT_B_NZ:
                                n[(*nl)++] = 0x85;
                                break;
                            default:
                                ; // never goes here
                        }
                        // relative address
                        *((uint32_t*)(n + (*nl))) = 5;
                        *nl += 4;
                        // jmp 12
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jmp +12\n", n+(*nl));
#endif
                        n[(*nl)++] = 0xe9;
                        // relative address
                        *((uint32_t*)(n + (*nl))) = 12;
                        *nl += 4;

                        // trampoline
                        entry_to_update = (uint64_t)(n + (*nl));
                        // movq $trampoline, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov $%#lx, %%rax\n", n+(*nl), (t+(*tl)));
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xb8;
                        // trampoline entry
                        *((uint64_t*)(n + (*nl))) = (uint64_t)(t + (*tl));
                        *nl += SIZE_E;
                        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                        n[(*nl)++] = 0xff;
                        n[(*nl)++] = 0xe0;

                        add_trampoline_new(i->opr3, entry_to_update, t, tl, TRAMP_TYPE_DEFAULT);

                    } else {
                        // JUMP_R
                        error("Translate: JUMP_R is not implemented.\n");
                    }
                } else {
                    *nl += 6 + 5 + 2 + SIZE_E + 2;
                }
            }
            break;

        case OPT_B_IJ:
            // report error if addrm1 is immediate
            if (i->addrm1 == ADDRM_IMMEDIATE) {
                error("B_IJ: addrm1 immediate_uq is not supported.\n");
            }
            // READ(M, %rdi)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
            if (is_write_n) {
                // jmp to sys_indirect_jump

                // movq sys_indirect_jump, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx mov $sys_indirect_jump ,%%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0xa1;
                *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_INDIRECT_JUMP);
                *nl += SIZE_E;

                // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0xff;
                n[(*nl)++] = 0xe0;
            } else {
                *nl += 2 + SIZE_E + 2;
            }
            break;
        default:
            error("Translate: undefined/unsupported B OPTION.\n");
    }

    return 0;
}

// // obsolete
// int translate2x86_64_mov_v0(
//         instr_t *i, int is_write_n,
//         char *n, uint64_t *nl, char *t, uint64_t *tl)
// {
//     // read in opr1 to %rax
//     switch (i->addrm1) {
//         case ADDRM_IMMEDIATE:
//             if (is_write_n) {
//                 // movq $opr1 %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                 zlogf("native instr: @%#lx movq $%#lx, %%rax\n", n+(*nl), i->opr1);
// #endif
//                 n[(*nl)++] = 0x48;
//                 n[(*nl)++] = 0xb8;
//                 *((uint64_t*)(n + (*nl))) = i->opr1;
//                 *nl += SIZE_E;
//             } else {
//                 *nl += 2 + SIZE_E;
//             }
//             break;
//         case ADDRM_ABSOLUTE:
//             if (is_write_n) {
//                 // movq opr2, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                 zlogf("native instr: @%#lx movq %#lx, %%rax\n", n+(*nl), i->opr2);
// #endif
//                 n[(*nl)++] = 0x48;
//                 n[(*nl)++] = 0xa1;
//                 *((uint64_t*)(n + (*nl))) = i->opr1;
//                 *nl += SIZE_E;
//             } else {
//                 *nl += 2 + SIZE_E;
//             }
//             break;
//         case ADDRM_INDIRECT:
//             if (is_write_n) {
//                 // movq opr2, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                 zlogf("native instr: @%#lx movq %#lx, %%rax\n", n+(*nl), i->opr2);
// #endif
//                 n[(*nl)++] = 0x48;
//                 n[(*nl)++] = 0xa1;
//                 *((uint64_t*)(n + (*nl))) = i->opr1;
//                 *nl += SIZE_E;
//                 // movq (%rax), %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                 zlogf("native instr: @%#lx movq (%%rax), %%rax\n", n+(*nl));
// #endif
//                 n[(*nl)++] = 0x48;
//                 n[(*nl)++] = 0x8b;
//                 n[(*nl)++] = 0x00;
//             } else {
//                 *nl += 2 + SIZE_E + 3;
//             }
//             break;
//         case ADDRM_DISPLACEMENT:
//         default:
//             error("Translate: undefined/unsupported CONV ADDRM1.\n");
//     }
//     // write %rax to opr2
//     switch (i->l2) {
//         case LEN_Q:
//             switch (i->addrm2) {
//                 case ADDRM_ABSOLUTE:
//                     if (is_write_n) {
//                         // movq %rax, opr2
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx mov %%rax, %#lx\n", n+(*nl), i->opr2);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0xa3;
//                         *((uint64_t*)(n + (*nl))) = i->opr2;
//                         *nl += SIZE_E;
//                     } else {
//                         *nl += 2 + SIZE_E;
//                     }
//                     break;
//                 case ADDRM_INDIRECT:
//                     if (is_write_n) {
//                         // movq %rax, %rdi
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movq %%rax, %%rdi\n", n+(*nl));
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x89;
//                         n[(*nl)++] = 0xc7;
//                         // movq opr2, %rax
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movq %#lx, %%rax\n", n+(*nl), i->opr2);
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0xa1;
//                         *((uint64_t*)(n + (*nl))) = i->opr2;
//                         *nl += SIZE_E;
//                         // movq %rdi, (%rax)
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                         zlogf("native instr: @%#lx movq %%rdi, (%%rax)\n", n+(*nl));
// #endif
//                         n[(*nl)++] = 0x48;
//                         n[(*nl)++] = 0x89;
//                         n[(*nl)++] = 0x38;
//                     } else {
//                         *nl += 3 + 2 + SIZE_E + 3;
//                     }
//                     break;
//                 case ADDRM_DISPLACEMENT:
//                 case ADDRM_IMMEDIATE:
//                 default:
//                     error("Translate: undefined/unsupported CONV ADDRM2.\n");
//             }
//             break;
//         case LEN_L:
//         case LEN_B:
//         default:
//             error("Translate: undefined/unsupported CONV LEN2.\n");
//     }
// 
//     return 0;
// }

int within_32bit(uint64_t n)
{
    // long t = 0;
    long u = 0;
    // int t2 = 0;
    // t2 = *(int*)(&n);
    // t = (long) t2;
    u = *(long*)(&n);

    if (u > ((0x1 << 31) - 1)) return 0;

    if (u < (0 - (0x1 << 31))) return 0;

    return 1;

    // return (t == u);
}

void instr2x86_64_opr(unsigned int attr, unsigned int addrm, uint64_t opr, uint32_t disp, x86_64_opr_t *to) 
{
    to->class0 = X86_64_CLASS0_NOT_SUPPORTED;
    // only for SQ/UQ currently
    switch (attr) {
        case ATTR_SE:
        case ATTR_UE:
            break;
        default:
            error("instr2x86_64_opr: not implemented.\n");
            return;
    }

    switch (addrm) {
        case ADDRM_IMMEDIATE:
            if (within_32bit(opr)) {
                to->class0 = X86_64_CLASS0_NO_MEM;
                to->class1 = X86_64_CLASS1_IMM;
                to->opr = opr;
                to->addrm = X86_64_ADDRM_IMMEDIATE_SL;
            } else {
                // zlogf("not within_32bit.\n");
                return;
            }
            break;
        case ADDRM_ABSOLUTE:
            if (is_in_reg_file(opr)) {
                to->class0 = X86_64_CLASS0_NO_MEM;
                to->class1 = X86_64_CLASS1_REG;
                to->opr = opr_reg(opr);
                to->addrm = X86_64_ADDRM_REG;
            } else {
                return;
            }
            break;
        case ADDRM_INDIRECT:
            if (is_in_reg_file(opr)) {
                to->class0 = X86_64_CLASS0_MEM;
                to->class1 = X86_64_CLASS1_MEM_REG;
                to->opr = opr_reg(opr);
                to->addrm = X86_64_ADDRM_INDIRECT;
            } else {
                return;
            }
            break;
        case ADDRM_DISPLACEMENT:
            if (is_in_reg_file(opr)) {
                to->class0 = X86_64_CLASS0_MEM;
                to->class1 = X86_64_CLASS1_MEM_REG;
                to->opr = opr_reg(opr);
                to->disp = disp;
                to->addrm = X86_64_ADDRM_DISPLACEMENT;
            } else {
                return;
            }
            break;
        default:
            error("instr2x86_64_opr: not implemented.\n");
            return;
    }

    return;
}

void new_instr2x86_64_oprs(instr_t *i, x86_64_opr_t *oprs, int n)
{
    int j;
    for (j = 0; j < n; j++) {
        instr2x86_64_opr(i->attr, i->addrms[j], i->oprs[j], i->disps[j], oprs+j);
    }
    return;
}

void instr2x86_64_oprs(instr_t *i, x86_64_opr_t *oprs, int n)
{
    if (n != 3) {
        error("instr2x86_64_oprs: only 3 for n is supported.\n");
    }

    instr2x86_64_opr(i->attr, i->addrm1, i->opr1, i->disp1, oprs);
    instr2x86_64_opr(i->attr, i->addrm2, i->opr2, i->disp2, oprs+1);
    instr2x86_64_opr(i->attr, i->addrm3, i->opr3, i->disp3, oprs+2);

    return;
}

void print_x86_64_opr(x86_64_opr_t *opr)
{
    zlogf("x86_64_opr:"
            "class0: %d, "
    "class1: %d, "
    "addrm: %d, "
    "opr: %#lx, "
    "disp: %d\n", opr->class0, opr->class1, opr->addrm, opr->opr, opr->disp);

}

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_mov(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // TODO: mov b to long and long to b
    uint32_t class0_comb = 0;
    x86_64_opr_t oprs[2];

    // optimize SQ and UQ only
    if ( ! (i->mattr1 == ATTR_SE || i->mattr1 == ATTR_UE) ) {
        return 0;
    }
    if ( ! (i->mattr2 == ATTR_SE || i->mattr2 == ATTR_UE) ) {
        return 0;
    }

    // print_instr(i);

    instr2x86_64_opr(i->mattr1, i->addrm1, i->opr1, i->disp1, &oprs[0]);
    instr2x86_64_opr(i->mattr2, i->addrm2, i->opr2, i->disp2, oprs+1);

    // print_x86_64_opr(&oprs[0]);
    // print_x86_64_opr(oprs+1);

    if (
            oprs[0].class0 == X86_64_CLASS0_NOT_SUPPORTED ||
            oprs[1].class0 == X86_64_CLASS0_NOT_SUPPORTED
       ) {
        return 0;
    }

    class0_comb = (oprs[0].class0 << 8) + oprs[1].class0;
    switch (class0_comb) {
        case (X86_64_CLASS0_NO_MEM << 8) + X86_64_CLASS0_MEM:
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             if (is_write_n) {
//                 zlogf("@%#lx mov non_mem, mem\n", n+(*nl));
//             }
// #endif
            x86_64_encode_movq(is_write_n, n, nl, oprs, oprs+1);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 8) + X86_64_CLASS0_NO_MEM:
            // return 0;
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             if (is_write_n) {
//                 zlogf("@%#lx mov non_mem, non_mem\n", n+(*nl));
//             }
// #endif
            // zlogf("!!! Check here.\n");
            x86_64_encode_movq(is_write_n, n, nl, oprs, oprs+1);
            return 1;
            break;
        case (X86_64_CLASS0_MEM << 8) + X86_64_CLASS0_NO_MEM:
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             if (is_write_n) {
//                 zlogf("@%#lx mov mem, non_mem\n", n+(*nl));
//             }
// #endif
            x86_64_encode_movq(is_write_n, n, nl, oprs, oprs+1);
            return 1;
            break;
        case (X86_64_CLASS0_MEM << 8) + X86_64_CLASS0_MEM:
            return 0; // since the default one can handle it well already
            break;
        default:
            break;
    }

//     // TODO: not implemnted yet
//     int oprreg1 = -1;
//     int oprreg2 = -1;
//     int oprregcomb = 0;
// 
//     // if there is optimization opportunity
//     // immediate -> oprreg
//     if ( 0 &&
//             (i->addrm1 == ADDRM_IMMEDIATE ) &&
//             (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) ) && 
//             (i->mattr1 == MATTR_SE || i->mattr1 == MATTR_UE) && 
//             (i->mattr2 == MATTR_SE || i->mattr2 == MATTR_UE) 
//             ) {
//         // determine oprreg for the opr
//         // tricky here: just map the REG_FILEX
//         oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
//         
//         if (is_write_n) {
// 
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             zlogf("native instr: @%#lx movq $%#lx, %%oprreg %d\n", n+(*nl), i->opr1, oprreg2 );
// #endif
//             if (oprreg2 == I0_REG_FILE0 || oprreg2 == I0_REG_FILE1 ) {
//                 // %rbx and %rcx are different from others
//                 n[(*nl)++] = 0x48;
//             }
//             else {
//                 n[(*nl)++] = 0x49;
//             }
// 
//             switch (oprreg2) {
//                 case I0_REG_FILE0:
//                     n[(*nl)++] = 0xbb;
//                     break;
//                 case I0_REG_FILE1:
//                     n[(*nl)++] = 0xb9;
//                     break;
//                 case I0_REG_FILE2:
//                     n[(*nl)++] = 0xbf;
//                     break;
//                 case I0_REG_FILE3:
//                     n[(*nl)++] = 0xbe;
//                     break;
//                 case I0_REG_FILE4:
//                     n[(*nl)++] = 0xbd;
//                     break;
//                 case I0_REG_FILE5:
//                     n[(*nl)++] = 0xba;
//                     break;
//                 case I0_REG_FILE6:
//                     n[(*nl)++] = 0xb9;
//                     break;
//                 case I0_REG_FILE7:
//                     n[(*nl)++] = 0xb8;
//                     break;
//                 default: 
//                     error("translate2x86_64_mov imm -> reg: not implemented mov oprreg.\n");
//             }
// 
//             *((uint64_t*)(n + (*nl))) = i->opr1;
//             *nl += SIZE_E;
// 
//         }
//         else {
//             *nl += 2 + SIZE_E;
//         }
// 
//         // return
//         return 1;
//     }
// 
//     // reg to reg
//     if ( 0 &&  (i->mattr1 == MATTR_SE || i->mattr2 == MATTR_SE) && 
//             (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
//             (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) )  ) {
//         // Two register operation
//         // determine oprreg for the opr
//         // tricky here: just map the REG_FILEX
//         oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;
//         oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
// 
//         // reg combination
//         oprregcomb = (oprreg1 << 16) + oprreg2;
// 
//         if (is_write_n) {
//             switch (oprregcomb) {
//                 case (I0_REG_FILE0 << 16) + I0_REG_FILE1:
//                     /* add %rbx to %rcx */
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx add %%rbx, %%rcx\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0x48; 
//                     n[(*nl)++] = 0x01; 
//                     n[(*nl)++] = 0xd9; 
//                     break;
//                 default: error("translate2x86_64_mov: not implemented add oprregcomb.\n");
//             }
// 
//         }
//         else {
//             *nl += 3;
//         }
// 
//         // return
//         return 1;
//     }
// 
// 
//     /*
//     // immediate -> (oprreg)
//     if ( 0 &&  (i->mattr1 == MATTR_SE || i->mattr2 == MATTR_SE) && 
//             (i->addrm1 == ADDRM_IMMEDIATE ) &&
//             (i->addrm2 == ADDRM_INDIRECT && is_in_reg_file(i->opr2) )  ) {
//         // determine oprreg for the opr
//         // tricky here: just map the REG_FILEX
//         oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
// 
//         // TODO other regs
//         
//         if (is_write_n) {
// 
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             zlogf("native instr: @%#lx movq $%#lx, (%%rcx)\n", n+(*nl), i->opr1 );
// #endif
//             if (oprreg2 == I0_REG_FILE1 || oprreg2 == I0_REG_FILE2 ) {
//                 // %rbx and %rcx are different from others
//                 n[(*nl)++] = 0x48;
//             }
//             else {
//                 n[(*nl)++] = 0x49;
//             }
// 
//             switch (oprreg2) {
//                 case I0_REG_FILE1:
//                     n[(*nl)++] = 0x48; 
//                     n[(*nl)++] = 0xd9;
//                     break;
//                 default: 
//                     error("translate2x86_64_mov imm -> reg: not implemented mov oprreg.\n");
//             }
// 
//             *((uint64_t*)(n + (*nl))) = i->opr1;
//             *nl += SIZE_E;
// 
//         }
//         else {
//             *nl += 2 + SIZE_E;
//         }
// 
//         // return
//         return 0;
// 
//     }
//     */
// 

    return 0;
}

int translate2x86_64_mov(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    int modes = 0;

    // optimization opportunities
    if (opt_translate2x86_64_mov(i, is_write_n, n, nl, t, tl)) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("mov: optimized.\n");
#endif
        return 0;
    }

    // // default one: fall back

    modes = ((i->mattr1) << 16) + (i->mattr2);

    switch (modes) {
        case (MATTR_SE << 16) + MATTR_SE:
        case (MATTR_SE << 16) + MATTR_UE:
        case (MATTR_UE << 16) + MATTR_SE:
        case (MATTR_UE << 16) + MATTR_UE:
        case (MATTR_FD << 16) + MATTR_FD:
            // read in opr1 to %rdi
            // READ(D1, %rdi)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
            // write %rdi to opr2
            // WRITE(%rdi, M)
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RDI, n, nl, is_write_n);
            break;
        case (MATTR_SE << 16) + MATTR_FD:
            // read in opr1 to %rax
            // READ(D1, %rax)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // convert
            if (is_write_n) {
                // cvtsi2sdq   %rax, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cvtsi2sdq %%rax, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0xf2;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x2a;
                n[(*nl)++] = 0xc0;

                // movq %xmm0, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%xmm0, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x7e;
                n[(*nl)++] = 0xc0;
            } else {
                *nl += 10;
            }

            // write %rax to opr2
            // WRITE(%rax, M)
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);

            break;
        case (MATTR_FD << 16) + MATTR_SE:
            // read in opr1 to %rax
            // READ(D1, %rax)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // convert
            if (is_write_n) {
                // movq %rax, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rax, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xc0;
                // cvttsd2siq  %xmm0, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cvtsd2siq %%xmm0, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0xf2;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x2c;
                n[(*nl)++] = 0xc0;
            } else {
                *nl += 10;
            }

            // write %rax to opr2
            // WRITE(%rax, M)
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);

            break;

        case (MATTR_FS << 16) + MATTR_SE:
        case (MATTR_SE << 16) + MATTR_FS:
            error("translate2x86_64_mov: undefined/unsupported/unimplemented mattr combination. F<->SQ\n");
            break;

        case (MATTR_SF << 16) + MATTR_SE:
        case (MATTR_SF << 16) + MATTR_UE:
            // signed extension
            // read opr1 to %eax
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // signed extend %eax to %rax
            if (is_write_n) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cltq\n", n+(*nl));
#endif
                // cltq
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x98;
            }
            else {
                *nl += 2;
            }

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);

            break;
        case (MATTR_UF << 16) + MATTR_SE:
        case (MATTR_UF << 16) + MATTR_UE:
            // zero extension
            // read opr1 to %eax
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
           
            break;
        case (MATTR_SE << 16) + MATTR_SF:
        case (MATTR_SE << 16) + MATTR_UF:
        case (MATTR_UE << 16) + MATTR_SF:
        case (MATTR_UE << 16) + MATTR_UF:
            // read opr1 to %rax
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // write %eax to opr2
            _translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n, i->mattr2);
 
            break;
            /*
        case (MATTR_UB << 16) + MATTR_SE:
        case (MATTR_UB << 16) + MATTR_UE:
            // zero extension
            //
            // read opr1 to %eax
            // _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, 0, n, nl, is_write_n, i->mattr1);

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
            */
        case (MATTR_SE << 16) + MATTR_SB:
        case (MATTR_SE << 16) + MATTR_UB:
        case (MATTR_UE << 16) + MATTR_SB:
        case (MATTR_UE << 16) + MATTR_UB:
            // read opr1 to %al
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // write %al to opr2
            _translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n, i->mattr2);
 
            break;
        case (MATTR_SB << 16) + MATTR_SE:
        // case (MATTR_SB << 16) + MATTR_UE:
            // signed extension
            //
            // read opr1 to %al
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // signed extend %al to %rax
            if (is_write_n) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movsbq %%al, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0xbe;
                n[(*nl)++] = 0xc0;
            }
            else {
                *nl += 4;
            }

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
            break;
        case (MATTR_UB << 16) + MATTR_SB:
        case (MATTR_UB << 16) + MATTR_UB:
        case (MATTR_SB << 16) + MATTR_SB:
        case (MATTR_SB << 16) + MATTR_UB:
            // read opr1 to %al
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // write %al to opr2
            // translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
            _translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n, i->mattr2);
            break;
        default:
            error("translate2x86_64_mov: default: undefined/unsupported/unimplemented mattr combination.\n");
    }
    
    // TODO support more

    return 0;
}

int translate2x86_64_int(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    if (is_write_n) {
        
        // movq $IDT_BASE, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq $IDT_BASE, %%rdx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xba;
        *((uint64_t*)(n + (*nl))) = IDT_BASE;
        *nl += SIZE_E;
 
        // movq $opr1, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq $%#lx, %%rax\n", n+(*nl), (i->opr1)*8);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xb8;
        *((uint64_t*)(n + (*nl))) = (i->opr1)*8;
        *nl += SIZE_E;

        // addq %rax, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx addq %%rax, %%rdx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x01;
        n[(*nl)++] = 0xc2;
        
        // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x8d;
        n[(*nl)++] = 0x05;
        n[(*nl)++] = 0x00;
        n[(*nl)++] = 0x00;
        n[(*nl)++] = 0x00;
        n[(*nl)++] = 0x00;
        
        // pushq %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x50;

        // movq (%rdx), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq (%%rdx), %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x8b;
        n[(*nl)++] = 0x02;

        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0xff;
        n[(*nl)++] = 0xe0;
    } else {
        *nl += 2 + SIZE_E + 2 + SIZE_E + 3 + 7 + 1 + 3 + 2;
    }

    return 0;
}


int translate2x86_64_exit(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    if (is_write_n) {
        // movq $option %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq $%#lx, %%rdi\n", n+(*nl), i->option);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xbf;
        *((uint64_t*)(n + (*nl))) = i->option;
        *nl += SIZE_E;
        // movq back_runner_wrapper, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq $back_runner_wrapper, %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xa1;
        *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_BACK_RUNNER_WRAPPER);
        *nl += SIZE_E;
        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0xff;
        n[(*nl)++] = 0xe0;
    } else {
        *nl += 2 + SIZE_E + 2 + SIZE_E + 2;
    }

    return 0;
}

int translate2x86_64_div(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // if (i->attr == ATTR_FD) {
    //     error("translate2x86_64_div: ATTR_FD not implemented yet.\n");
    // }

    if (i->attr == ATTR_FS) {
        error("translate2x86_64_div: ATTR_FS not implemented yet.\n");
    }

    // READ(D2, %rsi)
    translate2x86_64_read(i->opr2, i->disp2, i->addrm2, X86_64_R_RSI, n, nl, is_write_n);
    // READ(D1, %rax)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

    if ( i->attr == ATTR_SE )
    {
        if (is_write_n) {

            // movq %rax, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%rax, %%rdx\n", n+(*nl));
#endif
            n[(*nl)++] = 0x48;
            n[(*nl)++] = 0x89;
            n[(*nl)++] = 0xc2;

            // sarq $63, %rdx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx sarq $63, %%rdx\n", n+(*nl));
#endif
            n[(*nl)++] = 0x48;
            n[(*nl)++] = 0xc1;
            n[(*nl)++] = 0xfa;
            n[(*nl)++] = 0x3f;

            // idivq %rsi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx idivq %%rsi\n", n+(*nl));
#endif
            n[(*nl)++] = 0x48;
            n[(*nl)++] = 0xf7;
            n[(*nl)++] = 0xfe;
        }
        else {
            *nl += 3 + 4 + 3;
        }
    }
    else if (i->attr == ATTR_UE) {
        if (is_write_n) {
            // movl $0, %edx
            n[(*nl)++] = 0xba;
            n[(*nl)++] = 0x00;
            n[(*nl)++] = 0x00;
            n[(*nl)++] = 0x00;
            n[(*nl)++] = 0x00;

            // divq %rsi
            n[(*nl)++] = 0x48;
            n[(*nl)++] = 0xf7;
            n[(*nl)++] = 0xf6;
        }
        else {
            *nl += 5 + 3;
        }
    }
    else if (i->attr == ATTR_FD) {
        // TODO later: working here
        if (is_write_n) {
            //  movsd %rax, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%rax, %%xmm1\n", n+(*nl));
#endif
            n[(*nl)++] = 0x66;
            n[(*nl)++] = 0x48;
            n[(*nl)++] = 0x0f;
            n[(*nl)++] = 0x6e;
            n[(*nl)++] = 0xc8;

            //  movsd %rsi, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%rsi, %%xmm0\n", n+(*nl));
#endif
            n[(*nl)++] = 0x66;
            n[(*nl)++] = 0x48;
            n[(*nl)++] = 0x0f;
            n[(*nl)++] = 0x6e;
            n[(*nl)++] = 0xc6;

            //  divsd %xmm0, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx divsd %%xmm0, %%xmm1\n", n+(*nl));
#endif
            n[(*nl)++] = 0xf2;
            n[(*nl)++] = 0x0f;
            n[(*nl)++] = 0x5e;
            n[(*nl)++] = 0xc8;

            //  movq %xmm1, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%xmm1, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x7e;
                n[(*nl)++] = 0xc8;
        }
        else {
            *nl += 5 + 5 + 4 + 5;
        }
    }
    else {
        error("Wrong attr for translate2x86_64_div.\n");
    }

    // WRITE(%rax, M)
    translate2x86_64_write(i->opr3, i->disp3, i->addrm3, X86_64_R_RAX, n, nl, is_write_n);

    return 0;
}

// leaq(reg1, reg2) -> reg2
int x86_64_encode_leaq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        reg2_en = x86_64_reg_encoding(reg2);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg2_en & 0x8) >> 1; // R
        c += (reg2_en & 0x8) >> 2; // X
        c += (reg1_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // Opcode
        n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // ModRM
        c = 0x00; // mod
        c += (reg2_en & 0x7) << 3; // rrr
        c += 0x04; // r/m
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
        c = 0x00; // ss 1 times
        c += (reg2_en & 0x7) << 3; // xxx
        c += (reg1_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
    } else {
        *nl += 4;
    }
    return 0;

    return 0;
}

// reg1 OP reg2 -> reg2
int x86_64_encode_addq_subq_rr(int is_add, int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        reg2_en = x86_64_reg_encoding(reg2);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg1_en & 0x8) >> 1; // R
        c += (reg2_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // Opcode
        if (is_add) {
            n[(*nl)++] = 0x01;
        } else {
            // sub
            n[(*nl)++] = 0x29;
        }
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // ModRM
        c = 0xc0; // mod
        c += (reg1_en & 0x7) << 3; // rrr
        c += (reg2_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
    } else {
        *nl += 3;
    }
    return 0;
}

inline int x86_64_encode_subq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx sub %%oprreg %d, %%oprreg %d\n", n+(*nl), reg1, reg2);
    }
#endif
    return x86_64_encode_addq_subq_rr(0, is_write_n, n, nl, reg1, reg2);
}

inline int x86_64_encode_addq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
#ifdef BT_OPT_ADDQ_LEAQ
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx leaq(%%oprreg %d, %%oprreg %d), %%oprreg %d\n", n+(*nl), reg1, reg2, reg2);
    }
#endif
    return x86_64_encode_leaq_rr(is_write_n, n, nl, reg1, reg2);
#else
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx add %%oprreg %d, %%oprreg %d\n", n+(*nl), reg1, reg2);
    }
#endif
    return x86_64_encode_addq_subq_rr(1, is_write_n, n, nl, reg1, reg2);
#endif
}

// leaq imm(reg1) -> reg1
int x86_64_encode_leaq_ir(int is_write_n, char *n, uint64_t *nl, uint32_t imm, int reg1)
{
    // error("Not implemented yet.\n");
    int reg1_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg1_en & 0x8) >> 1; // R
        // X = 0
        c += (reg1_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // Opcode
        n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // ModRM
        c = 0x80; // mod
        c += (reg1_en & 0x7) << 3; // rrr
        c += (reg1_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
        }
#endif
        // imm
        *((int32_t*)(n + (*nl))) = (int32_t)imm;
        *nl += SIZE_F;
    } else {
        *nl += 3 + SIZE_F;
    }
    return 0;
}

inline int x86_64_encode_addq_ir(int is_write_n, char *n, uint64_t *nl, uint32_t imm, int reg1)
{
#ifdef BT_OPT_ADDQ_LEAQ

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx leaq(%%oprreg %d, %#x), %%oprreg %d\n", n+(*nl), reg1, imm, reg1);
    }
#endif
    return x86_64_encode_leaq_ir(is_write_n, n, nl, imm, reg1);

#else
    error("BT_OPT_ADDQ_LEAQ should always be defined.\n");
#endif
}

inline int x86_64_encode_subq_ir(int is_write_n, char *n, uint64_t *nl, uint32_t imm, int reg1)
{
    // error("Not implemented yet.\n");

    int reg1_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        // R = 0
        // X = 0
        c += (reg1_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // opcode
        // sub
        n[(*nl)++] = 0x81;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // ModRM
        c = 0xc0; // mod
        c += (0x5) << 3; // rrr
        c += (reg1_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
        }
#endif
        // imm
        *((int32_t*)(n + (*nl))) = (int32_t)imm;
        *nl += SIZE_F;
    } else {
        *nl += 3 + SIZE_F;
    }
    return 0;
}

inline int x86_64_opr_equal(x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{
    if (opr1 == opr2) return 1; // true if they are the same
    if (opr1->class0 != opr2->class0) return 0;
    if (opr1->class1 != opr2->class1) return 0;
    if (opr1->addrm != opr2->addrm) return 0;
    if (opr1->opr != opr2->opr) return 0;
    if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
        if (opr1->disp != opr2->disp) {
            return 0;
        }
    }
    return 1;
}

int x86_64_encode_movq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{

    int reg1_en = 0;
    int reg2_en = 0;
    x86_64_opr_t *tmp;
    char c;
    char opcode = 0;

#ifdef _DEBUG_MOVQ_
    zlogf("opr1: \n");
    print_x86_64_opr(opr1);
    zlogf("opr2: \n");
    print_x86_64_opr(opr2);
#endif

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx movq A, B\n", n+(*nl));
    }
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("with $imm -- ");
#endif
        if (is_write_n) {
            reg2_en = 0;
            reg1_en = x86_64_reg_encoding(opr2->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            // R 0
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0xc7;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr2->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 
            // rrr 000
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            // disp
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr2->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }
            // imm
            *((uint32_t*)(n + (*nl))) = *(uint32_t*)(&(opr1->opr));
            *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%x \n", n[(*nl)-4]);
#endif
        } else {
            *nl += 3 + 4;
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }

    } else {
        if (opr1->addrm == X86_64_ADDRM_REG) {
            opcode = 0x89;
            // now exchange reg1/reg2
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("exchange it -- ");
#endif
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        } else if (opr2->addrm == X86_64_ADDRM_REG) {
            opcode = 0x8b;
        } else {
            error("Should not reach here 5629.\n");
        }

        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr); // B
            reg2_en = x86_64_reg_encoding(opr2->opr); // R
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg2_en & 0x8) >> 1; // R
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = opcode;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr1->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 

            c += (reg2_en & 0x7) << 3; // rrr
            c += (reg1_en & 0x7); // r/m
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif 
            // disp
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr1->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }

        } else {
            *nl += 3;
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }
    }

    return 0;
}

int x86_64_encode_leaq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2, x86_64_opr_t *opr3)
{
    int reg1_en = 0;
    int reg2_en = 0;
    int reg3_en = 0;
    char c;
    x86_64_opr_t imm;

    x86_64_opr_t *tmp = NULL;
    
    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL && opr2->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
        // error("leaq: imm, imm: should not reach here.\n");
        // calculate it and mov to destination
        imm.opr = opr1->opr + opr2->opr;
        if (within_32bit(imm.opr)) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("add: optimized with constant porpogation. 5801 \n");
#endif
            imm.class0 = opr1->class0;
            imm.class1 = opr1->class1;
            imm.addrm = opr1->addrm;
            // mov
            x86_64_encode_movq(is_write_n, n, nl, &imm, opr3);
            return 0;
        } else {
            error("leaq: imm, imm: should not reach here. 5803\n");
        }
    }

    if (opr3->addrm != X86_64_ADDRM_REG) {
        error("leaq: 3rd oprand should be a register.\n");
    }

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx leaq A, B, C\n", n+(*nl));
    }
#endif
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("add: optimized with leaq \n");
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL || opr2->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("with $imm -- ");
#endif
        if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        }
        // opr1: reg, opr2: imm, opr3: reg
        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr);
            reg3_en = x86_64_reg_encoding(opr3->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg3_en & 0x8) >> 1; // R
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            c = 0x80; // mod
            c += (reg3_en & 0x7) << 3; // rrr
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            *((uint32_t*)(n + (*nl))) = *(uint32_t*)(&(opr2->opr));
            *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%x \n", n[(*nl)-4]);
#endif
        } else {
            *nl += 3 + 4;
        }

    } else {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("(reg, reg) -> reg -- ");
#endif
        // opr1: reg, opr2: reg, opr3: reg
        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr);
            reg2_en = x86_64_reg_encoding(opr2->opr);
            reg3_en = x86_64_reg_encoding(opr3->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg3_en & 0x8) >> 1; // R
            c += (reg2_en & 0x8) >> 2; // X
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            if (opr1->opr == I0_REG_FILE4) {
                // %r13 must have displacement of 0 with mod 0
                c = 0x40; // mod
            } else {
                c = 0x00; // mod
            }
            c += (reg3_en & 0x7) << 3; // rrr
            c += 0x04; // r/m
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            c = 0x00; // ss 1 times
            c += (reg2_en & 0x7) << 3; // xxx
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            if (opr1->opr == I0_REG_FILE4) {
                // %r13 must have displacement of 0
                n[(*nl)++] = 0;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            }
        } else {
            *nl += 4;
            if (opr1->opr == I0_REG_FILE4) {
                // %r13 must have displacement of 0
                *nl += 1;
            }

        }
    }

    return 0;
}

int x86_64_encode_addq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    x86_64_opr_t *tmp;
    char c;
    char opcode = 0;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx addq A, B\n", n+(*nl));
    }
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
        if (is_write_n) {
            reg2_en = 0;
            reg1_en = x86_64_reg_encoding(opr2->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            // R 0
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0x81;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr2->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 
            // rrr 000
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            // disp
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr2->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }
            // imm
            *((uint32_t*)(n + (*nl))) = *(uint32_t*)(&(opr1->opr));
            *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%x \n", n[(*nl)-4]);
#endif
        } else {
            *nl += 3 + 4;
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }

    } else {
        if (opr1->addrm == X86_64_ADDRM_REG) {
            opcode = 0x01;
            // now exchange reg1/reg2
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        } else if (opr2->addrm == X86_64_ADDRM_REG) {
            opcode = 0x03;
        } else {
            error("Should not reach here 5938.\n");
        }

        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr); // B
            reg2_en = x86_64_reg_encoding(opr2->opr); // R
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg2_en & 0x8) >> 1; // R
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = opcode;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr1->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 

            c += (reg2_en & 0x7) << 3; // rrr
            c += (reg1_en & 0x7); // r/m
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif 
            // disp
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr1->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }

        } else {
            *nl += 3;
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }
    }

    return 0;
}

int x86_64_encode_subq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    x86_64_opr_t *tmp;
    char c;
    char opcode = 0;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx subq A, B\n", n+(*nl));
    }
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
        if (is_write_n) {
            reg2_en = 0;
            reg1_en = x86_64_reg_encoding(opr2->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            // R 0
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0x81;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr2->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 
            c += (5 & 0x7) << 3; // rrr 5
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            // disp
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr2->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }
            // imm
            *((uint32_t*)(n + (*nl))) = *(uint32_t*)(&(opr1->opr));
            *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%x \n", n[(*nl)-4]);
#endif
        } else {
            *nl += 3 + 4;
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }

    } else {
        if (opr1->addrm == X86_64_ADDRM_REG) {
            opcode = 0x29;
            // now exchange reg1/reg2
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        } else if (opr2->addrm == X86_64_ADDRM_REG) {
            opcode = 0x2b;
        } else {
            error("Should not reach here 5938.\n");
        }

        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr); // B
            reg2_en = x86_64_reg_encoding(opr2->opr); // R
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg2_en & 0x8) >> 1; // R
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = opcode;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr1->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 

            c += (reg2_en & 0x7) << 3; // rrr
            c += (reg1_en & 0x7); // r/m
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif 
            // disp
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr1->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }

        } else {
            *nl += 3;
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }
    }

    return 0;
}

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_sub(
        x86_64_opr_t *oprs, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t *r = NULL;
    x86_64_opr_t *A = NULL;
    x86_64_opr_t *B = NULL;
    x86_64_opr_t *C = NULL;
    x86_64_opr_t opr_rdi;

    // %rdi
    opr_rdi.class0 = X86_64_CLASS0_NO_MEM;
    opr_rdi.class1 = X86_64_CLASS1_REG;
    opr_rdi.opr = X86_64_R_RDI;
    opr_rdi.addrm = X86_64_ADDRM_REG;

    A = oprs;
    B = oprs + 1;
    C = oprs + 2;

    // sub:sq A, B, C

    // C in RF && C is not in B
    if (C->addrm == X86_64_ADDRM_REG && C->opr != B->opr) {
        r = C;
    } else {
        r = &opr_rdi;
    }

    // A->r if A != r
    if (! x86_64_opr_equal(A, r)) {
        x86_64_encode_movq(is_write_n, n, nl, A, r);
    }

    // subq B, r
    x86_64_encode_subq(is_write_n, n, nl, B, r);

    // r->C if C != r
    if (! x86_64_opr_equal(C, r)) {
        x86_64_encode_movq(is_write_n, n, nl, r, C);
    }

    return 1;
}

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_add(
        x86_64_opr_t *oprs, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t *opr1 = NULL;
    x86_64_opr_t *opr2 = NULL;
    x86_64_opr_t *opr3 = NULL;
    x86_64_opr_t *tmp;
    x86_64_opr_t opr_rdi;
    uint32_t class0_comb = 0;

    // %rdi
    opr_rdi.class0 = X86_64_CLASS0_NO_MEM;
    opr_rdi.class1 = X86_64_CLASS1_REG;
    opr_rdi.opr = X86_64_R_RDI;
    opr_rdi.addrm = X86_64_ADDRM_REG;

    class0_comb = (oprs[0].class0 << 16) + (oprs[1].class0 << 8) + (oprs[2].class0);

    opr1 = oprs;
    opr2 = oprs + 1;
    opr3 = oprs + 2;

    switch(class0_comb) {
        case (X86_64_CLASS0_MEM << 16) + (X86_64_CLASS0_MEM << 8) + (X86_64_CLASS0_MEM):
            // return 0; // debug
            x86_64_encode_movq(is_write_n, n, nl, opr1, &opr_rdi);
            x86_64_encode_addq(is_write_n, n, nl, opr2, &opr_rdi);
            x86_64_encode_movq(is_write_n, n, nl, &opr_rdi, opr3);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_MEM << 8) + (X86_64_CLASS0_MEM):
            // conduct to next situation
            opr1 = oprs + 1;
            opr2 = oprs;
        case (X86_64_CLASS0_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_MEM):
            if (x86_64_opr_equal(opr1, opr3)) {
                // return 0; // debug
                x86_64_encode_addq(is_write_n, n, nl, opr2, opr3);
            } else {
                // return 0; // debug
                x86_64_encode_movq(is_write_n, n, nl, opr1, &opr_rdi);
                x86_64_encode_leaq(is_write_n, n, nl, &opr_rdi, opr2, &opr_rdi);
                x86_64_encode_movq(is_write_n, n, nl, &opr_rdi, opr3);
            }
            return 1;
            break;
        case (X86_64_CLASS0_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_NO_MEM):
            // conduct to next situation
            opr1 = oprs + 1;
            opr2 = oprs;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_MEM << 8) + (X86_64_CLASS0_NO_MEM):
            // no reg (3rd oprand) in mem since we will change the reg
            if (opr3->opr == opr2->opr) {
                return 0;
            }
            // return 0; // debug
            x86_64_encode_movq(is_write_n, n, nl, opr1, opr3);
            x86_64_encode_addq(is_write_n, n, nl, opr2, opr3);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_MEM):
            // return 0; // debug
            x86_64_encode_leaq(is_write_n, n, nl, opr1, opr2, &opr_rdi);
            x86_64_encode_movq(is_write_n, n, nl, &opr_rdi, opr3);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_NO_MEM):
            // return 0; // debug
            x86_64_encode_leaq(is_write_n, n, nl, opr1, opr2, opr3);
            return 1;
            break;
        default:
            error("Should not reach here 5751\n");
            break;
    }

    return 0;
}

// optimized: return 1
// otherwise return 0
// only signed is supported
int opt_translate2x86_64_mul(
        instr_t *i,
        x86_64_opr_t *oprs, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // return 0;

    x86_64_opr_t *r = NULL;
    x86_64_opr_t *A = NULL;
    x86_64_opr_t *B = NULL;
    x86_64_opr_t *C = NULL;
    x86_64_opr_t opr_rdi;

    instr_t tmpi;

    A = oprs;
    B = oprs + 1;
    C = oprs + 2;

    // make a copy
    tmpi = *i;

    // take care mul A, imm, C only
    if (B->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
        // use shift
        switch (B->opr) {
            case 1:
                // tested okay
                // return 0;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("Optimized mul with mov.\n");
#endif
                // mov A->C
                tmpi.opcode = OP_CONV;
                tmpi.mattr1 = tmpi.attr;
                tmpi.mattr2 = tmpi.attr;
                tmpi.addrm2 = tmpi.addrm3;
                tmpi.opr2 = tmpi.opr3;
                tmpi.disp2 = tmpi.disp3;
                translate2x86_64_mov(&tmpi, is_write_n, n, nl, t, tl);
                return 1;
            case 2:
                tmpi.opr2 = 1;
                goto case_2_4_8_goto;
            case 4:
                tmpi.opr2 = 2;
                goto case_2_4_8_goto;
            case 8:
                tmpi.opr2 = 3;
case_2_4_8_goto:
                // return 0;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("Optimized mul with shift. %d\n", tmpi.opr2);
#endif
                tmpi.opcode = OP_SHIFT;
                tmpi.option = OPT_SHIFT_L;
                tmpi.addrm2 = ADDRM_IMMEDIATE;
                translate2x86_64_shift(&tmpi, is_write_n, n, nl, t, tl);
                return 1;
            default:
                return 0;
        }
    }

    return 0;
}

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_alu_op(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t oprs[4];

    // optimize SQ and UQ only
    if ( ! (i->attr == ATTR_SE || i->attr == ATTR_UE) ) {
        return 0;
    }
    // optimize ADD and SUB only
    if ( ! (i->opcode == OP_ADD || i->opcode == OP_SUB || i->opcode == OP_MUL) ) {
        return 0;
    }

    instr2x86_64_oprs(i, oprs, 3);

    if (
            oprs[0].class0 == X86_64_CLASS0_NOT_SUPPORTED ||
            oprs[1].class0 == X86_64_CLASS0_NOT_SUPPORTED ||
            oprs[2].class0 == X86_64_CLASS0_NOT_SUPPORTED
       ) {
        return 0;
    }

    switch (i->opcode) {
        case OP_ADD:
            return opt_translate2x86_64_add(oprs, is_write_n, n, nl, t, tl);
            break;
        case OP_SUB:
            return opt_translate2x86_64_sub(oprs, is_write_n, n, nl, t, tl);
            break;
        case OP_MUL:
            if ( i->attr == ATTR_SE ) {
                // optimize signed multiply only currently
                return opt_translate2x86_64_mul(i, oprs, is_write_n, n, nl, t, tl);
            } else {
                return 0;
            }
            break;
        default:
            return 0;
            break;
    }

    // old ones:
    //
    // int oprreg1 = -1;
    // int oprreg2 = -1;
    // int oprregcomb = 0;

    // // if there is optimization opportunity
    // // reg1 OP reg2 -> reg1 
    // // OP: add / sub
    // if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) && 
    //         (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
    //         (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) ) &&
    //         (i->opr1 == i->opr3) &&
    //         (i->opcode == OP_ADD || i->opcode == OP_SUB) ) {
    //     // Two register operation
    //     // determine oprreg for the opr
    //     // tricky here: just map the REG_FILEX
    //     oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;
    //     oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;

    //     // reg combination
    //     // oprregcomb = (oprreg1 << 16) + oprreg2;


    //     if (i->opcode == OP_ADD) {
    //         x86_64_encode_addq_rr(is_write_n, n, nl, oprreg2, oprreg1);
    //     } else if (i->opcode == OP_SUB) {
    //         x86_64_encode_subq_rr(is_write_n, n, nl, oprreg2, oprreg1);
    //     } else {
    //         error("Should not get here. 4827");
    //     }

    //     // return
    //     return 1;
    // }
    // // reg1 OP imm -> reg1 
    // // OP: add / sub
    // else if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) && 
    //         (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
    //         ((i->addrm2 == ADDRM_IMMEDIATE) && ! (i->opr2 & 0xFFFFFFFF00000000) /* 32bit only */ ) &&
    //         (i->opr1 == i->opr3) &&
    //         (i->opcode == OP_ADD || i->opcode == OP_SUB) ) {
    //     // determine oprreg for the opr
    //     // tricky here: just map the REG_FILEX
    //     oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;

    //     if (i->opcode == OP_ADD) {
    //         // return 0; // i0ble it
    //         x86_64_encode_addq_ir(is_write_n, n, nl, (uint32_t)(i->opr2), oprreg1);
    //     } else if (i->opcode == OP_SUB) {
    //         // return 0; // i0ble it
    //         x86_64_encode_subq_ir(is_write_n, n, nl, (uint32_t)(i->opr2), oprreg1);
    //     } else {
    //         error("Should not get here. 5011");
    //     }

    //     // return
    //     return 1;
    // }

    return 0;
}

int translate2x86_64_alu_op(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{

    if (i->attr == ATTR_FS) {
        error("translate2x86_64_op: ATTR_FS not implemented yet.\n");
    }

    // optimization opportunities
    if (opt_translate2x86_64_alu_op(i, is_write_n, n, nl, t, tl)) {
        return 0;
    }

    // // default one: handle all situation

    // READ(D1, %rdi)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
    // READ(D2, %rsi)
    translate2x86_64_read(i->opr2, i->disp2, i->addrm2, X86_64_R_RSI, n, nl, is_write_n);

    if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) )
    {
        // optimization for add: use leq instead
#ifdef BT_OPT_ADDQ_LEAQ
        if (i->opcode == OP_ADD) {
            if (is_write_n) {
                // leaq (%rsi, %rdi), %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx leaq (%%rsi, %%rdi), %%rdi\n", n+(*nl));
#endif

                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x8d;
                n[(*nl)++] = 0x3c;
                n[(*nl)++] = 0x3e;
            } else {
                *nl += 4;
            }
        }
        else {
#endif
        // others
        // default for add

            if (is_write_n) {
                // op (integer) %rsi, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx op (integer) %%rsi, %%rdi\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                if (i->opcode == OP_MUL) {
                    n[(*nl)++] = 0x0f;
                    n[(*nl)++] = 0xaf;
                    n[(*nl)++] = 0xfe;
                } else {
                    switch(i->opcode) {
                        case OP_ADD:
                            n[(*nl)++] = 0x01;
                            break;
                        case OP_SUB:
                            n[(*nl)++] = 0x29;
                            break;
                        case OP_AND:
                            n[(*nl)++] = 0x21;
                            break;
                        case OP_OR:
                            n[(*nl)++] = 0x09;
                            break;
                        case OP_XOR:
                            n[(*nl)++] = 0x31;
                            break;
                        default:
                            ; // should never reach here
                    }

                    n[(*nl)++] = 0xf7;
                }
            } else {
                if (i->opcode == OP_MUL) {
                    *nl += 4;
                } else {
                    *nl += 3;
                }
            }
#ifdef BT_OPT_ADDQ_LEAQ
        }
#endif
    }
    else if (i->attr == ATTR_FD) {
        if (is_write_n) {

            if (i->opcode == OP_SUB) {
                //  movq %rdi, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rdi, %%xmm1\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xcf;

                //  movq %rsi, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rsi, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xc6;
                
            }
            else {
                //  movq %rdi, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rdi, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xc7;
                
                //  movq %rsi, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rsi, %%xmm1\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xce;
            }

            //  op %xmm0, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx op () %%xmm0, %%xmm1\n", n+(*nl));
#endif
                n[(*nl)++] = 0xf2;
                n[(*nl)++] = 0x0f;

            switch(i->opcode) {
                case OP_ADD:
                    n[(*nl)++] = 0x58;
                    break;
                case OP_MUL:
                    n[(*nl)++] = 0x59;
                    break;
                case OP_SUB:
                    n[(*nl)++] = 0x5c;
                    break;
                default:
                    // should never reach here
                    error("translate2x86_64_op: not supported opcode for D.\n");
            }
            n[(*nl)++] = 0xc8;

            //  movq %xmm1, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%xmm1, %%rdi\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x7e;
                n[(*nl)++] = 0xcf;

        }
        else {
            *nl += 5 + 5 + 4 + 5;
        }
    }

    // WRITE(%rdi, M)
    translate2x86_64_write(i->opr3, i->disp3, i->addrm3, X86_64_R_RDI, n, nl, is_write_n);

    return 0;
}

// Note: only the least significant 8 bits of opr2 are used.
int translate2x86_64_shift(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // check addrm2
    if (i->addrm2 != ADDRM_IMMEDIATE) {
        error("translate2x86_64_op: SHIFT only support addrm2 == ADDRM_IMMEDIATE.\n");
    }
   
    // READ(D1, %rdi)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);

    if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) )
    {
        if (is_write_n) {
            // shift (l/r) $opr2, %rdi
            if (i->option == OPT_SHIFT_L) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx shl $%#lx, %%rdi\n", n+(*nl), i->opr2 & 0xff);
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0xc1;
                n[(*nl)++] = 0xe7;
                n[(*nl)++] = (char)(i->opr2 & 0xff);
            }
            else if (i->option == OPT_SHIFT_R) {
                if (i->attr == ATTR_UE) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx shr $%#lx, %%rdi\n", n+(*nl), i->opr2 & 0xff);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xc1;
                    n[(*nl)++] = 0xef;
                    n[(*nl)++] = (char)(i->opr2 & 0xff);
                }
                else {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx sar $%#lx, %%rdi\n", n+(*nl), i->opr2 & 0xff);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xc1;
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = (char)(i->opr2 & 0xff);
                }
            }
            else {
                error("translate2x86_64_op: SHIFT not supported option.\n");
            }
        } else {
            *nl += 4;
        }
    }
    else {
        // error 
        error("translate2x86_64_op: SHIFT only support SQ or UQ.\n");
    }

    // WRITE(%rdi, M)
    translate2x86_64_write(i->opr3, i->disp3, i->addrm3, X86_64_R_RDI, n, nl, is_write_n);

    return 0;
}


int translate2x86_64_spawn(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // READ(M1, %rdi)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
    // READ(M2, %rsi)
    translate2x86_64_read(i->opr2, i->disp2, i->addrm2, X86_64_R_RSI, n, nl, is_write_n);
    // READ(M3, %rdx)
    translate2x86_64_read(i->opr3, i->disp3, i->addrm3, X86_64_R_RDX, n, nl, is_write_n);
    // READ(M4, %rax)
    translate2x86_64_read(i->opr4, i->disp4, i->addrm4, X86_64_R_RAX, n, nl, is_write_n);
    if (is_write_n) {
        // save %rcx as it is in register file
        // pushq %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x51;

        // movq %rax, %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%rax, %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x89;
        n[(*nl)++] = 0xc1;
        
        // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x8d;
        n[(*nl)++] = 0x05;
        n[(*nl)++] = 0x00;
        n[(*nl)++] = 0x00;
        n[(*nl)++] = 0x00;
        n[(*nl)++] = 0x00;
        
        // pushq %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x50;
       
        // movq sys_new_runner, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq sys_new_runner, %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xa1;
        *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_NEW_RUNNER);
        *nl += SIZE_E;

        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0xff;
        n[(*nl)++] = 0xe0;

        // Note: %rax is poped by sys_new_runner
        //
        // pop %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x59;

    } else {
        *nl += 1 + 3 + 7 + 1 + 2 + SIZE_E + 2 + 1;
    }

    return 0;
}

int translate2x86_64_strcmp(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t oprs[5];
    x86_64_opr_t reg;

    new_instr2x86_64_oprs(i, oprs, 5);

#ifdef _DEBUG_SCMP_
    int debug_i;
    for (debug_i = 0; debug_i < 5; debug_i++) {
        print_x86_64_opr(oprs+debug_i);
    }
#endif

    if (is_write_n) {

        // save %rcx as it is in register file
        // pushq %rcx
        // pushq %r10
        // pushq %r9
        // pushq %r8
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x51;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%r10\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x52;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%r9\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x51;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%r8\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x50;
    } else {
        *nl += 1 + 2 + 2 + 2;
    }

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr:     to %%rdi, %%rsi, %%rdx, %%rcx\n", n+(*nl));
#endif

    reg.class0 = X86_64_CLASS0_NO_MEM;
    reg.class1 = X86_64_CLASS1_REG;
    reg.addrm = X86_64_ADDRM_REG;

    reg.opr = X86_64_R_RDI;
    x86_64_encode_movq(is_write_n, n, nl, oprs, &reg);
    reg.opr = X86_64_R_RSI;
    x86_64_encode_movq(is_write_n, n, nl, oprs+1, &reg);
    reg.opr = X86_64_R_RDX;
    x86_64_encode_movq(is_write_n, n, nl, oprs+2, &reg);
    reg.opr = X86_64_R_RCX;
    x86_64_encode_movq(is_write_n, n, nl, oprs+3, &reg);

    if (is_write_n) {
        // mov strcmp %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq $%#lx, %%rax\n", n+(*nl), _strncmp);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xb8;
        *((uint64_t*)(n + *nl)) = (uint64_t)_strncmp;
        *nl += SIZE_E;
        // callq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx callq *%%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0xff;
        n[(*nl)++] = 0xd0;
    } else {
        *nl += 2 + SIZE_E + 2;
    }

    if (is_write_n) {
        // pop %r8
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%r10\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x58;

        // pop %r9
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%r9\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x59;

        // pop %r10
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%r8\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x5a;

        // pop %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x59;

    } else {
        *nl += 2 + 2 + 2 + 1;
    }

    // result
    reg.opr = X86_64_R_RAX;
    x86_64_encode_movq(is_write_n, n, nl, &reg, oprs+4);

    // error("strcmp: not implemented yet.\n");
    return 0;
}

int translate2x86_64_(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    return 0;
}

// n: native code base
// nl: native code current len
int translate2x86_64(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    switch (i->opcode) {
        case OP_NOP:
            translate2x86_64_nop(i, is_write_n, n, nl, t, tl);
            break;
        case OP_B:
            translate2x86_64_br(i, is_write_n, n, nl, t, tl);
            break;
        case OP_CONV:
// #ifdef _CONV_V0_
//            translate2x86_64_mov_v0(i, is_write_n, n, nl, t, tl);
// #else
            translate2x86_64_mov(i, is_write_n, n, nl, t, tl);
// #endif
            break;
        case OP_INT:
            translate2x86_64_int(i, is_write_n, n, nl, t, tl);
            break;
        case OP_EXIT:
            translate2x86_64_exit(i, is_write_n, n, nl, t, tl);
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_MUL:
            translate2x86_64_alu_op(i, is_write_n, n, nl, t, tl);
            break;
        case OP_DIV:
            translate2x86_64_div(i, is_write_n, n, nl, t, tl);
            break;
        case OP_SHIFT:
            translate2x86_64_shift(i, is_write_n, n, nl, t, tl);
            break;
        case OP_SPAWN:
            translate2x86_64_spawn(i, is_write_n, n, nl, t, tl);
            break;
        case OP_SCMP:
            translate2x86_64_strcmp(i, is_write_n, n, nl, t, tl);
            break;
        case OP_UNDEFINED:
        default:
            zlogf_time("Translate: undefined/unsupported opcode: %#lx\n", i->opcode);
            warning("Translate: undefined/unsupported opcode\n");
            break;
    }

    return 0;
}

inline void byte_padding_readin(uint64_t *dl, unsigned int b)
{
    *dl += (b + 7) / 8;
}

inline void decode_readin_opr_I_b(uint64_t *opr, char *d, uint64_t *dl)
{
    *opr = (uint64_t)(*((uint8_t*)(d+ (*dl))));
    *dl += SIZE_B;
}

inline void decode_readin_opr_I(uint64_t *opr, char *d, uint64_t *dl)
{
    *opr = *((uint64_t*)(d+ (*dl)));
    *dl += SIZE_E;
}

inline void decode_readin_opr_M(uint64_t *opr, uint32_t *disp, unsigned int addrm, char *d, uint64_t *dl)
{
    switch (addrm) {
        case ADDRM_ABSOLUTE:
        case ADDRM_INDIRECT:
            *opr = *((uint64_t*)(d+ (*dl)));
            *dl += SIZE_E;
            break;
        case ADDRM_DISPLACEMENT:
            *disp = *((uint32_t*)(d+ (*dl)));
            *dl += SIZE_F;
            *opr = *((uint64_t*)(d+ (*dl)));
            *dl += SIZE_E;
            break;
        case ADDRM_IMMEDIATE: // should never appear
            error("Decode_readin_opr_M: should not use immediate addressing mode here!\n");
            break;
        default:
            error("Decode_readin_opr_M: undefined/unsupported ADDRM.\n");
    }
}

inline void _decode_readin_opr_D(uint64_t *opr, uint32_t *disp, unsigned int addrm, char *d, uint64_t *dl, unsigned int mattr)
{
#ifdef _DEBUG_MORE_DECODE_
    zlogf("decode_readin_opr_D: addrm: %d. mattr: %d\n", addrm, mattr);
#endif
    switch (addrm) {
        case ADDRM_IMMEDIATE:
            switch (mattr) {
                case MATTR_SB:
                case MATTR_UB:
                    *opr = *((uint8_t*)(d+ (*dl)));
                    *dl += SIZE_B;
                    break;
                case MATTR_SF:
                case MATTR_UF:
                case MATTR_FS:
                    *opr = *((uint32_t*)(d+ (*dl)));
                    *dl += SIZE_F;
                    break;
                default:
                    // D, Q
                    *opr = *((uint64_t*)(d+ (*dl)));
                    *dl += SIZE_E;
            }
            break;
        case ADDRM_ABSOLUTE:
        case ADDRM_INDIRECT:
            *opr = *((uint64_t*)(d+ (*dl)));
            *dl += SIZE_E;
            break;
        case ADDRM_DISPLACEMENT:
            *disp = *((uint32_t*)(d+ (*dl)));
            *dl += SIZE_F;
            *opr = *((uint64_t*)(d+ (*dl)));
            *dl += SIZE_E;
            break;
        default:
            error("Decode_readin_opr_D: undefined/unsupported ADDRM.\n");
    }
}

inline void decode_readin_opr_D(uint64_t *opr, uint32_t *disp, unsigned int addrm, char *d, uint64_t *dl)
{
    // undefined Q (64-bit) by default
    _decode_readin_opr_D(opr, disp, addrm, d, dl, MATTR_UE);
}

/*
 * lengths start from 0 and are relative
 */
int _translate_i0_code(
        char *i0_code, uint64_t i0_code_len, 
        int is_write_native_code,
        char *native_code, uint64_t *pnative_code_len, 
        char *trampoline, uint64_t *ptrampoline_len)
{
    char *d = i0_code;
    char *n = native_code;
    char *t = trampoline;

    uint64_t dl = 0;
    uint64_t prev_dl = 0;
    uint64_t tl = 0;
    uint64_t nl = 0;

    uint32_t c = 0;
    unsigned int b = 0;
    unsigned int tmp = 0;
    instr_t instr;

    long waiting_list_pos = 0;

    // if (!is_write_native_code) {
    //     error("translate_i0_code: is_write_native_code should always be true.\n");
    // }

    zlogf_time("Translate code from %#lx. is_write_native_code: %d.\n", i0_code, is_write_native_code);

    initial_instr(&instr);

    while (dl < i0_code_len) {
        prev_dl = dl;
        b = 0;

#ifdef _DEBUG_
        initial_instr(&instr);
#endif

        instr.addr = (uint64_t)(&(d[dl]));

        instr.addr_size_mode = bit2uint(d+dl, &b, BIT_LEN_ADDR_SIZE_MODE);

        // c = d[dl++];
        // tmp = c;
        c = bit2uint(d+dl, &b, BIT_LEN_OPCODE - 8);
        instr.opcode = (c << 8) | bit2uint(d+dl, &b, 8);

        // tmp & 0xFF;
#ifdef _DEBUG_MORE_
        zlogf("%#lx: Opcode: %#lx\n", instr.addr, instr.opcode);
#endif
        // decode
        switch (instr.opcode) {
            case OP_NOP:
                byte_padding_readin(&dl, b);
                break;
            case OP_CONV:
// #ifdef _CONV_V0_
//                 instr.option = bit2uint(d+dl, &b, BIT_LEN_OPT_CONV);
//                 instr.l1 = bit2uint(d+dl, &b, BIT_LEN_LEN);
//                 instr.l2 = bit2uint(d+dl, &b, BIT_LEN_LEN);
//                 instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
//                 instr.addrm2 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
//                 byte_padding_readin(&dl, b);
//                 // read in opr1
//                 switch (instr.l1) {
//                     case LEN_Q:
//                         instr.opr1 = *((uint64_t*)(d+dl));
//                         dl += SIZE_E;
//                         break;
//                     case LEN_L:
//                     case LEN_B:
//                     default:
//                         error("Undefined/unsupported CONV LEN1.\n");
//                 }
//                 // read in opr2
//                 switch (instr.l2) {
//                     case LEN_Q:
//                         instr.opr2 = *((uint64_t*)(d+dl));
//                         dl += SIZE_E;
//                         break;
//                     case LEN_L:
//                     case LEN_B:
//                     default:
//                         error("Undefined/unsupported CONV LEN2.\n");
//                 }
// #else
                instr.mattr1 = bit2uint(d+dl, &b, BIT_LEN_MATTR);
                instr.mattr2 = bit2uint(d+dl, &b, BIT_LEN_MATTR);
                instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm2 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                byte_padding_readin(&dl, b);
                // read in opr1
                _decode_readin_opr_D(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl, instr.mattr1);
                // read in opr2
                _decode_readin_opr_D(&(instr.opr2), &(instr.disp2), instr.addrm2, d, &dl, instr.mattr2);
// #endif
                break;
            case OP_B:
                instr.option = bit2uint(d+dl, &b, BIT_LEN_OPT_B);
                switch (instr.option) {
                    case OPT_B_J:
                        instr.ra = bit2uint(d+dl, &b, BIT_LEN_RA);
                        byte_padding_readin(&dl, b);
                        // absolute address
                        instr.opr1 = *((uint64_t*)(d+dl));
                        dl += SIZE_E;
                        break;
                    case OPT_B_IJ:
                        instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                        byte_padding_readin(&dl, b);
                        // M
                        decode_readin_opr_M(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl);
                        break;
                    case OPT_B_LE:
                    case OPT_B_L:
                    case OPT_B_E:
                    case OPT_B_NE:
                    case OPT_B_SL:
                        instr.attr = bit2uint(d+dl, &b, BIT_LEN_ATTR);
                        instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                        instr.addrm2 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                        instr.ra = bit2uint(d+dl, &b, BIT_LEN_RA);
                        byte_padding_readin(&dl, b);
                        if (instr.attr == ATTR_FS) {
                            error("Decode: OP:f not implemented yet.\n");
                        }
                        else {
                            // D1
                            decode_readin_opr_D(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl);
                            // D2
                            decode_readin_opr_D(&(instr.opr2), &(instr.disp2), instr.addrm2, d, &dl);
                            // I
                            decode_readin_opr_I(&(instr.opr3), d, &dl);
                        }
                        break;
                    case OPT_B_Z:
                    case OPT_B_NZ:
                        instr.attr = bit2uint(d+dl, &b, BIT_LEN_ATTR);
                        instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                        instr.ra = bit2uint(d+dl, &b, BIT_LEN_RA);
                        byte_padding_readin(&dl, b);
                        if (instr.attr == ATTR_FS) {
                            error("Decode: OP:f not implemented yet.\n");
                        }
                        else {
                            // D1
                            decode_readin_opr_D(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl);
                            // I
                            decode_readin_opr_I(&(instr.opr3), d, &dl);
                        }
                        break;
                    default:
                        error("Decode: Undefined/unsupported B OPTION.\n");
                }
                break;
            case OP_EXIT:
                instr.option = bit2uint(d+dl, &b, BIT_LEN_OPT_EXIT);
                byte_padding_readin(&dl, b);
                break;
            case OP_INT:
                // I
                byte_padding_readin(&dl, b);
                decode_readin_opr_I_b(&(instr.opr1), d, &dl);
                break;
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_AND:
            case OP_OR:
            case OP_XOR:
                instr.attr = bit2uint(d+dl, &b, BIT_LEN_ATTR);
                instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm2 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm3 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                byte_padding_readin(&dl, b);
                // D1
#ifdef _DEBUG_MORE_DECODE_
                zlogf("Decode D1\n");
#endif
                decode_readin_opr_D(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl);
                // D2
#ifdef _DEBUG_MORE_DECODE_
                zlogf("Decode D2\n");
#endif
                decode_readin_opr_D(&(instr.opr2), &(instr.disp2), instr.addrm2, d, &dl);
                // M 
#ifdef _DEBUG_MORE_DECODE_
                zlogf("Decode M\n");
#endif
                decode_readin_opr_M(&(instr.opr3), &(instr.disp3), instr.addrm3, d, &dl);
                break;

            case OP_SHIFT:
                instr.option = bit2uint(d+dl, &b, BIT_LEN_OPT_SHIFT);
                instr.attr = bit2uint(d+dl, &b, BIT_LEN_ATTR);
                instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm2 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm3 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                byte_padding_readin(&dl, b);
                // D1
#ifdef _DEBUG_MORE_DECODE_
                zlogf("Decode D1\n");
#endif
                decode_readin_opr_D(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl);
                // I
#ifdef _DEBUG_MORE_DECODE_
                zlogf("Decode D2 (I only)\n");
#endif
                decode_readin_opr_D(&(instr.opr2), &(instr.disp2), instr.addrm2, d, &dl);
                // M 
#ifdef _DEBUG_MORE_DECODE_
                zlogf("Decode M\n");
#endif
                decode_readin_opr_M(&(instr.opr3), &(instr.disp3), instr.addrm3, d, &dl);
                break;
 
            case OP_SPAWN:
                instr.addrm1 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm2 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm3 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrm4 = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                byte_padding_readin(&dl, b);
                // M1
                decode_readin_opr_M(&(instr.opr1), &(instr.disp1), instr.addrm1, d, &dl);
                // M2
                decode_readin_opr_M(&(instr.opr2), &(instr.disp2), instr.addrm2, d, &dl);
                // M3
                decode_readin_opr_M(&(instr.opr3), &(instr.disp3), instr.addrm3, d, &dl);
                // M4
                decode_readin_opr_M(&(instr.opr4), &(instr.disp4), instr.addrm4, d, &dl);
                break;
            case OP_SCMP:
                instr.addrms[0] = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrms[1] = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrms[2] = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrms[3] = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                instr.addrms[4] = bit2uint(d+dl, &b, BIT_LEN_ADDRM);
                byte_padding_readin(&dl, b);
                // M1
                decode_readin_opr_M(instr.oprs, instr.disps, instr.addrms[0], d, &dl);
                // D1
                decode_readin_opr_D(instr.oprs+1, instr.disps+1, instr.addrms[1], d, &dl);
                // M2
                decode_readin_opr_M(instr.oprs+2, instr.disps+2, instr.addrms[2], d, &dl);
                // D2
                decode_readin_opr_D(instr.oprs+3, instr.disps+3, instr.addrms[3], d, &dl);
                // M3
                decode_readin_opr_M(instr.oprs+4, instr.disps+4, instr.addrms[4], d, &dl);
                break;
            case OP_UNDEFINED:
            default:
                zlogf_time("Decode: undefined/unsupported opcode: @%#lx: %#lx\n", instr.addr, instr.opcode);
                error("Decode: undefined/unsupported opcode.\n");
                break;
        }
// #ifdef _DEBUG_
// #ifdef _DEBUG_MORE_
// #ifdef _DEBUG_PRINT_INSTR_FIND_ADDR_
//     if (is_write_native_code) {
//         print_instr(&instr);
// #else
#ifdef _DEBUG_PRINT_INSTR_
        if (is_write_native_code) {
            print_instr(&instr);
        }
#endif
// #endif
// #endif

// #ifdef _DEBUG_PRINT_INSTR_FIND_ADDR_
//     if (is_write_n) {
//         zlogf("native instr at @%#lx\n", n+(nl));
// #endif

        // add jump table entry of br to waiting list
        if (
                instr.opcode == OP_B &&
                instr.option != OPT_B_IJ &&
                instr.ra == JUMP_A
                ) {
            char* j_target = NULL;
            // add target to waiting list
            switch (instr.option) {
                case OPT_B_J:
                    j_target = (char*)instr.opr1;
                    break;
                case OPT_B_IJ:
                    error("Waiting list: Should not reach here: OPT_B_IJ.\n");
                    break;
                case OPT_B_LE:
                case OPT_B_L:
                case OPT_B_E:
                case OPT_B_NE:
                case OPT_B_SL:
                case OPT_B_Z:
                case OPT_B_NZ:
                    j_target = (char*)instr.opr3;
                    break;
                default:
                    error("Waiting list: Undefined/unsupported B OPTION.\n");
            }

            waiting_list_pos = in_waiting_list(j_target);
            if (waiting_list_pos == -1) {
                add_waiting_list((char*)j_target);

#ifdef _DEBUG_WAITING_LIST_
                zlogf_time("waiting list: added %#lx from %#lx\n", j_target, d+prev_dl);
#endif
            }
        }

        waiting_list_pos = in_waiting_list(d+prev_dl);

        // if: in waiting list
        if (waiting_list_pos != -1) {
#ifdef _DEBUG_WAITING_LIST_
            zlogf_time("waiting list: found %#lx\n", d+prev_dl);
#endif

            add_blocks_entry(d+prev_dl, n+nl);

#ifdef _DEBUG_WAITING_LIST_
            zlogf_time("waiting list: added %#lx\n", d+prev_dl);
#endif
            waiting_list_found(waiting_list_pos);
        } else {
#ifdef _DEBUG_WAITING_LIST_
            zlogf_time("waiting list: not found %#lx\n", d+prev_dl);
#endif
        }

        // check whether this is an profiling point
        // if it is:
        //   insert profiling code
#ifdef _PROFILING_
        if (is_profiling_entry((uint64_t)(d+prev_dl))) {
            gen_profiling_code((uint64_t)(d+prev_dl), is_write_native_code, n, &nl, t, &tl);
        }
#endif
        // translate
        translate2x86_64(&instr, is_write_native_code, n, &nl, t, &tl);

        // Note: causion here
        // add jump table entry: next instruction after branch
        // and not the last instruction to translate
        if (instr.opcode == OP_B && dl < i0_code_len) {
            add_blocks_entry(d+dl, n+nl);
        }

    }

    *pnative_code_len = nl;
    if (is_write_native_code) {
        *ptrampoline_len = tl;
    }

    return 0;
}

int translate_i0_code(
        char *i0_code, uint64_t i0_code_len, 
        int is_write_native_code,
        char *native_code, uint64_t *pnative_code_len, 
        char *trampoline, uint64_t *ptrampoline_len)
{
    uint64_t t1, t2;

    t1 = 0;
    t2 = 0;

    initial_waiting_list();

    _translate_i0_code(i0_code, i0_code_len, is_write_native_code, native_code, pnative_code_len, trampoline, ptrampoline_len);

    // find the entries the waiting list
    // _translate_i0_code(i0_code, i0_code_len, 0, native_code, pnative_code_len, trampoline, ptrampoline_len);
    _translate_i0_code(i0_code, i0_code_len, 0, native_code, &t1, trampoline, &t2);

    return 0;
}

void PrintPBlockMeta(void)
{
	uint64_t i;
	if(_pblockmeta!=NULL)
	{
		ModLog(	"┌───────_pblockmeta──────────\n"
				"├──next_native == 0x%lx\n"
				"├──next_trampoline == 0x%lx\n"
				"├──jump table number == %ul",
				_pblockmeta->next_native,
				_pblockmeta->next_trampoline,
				_pblockmeta->num);
		for(i=0;i<_pblockmeta->num;i++)
		{
			printf( "├────i0 == 0x%lx\n"
					"├────native == 0x%lx\n",
					_pblockmeta->blocks[i].i0,
					_pblockmeta->blocks[i].native);
		}
		ModLog( "└──────────────────────────\n");
	}
}

uint64_t get_native_code_address(char *target)
{
	printf("trying to translate i0 starting @ addr: 0x%lx",target);
    char *i0_code = NULL;
    uint64_t i0_code_len = 0;
    char *native_code = NULL;
    uint64_t native_code_len = 0;
    char *trampoline_code = NULL;
    uint64_t trampoline_code_len = 0;

    uint64_t native_target = 0;

    int i = 0;
    int found = 0;
    int is_write = 0;

#ifdef _DEBUG_FIND_ADDR_
    zlogf("get_native_code_address() is called. target: %#llx. \n", target);
#endif
   
    found = 0;
    for (i = 0; i < _pblockmeta->num; i++) {

#ifdef _DEBUG_FIND_ADDR_
        zlogf("jump table: i0: %#llx, native: %#llx. \n", 
                _pblockmeta->blocks[i].i0, _pblockmeta->blocks[i].native);
#endif

        /*
        if ((_pblockmeta->blocks[i].i0 <= target)
            && (_pblockmeta->blocks[i].i0 + _pblockmeta->blocks[i].i0_len > target)
           ) {
            found = 1;
            break;
        }
        */
        if ((_pblockmeta->blocks[i].i0 == target)
           ) {
            found = 1;
            break;
        }
 
    }

    if (found) {
 
#ifdef _DEBUG_
        zlogf("Already translated. Find out the address.\n");
#endif

        // // already translated, find the native address
        // i0_code = _pblockmeta->blocks[i].i0;
        // i0_code_len = target - i0_code;
        native_code = _pblockmeta->blocks[i].native;

        // is_write = 0;
  
#ifdef _DEBUG_
        // zlogf("Translate length: %#llx to find the the address. \n", i0_code_len);
#endif

        // // counter the time for find the native code address
        // begin_cur_perf_counter(&perf_counter_get_native_code_address);


        // // fake translate to find the native_fi
        // translate_i0_code(i0_code, i0_code_len, is_write, NULL, &native_code_len, NULL, NULL);
        // // native_code = _pblockmeta->next_native;
        // // translate_i0_code(i0_code, i0_code_len, is_write, native_code, &native_code_len, NULL, NULL);

        // end_cur_perf_counter(&perf_counter_get_native_code_address);

        // native_target = (uint64_t)(native_code + native_code_len);

        native_target = (uint64_t)native_code;

    } else {

#ifdef _DEBUG_
        zlogf("Not translated yet. Translate it.\n");
#endif

        // not translated yet, translate a block
        i0_code = target;

        // calculate suitable LEN
        i0_code_len = _pi0codemeta->i0_code_file_len - ((uint64_t)i0_code - I0_CODE_BEGIN);

        if (i0_code_len > I0_TRANSLATE_BLOCK_LEN) {
            i0_code_len = I0_TRANSLATE_BLOCK_LEN;
        }

        native_code = _pblockmeta->next_native;
        trampoline_code = _pblockmeta->next_trampoline;

        is_write = 1;

        // add record into pblocmeta
        // _pblockmeta->blocks[_pblockmeta->num].i0 = i0_code;
        // // _pblockmeta->blocks[_pblockmeta->num].i0_len = i0_code_len;
        // _pblockmeta->blocks[_pblockmeta->num].native = native_code;
        // // _pblockmeta->blocks[_pblockmeta->num].native_len = native_code_len;
        // _pblockmeta->num += 1;
        add_blocks_entry(i0_code, native_code);

        begin_cur_perf_counter(&perf_counter_bin_tran);

        // translate the block
        translate_i0_code(i0_code, i0_code_len, is_write, 
                native_code, &native_code_len, trampoline_code, &trampoline_code_len);

        end_cur_perf_counter(&perf_counter_bin_tran);

        zlogf_time("Finish translating.\n");

        // update code addresses
        _pblockmeta->next_native += native_code_len;
        _pblockmeta->next_trampoline += trampoline_code_len;

        native_target = (uint64_t)(native_code);
    }

#ifdef _DEBUG_
    zlogf("get_native_code_address: %#lx -> %#lx\n", target, native_target);
#endif

    return native_target;
}

// to be updated code: entry
void* _sys_trampoline_handler(char *entry, char *target, long type)
{
    char *m = entry;
    uint64_t ml = 0;

    uint64_t native_code = 0;

#ifdef _DEBUG_
    zlogf("_sys_trampoline_handler is called\n");
    zlogf("entry: %#lx; target: %#lx; type: %d\n", entry, target, type);
#endif

    if (target == 0) {
        error("_sys_trampoline_handler is given a 0 target. Should not happen.\n");
    }

    native_code = get_native_code_address(target);

    // error("_sys_trampoline_handler is not implemented.");

    if (type == TRAMP_TYPE_JMP) {
        // error("TRAMP_TYPE_JMP: not implemented yet.\n");
        // update trampoline entry
        if (x86_64_in_jmp_range((uint64_t)m+5, native_code)) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: update tramp: @%#lx jmp %d\n", m+(ml), native_code - (uint64_t)m - 5L );
#endif
            m[ml++] = 0xe9;
            *((uint32_t*)(m + ml)) = native_code - (uint64_t)m - 5L;
            ml += 4;

        } else {
            error("sys_trampoline_handler: relative_address out of jmp range.\n");
        }
    } else if (type == TRAMP_TYPE_JCC) {
        // error("TRAMP_TYPE_JCC: not implemented yet.\n");
        // update trampoline entry
        if (x86_64_in_jmp_range((uint64_t)m+6, native_code)) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: update tramp: @%#lx jcc %d\n", m+(ml), native_code - (uint64_t)m - 6L );
#endif
            ml += 2; // skip the first 2 bytes in jcc and only update the relative address
            
            *((uint32_t*)(m + ml)) = native_code - (uint64_t)m - 6L;
            ml += 4;
        } else {
            error("sys_trampoline_handler: relative_address out of jmp range.\n");
        }
    } else if (type == TRAMP_TYPE_DEFAULT) {

        warning("Trampoline default handling is reached. This may work. But this is not we want. So I quit here.\n");
        // error("Trampoline default handling is reached. This may work. But this is not we want. So I quit here.\n");

        // update trampoline entry
        // movq $native_code, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: update tramp: @%#lx movq $%#lx, %%rax\n", m+(ml), native_code);
#endif
        m[ml++] = 0x48;
        m[ml++] = 0xb8;
        *((uint64_t*)(m + ml)) = native_code;
        ml += SIZE_E;
        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: update tramp: @%#lx jmpq *%%rax\n", m+(ml));
#endif
        m[ml++] = 0xff;
        m[ml++] = 0xe0;

    } else {
        error("Unsupported trampoline type.\n");
    }

#ifdef _DEBUG_MORE_
    zlogf("_sys_trampoline_handler returns\n");
#endif

    return( (void*)native_code );
}

// overflow in the hash table is not supported yet
uint64_t _sys_indirect_jump_handler(char *target)
{
    char* entry = NULL;
    long native_entry = 0;

    char* t = NULL;
    long tl = 0;

#ifdef _DEBUG_
    zlogf_time("_sys_indirect_jump_handler is called for %#lx.\n", target);
#endif

    if (target == 0) {
        error("_sys_indirect_jump_handler is given a 0 target. Should not happen.\n");
    }

    // hash to find the table entry

    __asm__ __volatile__ (
            "movq %1, %0\t\n"
            "andq $0x3fff, %0\t\n"
            "shlq $0x6, %0\t\n"
            "movq $0x130000000, %%rsi\t\n"
            "addq %%rsi, %0\t\n"
            : "=r" (entry)
            : "r" (target)
        );

#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
    zlogf_time("_sys_indirect_jump_handler: the entry for %#lx is %#lx .\n", target, entry);
#endif

    // currently, no overflow is supported. So update the entry directly

    // get the native entry address for the target
    native_entry = get_native_code_address(target);

    // update the table entry

    t = entry;
    tl = 0;

    // movq target, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx movq $%#lx, %%rax\n", t+(tl), target);
#endif
    t[tl++] = 0x48;
    t[tl++] = 0xb8;
    *((uint64_t*)(t + tl)) = (uint64_t)target;
    tl += SIZE_E;

    // cmpq %rax, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx cmpq %rax, %%rdi\n", t+(tl));
#endif
    t[tl++] = 0x48;
    t[tl++] = 0x39;
    t[tl++] = 0xc7;
    
    // jne +0xc
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx jne +0xc\n", t+(tl));
#endif
    t[tl++] = 0x75;
    t[tl++] = 0x0c;

    // movq native_entry, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx movq $%#lx, %%rax\n", t+(tl), native_entry);
#endif
    t[tl++] = 0x48;
    t[tl++] = 0xb8;
    *((uint64_t*)(t + tl)) = native_entry;
    tl += SIZE_E;

    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx jmpq *%%rax\n", t+(tl));
#endif
    t[tl++] = 0xff;
    t[tl++] = 0xe0;
    
    // movq sys_indirect_jump_handler, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx movq sys_indirect_jump_handler, %%rax\n", t+(tl));
#endif
    t[tl++] = 0x48;
    t[tl++] = 0xa1;
    *((uint64_t*)(t + tl)) = SYS_INDIR_JUMP_HANDLER;
    tl += SIZE_E;

    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr: update ji hash entry: @%#lx jmpq *%%rax\n", t+(tl));
#endif
    t[tl++] = 0xff;
    t[tl++] = 0xe0;


    // error("_sys_indirect_jump_handler is not fully implemented yet. \n");

    // return the native_entry and sys_indirect_jump_handler will jump
    // there directly

    return native_entry;
}

uint64_t run_i0_code(uint64_t fi)
{
	ModLog("preparing to run i0 code @ addr: 0x%lx\n",fi);
	PrintPBlockMeta();
    uint64_t result = 0;
    uint64_t native_fi = 0;
#ifdef _DEBUG_
    zlogf("run_i0_code()\n");
#endif
    // get native_fi
    native_fi = get_native_code_address((char*)fi);

    // start the runner
#ifdef _DEBUG_
    zlogf("Call sys_runner_wrapper\n");
#endif

    result = sys_runner_wrapper(native_fi, NULL);
#ifdef _DEBUG_
    zlogf("run_i0_code() returns: 0x%02lx\n", result);
#endif
    return result;
}

