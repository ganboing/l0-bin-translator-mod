#ifndef _BIN_TRANSLATOR_H_
#define _BIN_TRANSLATOR_H_

#include "DecodeI0.h"

enum {
    TRAMP_TYPE_JMP = 0,
    TRAMP_TYPE_JCC = 1,
    TRAMP_TYPE_DEFAULT = 2,
};

// jump range (32 bit)
#define X86_64_JUMP_RANG_UPPER (0x1 << 31 - 1)
#define X86_64_JUMP_RANG_LOWER (0 - 0x1 << 31)

// jump table waiting list
#define WAITING_LIST_SIZE (0x1024)

// note: < 2^8-1
enum {
    X86_64_CLASS0_NOT_SUPPORTED = -1, // 
    X86_64_CLASS0_NO_MEM = 0, // reg or imm
    X86_64_CLASS0_MEM = 1 // ref memory
} x86_64_class0;

// note: < 2^8-1
enum {
    // X86_64_CLASS1_NOT_SUPPORTED = -1,
    X86_64_CLASS1_IMM = 0, //imm
    X86_64_CLASS1_REG = 1,
    X86_64_CLASS1_MEM_REG = 2, // memory ref via reg
    // X86_64_CLASS1_MEM_MEM = 3 // memory ref via memory (actually not directly supported in x86-64)
} x86_64_class1;

// note: < 2^8-1
enum {
    X86_64_ADDRM_IMMEDIATE_SL = 0, // $imm
    X86_64_ADDRM_REG = 1, // reg
    X86_64_ADDRM_INDIRECT = 2, // (reg)
    X86_64_ADDRM_DISPLACEMENT = 3, // disp(reg)
} x86_64_addrm;

typedef struct _x86_64_opr {
    uint32_t class0; // layer 0 class
    uint32_t class1; // layer 1 class
    uint32_t addrm; // addressing mode
    uint64_t opr; // oprand encoding: reg / absolute / base
    uint32_t disp; // displacement
} x86_64_opr_t;

typedef struct _i0_code_meta_t {
    uint64_t i0_code_file_len;
} i0_code_meta_t;

typedef struct _translated_blocks_t {
    // address to store native code and trampoline code next translation
    char *next_native;
    char *next_trampoline;
    // jump table
    uint64_t num;
    struct {
        char *i0;
        // uint64_t i0_len;
        char *native;
        // uint64_t native_len;
    } blocks[0];
} translated_blocks_t;

void sys_trampoline_handler(char *entry, char *target);
void sys_indirect_jump_handler(char *target);
uint64_t sys_runner_wrapper(uint64_t fi, uint64_t* sb);
uint64_t sys_back_runner_wrapper();
uint64_t sys_new_runner(
        uint64_t* pstack, uint64_t* pheap_ranges, 
        uint64_t* pwatched_ranges, uint64_t* pfi);
void sys_indirect_jump(uint64_t target);
void sys_idt_03();
void sys_idt_08();
void sys_idt_80();
void sys_stdout_q(uint64_t in);
uint64_t sys_stdin_q();

uint64_t get_native_code_address(char *target);
int initial_sys_call_table();
int initial_translator_data();
int initial_i0_code_meta_data();
int initial_i0_code_meta_data_code_len(uint64_t len);
int translate_i0_code(
        char *i0_code, uint64_t i0_code_len, 
        int is_write_native_code,
        char *native_code, uint64_t *pnative_code_len, 
        char *trampoline, uint64_t *trampoline_len);
uint64_t run_i0_code(uint64_t fi);
int load_i0_code(char *file, char *i0_code, uint64_t *i0_code_len);
inline void translate2x86_64_read_reg_file_base_disp(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr);


#endif // _BIN_TRANSLATOR_H_

